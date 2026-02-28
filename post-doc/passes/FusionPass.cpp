//===- FusionPass.cpp - Operator Fusion Pass --------*- C++ -*-===//

#include "FusionPass.h"
#include "AIAccelOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include <iostream>
#include <algorithm>

using namespace mlir;
using namespace mlir::aiacccel;

//===----------------------------------------------------------------------===//
// DataFlowGraph Implementation
//===----------------------------------------------------------------------===//

void FusionPass::DataFlowGraph::build(Operation* op) {
  for (auto& operand : op->getOperands()) {
    if (auto defOp = operand.getDefiningOp()) {
      defs[op].push_back(defOp);
      uses[defOp].push_back(op);
    }
  }
}

bool FusionPass::DataFlowGraph::canFuse(Operation* producer, Operation* consumer) {
  // 같은 block에 있어야 함
  if (producer->getBlock() != consumer->getBlock()) {
    return false;
  }
  
  // producer의 결과가 consumer의 입력으로만 사용되어야 함
  for (auto* user : uses[producer]) {
    if (user != consumer && user->getBlock() == producer->getBlock()) {
      return false;  // 다른 곳에서도 사용 중
    }
  }
  
  // producer가 consumer 이전에 정의되어야 함
  auto* block = producer->getBlock();
  bool producerFound = false;
  for (auto& op : block->getOperations()) {
    if (&op == producer) {
      producerFound = true;
    }
    if (&op == consumer && !producerFound) {
      return false;
    }
  }
  
  return true;
}

//===----------------------------------------------------------------------===//
// Fusion 가능 여부 검사
//===----------------------------------------------------------------------===//

bool FusionPass::canFuseOps(Operation* producer, Operation* consumer) {
  // 지원하는 연산자 조합 확인
  auto producerName = producer->getName().getStringRef();
  auto consumerName = consumer->getName().getStringRef();
  
  // Fusion 규칙 정의
  std::set<std::pair<std::string, std::string>> fusiblePairs = {
    {"aiacccel.conv2d", "aiacccel.relu"},           // Conv2D -> ReLU
    {"aiacccel.conv2d", "aiacccel.batch_norm"},     // Conv2D -> BatchNorm
    {"aiacccel.relu", "aiacccel.matmul"},           // ReLU -> MatMul
    {"aiacccel.matmul", "aiacccel.relu"},           // MatMul -> ReLU
    {"aiacccel.matmul", "aiacccel.add"},            // MatMul -> Add
    {"aiacccel.reduce", "aiacccel.softmax"},        // Reduce -> Softmax
    {"aiacccel.batch_norm", "aiacccel.relu"},       // BatchNorm -> ReLU
    {"aiacccel.transpose", "aiacccel.matmul"},      // Transpose -> MatMul
  };
  
  auto key = std::make_pair(std::string(producerName), std::string(consumerName));
  return fusiblePairs.count(key) > 0;
}

//===----------------------------------------------------------------------===//
// 메모리 절약 추정
//===----------------------------------------------------------------------===//

int64_t FusionPass::estimateMemorySavings(Operation* producer, Operation* consumer) {
  // 단순 추정: producer의 output 크기
  // 실제로는 텐서 타입 분석 필요
  
  // Placeholder: 고정값 반환
  return 1024;  // bytes
}

//===----------------------------------------------------------------------===//
// 두 연산자 병합
//===----------------------------------------------------------------------===//

