# AIAccel Dialect CMake Configuration

project(AIAccel LANGUAGES CXX C)

# MLIR 요구사항
find_package(MLIR REQUIRED CONFIG)

# 포함 디렉토리
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${MLIR_INCLUDE_DIRS})

# TableGen 파일 생성
set(AIACCCEL_TABLEGEN_FILES
  dialect/AIAccel.td
)

# TableGen 처리
mlir_tablegen_on_demand(AIAccelOps
  dialect/AIAccel.td
  -gen-op-defs)

mlir_tablegen_on_demand(AIAccelDialect
  dialect/AIAccel.td
  -gen-dialect-defs)

# AIAccel 라이브러리
add_library(AIAccelDialect
  dialect/AIAccelOps.cpp
  passes/FusionPass.cpp
  passes/BufferizationPass.cpp
  passes/LoopTilingPass.cpp
  passes/BackendPass.cpp
  passes/IntegrationPass.cpp
  ${AIACCCEL_TABLEGEN_FILES}
)

target_include_directories(AIAccelDialect PUBLIC
  ${MLIR_INCLUDE_DIRS}
)

target_link_libraries(AIAccelDialect PUBLIC
  MLIRIR
  MLIRSupport
  MLIROptLib
)

# 테스트
enable_testing()

add_test(
  NAME AIAccelTest
  COMMAND mlir-opt -allow-unregistered-dialect test/AIAccel_test.mlir -verify-diagnostics
)

add_test(
  NAME FusionPassTest
  COMMAND mlir-opt -allow-unregistered-dialect -aiacccel-fusion test/FusionPass_test.mlir -verify-diagnostics
)

add_test(
  NAME BufferizationPassTest
  COMMAND mlir-opt -allow-unregistered-dialect -aiacccel-bufferization test/BufferizationPass_test.mlir -verify-diagnostics
)

add_test(
  NAME LoopTilingPassTest
  COMMAND mlir-opt -allow-unregistered-dialect -aiacccel-loop-tiling test/LoopTiling_test.mlir -verify-diagnostics
)

add_test(
  NAME BackendPassTest
  COMMAND mlir-opt -allow-unregistered-dialect -aiacccel-backend test/Backend_test.mlir -verify-diagnostics
)

add_test(
  NAME IntegrationPassTest
  COMMAND mlir-opt -allow-unregistered-dialect -aiacccel-integration test/Integration_test.mlir -verify-diagnostics
)

