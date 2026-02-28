// Integration Pass Test Suite (9 end-to-end tests)
// Week 8: Complete Compiler Pipeline Verification

// Test 1: Simple E2E Pipeline (MatMul)
func.func @test_e2e_matmul() {
  // Full pipeline:
  // 1. Fusion: (none for MatMul alone)
  // 2. Bufferization: DRAM allocation
  // 3. Loop tiling: 3D loop [32, 32, 8]
  // 4. Backend: GPU kernel generation
  %0 = "aiacccel.matmul"() : () -> i64
  "aiacccel.store"(%0) : (i64) -> ()
  return
}

// Test 2: Fusion Pipeline (Conv2D + ReLU)
func.func @test_fusion_pipeline() {
  // Fusion: Conv2D → ReLU (single kernel)
  // Memory: Shared memory optimization
  // Tiling: 4D loop [32, 32, 8, 8]
  // Backend: Fused CUDA kernel
  %conv = "aiacccel.conv2d"() : () -> i64
  %relu = "aiacccel.relu"(%conv) : (i64) -> i64
  "aiacccel.store"(%relu) : (i64) -> ()
  return
}

// Test 3: Memory Optimization Pipeline
func.func @test_memory_pipeline() {
  // Bufferization: SRAM allocation for small tensors
  // Double-buffering: Ping-pong buffers
  // Tiling: Cache-friendly tile sizes
  // Backend: Optimized memory transfers
  %alloc = "aiacccel.alloc"() {size = 32768 : i64} : () -> memref<8192xi32>
  %load = "aiacccel.load"() : () -> i64
  %compute = "aiacccel.matmul"() : () -> i64
  "aiacccel.store"(%compute) : (i64) -> ()
  "aiacccel.dealloc"(%alloc) : (memref<8192xi32>) -> ()
  return
}

// Test 4: Loop Optimization Pipeline
func.func @test_loop_optimization() {
  // Fusion: MatMul + Add (single kernel)
  // Bufferization: Hierarchical memory
  // Tiling: Perfect loop nest with optimal tile sizes
  // Backend: Parallel GPU execution
  %mat = "aiacccel.matmul"() : () -> i64
  %bias = "aiacccel.load"() : () -> i64
  %add = "aiacccel.add"(%mat, %bias) : (i64, i64) -> i64
  "aiacccel.store"(%add) : (i64) -> ()
  return
}

// Test 5: Complex E2E (Conv2D + BatchNorm + ReLU chain)
func.func @test_complex_fusion_chain() {
  // Fusion: Conv2D → BatchNorm → ReLU (3-kernel → 1 fused)
  // Bufferization: SRAM for intermediate results
  // Tiling: Optimized for cache locality
  // Backend: Single-kernel execution on GPU
  %conv = "aiacccel.conv2d"() : () -> i64
  %bn = "aiacccel.batch_norm"(%conv) : (i64) -> (i64, i64, i64)
  %relu = "aiacccel.relu"(%bn#0) : (i64) -> i64
  "aiacccel.store"(%relu) : (i64) -> ()
  return
}

// Test 6: Async Pipeline E2E
func.func @test_async_pipeline_e2e() {
  // Full async pipeline with H2D/D2H overlap
  // Bufferization: Split memory (Host/Device)
  // Backend: Async kernel launches
  // Pipelining: H2D | Compute | D2H overlap
  %data = "aiacccel.load"() : () -> i64
  %compute = "aiacccel.matmul"() : () -> i64
  %result = "aiacccel.relu"(%compute) : (i64) -> i64
  "aiacccel.store"(%result) : (i64) -> ()
  return
}

// Test 7: Nested Loop Optimization
func.func @test_nested_loops_e2e() {
  // Fusion: Multiple MatMul operations
  // Bufferization: Hierarchical (HBM + DRAM)
  // Tiling: 3-level nesting (super-tile, tile, element)
  // Backend: Multi-level parallelization
  %a = "aiacccel.load"() : () -> i64
  %b = "aiacccel.load"() : () -> i64
  %m1 = "aiacccel.matmul"() : () -> i64
  %c = "aiacccel.load"() : () -> i64
  %m2 = "aiacccel.matmul"() : () -> i64
  %result = "aiacccel.add"(%m1, %m2) : (i64, i64) -> i64
  "aiacccel.store"(%result) : (i64) -> ()
  return
}

// Test 8: ResNet Block (Full Optimization)
func.func @test_resnet_full_optimization() {
  // Fusion: Conv+BN+ReLU chains
  // Bufferization: Smart buffer allocation
  // Tiling: Spatial and kernel dimension tiling
  // Backend: Residual connection optimization

  %input = "aiacccel.load"() : () -> i64

  // Main path
  %conv1 = "aiacccel.conv2d"() : () -> i64
  %bn1 = "aiacccel.batch_norm"(%conv1) : (i64) -> (i64, i64, i64)
  %relu1 = "aiacccel.relu"(%bn1#0) : (i64) -> i64

  %conv2 = "aiacccel.conv2d"() : () -> i64
  %bn2 = "aiacccel.batch_norm"(%conv2) : (i64) -> (i64, i64, i64)

  // Residual
  %add = "aiacccel.add"(%relu1, %bn2#0) : (i64, i64) -> i64
  %output = "aiacccel.relu"(%add) : (i64) -> i64

  "aiacccel.store"(%output) : (i64) -> ()
  return
}

// Test 9: Complete Model E2E (Multiple Functions)
func.func @test_complete_model() {
  // Full compilation pipeline on entire model
  // All 4 optimization layers applied
  // End-to-end validation from input to output

  // Pretrain: Load input
  %input = "aiacccel.load"() : () -> i64

  // Layer 1: Conv + Activation
  %conv1 = "aiacccel.conv2d"() : () -> i64
  %relu1 = "aiacccel.relu"(%conv1) : (i64) -> i64

  // Layer 2: MatMul + Bias
  %w = "aiacccel.load"() : () -> i64
  %matmul = "aiacccel.matmul"() : () -> i64
  %bias = "aiacccel.load"() : () -> i64
  %add = "aiacccel.add"(%matmul, %bias) : (i64, i64) -> i64

  // Layer 3: Reduction + Softmax
  %reduce = "aiacccel.reduce"() {axis = 1 : i32, reduce_type = "max"} : () -> i64
  %softmax = "aiacccel.softmax"(%reduce) {axis = 1 : i32} : (i64) -> i64

  // Output
  "aiacccel.store"(%softmax) : (i64) -> ()
  return
}
