//===- FusionPass.h - Operator Fusion Pass --------*- C++ -*-===//

#ifndef AIACCCEL_FUSION_PASS_H
#define AIACCCEL_FUSION_PASS_H

#include "mlir/Pass/Pass.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/PatternMatch.h"
#include <vector>
#include <set>

namespace mlir {
namespace aiacccel {

//===----------------------------------------------------------------------===//
// FusionPass - Operator Fusion for Memory Optimization
//===----------------------------------------------------------------------===//

class FusionPass : public PassWrapper<FusionPass, OperationPass<func::FuncOp>> {
public:
  FusionPass() = default;
  
  StringRef getArgument() const final { return "aiacccel-fusion"; }
  StringRef getDescription() const final {
    return "Fuse consecutive AI operations to reduce memory traffic";
  }
  
  void runOnOperation() override;
  
private:
  // 데이터 흐름 분석
  struct DataFlowGraph {
    std::map<Operation*, std::vector<Operation*>> uses;
    std::map<Operation*, std::vector<Operation*>> defs;
    
    void build(Operation* op);
    bool canFuse(Operation* producer, Operation* consumer);
  };
  
  // Fusion 후보 찾기
  bool findFusionCandidates(func::FuncOp func,
                            std::vector<std::pair<Operation*, Operation*>>& candidates);
  
  // 두 연산자 병합
  LogicalResult fuseTwoOps(Operation* producer, Operation* consumer,
                           PatternRewriter& rewriter);
  
  // Fusion 가능 여부 검사
  bool canFuseOps(Operation* producer, Operation* consumer);
  
  // 메모리 영향 계산
  int64_t estimateMemorySavings(Operation* producer, Operation* consumer);
};

//===----------------------------------------------------------------------===//
// Pass 등록
//===----------------------------------------------------------------------===//

#define GEN_PASS_REGISTRATION
#include "AIAccel/AIAccelPasses.h.inc"

} // namespace aiacccel
} // namespace mlir

#endif // AIACCCEL_FUSION_PASS_H
