//===- AIAccel.td - AI Accelerator Dialect ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef AIACCCEL_DIALECT
#define AIACCCEL_DIALECT

include "mlir/IR/OpBase.td"

//===----------------------------------------------------------------------===//
// AIAccel Dialect Definition
//===----------------------------------------------------------------------===//

def AIAccel_Dialect : Dialect {
  let name = "aiacccel";
  let description = [{
    The AIAccel dialect is designed for optimizing AI/ML workloads on 
    heterogeneous accelerators (GPU, TPU, FPGA).
    
    Features:
    - Multi-level optimization (graph, memory, vector, target)
    - Automatic operator fusion and tiling
    - SRAM-aware bufferization
    - SIMD vectorization
    - Multi-target code generation
  }];
  let cppNamespace = "::aiacccel";
}

//===----------------------------------------------------------------------===//
// AIAccel Types
//===----------------------------------------------------------------------===//

class AIAccel_Type<string name> : TypeDef<AIAccel_Dialect, name> {}

def TensorType : AIAccel_Type<"Tensor"> {
  let mnemonic = "tensor";
  let description = "High-level tensor type with shape/dtype metadata";
}

def AcceleratorMemType : AIAccel_Type<"AcceleratorMem"> {
  let mnemonic = "accel_mem";
  let description = "Accelerator-specific memory (SRAM, HBM, etc)";
}

//===----------------------------------------------------------------------===//
// AIAccel Operations
//===----------------------------------------------------------------------===//

class AIAccel_Op<string mnemonic, list<Trait> traits = []> :
    Op<AIAccel_Dialect, mnemonic, traits> {}

def MatMulOp : AIAccel_Op<"matmul"> {
  let summary = "Matrix multiplication operation";
  let description = [{
    Performs matrix multiplication with automatic tiling and fusion.
    
    Example:
      %result = aiacccel.matmul %lhs, %rhs : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
  }];
  
  let arguments = (ins AnyType:$lhs, AnyType:$rhs);
  let results = (outs AnyType:$result);
  let assemblyFormat = "$lhs `,` $rhs attr-dict `:` functional-type(operands, results)";
}

def Conv2dOp : AIAccel_Op<"conv2d"> {
  let summary = "2D Convolution operation";
  let description = [{
    Performs 2D convolution with stride, padding, and dilation support.
  }];
  
  let arguments = (ins AnyType:$input, AnyType:$filter, OptionalAttr<I64ArrayAttr>:$strides);
  let results = (outs AnyType:$output);
  let assemblyFormat = "$input `,` $filter attr-dict `:` functional-type(operands, results)";
}

def FusionOp : AIAccel_Op<"fusion", [SingleBlockImplicitTerminator<"YieldOp">]> {
  let summary = "Fused kernel operation";
  let description = [{
    Represents a fused computation kernel combining multiple operations
    to minimize memory traffic.
  }];
  
  let arguments = (ins Variadic<AnyType>:$inputs);
  let results = (outs Variadic<AnyType>:$outputs);
  let regions = (region SizedRegion<1>:$region);
}

def TileOp : AIAccel_Op<"tile"> {
  let summary = "Tile operation for loop tiling";
  let description = [{
    Defines tiling parameters for memory hierarchy optimization.
    Uses polyhedral model for automatic tile size computation.
  }];
  
  let arguments = (ins I64ArrayAttr:$tile_sizes, AnyType:$tensor);
  let results = (outs AnyType:$tiled);
}

def YieldOp : AIAccel_Op<"yield", [Terminator]> {
  let summary = "Yield operation";
  let arguments = (ins Variadic<AnyType>:$operands);
  let assemblyFormat = "operands attr-dict `:` type(operands)";
}

#endif // AIACCCEL_DIALECT

//===----------------------------------------------------------------------===//
// Additional AIAccel Operations (Week 3 Implementation)
//===----------------------------------------------------------------------===//

// Element-wise Operations
def AddOp : AIAccel_Op<"add"> {
  let summary = "Element-wise addition";
  let arguments = (ins AnyType:$lhs, AnyType:$rhs);
  let results = (outs AnyType:$result);
  let assemblyFormat = "$lhs `,` $rhs attr-dict `:` functional-type(operands, results)";
}

def MulOp : AIAccel_Op<"mul"> {
  let summary = "Element-wise multiplication";
  let arguments = (ins AnyType:$lhs, AnyType:$rhs);
  let results = (outs AnyType:$result);
  let assemblyFormat = "$lhs `,` $rhs attr-dict `:` functional-type(operands, results)";
}

