//===- BufferizationPass.h - Memory Bufferization Pass -----*- C++ -*-===//

#ifndef AIACCCEL_BUFFERIZATION_PASS_H
#define AIACCCEL_BUFFERIZATION_PASS_H

#include "mlir/Pass/Pass.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/PatternMatch.h"
#include <vector>
#include <map>
#include <memory>

namespace mlir {
namespace aiacccel {

//===----------------------------------------------------------------------===//
// Memory Layout Analysis
//===----------------------------------------------------------------------===//

enum class MemoryLevel {
  REGISTER,  // 레지스터 (fastest, smallest)
  L1_CACHE,  // L1 캐시
  L2_CACHE,  // L2 캐시
  SRAM,      // SRAM (on-chip, 중간)
  DRAM,      // DRAM (off-chip, 가장 느림)
  HBM        // High-Bandwidth Memory (GPU/TPU)
};

struct MemoryTile {
  Value tensor;
  int64_t size;        // bytes
  MemoryLevel level;
  int64_t tileSize;    // 타일 크기
  bool doubleBuffered; // Double-buffering 여부

  MemoryTile(Value t, int64_t s, MemoryLevel l)
    : tensor(t), size(s), level(l), tileSize(0), doubleBuffered(false) {}
};

struct BufferStrategy {
  std::vector<MemoryTile> tiles;
  int64_t totalSRAMUsage;   // 총 SRAM 사용량
  int64_t totalDRAMUsage;   // 총 DRAM 사용량
  double bandwidthUtility;  // 대역폭 활용율

  BufferStrategy()
    : totalSRAMUsage(0), totalDRAMUsage(0), bandwidthUtility(0.0) {}
};

//===----------------------------------------------------------------------===//
// BufferizationPass - Memory Bufferization for Performance
//===----------------------------------------------------------------------===//

class BufferizationPass
    : public PassWrapper<BufferizationPass, OperationPass<func::FuncOp>> {
public:
  BufferizationPass() = default;

  StringRef getArgument() const final { return "aiacccel-bufferization"; }
  StringRef getDescription() const final {
    return "Bufferize operations with SRAM-aware optimization and double-buffering";
  }

  void runOnOperation() override;

private:
  // 메모리 계층 구조 분석
  struct MemoryHierarchy {
    // 각 메모리 레벨의 크기 (bytes)
    static constexpr int64_t REGISTER_SIZE = 8 * 1024;        // 8 KB
    static constexpr int64_t L1_CACHE_SIZE = 32 * 1024;       // 32 KB
    static constexpr int64_t L2_CACHE_SIZE = 256 * 1024;      // 256 KB
    static constexpr int64_t SRAM_SIZE = 512 * 1024;          // 512 KB (on-chip)
    static constexpr int64_t DRAM_SIZE = 8LL * 1024 * 1024;   // 8 MB

    // 접근 지연시간 (cycles)
    static constexpr int REGISTER_LATENCY = 0;
    static constexpr int L1_CACHE_LATENCY = 4;
    static constexpr int L2_CACHE_LATENCY = 11;
    static constexpr int SRAM_LATENCY = 10;
    static constexpr int DRAM_LATENCY = 50;
    static constexpr int HBM_LATENCY = 100;

    // 대역폭 (GB/s)
    static constexpr double REGISTER_BW = 256.0;
    static constexpr double L1_CACHE_BW = 128.0;
    static constexpr double L2_CACHE_BW = 64.0;
    static constexpr double SRAM_BW = 32.0;
    static constexpr double DRAM_BW = 8.0;
    static constexpr double HBM_BW = 256.0;
  };

  // 텐서 크기 추정
  int64_t estimateTensorSize(Value tensor);

  // 최적 메모리 레벨 결정
  MemoryLevel chooseMemoryLevel(Value tensor, int64_t accessCount);

  // Double-buffering 여부 판단
  bool shouldDoubleBuffer(Operation* op, Value tensor);

  // 메모리 타일 크기 계산
  int64_t calculateTileSize(Value tensor, MemoryLevel level);

  // 버퍼화 전략 생성
  std::unique_ptr<BufferStrategy> planBufferizationStrategy(func::FuncOp func);

  // Alloc/Dealloc 삽입
  LogicalResult insertBufferOps(func::FuncOp func, const BufferStrategy& strategy);

  // Double-buffering 구현
  LogicalResult implementDoubleBuffering(Operation* producer,
                                         Operation* consumer,
                                         const MemoryTile& tile);

  // 캐시 친화적 루프 변환
  LogicalResult applyCacheOptimization(func::FuncOp func);
};

//===----------------------------------------------------------------------===//
// Pass 등록
//===----------------------------------------------------------------------===//

#define GEN_PASS_REGISTRATION
#include "AIAccel/AIAccelPasses.h.inc"

} // namespace aiacccel
} // namespace mlir

#endif // AIACCCEL_BUFFERIZATION_PASS_H
