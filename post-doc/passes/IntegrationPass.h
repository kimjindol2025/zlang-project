//===- IntegrationPass.h - Integrated Compiler Pipeline ---*- C++ -*-===//

#ifndef AIACCCEL_INTEGRATION_PASS_H
#define AIACCCEL_INTEGRATION_PASS_H

#include "mlir/Pass/Pass.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/PatternMatch.h"
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace mlir {
namespace aiacccel {

//===----------------------------------------------------------------------===//
// Compiler Pipeline Stages
//===----------------------------------------------------------------------===//

enum class OptimizationLevel {
  O0,  // No optimization
  O1,  // Basic (Fusion)
  O2,  // Intermediate (Fusion + Bufferization)
  O3,  // Aggressive (All passes)
  Os   // Size optimization
};

struct PipelineStats {
  std::string stageName;
  int64_t inputOps;           // 입력 연산자 수
  int64_t outputOps;          // 출력 연산자 수
  double executionTimeMs;     // 이 단계 실행 시간
  std::string optimizations;  // 적용된 최적화
};

struct CompilerMetrics {
  // 입출력 통계
  int64_t totalOpsProcessed;
  int64_t fusedOps;
  int64_t bufferizedOps;
  int64_t tiledLoops;
  int64_t generatedKernels;

  // 성능 추정
  double estimatedSpeedup;     // 예상 성능 향상도
  double estimatedMemorySaving; // 추정 메모리 절감
  double estimatedPowerSaving;  // 추정 전력 절감

  // 컴파일 통계
  double totalCompileTimeMs;
  int totalPasses;
  std::vector<PipelineStats> stageStats;

  CompilerMetrics()
    : totalOpsProcessed(0), fusedOps(0), bufferizedOps(0),
      tiledLoops(0), generatedKernels(0), estimatedSpeedup(1.0),
      estimatedMemorySaving(0.0), estimatedPowerSaving(0.0),
      totalCompileTimeMs(0.0), totalPasses(0) {}
};

//===----------------------------------------------------------------------===//
// IntegrationPass - Complete Pipeline Orchestration
//===----------------------------------------------------------------------===//

class IntegrationPass
    : public PassWrapper<IntegrationPass, OperationPass<func::FuncOp>> {
public:
  IntegrationPass() = default;
  explicit IntegrationPass(OptimizationLevel level) : optLevel(level) {}

  StringRef getArgument() const final { return "aiacccel-integration"; }
  StringRef getDescription() const final {
    return "Integrated AI-Accelerator compilation pipeline (Week 1-7)";
  }

  void runOnOperation() override;

private:
  OptimizationLevel optLevel = OptimizationLevel::O3;
  CompilerMetrics metrics;
  std::chrono::high_resolution_clock::time_point pipelineStart;

  // 파이프라인 단계 실행
  LogicalResult runFusionPass(func::FuncOp func);
  LogicalResult runBufferizationPass(func::FuncOp func);
  LogicalResult runLoopTilingPass(func::FuncOp func);
  LogicalResult runBackendPass(func::FuncOp func);

  // 검증
  LogicalResult validatePipeline(func::FuncOp func);
  LogicalResult verifySemanticsPreserved(func::FuncOp func);

  // 성능 분석
  void analyzePerformance(func::FuncOp func);
  void estimateSpeedup();
  void estimateMemorySavings();

  // 메트릭 수집
  void collectMetrics(func::FuncOp func, const std::string& stageName);
  void printCompilerReport();

  // 최적화 레벨별 전략
  void selectOptimizationPasses();

  // 최종 코드 생성
  LogicalResult generateFinalCode(func::FuncOp func);
};

//===----------------------------------------------------------------------===//
// Pass 등록
//===----------------------------------------------------------------------===//

#define GEN_PASS_REGISTRATION
#include "AIAccel/AIAccelPasses.h.inc"

} // namespace aiacccel
} // namespace mlir

#endif // AIACCCEL_INTEGRATION_PASS_H