LogicalResult FusionPass::fuseTwoOps(Operation* producer, Operation* consumer,
                                      PatternRewriter& rewriter) {
  std::cout << "【 Fusion: " << producer->getName().getStringRef() 
            << " + " << consumer->getName().getStringRef() << " 】" << std::endl;
  
  // FusionOp 생성
  OpBuilder builder(rewriter.getContext());
  builder.setInsertionPoint(producer);
  
  // 입력 수집
  SmallVector<Value, 4> fusionInputs;
  for (auto operand : producer->getOperands()) {
    fusionInputs.push_back(operand);
  }
  for (auto operand : consumer->getOperands()) {
    // producer의 output을 제외하고 추가
    bool isProducerOutput = false;
    for (auto result : producer->getResults()) {
      if (operand == result) {
        isProducerOutput = true;
        break;
      }
    }
    if (!isProducerOutput) {
      fusionInputs.push_back(operand);
    }
  }
  
  // FusionOp 생성
  auto fusionOp = builder.create<aiacccel::FusionOp>(
    producer->getLoc(), consumer->getResultTypes(), fusionInputs);
  
  // Block 생성
  auto* block = new Block();
  fusionOp.getRegion().push_back(block);
  
  // Producer와 Consumer를 FusionOp 내부로 이동
  Block* originalBlock = producer->getBlock();
  producer->moveBefore(&block->back());
  consumer->moveBefore(&block->back());
  
  // YieldOp 추가
  SmallVector<Value, 4> yieldValues;
  for (auto result : consumer->getResults()) {
    yieldValues.push_back(result);
  }
  builder.setInsertionPointToEnd(block);
  builder.create<aiacccel::YieldOp>(consumer->getLoc(), yieldValues);
  
  // 사용처 업데이트
  rewriter.replaceOp(consumer, fusionOp->getResults());
  
  std::cout << "✅ Fusion 완료: 메모리 절약 = " 
            << estimateMemorySavings(producer, consumer) << " bytes" << std::endl;
  
  return success();
}

//===----------------------------------------------------------------------===//
// Fusion 후보 찾기
//===----------------------------------------------------------------------===//

bool FusionPass::findFusionCandidates(
    func::FuncOp func,
    std::vector<std::pair<Operation*, Operation*>>& candidates) {
  
  DataFlowGraph dfg;
  bool found = false;
  
  // 모든 연산자 쌍 검사
  for (auto& block : func.getBody()) {
    for (auto it = block.begin(); it != block.end(); ++it) {
      auto* producer = &(*it);
      dfg.build(producer);
      
      // 다음 연산자들 검사
      auto nextIt = std::next(it);
      while (nextIt != block.end()) {
        auto* consumer = &(*nextIt);
        dfg.build(consumer);
        
        // Fusion 가능 여부 확인
        if (canFuseOps(producer, consumer) && 
            dfg.canFuse(producer, consumer)) {
          candidates.push_back({producer, consumer});
          found = true;
          std::cout << "🔍 Fusion 후보 발견: " 
                    << producer->getName().getStringRef() << " -> "
                    << consumer->getName().getStringRef() << std::endl;
          break;  // 한 producer당 하나의 consumer만 선택
        }
        ++nextIt;
      }
    }
  }
  
  return found;
}

//===----------------------------------------------------------------------===//
// Main Pass Implementation
//===----------------------------------------------------------------------===//

void FusionPass::runOnOperation() {
  func::FuncOp func = getOperation();
  
  std::cout << "\n【 AIAccel Fusion Pass 시작 】" << std::endl;
  std::cout << "함수: " << func.getName() << std::endl;
  std::cout << std::string(50, '=') << std::endl;
  
  // Fusion 후보 찾기
  std::vector<std::pair<Operation*, Operation*>> candidates;
  int roundNum = 0;
  
  while (findFusionCandidates(func, candidates)) {
    roundNum++;
    std::cout << "\n【 Fusion Round " << roundNum << " 】" << std::endl;
    std::cout << "발견된 후보: " << candidates.size() << "개" << std::endl;
    
    // 각 후보 병합
    PatternRewriter rewriter(getContext());
    int fusionCount = 0;
    
    for (auto [producer, consumer] : candidates) {
      if (fusionTwoOps(producer, consumer, rewriter).succeeded()) {
        fusionCount++;
      }
    }
    
    std::cout << "✅ Round " << roundNum << " 병합 완료: " 
              << fusionCount << "개" << std::endl;
    
    candidates.clear();
  }
  
  std::cout << "\n【 Fusion Pass 완료 】" << std::endl;
  std::cout << "총 Fusion Round: " << roundNum << std::endl;
  std::cout << std::string(50, '=') << std::endl << std::endl;
}

//===----------------------------------------------------------------------===//
// Pass 등록
//===----------------------------------------------------------------------===//

std::unique_ptr<OperationPass<func::FuncOp>> createFusionPass() {
  return std::make_unique<FusionPass>();
}

#define GEN_PASS_REGISTRATION
#include "AIAccel/AIAccelPasses.cpp.inc"

