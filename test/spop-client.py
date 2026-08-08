#!/usr/bin/env python3
#
# SPOP client for the spoa-mirror regression tests, driven by spop-test.sh.
#
# The protocol is implemented here on its own, without using any part of the
# program that is tested, so that a defect in the encoder or in the decoder
# of the program cannot hide itself.
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
"""Talk SPOP to a running spoa-mirror and check what comes back."""

import argparse
import select
import socket
import struct
import sys
import threading
import time

# frame types
FRM_UNSET            = 0
FRM_HAPROXY_HELLO    = 1
FRM_HAPROXY_DISCON   = 2
FRM_HAPROXY_NOTIFY   = 3
FRM_AGENT_HELLO      = 101
FRM_AGENT_DISCON     = 102
FRM_AGENT_ACK        = 103

# frame flags
FL_FIN               = 0x00000001
FL_ABRT              = 0x00000002

# data types, and the flag that marks a true boolean
T_NULL, T_BOOL, T_INT32, T_UINT32, T_INT64, T_UINT64 = 0, 1, 2, 3, 4, 5
T_IPV4, T_IPV6, T_STR, T_BIN = 6, 7, 8, 9
FL_TRUE              = 0x10

# status codes that the tests expect
ERR_NONE             = 0
ERR_TOO_BIG          = 3
ERR_INVALID          = 4
ERR_FRAG_NOT_SUPP    = 10
ERR_INTERLACED       = 11

# actions and variable scopes
ACT_SET_VAR          = 1
SCOPE_SESS           = 1

MAX_FRAME_SIZE       = 16380

# the port of the socket that the mirrored requests are sent to
MIRROR_PORT          = 0


class Failure(Exception):
    """A test case that did not get what it expected."""


def varint_encode(value):
    """Encode an integer the way the SPOP varint is defined."""
    out = bytearray()
    if value >= 240:
        out.append((value & 0xff) | 240)
        value = (value - 240) >> 4
        while value >= 128:
            out.append((value & 0xff) | 128)
            value = (value - 128) >> 7
    out.append(value)
    return bytes(out)


def varint_decode(buf, pos):
    """Decode a varint, and return it together with the new position."""
    if pos >= len(buf):
        raise Failure("varint runs past the end of the frame")
    value = buf[pos]
    pos += 1
    if value >= 240:
        shift = 4
        while True:
            if pos >= len(buf):
                raise Failure("varint runs past the end of the frame")
            byte = buf[pos]
            pos += 1
            value += byte << shift
            shift += 7
            if byte < 128:
                break
    return value, pos


def buffer_encode(data):
    """Encode a string or a binary block: its length, then its bytes."""
    if isinstance(data, str):
        data = data.encode()
    return varint_encode(len(data)) + data


def buffer_decode(buf, pos):
    n, pos = varint_decode(buf, pos)
    if pos + n > len(buf):
        raise Failure("buffer of %d bytes runs past the end of the frame" % n)
    return buf[pos:pos + n], pos + n


def data_encode(dtype, value=None):
    """Encode a typed data: the type byte, then the value it needs."""
    if dtype == T_NULL:
        return bytes([T_NULL])
    if dtype == T_BOOL:
        return bytes([T_BOOL | (FL_TRUE if value else 0)])
    if dtype in (T_INT32, T_UINT32, T_INT64, T_UINT64):
        return bytes([dtype]) + varint_encode(value)
    if dtype == T_IPV4:
        return bytes([T_IPV4]) + socket.inet_pton(socket.AF_INET, value)
    if dtype == T_IPV6:
        return bytes([T_IPV6]) + socket.inet_pton(socket.AF_INET6, value)
    if dtype in (T_STR, T_BIN):
        return bytes([dtype]) + buffer_encode(value)
    raise Failure("cannot encode the data type %d" % dtype)


def data_decode(buf, pos):
    """Decode a typed data, and return its type, value and new position."""
    if pos >= len(buf):
        raise Failure("typed data runs past the end of the frame")
    byte = buf[pos]
    pos += 1
    dtype = byte & 0x0f
    if dtype == T_NULL:
        return dtype, None, pos
    if dtype == T_BOOL:
        return dtype, (byte & 0xf0) == FL_TRUE, pos
    if dtype in (T_INT32, T_UINT32, T_INT64, T_UINT64):
        value, pos = varint_decode(buf, pos)
        return dtype, value, pos
    if dtype == T_IPV4:
        if pos + 4 > len(buf):
            raise Failure("IPv4 address runs past the end of the frame")
        return dtype, socket.inet_ntop(socket.AF_INET, buf[pos:pos + 4]), pos + 4
    if dtype == T_IPV6:
        if pos + 16 > len(buf):
            raise Failure("IPv6 address runs past the end of the frame")
        return dtype, socket.inet_ntop(socket.AF_INET6, buf[pos:pos + 16]), pos + 16
    if dtype in (T_STR, T_BIN):
        value, pos = buffer_decode(buf, pos)
        return dtype, value, pos
    raise Failure("cannot decode the data type %d" % dtype)


