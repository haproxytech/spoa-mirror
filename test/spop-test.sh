#!/bin/sh -u
#
# SPOP regression tests for the spoa-mirror program.
#
# Every test case starts a new instance of the program, talks to it over the
# SPOP protocol using the spop-client.py client and checks what the program
# answers.  The cases cover the frame exchange that HAProxy performs, and the
# frames that were decoded incorrectly in the past.
#
# The overflow of the buffer that accumulates a fragmented frame, the header
# that a truncated header block leaves behind, and the cURL data of a worker
# that stops can only be seen when the program is built with the address
# sanitizer, so the cases that need it are reported as skipped with any other
# build.  A case or an argument check that needs an option the program was
# not built with is skipped as well.
#
# A case whose mirrored requests have to stay in flight is given a socket that
# the script listens on without ever answering; its port is the port of the
# case raised by 1000.
#
# A case fails when the client reports an error, when a sanitizer writes to
# the log of the program, or when the program is no longer running once the
# client is done.
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
  SH_ARG_AGENT=
   SH_OPT_KEEP=
   SH_OPT_LIST=
   SH_ARG_PORT="22345"
SH_ARG_RUNTIME="30s"
SH_OPT_VERBOSE=

    SH_PROGRAM="$(basename "${0}")"
        SH_DIR="$(dirname "${0}")"
     SH_CLIENT="${SH_DIR}/spop-client.py"
      SH_CASES="handshake healthcheck frame_size notify_ack unknown_message multi_message pipelining iprep_action iprep_ipv6 mirror_hdrs mirror_body mirror_url mirror_stop fragmented frag_abort frag_interlaced frag_cap no_frag_cap oversized bad_kv_item no_hello unknown_frame abrupt_close many_clients handover disconnect"
 SH_MIRROR_URL="http://127.0.0.1:1/"
      SH_AGENT=
   SH_AGENT_RC="0"
      SH_ALIVE=
       SH_ASAN=
       SH_HELP=
        SH_PID=
   SH_SINK_PID=
  SH_SINK_PORT="0"
     SH_TMPDIR=
     SH_PASSED="0"
     SH_FAILED="0"
    SH_SKIPPED="0"

         SH_EX_OK="0"
      SH_EX_USAGE="64"
    SH_EX_NOINPUT="66"
SH_EX_UNAVAILABLE="69"
   SH_EX_SOFTWARE="70"
   SH_EX_TEMPFAIL="75"


sh_usage ()
{
	echo
	echo "Usage: ${SH_PROGRAM} [-a AGENT] [-p PORT] [-t TIME] [-k] [-l] [-v] [-h] [CASE]..."
	echo
	echo "Options are:"
	echo "  -a AGENT  Path to the spoa-mirror program (default: search the build)."
	echo "  -p PORT   First port used by the test cases (default: 22345)."
	echo "  -t TIME   Time the program is allowed to run in a case (default: 30s)."
	echo "  -k        Keep the temporary directory even when all the cases pass."
	echo "  -l        List the test cases and exit."
	echo "  -v        Show every line that a test case reported."
	echo "  -h        Show this text."
	echo
	echo "Test cases: argcheck ${SH_CASES}"
	echo
}

sh_list ()
{
	echo "argcheck"
	for _loop_name in ${SH_CASES}; do
		echo "${_loop_name}"
	done
}

sh_known_case ()
{
	_arg_case="${1}"

	if test "${_arg_case}" = "argcheck"; then
		return 0
	fi

	for _loop_name in ${SH_CASES}; do
		if test "${_loop_name}" = "${_arg_case}"; then
			return 0
		fi
	done

	return 1
}

sh_cleanup ()
{
	if test -n "${SH_PID}"; then
		kill -TERM "${SH_PID}" 2>/dev/null
		wait "${SH_PID}" 2>/dev/null
		SH_PID=
	fi

	sh_stop_sink

	if test -n "${SH_TMPDIR}" -a -d "${SH_TMPDIR}" -a "${SH_FAILED}" -eq 0 -a "${SH_OPT_KEEP}" != "true"; then
		rm -rf "${SH_TMPDIR}"
	fi
}

sh_interrupt ()
{
	echo
	echo "Interrupted."

	sh_cleanup

	exit "${SH_EX_TEMPFAIL}"
}

