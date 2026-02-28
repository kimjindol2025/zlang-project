// Loop Tiling Pass Test Suite (10 test cases)
// Week 6: Polyhedral Loop Optimization Verification

// Test 1: Simple 2D Loop (Transpose-like)
func.func @test_2d_loop() {
  // Original: i,j from 0 to 256
  // Tiled: i_outer, j_outer by 32, i_inner, j_inner
  %i_outer = "aiacccel.load"() : () -> i64
  %j_outer = "aiacccel.load"() : () -> i64
  %transpose = "aiacccel.transpose"() : () -> i64
  "aiacccel.store"(%transpose) : (i64) -> ()
  return
}

// Test 2: 3D Loop Perfect Nest (MatMul-like)
func.func @test_3d_loop_matmul() {
  // Original: i,j,k from 0 to 256
  // Tiled: i_outer,j_outer,k_outer by [32,32,8]
  // Then i_inner,j_inner,k_inner
  %i = "aiacccel.load"() : () -> i64
  %j = "aiacccel.load"() : () -> i64
  %k = "aiacccel.load"() : () -> i64
  %matmul = "aiacccel.matmul"() : () -> i64
  %acc = "aiacccel.add"(%matmul) : (i64) -> i64
  "aiacccel.store"(%acc) : (i64) -> ()
  return
}

// Test 3: 4D Loop (Conv2D)
func.func @test_4d_loop_conv2d() {
  // Original: y,x,kh,kw from 0 to 256
  // Tiled: y_tile[32], x_tile[32], kh_tile[8], kw_tile[8]
  %y = "aiacccel.load"() : () -> i64
  %x = "aiacccel.load"() : () -> i64
  %conv = "aiacccel.conv2d"() : () -> i64
  %relu = "aiacccel.relu"(%conv) : (i64) -> i64
  "aiacccel.store"(%relu) : (i64) -> ()
  return
}

// Test 4: Loop with RAW Dependency
func.func @test_loop_raw_dependency() {
  // A[i] = B[i] + C[i]
  // B[i] = load()
  %load = "aiacccel.load"() : () -> i64
  %b = "aiacccel.load"() : () -> i64
  %c = "aiacccel.load"() : () -> i64
  %add = "aiacccel.add"(%load, %c) : (i64, i64) -> i64
  "aiacccel.store"(%add) : (i64) -> ()
  return
}

// Test 5: Reduction Loop (Sum)
func.func @test_reduction_loop() {
  // sum = 0
  // for i in 0..256:
  //   sum += A[i]
  %init = "aiacccel.load"() : () -> i64
  %data = "aiacccel.load"() : () -> i64
  %reduce = "aiacccel.reduce"() {axis = 0 : i32, reduce_type = "sum"} : () -> i64
  %sum = "aiacccel.add"(%reduce) : (i64) -> i64
  "aiacccel.store"(%sum) : (i64) -> ()
  return
}

// Test 6: Loop Permutation (i,j → j,i)
func.func @test_loop_permutation() {
  // Original order: for i for j
  // Permuted to: for j for i
  // Benefits: cache-friendly column access
  %j = "aiacccel.load"() : () -> i64
  %i = "aiacccel.load"() : () -> i64
  %access = "aiacccel.load"() : () -> i64
  %process = "aiacccel.relu"(%access) : (i64) -> i64
  "aiacccel.store"(%process) : (i64) -> ()
  return
}

// Test 7: Parallelizable Loop (No dependencies)
func.func @test_parallelizable_loop() {
  // Loop-independent: can parallelize
  // #pragma omp parallel for
  %0 = "aiacccel.load"() : () -> i64
  %1 = "aiacccel.matmul"() : () -> i64
  %2 = "aiacccel.relu"(%1) : (i64) -> i64
  "aiacccel.store"(%2) : (i64) -> ()
  return
}

// Test 8: Nested Tiling (3-level hierarchy)
func.func @test_nested_tiling() {
  // Tile 1: Super-tile (DRAM → SRAM)
  // Tile 2: Main tile (SRAM → L2)
  // Tile 3: Element loops (L2 → Registers)
  %super = "aiacccel.load"() : () -> i64
  %main = "aiacccel.matmul"() : () -> i64
  %elem = "aiacccel.add"(%main) : (i64) -> i64
  "aiacccel.store"(%elem) : (i64) -> ()
  return
}

// Test 9: Stencil Pattern (Data locality)
func.func @test_stencil_pattern() {
  // Stencil computation: A[i] = B[i-1] + B[i] + B[i+1]
  // Tiling helps reuse boundary elements
  %left = "aiacccel.load"() : () -> i64
  %center = "aiacccel.load"() : () -> i64
  %right = "aiacccel.load"() : () -> i64
  %add1 = "aiacccel.add"(%left, %center) : (i64, i64) -> i64
  %add2 = "aiacccel.add"(%add1, %right) : (i64, i64) -> i64
  "aiacccel.store"(%add2) : (i64) -> ()
  return
}

// Test 10: Resnet Block with Tiling
func.func @test_resnet_block_tiling() {
  // Conv2D + BatchNorm + ReLU chain with tile optimization
  // Tile: [32x32] spatial × [8] kernel
  %inp = "aiacccel.load"() : () -> i64
  %conv1 = "aiacccel.conv2d"() : () -> i64
  %bn1 = "aiacccel.batch_norm"(%conv1) : (i64) -> (i64, i64, i64)
  %relu1 = "aiacccel.relu"(%bn1#0) : (i64) -> i64

  %conv2 = "aiacccel.conv2d"() : () -> i64
  %bn2 = "aiacccel.batch_norm"(%conv2) : (i64) -> (i64, i64, i64)

  %add = "aiacccel.add"(%relu1, %bn2#0) : (i64, i64) -> i64
  %relu2 = "aiacccel.relu"(%add) : (i64) -> i64

  "aiacccel.store"(%relu2) : (i64) -> ()
  return
}