def kv_encode(name, dtype, value=None):
    """Encode one key/value item: the name, then the typed value."""
    return buffer_encode(name) + data_encode(dtype, value)


def kv_decode(payload, pos):
    """Decode all the key/value items that follow the frame header."""
    items = {}
    while pos < len(payload):
        name, pos = buffer_decode(payload, pos)
        _, value, pos = data_decode(payload, pos)
        if isinstance(value, bytes):
            value = value.decode(errors="replace")
        items[name.decode(errors="replace")] = value
    return items


def actions_decode(payload, pos):
    """Decode the actions that an ACK frame carries."""
    actions = []
    while pos < len(payload):
        if pos + 2 > len(payload):
            raise Failure("action header runs past the end of the frame")
        atype = payload[pos]
        nbargs = payload[pos + 1]
        pos += 2
        if atype != ACT_SET_VAR:
            raise Failure("unexpected action type %d" % atype)
        if nbargs != 3:
            raise Failure("SET-VAR action with %d arguments" % nbargs)
        if pos >= len(payload):
            raise Failure("action scope runs past the end of the frame")
        scope = payload[pos]
        pos += 1
        name, pos = buffer_decode(payload, pos)
        dtype, value, pos = data_decode(payload, pos)
        actions.append((scope, name.decode(errors="replace"), dtype, value))
    return actions


class Frame:
    """One SPOP frame: the header fields and the payload that follows."""

    def __init__(self, ftype, flags=FL_FIN, stream_id=0, frame_id=0, payload=b""):
        self.ftype = ftype
        self.flags = flags
        self.stream_id = stream_id
        self.frame_id = frame_id
        self.payload = payload

    def encode(self):
        body = bytes([self.ftype]) + struct.pack("!I", self.flags)
        body += varint_encode(self.stream_id) + varint_encode(self.frame_id)
        body += self.payload
        return struct.pack("!I", len(body)) + body

    @classmethod
    def decode(cls, raw):
        if len(raw) < 6:
            raise Failure("frame of %d bytes is too short" % len(raw))
        ftype = raw[0]
        (flags,) = struct.unpack("!I", raw[1:5])
        stream_id, pos = varint_decode(raw, 5)
        frame_id, pos = varint_decode(raw, pos)
        return cls(ftype, flags, stream_id, frame_id, raw[pos:])

    def __str__(self):
        return "type=%d flags=0x%08x stream-id=%d frame-id=%d payload=%d bytes" % (
            self.ftype, self.flags, self.stream_id, self.frame_id, len(self.payload))


def connect(port, timeout=5.0, what="the agent"):
    """Open a socket to a port, waiting until something listens on it."""
    deadline = time.monotonic() + timeout
    while True:
        try:
            return socket.create_connection(("127.0.0.1", port), timeout=timeout)
        except OSError:
            if time.monotonic() >= deadline:
                raise Failure("cannot connect to %s on port %d" % (what, port))
            time.sleep(0.05)


class Agent:
    """The connection to the program that is tested."""

    def __init__(self, port, timeout=5.0):
        self.timeout = timeout
        self.sock = connect(port, timeout)

    def send(self, frame):
        self.sock.sendall(frame.encode())

    def send_raw(self, data):
        self.sock.sendall(data)

    def recv(self):
        """Read one frame; a NULL result means the agent closed the socket."""
        hdr = self.recv_exactly(4)
        if hdr is None:
            return None
        (n,) = struct.unpack("!I", hdr)
        if n == 0 or n > 65536:
            raise Failure("the agent announced a frame of %d bytes" % n)
        body = self.recv_exactly(n)
        if body is None:
            raise Failure("the agent closed the socket inside a frame")
        return Frame.decode(body)

    def recv_exactly(self, n):
        data = b""
        while len(data) < n:
            try:
                chunk = self.sock.recv(n - len(data))
            except socket.timeout:
                raise Failure("the agent did not answer within %.0fs" % self.timeout)
            if not chunk:
                if not data:
                    return None
                raise Failure("the agent sent %d of %d bytes" % (len(data), n))
            data += chunk
        return data

    def at_eof(self):
        """True when the agent closed the connection, reset included."""
        try:
            return self.sock.recv(1) == b""
        except ConnectionResetError:
            return True
        except socket.timeout:
            return False

    def close(self):
        self.sock.close()


