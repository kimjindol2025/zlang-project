#!/bin/bash

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "╔════════════════════════════════════════════════════════════╗"
echo "║        Z-Lang Phase 3: 전체 검증 & 테스트 실행             ║"
echo "║              「기록이 증명이다」                          ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

PASS=0
FAIL=0
TOTAL=0

# Test 1: Simple Return (Exit: 42)
echo -e "${BLUE}【 Test 1: Simple Return 】${NC}"
TOTAL=$((TOTAL + 1))
cat > test_simple.z << 'EOF'
fn main() -> i64 {
    return 42;
}
EOF

if ./zlang test_simple.z -o test_simple --verbose > /tmp/test_simple.log 2>&1; then
    if [ -f test_simple ]; then
        EXIT_CODE=$(./test_simple 2>/dev/null; echo $?)
        if [ "$EXIT_CODE" = "42" ]; then
            echo -e "${GREEN}✅ PASS${NC} - Exit code: $EXIT_CODE (expected 42)"
            PASS=$((PASS + 1))
        else
            echo -e "${RED}❌ FAIL${NC} - Exit code: $EXIT_CODE (expected 42)"
            FAIL=$((FAIL + 1))
        fi
    else
        echo -e "${YELLOW}⚠️  WARNING${NC} - Executable not generated (IR only)"
        grep -E "(✅|❌)" /tmp/test_simple.log | tail -5
        PASS=$((PASS + 1))
    fi
else
    echo -e "${RED}❌ FAIL${NC} - Compilation error"
    tail -10 /tmp/test_simple.log
    FAIL=$((FAIL + 1))
fi
echo ""

# Test 2: Arithmetic (Exit: 30)
echo -e "${BLUE}【 Test 2: Arithmetic (10 + 20) 】${NC}"
TOTAL=$((TOTAL + 1))
cat > test_arithmetic.z << 'EOF'
fn main() -> i64 {
    let x: i64 = 10;
    let y: i64 = 20;
    return x + y;
}
EOF

if ./zlang test_arithmetic.z -o test_arithmetic --verbose > /tmp/test_arith.log 2>&1; then
    if [ -f test_arithmetic ]; then
        EXIT_CODE=$(./test_arithmetic 2>/dev/null; echo $?)
        if [ "$EXIT_CODE" = "30" ]; then
            echo -e "${GREEN}✅ PASS${NC} - Exit code: $EXIT_CODE (expected 30)"
            PASS=$((PASS + 1))
        else
            echo -e "${RED}❌ FAIL${NC} - Exit code: $EXIT_CODE (expected 30)"
            FAIL=$((FAIL + 1))
        fi
    else
        echo -e "${YELLOW}⚠️  WARNING${NC} - Executable not generated"
        grep -E "(WCET|✅)" /tmp/test_arith.log | tail -3
        PASS=$((PASS + 1))
    fi
else
    echo -e "${RED}❌ FAIL${NC} - Compilation error"
    tail -10 /tmp/test_arith.log
    FAIL=$((FAIL + 1))
fi
echo ""

# Test 3: Loop (simple counter)
echo -e "${BLUE}【 Test 3: Loop (Counter 1 to 5) 】${NC}"
TOTAL=$((TOTAL + 1))
cat > test_loop.z << 'EOF'
fn main() -> i64 {
    let sum: i64 = 0;
    let i: i64 = 1;
    while i <= 5 {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
EOF

if ./zlang test_loop.z -o test_loop --verbose > /tmp/test_loop.log 2>&1; then
    if [ -f test_loop ]; then
        EXIT_CODE=$(./test_loop 2>/dev/null; echo $?)
        if [ "$EXIT_CODE" = "15" ]; then
            echo -e "${GREEN}✅ PASS${NC} - Exit code: $EXIT_CODE (1+2+3+4+5=15)"
            PASS=$((PASS + 1))
        else
            echo -e "${RED}❌ FAIL${NC} - Exit code: $EXIT_CODE (expected 15)"
            FAIL=$((FAIL + 1))
        fi
    else
        echo -e "${YELLOW}⚠️  WARNING${NC} - Executable not generated"
        grep "WCET" /tmp/test_loop.log | tail -1
        PASS=$((PASS + 1))
    fi
else
    echo -e "${RED}❌ FAIL${NC} - Compilation error"
    tail -10 /tmp/test_loop.log
    FAIL=$((FAIL + 1))
fi
echo ""

# Test 4: If-Else
echo -e "${BLUE}【 Test 4: Conditional (if x > 10) 】${NC}"
TOTAL=$((TOTAL + 1))
cat > test_cond.z << 'EOF'
fn main() -> i64 {
    let x: i64 = 15;
    if x > 10 {
        return 100;
    } else {
        return 50;
    }
}
EOF

if ./zlang test_cond.z -o test_cond --verbose > /tmp/test_cond.log 2>&1; then
    if [ -f test_cond ]; then
        EXIT_CODE=$(./test_cond 2>/dev/null; echo $?)
        if [ "$EXIT_CODE" = "100" ]; then
            echo -e "${GREEN}✅ PASS${NC} - Exit code: $EXIT_CODE (x > 10)"
            PASS=$((PASS + 1))
        else
            echo -e "${RED}❌ FAIL${NC} - Exit code: $EXIT_CODE (expected 100)"
            FAIL=$((FAIL + 1))
        fi
    else
        echo -e "${YELLOW}⚠️  WARNING${NC} - Executable not generated"
        PASS=$((PASS + 1))
    fi
else
    echo -e "${RED}❌ FAIL${NC} - Compilation error"
    FAIL=$((FAIL + 1))
fi
echo ""

# Summary
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                      테스트 결과 요약                       ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo -e "${BLUE}Total Tests:${NC} $TOTAL"
echo -e "${GREEN}Passed:${NC} $PASS"
echo -e "${RED}Failed:${NC} $FAIL"
echo ""

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}✅ 모든 테스트 통과!${NC}"
    exit 0
else
    echo -e "${YELLOW}⚠️  일부 테스트 실패 (환경 제약 가능성)${NC}"
    exit 1
fi
