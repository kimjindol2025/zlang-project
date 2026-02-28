// Bufferization Pass Test Suite (12 test cases)
// Week 5: Memory Optimization Verification

// Test 1: Simple Tensor Allocation (SRAM)
func.func @test_simple_tensor_sram() {
  %0 = "aiacccel.alloc"() {size = 1024 : i64} : () -> memref<256xi32>
  %1 = "aiacccel.load"() : () -> i64
  %2 = "aiacccel.store"(%1) : (i64) -> ()
  "aiacccel.dealloc"(%0) : (memref<256xi32>) -> ()
  return
}

// Test 2: Large Tensor Allocation (DRAM)
func.func @test_large_tensor_dram() {
  %0 = "aiacccel.alloc"() {size = 8388608 : i64} : () -> memref<2097152xi32>
  %1 = "aiacccel.matmul"() : () -> i64
  "aiacccel.dealloc"(%0) : (memref<2097152xi32>) -> ()
  return
}

// Test 3: Double-buffering (Small tensor with high access)
func.func @test_double_buffering() {
  // Buffer A와 B를 동시에 할당
  %0 = "aiacccel.alloc"() {size = 512 : i64} : () -> memref<128xi32>
  %1 = "aiacccel.alloc"() {size = 512 : i64} : () -> memref<128xi32>
  // Producer 단계
  %2 = "aiacccel.conv2d"() : () -> i64
  // Consumer 단계 (다른 버퍼 사용)
  %3 = "aiacccel.relu"(%2) : (i64) -> i64
  "aiacccel.dealloc"(%0) : (memref<128xi32>) -> ()
  "aiacccel.dealloc"(%1) : (memref<128xi32>) -> ()
  return
}

// Test 4: Memory Hierarchy (L1 Cache)
func.func @test_memory_hierarchy_l1() {
  // 작은 데이터: L1 캐시에 적합
  %0 = "aiacccel.alloc"() {size = 256 : i64, level = 1 : i32} : () -> memref<64xi32>
  %1 = "aiacccel.matmul"() : () -> i64
  %2 = "aiacccel.relu"(%1) : (i64) -> i64
  "aiacccel.dealloc"(%0) : (memref<64xi32>) -> ()
  return
}

// Test 5: Memory Hierarchy (L2 Cache)
func.func @test_memory_hierarchy_l2() {
  // 중간 크기: L2 캐시에 적합
  %0 = "aiacccel.alloc"() {size = 16384 : i64, level = 2 : i32} : () -> memref<4096xi32>
  %1 = "aiacccel.conv2d"() : () -> i64
  %2 = "aiacccel.batch_norm"(%1) : (i64) -> (i64, i64, i64)
  "aiacccel.dealloc"(%0) : (memref<4096xi32>) -> ()
  return
}

// Test 6: Sequential Access Pattern (Cache-friendly)
func.func @test_sequential_access() {
  %0 = "aiacccel.alloc"() {size = 4096 : i64} : () -> memref<1024xi32>
  %ctr = "aiacccel.load"() : () -> i64
  // Sequential access: stride = 1
  %1 = "aiacccel.matmul"() : () -> i64
  %2 = "aiacccel.add"(%1) : (i64) -> i64
  "aiacccel.dealloc"(%0) : (memref<1024xi32>) -> ()
  return
}

// Test 7: Stride Optimization (Non-sequential to sequential)
func.func @test_stride_optimization() {
  %0 = "aiacccel.alloc"() {size = 8192 : i64, stride = 8 : i64} : () -> memref<2048xi32>
  // Original: stride = 8 (cache-unfriendly)
  %1 = "aiacccel.transpose"() {permutation = array<i64: 1, 0>} : () -> i64
  // After optimization: stride = 1 (cache-friendly)
  %2 = "aiacccel.matmul"(%1) : (i64) -> i64
  "aiacccel.dealloc"(%0) : (memref<2048xi32>) -> ()
  return
}

