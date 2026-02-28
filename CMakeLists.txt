cmake_minimum_required(VERSION 3.10)
project(zlang VERSION 0.1.0 LANGUAGES CXX)

# ============================================================================
# 【 Z-Lang LLVM 컴파일러 빌드 설정 】
#
# 목표: Z-Lang 소스코드 → LLVM IR → 실행 파일로의 완전한 컴파일 파이프라인
# ============================================================================

# C++ 17 표준 요구
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Wextra")

# ============================================================================
# LLVM 설정
# ============================================================================

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# LLVM 라이브러리 목록
llvm_map_components_to_libnames(llvm_libs core scalarOpts instCombine analysis)

# ============================================================================
# 소스 파일 수집
# ============================================================================

# Lexer (2.1 단계)
set(LEXER_SOURCES
    # src/lexer/Lexer.cpp  # 아직 구현 예정
)

# Parser (2.2 단계)
set(PARSER_SOURCES
    # src/parser/Parser.cpp  # 아직 구현 예정
)

# Semantic Analysis (2.3 단계)
set(SEMANTIC_SOURCES
    # src/semantic/TypeChecker.cpp  # 아직 구현 예정
)

# Code Generation (2.4 단계)
set(CODEGEN_SOURCES
    src/codegen/CodeGenerator.cpp
)

# 메인 드라이버
set(MAIN_SOURCES
    src/main.cpp
)

# 전체 소스 결합
set(ZLANG_SOURCES
    ${LEXER_SOURCES}
    ${PARSER_SOURCES}
    ${SEMANTIC_SOURCES}
    ${CODEGEN_SOURCES}
    ${MAIN_SOURCES}
)

# ============================================================================
# 실행 파일 생성
# ============================================================================

add_executable(zlang ${ZLANG_SOURCES})

# LLVM 라이브러리 링크
target_link_libraries(zlang ${llvm_libs})

# Include 경로 설정
target_include_directories(zlang PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${LLVM_INCLUDE_DIRS}
)

# ============================================================================
# 테스트 설정
# ============================================================================

enable_testing()

# Type Mapping 테스트
add_executable(test_type_mapping tests/TypeMappingTest.cpp ${CODEGEN_SOURCES})
target_link_libraries(test_type_mapping ${llvm_libs})
target_include_directories(test_type_mapping PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${LLVM_INCLUDE_DIRS}
)

add_test(NAME TypeMappingTest COMMAND test_type_mapping)

# ============================================================================
# 설치 설정
# ============================================================================

install(TARGETS zlang
    RUNTIME DESTINATION bin
)

# ============================================================================
# 빌드 정보 출력
# ============================================================================

message(STATUS "")
message(STATUS "【 Z-Lang 컴파일러 빌드 설정 】")
message(STATUS "")
message(STATUS "LLVM Version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM Libs: ${llvm_libs}")
message(STATUS "LLVM Include: ${LLVM_INCLUDE_DIRS}")
message(STATUS "")
message(STATUS "빌드 대상:")
message(STATUS "  - zlang (메인 컴파일러)")
message(STATUS "  - test_type_mapping (타입 검증)")
message(STATUS "")
message(STATUS "사용법:")
message(STATUS "  mkdir build && cd build")
message(STATUS "  cmake ..")
message(STATUS "  make")
message(STATUS "  ./zlang test.z -o test.ll")
message(STATUS "")
