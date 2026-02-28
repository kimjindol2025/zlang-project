// Fusion Pass Test Suite (15 test cases)
// Week 4: Operator Fusion Verification

// Test 1: Conv2D -> ReLU Fusion
func.func @test_conv2d_relu_fusion() {
  // Conv2D followed by ReLU should be fused
  %0 = "aiacccel.conv2d"() : () -> i64
  %1 = "aiacccel.relu"(%0) : (i64) -> i64
  return
}

// Test 2: Conv2D -> BatchNorm Fusion
func.func @test_conv2d_batchnorm_fusion() {
  %0 = "aiacccel.conv2d"() : () -> i64
  %1:3 = "aiacccel.batch_norm"(%0) {momentum = 0.9 : f32, epsilon = 1e-5 : f32} : (i64) -> (i64, i64, i64)
  return
}

// Test 3: ReLU -> MatMul Fusion
func.func @test_relu_matmul_fusion() {
  %0 = "aiacccel.relu"() : () -> i64
  %1 = "aiacccel.matmul"(%0) : (i64) -> i64
  return
}

// Test 4: MatMul -> ReLU Fusion
func.func @test_matmul_relu_fusion() {
  %0 = "aiacccel.matmul"() : () -> i64
  %1 = "aiacccel.relu"(%0) : (i64) -> i64
  return
}

// Test 5: MatMul -> Add Fusion
func.func @test_matmul_add_fusion() {
  %0 = "aiacccel.matmul"() : () -> i64
  %1 = "aiacccel.add"(%0) : (i64) -> i64
  return
}

// Test 6: Reduce -> Softmax Fusion
func.func @test_reduce_softmax_fusion() {
  %0 = "aiacccel.reduce"() {axis = 1 : i32, reduce_type = "sum"} : () -> i64
  %1 = "aiacccel.softmax"(%0) {axis = 1 : i32} : (i64) -> i64
  return
}

// Test 7: BatchNorm -> ReLU Fusion
func.func @test_batchnorm_relu_fusion() {
  %0:3 = "aiacccel.batch_norm"() {momentum = 0.9 : f32, epsilon = 1e-5 : f32} : () -> (i64, i64, i64)
  %1 = "aiacccel.relu"(%0#0) : (i64) -> i64
  return
}

// Test 8: Transpose -> MatMul Fusion
func.func @test_transpose_matmul_fusion() {
  %0 = "aiacccel.transpose"() {permutation = array<i64: 1, 0>} : () -> i64
  %1 = "aiacccel.matmul"(%0) : (i64) -> i64
  return
}

// Test 9: Chain Fusion (Conv2D -> ReLU -> MatMul)
func.func @test_chain_fusion() {
  %0 = "aiacccel.conv2d"() : () -> i64
  %1 = "aiacccel.relu"(%0) : (i64) -> i64
  %2 = "aiacccel.matmul"(%1) : (i64) -> i64
  return
}

// Test 10: No Fusion - Multiple Users
func.func @test_no_fusion_multiple_users() {
  // Conv2D output used in two places - should NOT fuse
  %0 = "aiacccel.conv2d"() : () -> i64
  %1 = "aiacccel.relu"(%0) : (i64) -> i64
  %2 = "aiacccel.add"(%0) : (i64) -> i64
  return
}

// Test 11: No Fusion - Different Block
func.func @test_no_fusion_different_block(%arg0: i1) {
  // Operations in different blocks - should NOT fuse
  "aiacccel.if"(%arg0) ({
    %0 = "aiacccel.conv2d"() : () -> i64
    "aiacccel.yield"(%0) : (i64) -> ()
  }, {
    %1 = "aiacccel.relu"() : () -> i64
    "aiacccel.yield"(%1) : (i64) -> ()
  }) : (i1) -> i64
  return
}

// Test 12: Memory Savings Estimation
func.func @test_memory_savings() {
  // Fusion should reduce memory for intermediate tensor
  %0 = "aiacccel.matmul"() : () -> i64
  %1 = "aiacccel.relu"(%0) : (i64) -> i64
  %2 = "aiacccel.softmax"(%1) {axis = 1 : i32} : (i64) -> i64
  return
}

// Test 13: GEMM -> ReLU Fusion (High-impact)
func.func @test_gemm_relu_fusion() {
  %0 = "aiacccel.gemm"() {alpha = 1.0 : f32, beta = 0.0 : f32} : () -> i64
  %1 = "aiacccel.relu"(%0) : (i64) -> i64
  return
}

// Test 14: Complex Real-world Fusion (ResNet Block)
func.func @test_resnet_block_fusion() {
  // Simulated ResNet residual block fusion
  %conv1 = "aiacccel.conv2d"() : () -> i64
  %relu1 = "aiacccel.relu"(%conv1) : (i64) -> i64
  %conv2 = "aiacccel.conv2d"() : () -> i64
  %relu2 = "aiacccel.relu"(%conv2) : (i64) -> i64
  %add = "aiacccel.add"(%relu1, %relu2) : (i64, i64) -> i64
  return
}

// Test 15: Data-dependent Fusion (with memory tracking)
func.func @test_data_dependent_fusion() {
  %0 = "aiacccel.load"() : () -> i64
  %1 = "aiacccel.conv2d"() : () -> i64
  %2 = "aiacccel.relu"(%1) : (i64) -> i64
  "aiacccel.store"(%2) : (i64) -> ()
  return
}

