#!/bin/bash

# Z-Lang LLVM 컴파일러 빌드 스크립트 (개선 버전 v2.0)
# 지원: CMake + 직접 컴파일

set -e

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 설정값
BUILD_DIR="build"
BUILD_TYPE="Release"
USE_CMAKE=true
JOBS=$(nproc)
VERBOSE=false

# 함수: 도움말
print_help() {
    echo "Z-Lang LLVM 컴파일러 빌드 스크립트 v2.0"
    echo ""
    echo "사용법: $0 [옵션]"
    echo ""
    echo "옵션:"
    echo "  -d, --debug            Debug 빌드 (Release 권장)"
    echo "  -j, --jobs N           병렬 컴파일 개수 (기본값: CPU 코어)"
    echo "  -c, --cmake            CMake 사용 (권장)"
    echo "  -g, --gcc              GCC 직접 컴파일 (간단함)"
    echo "  --clean                빌드 디렉토리 초기화"
    echo "  -v, --verbose          상세 로그"
    echo "  -h, --help             도움말"
}

log_info() { echo -e "${GREEN}✅ $1${NC}"; }
log_warn() { echo -e "${YELLOW}⚠️  $1${NC}"; }
log_error() { echo -e "${RED}❌ $1${NC}"; }
log_debug() { [ "$VERBOSE" = true ] && echo -e "${BLUE}🔧 $1${NC}"; }

# 함수: 의존성 확인
check_deps() {
    log_info "의존성 확인 중..."

    if ! command -v g++ &> /dev/null; then
        log_error "g++가 설치되어 있지 않습니다. sudo apt-get install build-essential"
        exit 1
    fi

    if ! command -v llvm-config &> /dev/null; then
        log_error "llvm-config가 없습니다. sudo apt-get install llvm-dev"
        exit 1
    fi

    log_info "g++: $(g++ --version | head -1)"
    log_info "LLVM: $(llvm-config --version)"
}

# 함수: CMake 빌드
build_with_cmake() {
    if ! command -v cmake &> /dev/null; then
        log_warn "CMake가 없어 GCC 직접 컴파일로 전환합니다"
        build_with_gcc
        return
    fi

    log_info "CMake를 이용한 빌드 시작"

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        $([ "$VERBOSE" = true ] && echo "-DCMAKE_VERBOSE_MAKEFILE=ON" || true)

    cmake --build . -- -j "$JOBS"
    cd ..

    log_info "CMake 빌드 완료"
}

# 함수: GCC 직접 컴파일
build_with_gcc() {
    log_info "GCC를 이용한 빌드 시작"

    LLVM_CONFIG="llvm-config"
    LLVM_FLAGS=$($LLVM_CONFIG --cxxflags --ldflags --libs core irreader)

    SOURCES="src/main.cpp \
             src/lexer/Lexer.cpp \
             src/parser/Parser.cpp \
             src/codegen/CodeGenerator.cpp \
             src/codegen/BackendCompiler.cpp \
             src/analysis/WCETAnalyzer.cpp"

    CPPFLAGS="-std=c++17 -fPIC -Wall -Wextra"
    if [ "$BUILD_TYPE" = "Debug" ]; then
        CPPFLAGS="$CPPFLAGS -g -O0"
    else
        CPPFLAGS="$CPPFLAGS -O2"
    fi

    log_debug "Compilation: g++ $CPPFLAGS -o zlang $SOURCES $LLVM_FLAGS"

    g++ $CPPFLAGS -o zlang $SOURCES $LLVM_FLAGS 2>&1

    log_info "GCC 빌드 완료"
}

# 함수: 테스트
run_tests() {
    log_info "테스트 시작"

    if [ -f "zlang" ]; then
        # 기본 테스트
        ./zlang --help > /dev/null 2>&1 && \
            log_info "테스트 통과: --help 작동"

        # WCET 테스트
        if [ -f "test_wcet.z" ]; then
            ./zlang test_wcet.z -o /tmp/test_wcet.ll > /dev/null 2>&1 && \
                log_info "테스트 통과: WCET 분석 작동"
        fi
    fi
}

# 메인 로직
main() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -d|--debug) BUILD_TYPE="Debug"; shift ;;
            -j|--jobs) JOBS="$2"; shift 2 ;;
            -c|--cmake) USE_CMAKE=true; shift ;;
            -g|--gcc) USE_CMAKE=false; shift ;;
            --clean) rm -rf "$BUILD_DIR" zlang; shift ;;
            -v|--verbose) VERBOSE=true; shift ;;
            -h|--help) print_help; exit 0 ;;
            *) log_error "알 수 없는 옵션: $1"; print_help; exit 1 ;;
        esac
    done

    echo "╔════════════════════════════════════════════════════════════╗"
    echo "║      Z-Lang LLVM 컴파일러 빌드 스크립트 v2.0             ║"
    echo "║         「기록이 증명이다」- 전설의 컴파일러             ║"
    echo "╚════════════════════════════════════════════════════════════╝"
    echo ""

    check_deps

    if [ "$USE_CMAKE" = true ]; then
        build_with_cmake
    else
        build_with_gcc
    fi

    # 최종 확인
    if [ -f "zlang" ]; then
        log_info "빌드 성공! 바이너리: $(pwd)/zlang"
        ls -lh zlang
        echo ""
        echo "사용법: ./zlang <source.z> [옵션]"
        echo "예제: ./zlang test_wcet.z -o output.ll --verbose"
    else
        log_error "빌드 실패"
        exit 1
    fi
}

main "$@"
