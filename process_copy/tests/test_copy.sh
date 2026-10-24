#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root/mod"
gcc -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror copy.c -I../include -o copy
cd "$root/src"
gcc -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror ./*.c -I../include -o process_copy

testdir=$(mktemp -d)
trap 'if [[ -n "${testdir:-}" && -d "$testdir" && "$testdir" == /tmp/tmp.* ]]; then rm -rf -- "$testdir"; fi' EXIT

: > "$testdir/empty"
"$root/src/process_copy" "$testdir/empty" "$testdir/empty-out"
cmp "$testdir/empty" "$testdir/empty-out"

printf 'hello world' > "$testdir/small"
"$root/src/process_copy" "$testdir/small" "$testdir/small-out"
cmp "$testdir/small" "$testdir/small-out"

head -c 103 /dev/urandom > "$testdir/uneven"
"$root/src/process_copy" "$testdir/uneven" "$testdir/uneven-out" 5
cmp "$testdir/uneven" "$testdir/uneven-out"
"$root/src/process_copy" "$testdir/uneven" "$testdir/many-out" 100
cmp "$testdir/uneven" "$testdir/many-out"

head -c 2097152 /dev/urandom > "$testdir/large"
printf '%4096s' x > "$testdir/large-out"
"$root/src/process_copy" "$testdir/large" "$testdir/large-out" 7
cmp "$testdir/large" "$testdir/large-out"

if "$root/src/process_copy" "$testdir/missing" "$testdir/out" 2 >/dev/null 2>&1; then
    echo '不存在的源文件应被拒绝' >&2
    exit 1
fi
if "$root/src/process_copy" "$testdir/small" "$testdir/out" 0 >/dev/null 2>&1; then
    echo '进程数 0 应被拒绝' >&2
    exit 1
fi
if "$root/src/process_copy" "$testdir/small" "$testdir/out" 101 >/dev/null 2>&1; then
    echo '进程数 101 应被拒绝' >&2
    exit 1
fi
if "$root/src/process_copy" "$testdir/small" "$testdir/out" abc >/dev/null 2>&1; then
    echo '非数字进程数应被拒绝' >&2
    exit 1
fi
if "$root/src/process_copy" "$testdir/small" "$testdir/small" 2 >/dev/null 2>&1; then
    echo '源文件与目标文件相同时应被拒绝' >&2
    exit 1
fi
cmp "$testdir/small" "$testdir/small-out"

echo '所有测试通过'
