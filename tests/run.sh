#!/bin/bash
# SysY 编译器回归测试脚本
# 用法: tests/run.sh [case_name...]   (无参数则跑全部)
# 每个用例: tests/cases/NAME.sy 必需; NAME.in (stdin) 与 NAME.out (预期stdout) 可选
# 判定: 编译器正常退出(0) + IR 可编译 + 运行输出与 NAME.out 一致(若有)

set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CASES="$ROOT/tests/cases"
COMPILER="$ROOT/build/Compiler"
RUNTIME="$ROOT/tests/runtime.c"
WORK="$ROOT/tests/.work"
mkdir -p "$WORK"

pass=0; fail=0; failed_names=()

run_case() {
    local name="$1"
    local sy="$CASES/$name.sy"
    cp "$sy" "$ROOT/testfile.txt"
    # 1) 运行编译器(10s 超时, 防死循环)
    (cd "$ROOT" && timeout 10 ./build/Compiler > "$WORK/$name.compile.log" 2>&1)
    local ec=$?
    if [ $ec -ne 0 ]; then
        echo "[FAIL] $name: compiler exit=$ec (124=timeout/hang, 139=segfault)"
        fail=$((fail+1)); failed_names+=("$name"); return
    fi
    # 2) 语义错误用例: 存在 NAME.err 时比对 error.txt 后返回(不再跑 IR)
    if [ -f "$CASES/$name.err" ]; then
        local expected_err actual_err
        expected_err=$(cat "$CASES/$name.err")
        actual_err=$(cat "$ROOT/error.txt" 2>/dev/null)
        if [ "$actual_err" == "$expected_err" ]; then
            echo "[PASS] $name (error.txt matched)"
            pass=$((pass+1))
        else
            echo "[FAIL] $name: error.txt mismatch"
            echo "  expected: $expected_err"
            echo "  actual:   $actual_err"
            fail=$((fail+1)); failed_names+=("$name")
        fi
        return
    fi
    # 3) 正常用例不应有语义错误
    if [ -s "$ROOT/error.txt" ]; then
        echo "[FAIL] $name: unexpected error.txt: $(tr '\n' ';' < "$ROOT/error.txt")"
        fail=$((fail+1)); failed_names+=("$name"); return
    fi
    clang -w -x ir "$ROOT/llvm_ir.txt" -x none "$RUNTIME" -o "$WORK/$name.bin" 2> "$WORK/$name.clang.log"
    if [ $? -ne 0 ]; then
        echo "[FAIL] $name: IR not compilable: $(head -3 "$WORK/$name.clang.log")"
        fail=$((fail+1)); failed_names+=("$name"); return
    fi
    local actual
    if [ -f "$CASES/$name.in" ]; then
        actual=$(timeout 5 "$WORK/$name.bin" < "$CASES/$name.in" 2>&1)
    else
        actual=$(timeout 5 "$WORK/$name.bin" 2>&1)
    fi
    # 4) 比对输出(若有预期文件)
    if [ -f "$CASES/$name.out" ]; then
        local expected
        expected=$(cat "$CASES/$name.out")
        if [ "$actual" == "$expected" ]; then
            echo "[PASS] $name"
            pass=$((pass+1))
        else
            echo "[FAIL] $name: output mismatch"
            echo "  expected: $(echo "$expected" | head -3)"
            echo "  actual:   $(echo "$actual" | head -3)"
            fail=$((fail+1)); failed_names+=("$name")
        fi
    else
        echo "[PASS] $name (ran, output: $(echo "$actual" | head -1 | cut -c1-60))"
        pass=$((pass+1))
    fi
}

if [ $# -gt 0 ]; then
    for n in "$@"; do run_case "$n"; done
else
    for sy in "$CASES"/*.sy; do
        [ -e "$sy" ] || continue
        run_case "$(basename "$sy" .sy)"
    done
fi

echo "==============================="
echo "PASS: $pass  FAIL: $fail"
[ $fail -gt 0 ] && echo "failed: ${failed_names[*]}"
exit $fail
