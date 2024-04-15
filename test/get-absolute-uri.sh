#!/bin/sh -u
#
echo "\
GET http://1.2.3.4:12345/index.html HTTP/1.1
Host: 1.2.3.4:12345
user-agent: netcat/1.2.3.4
accept: */*
connection: close
" | nc 127.0.0.1 10080