sh_find_agent ()
{
	for _loop_path in \
		"${SH_DIR}/../src/spoa-mirror" \
		"${SH_DIR}/../src/spoa-mirror_dbg" \
		"${SH_DIR}/../build/src/spoa-mirror" \
		"${SH_DIR}/../build/src/spoa-mirror_dbg" \
		"./src/spoa-mirror" \
		"./src/spoa-mirror_dbg"
	do
		if test -x "${_loop_path}"; then
			echo "${_loop_path}"

			return
		fi
	done
}

sh_have_asan ()
{
	_arg_agent="${1}"

	if ldd "${_arg_agent}" 2>/dev/null | grep -q "libasan"; then
		return 0
	fi
	if nm -D "${_arg_agent}" 2>/dev/null | grep -q "__asan_init"; then
		return 0
	fi

	return 1
}

sh_have_option ()
{
	_arg_option="${1}"

	echo "${SH_HELP}" | grep -q -- "${_arg_option}"
}

sh_needs_asan ()
{
	_arg_case="${1}"

	if test "${_arg_case}" = "fragmented" -o "${_arg_case}" = "mirror_hdrs" -o "${_arg_case}" = "mirror_stop"; then
		return 0
	fi

	return 1
}

sh_needs_option ()
{
	_arg_case="${1}"

	if test "${_arg_case}" = "mirror_url" -o "${_arg_case}" = "mirror_stop"; then
		echo "--mirror-url"
	fi
}

sh_needs_sink ()
{
	_arg_case="${1}"

	if test "${_arg_case}" = "mirror_stop"; then
		return 0
	fi

	return 1
}

sh_agent_options ()
{
	_arg_case="${1}"

	if test "${_arg_case}" = "fragmented" -o "${_arg_case}" = "frag_abort" -o "${_arg_case}" = "frag_interlaced" -o "${_arg_case}" = "frag_cap"; then
		echo "-c fragmentation -c pipelining"
	elif test "${_arg_case}" = "mirror_url"; then
		echo "-c pipelining -u ${SH_MIRROR_URL}"
	elif test "${_arg_case}" = "mirror_stop"; then
		echo "-c pipelining -u http://127.0.0.1:${SH_SINK_PORT}/"
	else
		echo "-c pipelining"
	fi
}

sh_asan_options ()
{
	_arg_case="${1}"

	# The leaks are only looked for where a case is written to show them.
	if test "${_arg_case}" = "mirror_hdrs" -o "${_arg_case}" = "mirror_body" -o "${_arg_case}" = "mirror_url" -o "${_arg_case}" = "mirror_stop"; then
		echo "detect_leaks=1:abort_on_error=0"
	else
		echo "detect_leaks=0:abort_on_error=0"
	fi
}

sh_scan_log ()
{
	_arg_log="${1}"

	grep -m 1 -E "AddressSanitizer|LeakSanitizer|runtime error:" "${_arg_log}" 2>/dev/null | cut -c1-58
}

sh_agent_death ()
{
	# The shell reports a signal as the status 128 plus its number.
	if test "${SH_AGENT_RC}" -gt 128; then
		echo "the program was killed by the signal $((SH_AGENT_RC - 128))"
	elif test "${SH_AGENT_RC}" -ne 0; then
		echo "the program exited with the status ${SH_AGENT_RC}"
	else
		echo "the program stopped before the case was over"
	fi
}

sh_report ()
{
	_arg_result="${1}"
	_arg_case="${2}"
	_arg_detail="${3}"

	printf '  %-4s  %-16s %s\n' "${_arg_result}" "${_arg_case}" "${_arg_detail}"
}

sh_start_sink ()
{
	_arg_log="${1}"

	python3 "${SH_CLIENT}" --sink --port "${SH_SINK_PORT}" > "${_arg_log}" 2>&1 &
	SH_SINK_PID="${!}"
}

sh_stop_sink ()
{
	if test -n "${SH_SINK_PID}"; then
		kill -TERM "${SH_SINK_PID}" 2>/dev/null
		wait "${SH_SINK_PID}" 2>/dev/null
		SH_SINK_PID=
	fi
}

sh_start_agent ()
{
	_arg_port="${1}"
	_arg_log="${2}"
	_arg_case="${3}"

	_var_asan="$(sh_asan_options "${_arg_case}")"

	set -- $(sh_agent_options "${_arg_case}")

	ASAN_OPTIONS="${_var_asan}" \
		"${SH_AGENT}" -r "${SH_ARG_RUNTIME}" -p "${_arg_port}" "${@}" > "${_arg_log}" 2>&1 &
	SH_PID="${!}"
	SH_ALIVE="true"
}

