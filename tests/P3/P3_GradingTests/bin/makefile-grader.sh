#!/bin/bash

set -euo pipefail
#set -x

die() {
	echo "$1"
	exit 1
}

check_exist() {
	[ -f "$mkpath" ] && total=$((total+1))
}

check_variables() {
	grep -q "CC=" "$mkpath" && total=$((total+1))
	grep -q "CFLAGS=" "$mkpath" && total=$((total+1))
	grep -q "LOGIN=" "$mkpath" && total=$((total+1))
	grep -q "SUBMITPATH=" "$mkpath" && total=$((total+1))
	grep -q '$@' "$mkpath" && total=$((total+1))
	grep -q '$<' "$mkpath" && total=$((total+1))
	grep -q '$^' "$mkpath" && total=$((total+1))
	true
}

check_targets() {
	grep -q 'all:' "$mkpath" && total=$((total+1))
	grep -q 'wsh:' "$mkpath" && total=$((total+1))
	grep -q '.PHONY' "$mkpath" && total=$((total+1))
	grep -q 'run:' "$mkpath" && total=$((total+1))
	grep -q 'pack:' "$mkpath" && total=$((total+1))
	grep -q 'tar.gz' "$mkpath" && total=$((total+1))
	grep -q 'submit' "$mkpath" && total=$((total+1))
	true
}

[ $# -ne 1 ] && die "Usage: $0 <path to Makefile>"

mkpath="$1"
total=0

check_exist || die "$total"
check_variables
check_targets

echo "$total"