def hello_payload(capabilities="pipelining", healthcheck=False, engine_id=None,
                  max_frame_size=MAX_FRAME_SIZE):
    """Build the payload of a HAPROXY-HELLO frame."""
    payload = kv_encode("supported-versions", T_STR, "2.0")
    payload += kv_encode("max-frame-size", T_UINT32, max_frame_size)
    if healthcheck:
        payload += kv_encode("healthcheck", T_BOOL, True)
    payload += kv_encode("capabilities", T_STR, capabilities)
    if engine_id is not None:
        payload += kv_encode("engine-id", T_STR, engine_id)
    return payload


def message(name, args=()):
    """Build one message of a NOTIFY frame: its name, then its arguments."""
    payload = buffer_encode(name) + bytes([len(args)])
    for arg_name, dtype, value in args:
        payload += buffer_encode(arg_name) + data_encode(dtype, value)
    return payload


def handshake(agent, capabilities="pipelining", **kwargs):
    """Exchange HELLO frames, and return the items the agent announced."""
    agent.send(Frame(FRM_HAPROXY_HELLO, FL_FIN, 0, 0,
                     hello_payload(capabilities, **kwargs)))
    answer = agent.recv()
    if answer is None:
        raise Failure("the agent closed the connection instead of answering HELLO")
    if answer.ftype != FRM_AGENT_HELLO:
        raise Failure("expected AGENT-HELLO, got %s" % answer)
    return kv_decode(answer.payload, 0)


def expect_ack(agent, stream_id, frame_id):
    """Read the answer, and check that it is the ACK of a given frame."""
    answer = agent.recv()
    if answer is None:
        raise Failure("the agent closed the connection instead of sending an ACK")
    if answer.ftype != FRM_AGENT_ACK:
        if answer.ftype == FRM_AGENT_DISCON:
            items = kv_decode(answer.payload, 0)
            raise Failure("got AGENT-DISCONNECT, status-code=%s (%s)" % (
                items.get("status-code"), items.get("message")))
        raise Failure("expected AGENT-ACK, got %s" % answer)
    if (answer.stream_id, answer.frame_id) != (stream_id, frame_id):
        raise Failure("ACK carries stream-id=%d frame-id=%d, expected %d and %d" % (
            answer.stream_id, answer.frame_id, stream_id, frame_id))
    return answer


def expect_disconnect(agent, status_code):
    """Read the answer, and check that it is a DISCONNECT with a status."""
    answer = agent.recv()
    if answer is None:
        raise Failure("the agent closed the connection without a DISCONNECT")
    if answer.ftype != FRM_AGENT_DISCON:
        raise Failure("expected AGENT-DISCONNECT, got %s" % answer)
    items = kv_decode(answer.payload, 0)
    if items.get("status-code") != status_code:
        raise Failure("DISCONNECT carries status-code=%s, expected %d" % (
            items.get("status-code"), status_code))
    return items


# --------------------------------------------------------------------------
# the test cases
# --------------------------------------------------------------------------

def case_handshake(port, report):
    """The HELLO exchange, and the capabilities that are negotiated."""
    agent = Agent(port)
    items = handshake(agent, "pipelining")
    if items.get("version") != "2.0":
        raise Failure("the agent announced the version %s" % items.get("version"))
    size = items.get("max-frame-size")
    if not isinstance(size, int) or size > MAX_FRAME_SIZE:
        raise Failure("the agent announced max-frame-size=%s" % size)
    caps = [c for c in str(items.get("capabilities", "")).split(",") if c != ""]
    if "pipelining" not in caps:
        raise Failure("the agent did not accept the pipelining capability")
    for cap in caps:
        if cap not in ("fragmentation", "pipelining", "async"):
            raise Failure("the agent announced an unknown capability '%s'" % cap)
    agent.close()

    # a client that announces nothing must not get the capability back
    agent = Agent(port)
    items = handshake(agent, "")
    caps_none = str(items.get("capabilities", ""))
    if "pipelining" in caps_none:
        raise Failure("the agent announced pipelining to a client without it")
    agent.close()

    report("version=%s max-frame-size=%d capabilities=%s" % (
        items.get("version"), size, ",".join(caps) if caps else "<none>"))