def ReLUOp : AIAccel_Op<"relu"> {
  let summary = "Rectified Linear Unit";
  let arguments = (ins AnyType:$input);
  let results = (outs AnyType:$output);
  let assemblyFormat = "$input attr-dict `:` functional-type(operands, results)";
}

def SoftmaxOp : AIAccel_Op<"softmax"> {
  let summary = "Softmax normalization";
  let arguments = (ins AnyType:$input, I32Attr:$axis);
  let results = (outs AnyType:$output);
  let assemblyFormat = "$input attr-dict `:` functional-type(operands, results)";
}

// Memory Operations
def LoadOp : AIAccel_Op<"load"> {
  let summary = "Load from accelerator memory";
  let arguments = (ins AnyType:$memref, Variadic<Index>:$indices);
  let results = (outs AnyType:$result);
  let assemblyFormat = "$memref `[` $indices `]` attr-dict `:` functional-type(operands, results)";
}

def StoreOp : AIAccel_Op<"store"> {
  let summary = "Store to accelerator memory";
  let arguments = (ins AnyType:$value, AnyType:$memref, Variadic<Index>:$indices);
  let assemblyFormat = "$value `,` $memref `[` $indices `]` attr-dict `:` type($value)";
}

def AllocOp : AIAccel_Op<"alloc"> {
  let summary = "Allocate accelerator memory";
  let arguments = (ins Variadic<Index>:$sizes, OptionalAttr<I64ArrayAttr>:$alignment);
  let results = (outs AnyType:$memref);
  let assemblyFormat = "operands attr-dict `:` type(results)";
}

def DeallocOp : AIAccel_Op<"dealloc"> {
  let summary = "Deallocate accelerator memory";
  let arguments = (ins AnyType:$memref);
  let assemblyFormat = "$memref attr-dict `:` type($memref)";
}

// Advanced Operations
def GEMMOp : AIAccel_Op<"gemm"> {
  let summary = "General matrix multiplication with scaling";
  let arguments = (ins AnyType:$A, AnyType:$B, AnyType:$C, 
                       F32Attr:$alpha, F32Attr:$beta);
  let results = (outs AnyType:$result);
  let assemblyFormat = "operands attr-dict `:` functional-type(operands, results)";
}

def TransposeOp : AIAccel_Op<"transpose"> {
  let summary = "Matrix transpose";
  let arguments = (ins AnyType:$input, I64ArrayAttr:$permutation);
  let results = (outs AnyType:$output);
  let assemblyFormat = "$input attr-dict `:` functional-type(operands, results)";
}

def ReduceOp : AIAccel_Op<"reduce"> {
  let summary = "Tensor reduction (sum/max/min)";
  let arguments = (ins AnyType:$input, I32Attr:$axis, 
                       StrAttr:$reduce_type);
  let results = (outs AnyType:$output);
  let assemblyFormat = "$input attr-dict `:` functional-type(operands, results)";
}

def BatchNormOp : AIAccel_Op<"batch_norm"> {
  let summary = "Batch normalization";
  let arguments = (ins AnyType:$input, AnyType:$gamma, AnyType:$beta,
                       F32Attr:$momentum, F32Attr:$epsilon);
  let results = (outs AnyType:$output, AnyType:$mean, AnyType:$variance);
  let assemblyFormat = "operands attr-dict `:` functional-type(operands, results)";
}

// Control Flow
def IfOp : AIAccel_Op<"if", [SingleBlockImplicitTerminator<"YieldOp">]> {
  let summary = "Conditional operation";
  let arguments = (ins I1:$condition);
  let regions = (region SizedRegion<2>:$thenRegion, SizedRegion<1>:$elseRegion);
  let results = (outs Variadic<AnyType>:$results);
}

def LoopOp : AIAccel_Op<"loop", [SingleBlockImplicitTerminator<"YieldOp">]> {
  let summary = "Loop operation";
  let arguments = (ins Index:$lower, Index:$upper, Index:$step,
                       Variadic<AnyType>:$init);
  let regions = (region SizedRegion<1>:$body);
  let results = (outs Variadic<AnyType>:$results);
}

// Synchronization
def SyncOp : AIAccel_Op<"sync"> {
  let summary = "Synchronization barrier";
  let arguments = (ins);
  let results = (outs);
}

def WaitOp : AIAccel_Op<"wait"> {
  let summary = "Wait for pending operations";
  let arguments = (ins OptionalAttr<I32Attr>:$timeout);
  let results = (outs I1:$status);
}

