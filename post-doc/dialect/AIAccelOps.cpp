//===- AIAccelOps.cpp - AI Accelerator Operations --------*- C++ -*-===//

#include "AIAccelOps.h"
#include "AIAccel.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/Interfaces/InferTypeOpInterface.h"
#include <iostream>

using namespace mlir;
using namespace mlir::aiacccel;

//===----------------------------------------------------------------------===//
// Utility Functions
//===----------------------------------------------------------------------===//

static LogicalResult verifyBinaryOp(Operation *op) {
  if (op->getNumOperands() != 2) {
    return op->emitOpError("expected 2 operands");
  }
  return success();
}

static LogicalResult verifyUnaryOp(Operation *op) {
  if (op->getNumOperands() != 1) {
    return op->emitOpError("expected 1 operand");
  }
  return success();
}

//===----------------------------------------------------------------------===//
// MatMulOp - 행렬 곱셈
//===----------------------------------------------------------------------===//

void MatMulOp::build(OpBuilder &builder, OperationState &state,
                      Type resultType, Value lhs, Value rhs) {
  state.addOperands({lhs, rhs});
  state.addTypes(resultType);
}

void MatMulOp::print(OpAsmPrinter &p) {
  p << "aiacccel.matmul " << getOperands();
  p.printOptionalAttrDict((*this)->getAttrs());
  p << " : ";
  p.printFunctionalType(getOperandTypes(), getResultTypes());
}

ParseResult MatMulOp::parse(OpAsmParser &parser, OperationState &result) {
  SmallVector<OpAsmParser::UnresolvedOperand, 2> operands;
  if (parser.parseOperandList(operands, 2) ||
      parser.parseOptionalAttrDict(result.attributes) ||
      parser.parseColon() ||
      parser.parseFunctionalType(operands, result.types))
    return failure();
  
  SmallVector<Value, 2> operandValues;
  for (auto operand : operands) {
    operandValues.push_back(operand.ssaValue);
  }
  result.addOperands(operandValues);
  return success();
}

LogicalResult MatMulOp::verify() {
  return verifyBinaryOp(getOperation());
}

//===----------------------------------------------------------------------===//
// Conv2dOp - 2D 합성곱
//===----------------------------------------------------------------------===//

void Conv2dOp::build(OpBuilder &builder, OperationState &state,
                     Type resultType, Value input, Value filter) {
  state.addOperands({input, filter});
  state.addTypes(resultType);
}

LogicalResult Conv2dOp::verify() {
  return verifyBinaryOp(getOperation());
}

//===----------------------------------------------------------------------===//
// ReLUOp - 활성화 함수
//===----------------------------------------------------------------------===//

void ReLUOp::build(OpBuilder &builder, OperationState &state,
                    Type resultType, Value input) {
  state.addOperands(input);
  state.addTypes(resultType);
}

LogicalResult ReLUOp::verify() {
  return verifyUnaryOp(getOperation());
}

//===----------------------------------------------------------------------===//
// SoftmaxOp - Softmax 정규화
//===----------------------------------------------------------------------===//

void SoftmaxOp::build(OpBuilder &builder, OperationState &state,
                      Type resultType, Value input, int32_t axis) {
  state.addOperands(input);
  state.addAttribute("axis", builder.getI32IntegerAttr(axis));
  state.addTypes(resultType);
}

LogicalResult SoftmaxOp::verify() {
  return verifyUnaryOp(getOperation());
}

//===----------------------------------------------------------------------===//
// GEMMOp - 일반화된 행렬 곱셈
//===----------------------------------------------------------------------===//

void GEMMOp::build(OpBuilder &builder, OperationState &state,
                   Type resultType, Value A, Value B, Value C,
                   float alpha, float beta) {
  state.addOperands({A, B, C});
  state.addAttribute("alpha", builder.getF32FloatAttr(alpha));
  state.addAttribute("beta", builder.getF32FloatAttr(beta));
  state.addTypes(resultType);
}