def case_healthcheck(port, report):
    """A HELLO that only checks the health closes the connection."""
    agent = Agent(port)
    handshake(agent, "pipelining", healthcheck=True)
    if not agent.at_eof():
        raise Failure("the agent kept the connection of a healthcheck open")
    agent.close()
    report("AGENT-HELLO answered, connection closed")


def case_frame_size(port, report):
    """The lower of the two announced frame sizes is the one that is kept."""
    agent = Agent(port)
    offered = 512
    items = handshake(agent, "pipelining", max_frame_size=offered)
    size = items.get("max-frame-size")
    if not isinstance(size, int) or size > offered:
        raise Failure("the agent announced max-frame-size=%s, %d was offered" % (
            size, offered))
    agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 1, 1, message("test")))
    answer = expect_ack(agent, 1, 1)
    # the 4 length bytes in front of a frame do not count against the size
    if len(answer.encode()) - 4 > size:
        raise Failure("the ACK of %d bytes passes the announced %d" % (
            len(answer.encode()) - 4, size))
    agent.close()
    report("%d bytes offered, the agent kept %d" % (offered, size))


def case_notify_ack(port, report):
    """A NOTIFY frame with a 'test' message is answered with an ACK."""
    agent = Agent(port)
    handshake(agent)
    args = (
        ("arg_str", T_STR, "value"),
        ("arg_int", T_UINT32, 4242),
        ("arg_ip", T_IPV4, "127.0.0.1"),
        ("arg_bool", T_BOOL, True),
        ("arg_null", T_NULL, None),
        ("arg_bin", T_BIN, b"\x00\x01\x02\xff"),
    )
    agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 7, 9, message("test", args)))
    answer = expect_ack(agent, 7, 9)
    if actions_decode(answer.payload, 0) != []:
        raise Failure("the ACK of a 'test' message carries an action")
    agent.close()
    report("ACK for %d arguments, no action" % len(args))


def case_unknown_message(port, report):
    """A message that the agent does not know is answered without an action."""
    agent = Agent(port)
    handshake(agent)
    args = (
        ("arg_str", T_STR, "value"),
        ("arg_int", T_UINT32, 1),
    )
    agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 1, 1,
                     message("no-such-message", args)))
    answer = expect_ack(agent, 1, 1)
    if actions_decode(answer.payload, 0) != []:
        raise Failure("the ACK of an unknown message carries an action")
    agent.close()
    report("ACK without an action")


def case_multi_message(port, report):
    """A NOTIFY that holds more than one message is answered with one ACK."""
    agent = Agent(port)
    handshake(agent)
    payload = message("test")
    payload += message("check-client-ip", (("ip", T_IPV4, "203.0.113.7"),))
    payload += message("no-such-message")
    agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 1, 1, payload))
    answer = expect_ack(agent, 1, 1)
    actions = actions_decode(answer.payload, 0)
    if len(actions) != 1:
        raise Failure("the ACK carries %d actions, expected one" % len(actions))
    if actions[0][1] != "ip_score":
        raise Failure("the action sets the variable '%s'" % actions[0][1])
    agent.close()
    report("3 messages answered with one ACK and one action")


def case_pipelining(port, report):
    """Several NOTIFY frames sent at once are answered one after another."""
    agent = Agent(port)
    handshake(agent)
    count = 5
    for i in range(1, count + 1):
        agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, i, i, message("test")))
    for i in range(1, count + 1):
        expect_ack(agent, i, i)
    agent.close()
    report("%d frames sent at once, %d ACKs in the same order" % (count, count))


def case_iprep_action(port, report):
    """The 'check-client-ip' message adds the ip_score action to the ACK."""
    agent = Agent(port)
    handshake(agent)
    payload = message("check-client-ip", (("ip", T_IPV4, "192.0.2.1"),))
    agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 1, 1, payload))
    answer = expect_ack(agent, 1, 1)
    actions = actions_decode(answer.payload, 0)
    if len(actions) != 1:
        raise Failure("the ACK carries %d actions, expected one" % len(actions))
    scope, name, dtype, value = actions[0]
    if scope != SCOPE_SESS:
        raise Failure("the action sets a variable of the scope %d" % scope)
    if name != "ip_score":
        raise Failure("the action sets the variable '%s'" % name)
    if dtype != T_UINT32 or not 0 <= value <= 100:
        raise Failure("the ip_score value is %s of the type %d" % (value, dtype))
    agent.close()
    report("SET-VAR sess.ip_score=%d" % value)


