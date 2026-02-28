#!/bin/bash

# Z-Lang 설치 스크립트

set -e

# 색상
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${GREEN}✅ $1${NC}"; }
log_error() { echo -e "${RED}❌ $1${NC}"; }
log_warn() { echo -e "${YELLOW}⚠️  $1${NC}"; }

echo "╔════════════════════════════════════════════════════════════╗"
echo "║          Z-Lang LLVM 컴파일러 설치 스크립트               ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# 1. 의존성 설치
log_info "Step 1: 의존성 설치"
echo "다음 패키지를 설치해야 합니다:"
echo "  - build-essential (g++, make)"
echo "  - cmake (권장)"
echo "  - llvm-dev (LLVM 라이브러리)"

if [ -f /etc/debian_version ]; then
    log_info "Ubuntu/Debian 감지"
    echo "설치: sudo apt-get install -y build-essential cmake llvm-dev"
    if command -v apt-get &> /dev/null; then
        read -p "지금 설치하시겠습니까? (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            sudo apt-get update
            sudo apt-get install -y build-essential cmake llvm-dev
            log_info "의존성 설치 완료"
        fi
    fi
elif [ -f /etc/redhat-release ]; then
    log_warn "RedHat/CentOS 감지"
    echo "설치: sudo dnf install -y gcc-c++ cmake llvm-devel"
fi

# 2. 빌드
log_info "Step 2: 빌드 시작"
if [ ! -x "./build.sh" ]; then
    chmod +x build.sh
fi

./build.sh --jobs $(nproc)

if [ ! -f "zlang" ]; then
    log_error "빌드 실패"
    exit 1
fi

log_info "빌드 완료"

# 3. 설치
log_info "Step 3: 시스템 설치"

INSTALL_PATH="/usr/local/bin/zlang"

if [ -w /usr/local/bin ]; then
    # sudo 없이 설치 가능
    cp zlang "$INSTALL_PATH"
    chmod +x "$INSTALL_PATH"
    log_info "설치 완료: $INSTALL_PATH"
else
    # sudo 필요
    sudo cp zlang "$INSTALL_PATH"
    sudo chmod +x "$INSTALL_PATH"
    log_info "설치 완료: $INSTALL_PATH (sudo 사용)"
fi

# 4. 검증
log_info "Step 4: 검증"

which zlang > /dev/null 2>&1 && \
    log_info "✓ zlang 명령어 확인 완료" || \
    log_warn "zlang 명령어 경로가 설정되지 않았습니다"

zlang --help > /dev/null 2>&1 && \
    log_info "✓ zlang --help 동작 확인" || \
    log_warn "zlang이 정상적으로 실행되지 않습니다"

# 5. 최종 정보
echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║              설치 완료! 🎉                                ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "사용법:"
echo "  zlang <source.z> [옵션]"
echo ""
echo "예제:"
echo "  zlang test.z -o output.ll"
echo "  zlang test.z -o output.ll --verbose"
echo "  zlang fibonacci.z --emit-ir --execute"
echo ""
echo "도움말:"
echo "  zlang --help"
echo ""
echo "다음 단계:"
echo "  1. 예제 파일 실행: zlang test_wcet.z -o output.ll"
echo "  2. LLVM IR 확인: cat output.ll"
echo "  3. 설명서 읽기: cat README.md"