LogicalResult GEMMOp::verify() {
  if (getNumOperands() != 3) {
    return emitOpError("expected 3 operands (A, B, C)");
  }
  return success();
}

//===----------------------------------------------------------------------===//
// TransposeOp - 전치 연산
//===----------------------------------------------------------------------===//

void TransposeOp::build(OpBuilder &builder, OperationState &state,
                        Type resultType, Value input,
                        ArrayRef<int64_t> permutation) {
  state.addOperands(input);
  state.addAttribute("permutation", builder.getI64ArrayAttr(permutation));
  state.addTypes(resultType);
}

LogicalResult TransposeOp::verify() {
  return verifyUnaryOp(getOperation());
}

//===----------------------------------------------------------------------===//
// ReduceOp - 텐서 축약
//===----------------------------------------------------------------------===//

void ReduceOp::build(OpBuilder &builder, OperationState &state,
                     Type resultType, Value input, int32_t axis,
                     StringRef reduceType) {
  state.addOperands(input);
  state.addAttribute("axis", builder.getI32IntegerAttr(axis));
  state.addAttribute("reduce_type", builder.getStringAttr(reduceType));
  state.addTypes(resultType);
}

LogicalResult ReduceOp::verify() {
  auto reduceType = getReduceType();
  if (reduceType != "sum" && reduceType != "max" && reduceType != "min") {
    return emitOpError("reduce_type must be 'sum', 'max', or 'min'");
  }
  return verifyUnaryOp(getOperation());
}

//===----------------------------------------------------------------------===//
// BatchNormOp - 배치 정규화
//===----------------------------------------------------------------------===//

void BatchNormOp::build(OpBuilder &builder, OperationState &state,
                        Type outputType, Type meanType, Type varianceType,
                        Value input, Value gamma, Value beta,
                        float momentum, float epsilon) {
  state.addOperands({input, gamma, beta});
  state.addAttribute("momentum", builder.getF32FloatAttr(momentum));
  state.addAttribute("epsilon", builder.getF32FloatAttr(epsilon));
  state.addTypes({outputType, meanType, varianceType});
}

LogicalResult BatchNormOp::verify() {
  if (getNumOperands() != 3) {
    return emitOpError("expected 3 operands (input, gamma, beta)");
  }
  if (getNumResults() != 3) {
    return emitOpError("expected 3 results (output, mean, variance)");
  }
  return success();
}

//===----------------------------------------------------------------------===//
// 메모리 연산: AllocOp
//===----------------------------------------------------------------------===//

void AllocOp::build(OpBuilder &builder, OperationState &state,
                    MemRefType type, ValueRange dynamicSizes) {
  state.addOperands(dynamicSizes);
  state.addTypes(type);
}

LogicalResult AllocOp::verify() {
  return success();
}

//===----------------------------------------------------------------------===//
// 메모리 연산: LoadOp
//===----------------------------------------------------------------------===//

void LoadOp::build(OpBuilder &builder, OperationState &state,
                   Type resultType, Value memref, ValueRange indices) {
  state.addOperands(memref);
  state.addOperands(indices);
  state.addTypes(resultType);
}

LogicalResult LoadOp::verify() {
  if (getMemref().getType().isa<MemRefType>() == false) {
    return emitOpError("operand must be a memref");
  }
  return success();
}

//===----------------------------------------------------------------------===//
// 제어 흐름: YieldOp (종료 연산)
//===----------------------------------------------------------------------===//

void YieldOp::build(OpBuilder &builder, OperationState &state,
                    ValueRange values) {
  state.addOperands(values);
}

//===----------------------------------------------------------------------===//
// TableGen 생성 코드 포함
//===----------------------------------------------------------------------===//

#define GET_OP_CLASSES
#include "AIAccel/AIAccelOps.cpp.inc"