def case_iprep_ipv6(port, report):
    """The 'check-client-ip' message takes an IPv6 address as well."""
    agent = Agent(port)
    handshake(agent)
    payload = message("check-client-ip", (("ip", T_IPV6, "2001:db8::1"),))
    agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 1, 1, payload))
    answer = expect_ack(agent, 1, 1)
    actions = actions_decode(answer.payload, 0)
    if len(actions) != 1:
        raise Failure("the ACK carries %d actions, expected one" % len(actions))
    scope, name, dtype, value = actions[0]
    if name != "ip_score":
        raise Failure("the action sets the variable '%s'" % name)
    if dtype != T_UINT32 or not 0 <= value <= 100:
        raise Failure("the ip_score value is %s of the type %d" % (value, dtype))
    agent.close()
    report("SET-VAR sess.ip_score=%d" % value)


def case_mirror_hdrs(port, report):
    """The HTTP headers of a 'mirror' message are decoded and released.

    A binary header block that ends after a header name left the header
    allocated for that name behind, which only a sanitized build shows.
    """
    agent = Agent(port)
    handshake(agent)

    # the headers as a single string, every one of them ended with CRLF
    hdrs = "Host: example.com\r\nAccept: */*\r\n"
    args = (
        ("arg_method", T_STR, "GET"),
        ("arg_path", T_STR, "/index.html"),
        ("arg_ver", T_STR, "1.1"),
        ("arg_hdrs", T_STR, hdrs),
    )
    agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 1, 1, message("mirror", args)))
    expect_ack(agent, 1, 1)

    # the headers as name/value pairs, the last name without its value
    block = buffer_encode("Host") + buffer_encode("example.com")
    block += buffer_encode("X-Truncated") + b"\x00"
    args = (
        ("arg_method", T_STR, "GET"),
        ("arg_path", T_STR, "/index.html"),
        ("arg_ver", T_STR, "1.1"),
        ("arg_hdrs", T_BIN, block),
    )
    agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 2, 2, message("mirror", args)))
    expect_ack(agent, 2, 2)
    agent.close()
    report("%d string and %d binary header bytes answered" % (len(hdrs), len(block)))


def case_mirror_body(port, report):
    """A 'mirror' message that carries a body is decoded and released."""
    agent = Agent(port)
    handshake(agent)
    body = b"name=value&" * 100
    args = (
        ("arg_method", T_STR, "POST"),
        ("arg_path", T_STR, "/post"),
        ("arg_ver", T_STR, "1.1"),
        ("arg_hdrs", T_STR, "Host: example.com\r\nContent-Length: %d\r\n" % len(body)),
        ("arg_body", T_BIN, body),
    )
    agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 1, 1, message("mirror", args)))
    expect_ack(agent, 1, 1)
    agent.close()
    report("body of %d bytes answered" % len(body))


def case_mirror_url(port, report):
    """The path of a 'mirror' message becomes the URL to mirror to.

    The agent runs with a mirror URL for this case, so the request target
    is sent in all of its forms: the origin form, the absolute form, and
    an absolute one that has no path at all.  Nothing listens on the port
    of the mirror URL, so every transfer fails as soon as it starts.
    """
    agent = Agent(port)
    handshake(agent)
    paths = (
        "/index.html?a=1",
        "http://example.com/absolute.html",
        "https://example.com:8443/secure.html",
        "http://example.com",
    )
    for i, path in enumerate(paths, 1):
        args = (
            ("arg_method", T_STR, "GET"),
            ("arg_path", T_STR, path),
            ("arg_ver", T_STR, "1.1"),
            ("arg_hdrs", T_STR, "Host: example.com\r\n"),
        )
        agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, i, i, message("mirror", args)))
        expect_ack(agent, i, i)
    agent.close()
    report("%d request targets answered" % len(paths))