sh_stop_agent ()
{
	if test -z "${SH_PID}"; then
		return
	fi

	# A program that is already gone did not survive the test case.
	if kill -0 "${SH_PID}" 2>/dev/null; then
		kill -TERM "${SH_PID}" 2>/dev/null
	else
		SH_ALIVE=
	fi

	wait "${SH_PID}" 2>/dev/null
	SH_AGENT_RC="${?}"
	SH_PID=
}

sh_run_case ()
{
	_arg_case="${1}"
	_var_log="${SH_TMPDIR}/${_arg_case}.log"
	_var_out="${SH_TMPDIR}/${_arg_case}.out"
	_var_option="$(sh_needs_option "${_arg_case}")"

	if test -n "${_var_option}" && ! sh_have_option "${_var_option}"; then
		SH_SKIPPED=$((SH_SKIPPED + 1))
		sh_report "skip" "${_arg_case}" "the program is built without '${_var_option}'"

		return
	fi

	SH_PORT=$((SH_PORT + 1))
	SH_SINK_PORT=$((SH_PORT + 1000))

	if sh_needs_sink "${_arg_case}"; then
		sh_start_sink "${SH_TMPDIR}/${_arg_case}-sink.log"
	fi

	sh_start_agent "${SH_PORT}" "${_var_log}" "${_arg_case}"

	python3 "${SH_CLIENT}" --port "${SH_PORT}" --mirror-port "${SH_SINK_PORT}" --case "${_arg_case}" > "${_var_out}" 2>&1
	_var_rc="${?}"

	sh_stop_agent
	sh_stop_sink

	_var_detail="$(head -n 1 "${_var_out}")"
	_var_found="$(sh_scan_log "${_var_log}")"

	if test -n "${_var_found}"; then
		_var_rc=1
		_var_detail="${_var_found}"
	elif test "${SH_ALIVE}" != "true"; then
		_var_rc=1
		_var_detail="$(sh_agent_death)"
	fi

	if test "${_var_rc}" -ne 0; then
		SH_FAILED=$((SH_FAILED + 1))
		sh_report "FAIL" "${_arg_case}" "${_var_detail}"
	elif sh_needs_asan "${_arg_case}" && test "${SH_ASAN}" != "true"; then
		SH_SKIPPED=$((SH_SKIPPED + 1))
		sh_report "skip" "${_arg_case}" "answered, the check needs a sanitized build"
	else
		SH_PASSED=$((SH_PASSED + 1))
		sh_report "ok" "${_arg_case}" "${_var_detail}"
	fi

	if test "${SH_OPT_VERBOSE}" = "true"; then
		echo "        port ${SH_PORT}, options: $(sh_agent_options "${_arg_case}")"
		sed -n '2,$p' "${_var_out}" | sed 's/^/        /'
	fi
}

sh_run_argcheck ()
{
	_arg_name="${1}"
	_arg_text="${2}"
	_var_out="${SH_TMPDIR}/argcheck-${_arg_name}.out"

	shift 2

	"${SH_AGENT}" "${@}" > "${_var_out}" 2>&1
	_var_rc="${?}"

	if test "${_var_rc}" -eq 0; then
		SH_FAILED=$((SH_FAILED + 1))
		sh_report "FAIL" "argcheck" "'${_arg_name}' was accepted"
	elif grep -q "${_arg_text}" "${_var_out}"; then
		SH_PASSED=$((SH_PASSED + 1))
		sh_report "ok" "argcheck" "'${_arg_name}' refused: ${_arg_text}"
	else
		SH_FAILED=$((SH_FAILED + 1))
		sh_report "FAIL" "argcheck" "'${_arg_name}' refused without the expected message"
	fi
}

sh_run_optcheck ()
{
	_arg_option="${1}"
	_arg_name="${2}"
	_arg_text="${3}"

	shift 3

	if sh_have_option "${_arg_option}"; then
		sh_run_argcheck "${_arg_name}" "${_arg_text}" "${@}"
	else
		SH_SKIPPED=$((SH_SKIPPED + 1))
		sh_report "skip" "argcheck" "'${_arg_name}' needs '${_arg_option}'"
	fi
}

