//===- LoopTilingPass.h - Polyhedral Loop Tiling Pass ----*- C++ -*-===//

#ifndef AIACCCEL_LOOP_TILING_PASS_H
#define AIACCCEL_LOOP_TILING_PASS_H

#include "mlir/Pass/Pass.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/PatternMatch.h"
#include <vector>
#include <map>
#include <memory>

namespace mlir {
namespace aiacccel {

//===----------------------------------------------------------------------===//
// Polyhedral Model Components
//===----------------------------------------------------------------------===//

// 반복 공간 정보
struct LoopNest {
  std::vector<int64_t> bounds;  // 각 루프의 범위
  int depth;                     // 루프 깊이
  std::string name;             // 루프 이름 (i, j, k, ...)

  LoopNest(const std::vector<int64_t>& b, int d, std::string n = "")
    : bounds(b), depth(d), name(n) {}
};

// 데이터 종속성
struct DataDependency {
  enum class Type {
    RAW,    // Read-After-Write (진정한 종속성)
    WAR,    // Write-After-Read (안티 종속성)
    WAW,    // Write-After-Write (출력 종속성)
    NONE    // 독립적
  };

  Type type;
  int64_t distance;        // 종속성 거리
  std::vector<int> dims;   // 어느 차원의 종속성?

  DataDependency(Type t = Type::NONE, int64_t d = 0)
    : type(t), distance(d) {}
};

// 타일 크기 정보
struct TileConfig {
  std::vector<int64_t> sizes;     // 각 차원의 타일 크기
  bool isPermutable;              // 루프 순서 변경 가능?
  bool isParallelizable;          // 병렬화 가능?
  double expectedSpeedup;         // 예상 성능 향상

  TileConfig() : isPermutable(false), isParallelizable(false), expectedSpeedup(1.0) {}
};

// Affine schedule
struct AffineSchedule {
  std::vector<std::vector<int64_t>> matrix;  // 스케줄 행렬
  std::vector<int64_t> offset;               // 오프셋 벡터

  AffineSchedule() = default;
};

//===----------------------------------------------------------------------===//
// LoopTilingPass - Polyhedral Loop Optimization
//===----------------------------------------------------------------------===//

class LoopTilingPass
    : public PassWrapper<LoopTilingPass, OperationPass<func::FuncOp>> {
public:
  LoopTilingPass() = default;

  StringRef getArgument() const final { return "aiacccel-loop-tiling"; }
  StringRef getDescription() const final {
    return "Apply polyhedral loop tiling for data locality and parallelism";
  }

  void runOnOperation() override;

private:
  // 루프 구조 분석
  struct LoopAnalysis {
    // Perfect nest 검증
    static constexpr int MAX_LOOP_DEPTH = 10;

    // 데이터 접근 패턴 분석
    struct AccessPattern {
      std::vector<int64_t> strides;  // 각 차원의 stride
      int64_t baseOffset;             // 기본 오프셋
      bool isLinear;                  // 선형 접근?

      AccessPattern() : baseOffset(0), isLinear(true) {}
    };
  };

  // 루프 구조 추출
  bool extractLoopNest(Operation* op, LoopNest& nest);

  // 루프 깊이 분석
  int analyzeLoopDepth(Operation* op);

  // 종속성 분석
  std::vector<DataDependency> analyzeDependencies(Operation* producer,
                                                   Operation* consumer);

  // 최적 타일 크기 결정
  TileConfig determineTileSize(const LoopNest& nest,
                               const std::vector<DataDependency>& deps);

  // Affine schedule 생성
  AffineSchedule generateAffineSchedule(const LoopNest& nest,
                                        const TileConfig& config);

  // 루프 타일링 변환 적용
  LogicalResult applyLoopTiling(Operation* op, const TileConfig& config);

  // 루프 순서 변경 (permutation)
  LogicalResult permuteLoops(Operation* op, const std::vector<int>& permutation);

  // 병렬화 설정
  LogicalResult applyParallelization(Operation* op, const TileConfig& config);

  // 지역성 분석
  double analyzeDataLocality(const LoopNest& nest,
                            const std::vector<DataDependency>& deps);

  // 성능 예측
  double predictSpeedup(const LoopNest& nest, const TileConfig& config);
};

//===----------------------------------------------------------------------===//
// Pass 등록
//===----------------------------------------------------------------------===//

#define GEN_PASS_REGISTRATION
#include "AIAccel/AIAccelPasses.h.inc"

} // namespace aiacccel
} // namespace mlir

#endif // AIACCCEL_LOOP_TILING_PASS_H