def case_mirror_stop(port, report):
    """The transfers that are still running when the program stops are released.

    The mirror URL of this case is the socket that the script listens on
    without ever answering, so the transfers are still added to the multi
    handle when the program is stopped, which is what the release of the
    cURL data has to take into account.
    """
    if MIRROR_PORT == 0:
        raise Failure("the port of the mirror socket is not given")

    # nothing stays in flight when the socket of the mirror is not there
    connect(MIRROR_PORT, what="the socket of the mirror").close()

    agent = Agent(port)
    handshake(agent)
    count = 4
    for i in range(1, count + 1):
        args = (
            ("arg_method", T_STR, "GET"),
            ("arg_path", T_STR, "/hang-%d.html" % i),
            ("arg_ver", T_STR, "1.1"),
            ("arg_hdrs", T_STR, "Host: example.com\r\n"),
        )
        agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, i, i, message("mirror", args)))
        expect_ack(agent, i, i)

    # the transfers need a moment to reach the socket of the mirror
    time.sleep(0.5)
    agent.close()
    report("%d transfers left running to the port %d" % (count, MIRROR_PORT))


def case_fragmented(port, report):
    """A NOTIFY split over two frames is accumulated and answered.

    The second fragment is much larger than the first one, which is what
    made the accumulating buffer overflow.
    """
    agent = Agent(port)
    handshake(agent, "fragmentation,pipelining")
    payload = message("test") * 90
    first = len(message("test")) * 7
    agent.send(Frame(FRM_HAPROXY_NOTIFY, 0, 3, 4, payload[:first]))
    agent.send(Frame(FRM_UNSET, FL_FIN, 3, 4, payload[first:]))
    expect_ack(agent, 3, 4)
    agent.close()
    report("%d + %d payload bytes accumulated" % (first, len(payload) - first))


def case_frag_abort(port, report):
    """An aborted fragment is dropped, and the connection stays usable."""
    agent = Agent(port)
    handshake(agent, "fragmentation,pipelining")
    payload = message("test") * 20
    agent.send(Frame(FRM_HAPROXY_NOTIFY, 0, 5, 6, payload))
    # the FIN flag has to accompany ABORT, which is what HAProxy sends
    agent.send(Frame(FRM_UNSET, FL_FIN | FL_ABRT, 5, 6, b""))
    agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 5, 7, message("test")))
    expect_ack(agent, 5, 7)
    agent.close()
    report("aborted frame ignored, the next frame answered")


def case_frag_interlaced(port, report):
    """A fragment of another frame is refused as an interlaced frame."""
    agent = Agent(port)
    handshake(agent, "fragmentation,pipelining")
    payload = message("test") * 20
    agent.send(Frame(FRM_HAPROXY_NOTIFY, 0, 11, 12, payload))
    agent.send(Frame(FRM_UNSET, FL_FIN, 21, 22, payload))
    expect_disconnect(agent, ERR_INTERLACED)
    agent.close()
    report("status-code=%d as expected" % ERR_INTERLACED)


def case_frag_cap(port, report):
    """The payload of a fragmented frame may not grow without an end.

    The fragments are sent until the agent refuses them, which it has to
    do once the accumulated payload passes the size that it allows.
    """
    agent = Agent(port)
    handshake(agent, "fragmentation,pipelining")

    chunk = (message("test") * 2700)[:16000]
    agent.send(Frame(FRM_HAPROXY_NOTIFY, 0, 3, 4, chunk))
    total = len(chunk)
    refused = False

    for _ in range(120):
        if select.select([agent.sock], [], [], 0.02)[0]:
            refused = True
            break
        agent.send(Frame(FRM_UNSET, 0, 3, 4, chunk))
        total += len(chunk)

    if not refused:
        raise Failure("the agent accumulated %d payload bytes without refusing" % total)

    expect_disconnect(agent, ERR_TOO_BIG)
    agent.close()
    report("refused after %d payload bytes accumulated" % total)


def case_no_frag_cap(port, report):
    """A fragmented frame is refused when fragmentation is not enabled."""
    agent = Agent(port)
    handshake(agent, "pipelining")
    agent.send(Frame(FRM_HAPROXY_NOTIFY, 0, 1, 1, message("test") * 20))
    expect_disconnect(agent, ERR_FRAG_NOT_SUPP)
    agent.close()
    report("status-code=%d as expected" % ERR_FRAG_NOT_SUPP)


def case_oversized(port, report):
    """A frame that announces more data than allowed is refused."""
    agent = Agent(port)
    agent.send_raw(struct.pack("!I", 200000) + b"\x01" + struct.pack("!I", FL_FIN))
    if not agent.at_eof():
        raise Failure("the agent kept the connection of an oversized frame open")
    agent.close()

    # the agent has to be alive and to serve the next client
    agent = Agent(port)
    handshake(agent)
    agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 1, 1, message("test")))
    expect_ack(agent, 1, 1)
    agent.close()
    report("connection closed, the agent kept running")


