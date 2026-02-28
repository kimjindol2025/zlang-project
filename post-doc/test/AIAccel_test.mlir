// AIAccel Operations Test Suite (20 test cases)
// Week 3: Operation Implementation Verification

// Test 1: MatMul Operation
func.func @test_matmul() {
  %0 = "aiacccel.matmul"() {llvm.bareptr} : () -> i64
  return
}

// Test 2: Conv2D Operation
func.func @test_conv2d() {
  %0 = "aiacccel.conv2d"() {strides = array<i64: 1, 1>} : () -> i64
  return
}

// Test 3: ReLU Activation
func.func @test_relu() {
  %0 = "aiacccel.relu"() : () -> i64
  return
}

// Test 4: Softmax Normalization
func.func @test_softmax() {
  %0 = "aiacccel.softmax"() {axis = 1 : i32} : () -> i64
  return
}

// Test 5: Element-wise Addition
func.func @test_add() {
  %0 = "aiacccel.add"() : () -> i64
  return
}

// Test 6: Element-wise Multiplication
func.func @test_mul() {
  %0 = "aiacccel.mul"() : () -> i64
  return
}

// Test 7: GEMM Operation
func.func @test_gemm() {
  %0 = "aiacccel.gemm"() {alpha = 1.0 : f32, beta = 0.0 : f32} : () -> i64
  return
}

// Test 8: Transpose Operation
func.func @test_transpose() {
  %0 = "aiacccel.transpose"() {permutation = array<i64: 1, 0>} : () -> i64
  return
}

// Test 9: Reduce Sum
func.func @test_reduce_sum() {
  %0 = "aiacccel.reduce"() {axis = 1 : i32, reduce_type = "sum"} : () -> i64
  return
}

// Test 10: Reduce Max
func.func @test_reduce_max() {
  %0 = "aiacccel.reduce"() {axis = 0 : i32, reduce_type = "max"} : () -> i64
  return
}

// Test 11: Batch Normalization
func.func @test_batch_norm() {
  %0:3 = "aiacccel.batch_norm"() {momentum = 0.9 : f32, epsilon = 1e-5 : f32} : () -> (i64, i64, i64)
  return
}

// Test 12: Memory Allocation
func.func @test_alloc() {
  %c1 = arith.constant 1 : index
  %0 = "aiacccel.alloc"(%c1) : (index) -> memref<?xf32>
  return
}

// Test 13: Memory Deallocation
func.func @test_dealloc(%arg0: memref<?xf32>) {
  "aiacccel.dealloc"(%arg0) : (memref<?xf32>) -> ()
  return
}

// Test 14: Synchronization
func.func @test_sync() {
  "aiacccel.sync"() : () -> ()
  return
}

// Test 15: Wait Operation
func.func @test_wait() {
  %0 = "aiacccel.wait"() {timeout = 1000 : i32} : () -> i1
  return
}

// Test 16: Fusion Operation
func.func @test_fusion() {
  %0 = "aiacccel.fusion"() ({
    %1 = "aiacccel.relu"() : () -> i64
    "aiacccel.yield"(%1) : (i64) -> ()
  }) : () -> i64
  return
}

// Test 17: Tile Operation
func.func @test_tile() {
  %0 = "aiacccel.tile"() {tile_sizes = array<i64: 32, 32>} : () -> i64
  return
}

// Test 18: If Operation (Conditional)
func.func @test_if() {
  %cond = arith.constant 1 : i1
  %0 = "aiacccel.if"(%cond) ({
    %1 = "aiacccel.relu"() : () -> i64
    "aiacccel.yield"(%1) : (i64) -> ()
  }, {
    %2 = "aiacccel.add"() : () -> i64
    "aiacccel.yield"(%2) : (i64) -> ()
  }) : (i1) -> i64
  return
}

// Test 19: Loop Operation
func.func @test_loop() {
  %c0 = arith.constant 0 : index
  %c10 = arith.constant 10 : index
  %c1 = arith.constant 1 : index
  %init = arith.constant 0.0 : f32
  
  %result = "aiacccel.loop"(%c0, %c10, %c1, %init) ({
    ^bb0(%i: index, %iter: f32):
      %new_iter = "aiacccel.add"() : () -> f32
      "aiacccel.yield"(%new_iter) : (f32) -> ()
  }) : (index, index, index, f32) -> f32
  
  return
}

// Test 20: Complex Fusion (Real-world Example)
func.func @test_complex_fusion() {
  %0 = "aiacccel.fusion"() ({
    // Conv2D -> ReLU -> MaxPool fusion
    %conv = "aiacccel.conv2d"() : () -> i64
    %relu = "aiacccel.relu"() : () -> i64
    %reduce = "aiacccel.reduce"() {axis = 1 : i32, reduce_type = "max"} : () -> i64
    "aiacccel.yield"(%reduce) : (i64) -> ()
  }) : () -> i64
  return
}