// Test 8: Temporal Locality (Data reuse)
func.func @test_temporal_locality() {
  %0 = "aiacccel.alloc"() {size = 2048 : i64} : () -> memref<512xi32>
  // Reuse distance = 2 (cache-friendly)
  %1 = "aiacccel.load"() : () -> i64
  %2 = "aiacccel.relu"(%1) : (i64) -> i64
  %3 = "aiacccel.add"(%2) : (i64) -> i64
  %4 = "aiacccel.load"() : () -> i64  // 다시 같은 버퍼 접근
  "aiacccel.dealloc"(%0) : (memref<512xi32>) -> ()
  return
}

// Test 9: Memory Tile Mapping (Blocking strategy)
func.func @test_memory_tile_mapping() {
  // 큰 행렬을 작은 타일로 분할
  %0 = "aiacccel.alloc"() {size = 65536 : i64, tile_size = 256 : i64} : () -> memref<16384xi32>
  %1 = "aiacccel.matmul"() : () -> i64
  %2 = "aiacccel.add"(%1) : (i64) -> i64
  "aiacccel.dealloc"(%0) : (memref<16384xi32>) -> ()
  return
}

// Test 10: Bandwidth Saturation (Multiple streams)
func.func @test_bandwidth_saturation() {
  // 대역폭 포화 상태 시뮬레이션
  %a0 = "aiacccel.alloc"() {size = 2048 : i64} : () -> memref<512xi32>
  %a1 = "aiacccel.alloc"() {size = 2048 : i64} : () -> memref<512xi32>
  %a2 = "aiacccel.alloc"() {size = 2048 : i64} : () -> memref<512xi32>

  // 3개 병렬 스트림
  %1 = "aiacccel.matmul"() : () -> i64
  %2 = "aiacccel.conv2d"() : () -> i64
  %3 = "aiacccel.transpose"() : () -> i64

  "aiacccel.dealloc"(%a0) : (memref<512xi32>) -> ()
  "aiacccel.dealloc"(%a1) : (memref<512xi32>) -> ()
  "aiacccel.dealloc"(%a2) : (memref<512xi32>) -> ()
  return
}

// Test 11: Resnet Block with Bufferization
func.func @test_resnet_block_bufferization() {
  // Residual connection with optimal buffering
  %inp = "aiacccel.alloc"() {size = 4096 : i64, level = 2 : i32} : () -> memref<1024xi32>

  // Main path
  %conv1 = "aiacccel.conv2d"() : () -> i64
  %bn1 = "aiacccel.batch_norm"(%conv1) : (i64) -> (i64, i64, i64)
  %relu1 = "aiacccel.relu"(%bn1#0) : (i64) -> i64

  %conv2 = "aiacccel.conv2d"() : () -> i64
  %bn2 = "aiacccel.batch_norm"(%conv2) : (i64) -> (i64, i64, i64)

  // Residual addition
  %add = "aiacccel.add"(%relu1, %bn2#0) : (i64, i64) -> i64
  %relu2 = "aiacccel.relu"(%add) : (i64) -> i64

  "aiacccel.dealloc"(%inp) : (memref<1024xi32>) -> ()
  return
}

// Test 12: Prefetching and Pipelining
func.func @test_prefetching_pipelining() {
  // Prefetch + Pipeline overlap
  %0 = "aiacccel.alloc"() {size = 8192 : i64} : () -> memref<2048xi32>

  // Load phase (prefetch)
  %data = "aiacccel.load"() : () -> i64

  // Compute phase (overlap with prefetch)
  %compute1 = "aiacccel.matmul"() : () -> i64
  %compute2 = "aiacccel.relu"(%compute1) : (i64) -> i64

  // Store phase
  "aiacccel.store"(%compute2) : (i64) -> ()

  "aiacccel.dealloc"(%0) : (memref<2048xi32>) -> ()
  return
}