def case_bad_kv_item(port, report):
    """A key/value item whose value runs past the end aborts the frame."""
    agent = Agent(port)
    payload = kv_encode("supported-versions", T_STR, "2.0")
    payload += buffer_encode("capabilities") + bytes([T_STR]) + varint_encode(200)
    payload += b"abc"
    agent.send(Frame(FRM_HAPROXY_HELLO, FL_FIN, 0, 0, payload))
    expect_disconnect(agent, ERR_INVALID)
    agent.close()
    report("status-code=%d as expected" % ERR_INVALID)


def expect_refused(agent, what):
    """Read the answer, and check that the agent refused the connection."""
    answer = agent.recv()
    if answer is None:
        raise Failure("the agent closed the connection of %s silently" % what)
    if answer.ftype != FRM_AGENT_DISCON:
        raise Failure("expected AGENT-DISCONNECT, got %s" % answer)
    if not agent.at_eof():
        raise Failure("the agent kept the connection of %s open" % what)
    return kv_decode(answer.payload, 0)


def case_no_hello(port, report):
    """A NOTIFY that arrives before any HELLO is refused."""
    agent = Agent(port)
    agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 1, 1, message("test")))
    items = expect_refused(agent, "a client that did not say HELLO")
    agent.close()
    report("AGENT-DISCONNECT status-code=%s, connection closed" % (
        items.get("status-code")))


def case_unknown_frame(port, report):
    """A frame of a type that the protocol does not define is refused."""
    agent = Agent(port)
    agent.send(Frame(42, FL_FIN, 0, 0, b""))
    items = expect_refused(agent, "a frame of an unknown type")
    agent.close()
    report("AGENT-DISCONNECT status-code=%s, connection closed" % (
        items.get("status-code")))


def case_abrupt_close(port, report):
    """A client that disappears inside a frame does not disturb the agent."""
    # a frame header that announces data which never arrives
    sock = connect(port)
    sock.sendall(struct.pack("!I", 500) + bytes([FRM_HAPROXY_HELLO])
                 + struct.pack("!I", FL_FIN))
    sock.close()

    # not even the length of the frame is complete
    sock = connect(port)
    sock.sendall(b"\x00\x00")
    sock.close()

    # a client that connects and says nothing at all
    connect(port).close()

    agent = Agent(port)
    handshake(agent)
    agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 1, 1, message("test")))
    expect_ack(agent, 1, 1)
    agent.close()
    report("3 clients left, the one that followed was answered")


def case_many_clients(port, report):
    """Many clients at once are spread over the workers and all answered."""
    count = 20
    agents = [Agent(port) for _ in range(count)]
    for agent in agents:
        handshake(agent)
    for i, agent in enumerate(agents, 1):
        agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, i, i, message("test")))
    for i, agent in enumerate(agents, 1):
        expect_ack(agent, i, i)
    for agent in agents:
        agent.close()
    report("%d clients handshaked and answered" % count)


def handover_busy(port, stop, errors, counts, index):
    """Keep one client answered without a pause, until the flag is set."""
    count = 0
    try:
        agent = Agent(port)
        handshake(agent)
        while not stop.is_set():
            count += 1
            agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 1, count, message("test")))
            expect_ack(agent, 1, count)
        agent.close()
    except Exception as e:
        errors.append("the client that keeps a worker busy: %s" % e)
    counts[index] = count


def handover_join(port, batch, deadline, errors, counts, index):
    """Connect several clients at once and greet them, until the time is over."""
    count = 0
    try:
        while time.monotonic() < deadline:
            agents = [Agent(port) for _ in range(batch)]
            for agent in agents:
                agent.send(Frame(FRM_HAPROXY_HELLO, FL_FIN, 0, 0, hello_payload()))
            for agent in agents:
                answer = agent.recv()
                if answer is None:
                    raise Failure("the agent closed the connection of a new client")
                if answer.ftype != FRM_AGENT_HELLO:
                    raise Failure("expected AGENT-HELLO, got %s" % answer)
                count += 1
            for agent in agents:
                agent.close()
    except Exception as e:
        errors.append("the client %d that joined: %s" % (count + 1, e))
    counts[index] = count