sh_run_argchecks ()
{
	sh_run_argcheck "runtime"    "runtime value not set"        -p 1
	sh_run_argcheck "backlog"    "invalid connection backlog"   -r 1s -b 0
	sh_run_argcheck "workers"    "invalid number of workers"    -r 1s -n 0
	sh_run_argcheck "port"       "invalid port"                 -r 1s -p 0
	sh_run_argcheck "port-max"   "invalid port"                 -r 1s -p 70000
	sh_run_argcheck "frame-min"  "max-frame-size not in range"  -r 1s -m 128
	sh_run_argcheck "frame-max"  "max-frame-size not in range"  -r 1s -m 2097152
	sh_run_argcheck "capability" "unsupported capability"       -r 1s -c bogus
	sh_run_argcheck "backend"    "invalid libev backend"        -r 1s -B bogus
	sh_run_argcheck "delay"      "invalid time format"          -r 1s -t bogus
	sh_run_argcheck "logfile"    "logfile name not defined"     -r 1s -l ""
	sh_run_argcheck "logmode"    "invalid logfile mode"         -r 1s -l "x:/dev/null"

	sh_run_optcheck "--mirror-url"        "url"        "Invalid URL scheme"        -r 1s -u "ftp://127.0.0.1/"
	sh_run_optcheck "--mirror-local-port" "port-range" "invalid port range"        -r 1s -P "10-5"
	sh_run_optcheck "--mirror-local-port" "port-empty" "port range is not defined" -r 1s -P ""
}


while getopts a:p:t:klvh c; do
	case "${c}" in
	  a)	SH_ARG_AGENT="${OPTARG}" ;;
	  p)	SH_ARG_PORT="${OPTARG}" ;;
	  t)	SH_ARG_RUNTIME="${OPTARG}" ;;
	  k)	SH_OPT_KEEP="true" ;;
	  l)	SH_OPT_LIST="true" ;;
	  v)	SH_OPT_VERBOSE="true" ;;
	  h)	sh_usage; exit "${SH_EX_OK}" ;;
	  \?)	sh_usage; exit "${SH_EX_USAGE}" ;;
	esac
done

shift $((OPTIND - 1))

if test "${SH_OPT_LIST}" = "true"; then
	sh_list

	exit "${SH_EX_OK}"
fi

if test -n "${SH_ARG_AGENT}"; then
	SH_AGENT="${SH_ARG_AGENT}"
else
	SH_AGENT="$(sh_find_agent)"
fi

if test -z "${SH_AGENT}" -o ! -x "${SH_AGENT}"; then
	echo "ERROR: the spoa-mirror program is not found, use the '-a' option"

	exit "${SH_EX_NOINPUT}"
fi

if test ! -r "${SH_CLIENT}"; then
	echo "ERROR: the SPOP client ${SH_CLIENT} is not found"

	exit "${SH_EX_NOINPUT}"
fi

if ! command -v python3 > /dev/null 2>&1; then
	echo "ERROR: python3 is needed to run the SPOP client"

	exit "${SH_EX_UNAVAILABLE}"
fi

for _loop_case in "${@}"; do
	if ! sh_known_case "${_loop_case}"; then
		echo "ERROR: '${_loop_case}' is not a test case, use the '-l' option"

		exit "${SH_EX_USAGE}"
	fi
done

trap sh_cleanup EXIT
trap sh_interrupt HUP INT TERM

# Raised before every case, so the first case runs on the given port.
SH_PORT=$((SH_ARG_PORT - 1))
SH_TMPDIR="$(mktemp -d)"
SH_HELP="$("${SH_AGENT}" -h 2>&1)"

if sh_have_asan "${SH_AGENT}"; then
	SH_ASAN="true"
fi

echo
echo "Testing ${SH_AGENT}"
if test "${SH_ASAN}" = "true"; then
	echo "The program is built with the address sanitizer."
else
	echo "The program is built without the address sanitizer."
fi
if test "${SH_OPT_VERBOSE}" = "true"; then
	echo "The logs are written to ${SH_TMPDIR}"
fi
echo

if test "${#}" -gt 0; then
	for _loop_case in "${@}"; do
		if test "${_loop_case}" = "argcheck"; then
			sh_run_argchecks
		else
			sh_run_case "${_loop_case}"
		fi
	done
else
	sh_run_argchecks
	for _loop_case in ${SH_CASES}; do
		sh_run_case "${_loop_case}"
	done
fi

echo
echo "${SH_PASSED} passed, ${SH_FAILED} failed, ${SH_SKIPPED} skipped"

if test "${SH_FAILED}" -gt 0; then
	echo "The logs of the program and of the client are kept in ${SH_TMPDIR}"
	echo

	exit "${SH_EX_SOFTWARE}"
fi

echo

exit "${SH_EX_OK}"
