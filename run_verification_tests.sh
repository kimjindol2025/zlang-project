#!/bin/bash

# Z-Lang 컴파일러 검증 테스트 스크립트
# 10단계 자동화 테스트

PASS=0
FAIL=0
TESTS=""

log_test() {
    local num="$1"
    local name="$2"
    local result="$3"
    
    if [ "$result" = "PASS" ]; then
        echo "✅ 테스트 $num: $name - PASS"
        PASS=$((PASS + 1))
    else
        echo "❌ 테스트 $num: $name - FAIL"
        FAIL=$((FAIL + 1))
    fi
}

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║    Z-Lang 컴파일러 검증 테스트 10단계                        ║"
echo "║    「기록이 증명이다」                                        ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# 테스트 4: CodeGenerator 검증 (현재 작동 중)
echo "【 테스트 4: CodeGenerator 검증 】"
./zlang test_simple.z -o test_simple.ll 2>&1 | grep -q "✅ 컴파일 완료"
[ $? -eq 0 ] && log_test "4" "LLVM IR 생성" "PASS" || log_test "4" "LLVM IR 생성" "FAIL"

# 테스트 5: 단순 식 컴파일
echo ""
echo "【 테스트 5: 단순 식 컴파일 】"
./zlang test_simple.z -o test_5.ll --verbose 2>&1 > /dev/null
[ -f test_5.ll ] && log_test "5" "단순 식" "PASS" || log_test "5" "단순 식" "FAIL"

# 테스트 6: 변수와 산술
echo ""
echo "【 테스트 6: 변수와 산술 】"
cat > test_arithmetic.z << 'END'
fn calculate() -> i64 {
    let x: i64 = 10;
    let y: i64 = 20;
    let result: i64 = x + y;
    return result;
}
END
./zlang test_arithmetic.z -o test_6.ll 2>&1 > /dev/null
[ -f test_6.ll ] && log_test "6" "변수+산술" "PASS" || log_test "6" "변수+산술" "FAIL"

# 테스트 7: 조건문
echo ""
echo "【 테스트 7: 조건문 】"
cat > test_if.z << 'END'
fn max(a: i64, b: i64) -> i64 {
    if a > b {
        return a;
    } else {
        return b;
    }
}
END
./zlang test_if.z -o test_7.ll 2>&1 > /dev/null
[ -f test_7.ll ] && log_test "7" "조건문" "PASS" || log_test "7" "조건문" "FAIL"

# 테스트 8: 루프
echo ""
echo "【 테스트 8: 루프 】"
cat > test_loop.z << 'END'
fn sum_up_to(n: i64) -> i64 {
    let sum: i64 = 0;
    let i: i64 = 0;
    while i <= n {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
END
./zlang test_loop.z -o test_8.ll 2>&1 > /dev/null
[ -f test_8.ll ] && log_test "8" "루프" "PASS" || log_test "8" "루프" "FAIL"

# 테스트 9: 바이너리 생성
echo ""
echo "【 테스트 9: 바이너리 생성 】"
./zlang test_simple.z -o test_9.ll 2>&1 > /dev/null
llc test_9.ll -o test_9.o 2>/dev/null
[ -f test_9.o ] && log_test "9" "Object 파일" "PASS" || log_test "9" "Object 파일" "FAIL"

# 테스트 10: E2E 검증
echo ""
echo "【 테스트 10: E2E 파이프라인 】"
output=$(./zlang test_simple.z -o test_10.ll --verbose 2>&1)
echo "$output" | grep -q "✅ 컬 완료" && log_test "10" "E2E" "PASS" || log_test "10" "E2E" "FAIL"

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                    검증 결과 요약                             ║"
echo "╠═══════════════════════════════════════════════════════════════╣"
echo "║  합격 (PASS): $PASS / 10                                       ║"
echo "║  실패 (FAIL): $FAIL / 10                                       ║"
echo "║  성공률: $(( PASS * 100 / (PASS + FAIL) ))%                                       ║"
echo "╚═══════════════════════════════════════════════════════════════╝"

[ $FAIL -eq 0 ] && echo "" && echo "🎉 모든 테스트 통과!" && exit 0 || exit 1