def case_handover(port, report):
    """A client that arrives while the workers are busy is answered as well.

    The main thread accepted the connection and started the read watcher
    of the client on the event loop that the worker was running at the
    same time, and a watcher that was started like that could be lost,
    which left the client without an answer.
    """
    busy, joining, batch, seconds = 16, 8, 4, 3.0
    stop = threading.Event()
    errors = []
    busy_counts = [0] * busy
    counts = [0] * joining
    deadline = time.monotonic() + seconds

    threads = [threading.Thread(target=handover_busy,
                                args=(port, stop, errors, busy_counts, i))
               for i in range(busy)]
    threads += [threading.Thread(target=handover_join,
                                 args=(port, batch, deadline, errors, counts, i))
                for i in range(joining)]
    for thread in threads:
        thread.start()
    for thread in threads[busy:]:
        thread.join(60)
    stop.set()
    for thread in threads[:busy]:
        thread.join(60)

    if errors:
        raise Failure(errors[0])

    # a load that did not happen would leave the case without its point
    if min(busy_counts) == 0:
        raise Failure("a client that had to keep a worker busy answered nothing")

    # the agent has to answer a client that comes after all of that
    agent = Agent(port)
    handshake(agent)
    agent.send(Frame(FRM_HAPROXY_NOTIFY, FL_FIN, 1, 1, message("test")))
    expect_ack(agent, 1, 1)
    agent.close()
    report("%d clients joined in %.1fs, %d busy clients answered %d frames" % (
        sum(counts), seconds, busy, sum(busy_counts)))


def case_disconnect(port, report):
    """A DISCONNECT frame is answered and the connection is closed."""
    agent = Agent(port)
    handshake(agent)
    payload = kv_encode("status-code", T_UINT32, ERR_NONE)
    payload += kv_encode("message", T_STR, "bye")
    agent.send(Frame(FRM_HAPROXY_DISCON, FL_FIN, 0, 0, payload))
    items = expect_disconnect(agent, ERR_NONE)
    if not agent.at_eof():
        raise Failure("the agent kept the connection open after a DISCONNECT")
    agent.close()
    report("status-code=%s (%s)" % (items.get("status-code"), items.get("message")))


CASES = {
    "handshake":       case_handshake,
    "healthcheck":     case_healthcheck,
    "frame_size":      case_frame_size,
    "notify_ack":      case_notify_ack,
    "unknown_message": case_unknown_message,
    "multi_message":   case_multi_message,
    "pipelining":      case_pipelining,
    "iprep_action":    case_iprep_action,
    "iprep_ipv6":      case_iprep_ipv6,
    "mirror_hdrs":     case_mirror_hdrs,
    "mirror_body":     case_mirror_body,
    "mirror_url":      case_mirror_url,
    "mirror_stop":     case_mirror_stop,
    "fragmented":      case_fragmented,
    "frag_abort":      case_frag_abort,
    "frag_interlaced": case_frag_interlaced,
    "frag_cap":        case_frag_cap,
    "no_frag_cap":     case_no_frag_cap,
    "oversized":       case_oversized,
    "bad_kv_item":     case_bad_kv_item,
    "no_hello":        case_no_hello,
    "unknown_frame":   case_unknown_frame,
    "abrupt_close":    case_abrupt_close,
    "many_clients":    case_many_clients,
    "handover":        case_handover,
    "disconnect":      case_disconnect,
}


def run_sink(port):
    """Listen on a port and never answer, so a transfer to it stays open."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("127.0.0.1", port))
    sock.listen(64)
    while True:
        time.sleep(3600)


def main():
    global MIRROR_PORT

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, help="port of the agent")
    parser.add_argument("--case", choices=sorted(CASES), help="test case to run")
    parser.add_argument("--mirror-port", type=int, default=0,
                        help="port the mirrored requests are sent to")
    parser.add_argument("--sink", action="store_true",
                        help="listen on the port and never answer")
    parser.add_argument("--list", action="store_true", help="list the test cases")
    args = parser.parse_args()

    if args.list:
        for name in sorted(CASES):
            print(name)
        return 0

    if args.sink:
        if args.port is None:
            parser.error("--port is needed to listen")
        return run_sink(args.port)

    if args.port is None or args.case is None:
        parser.error("both --port and --case are needed to run a test case")

    MIRROR_PORT = args.mirror_port
    detail = []
    try:
        CASES[args.case](args.port, detail.append)
    except Failure as e:
        print("%s" % e)
        return 1
    except OSError as e:
        print("%s" % e)
        return 1

    for line in detail:
        print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())
