//===- BackendPass.h - GPU/TPU Backend Code Generation ---*- C++ -*-===//

#ifndef AIACCCEL_BACKEND_PASS_H
#define AIACCCEL_BACKEND_PASS_H

#include "mlir/Pass/Pass.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/PatternMatch.h"
#include <vector>
#include <map>
#include <memory>
#include <string>

namespace mlir {
namespace aiacccel {

//===----------------------------------------------------------------------===//
// Backend Target Architecture
//===----------------------------------------------------------------------===//

enum class BackendTarget {
  CPU,        // x86-64, ARM64
  GPU_NVIDIA, // CUDA (Tesla, RTX, H100)
  GPU_AMD,    // HIP (CDNA, RDNA)
  TPU,        // Google TPU (v4, v5)
  FPGA        // Intel/Xilinx FPGA
};

enum class MemoryLayout {
  UNIFIED,    // Unified memory (GPU unified address space)
  SPLIT,      // Split memory (separate host/device)
  HIERARCHICAL // Multi-level (HBM, DRAM, SRAM)
};

enum class ComputeMode {
  SYNCHRONOUS,  // Blocking kernel calls
  ASYNCHRONOUS, // Non-blocking with events
  PIPELINED     // Overlapped compute & transfer
};

//===----------------------------------------------------------------------===//
// GPU/TPU Specific Information
//===----------------------------------------------------------------------===//

struct GPUInfo {
  std::string name;                    // GPU 이름
  int computeCapability;               // Compute capability (70, 80, 90)
  int64_t globalMemory;                // Global memory (bytes)
  int64_t sharedMemoryPerBlock;        // Shared memory per block
  int maxThreadsPerBlock;              // 최대 스레드/블록
  int maxBlocksPerDim;                 // 최대 블록 수/차원
  double peakFlopsFP32;                // FP32 peak FLOPS
  double peakBandwidth;                // Peak memory bandwidth (GB/s)

  GPUInfo()
    : name("Unknown"), computeCapability(70), globalMemory(0),
      sharedMemoryPerBlock(0), maxThreadsPerBlock(1024),
      maxBlocksPerDim(65535), peakFlopsFP32(0), peakBandwidth(0) {}
};

struct TPUInfo {
  std::string name;                    // TPU 버전 (v4, v5)
  int64_t matmulUnits;                 // 행렬곱 유닛 수
  int64_t vectorUnits;                 // 벡터 유닛 수
  int64_t hbmMemory;                   // HBM 메모리 (bytes)
  double peakTFLOPS;                   // Peak TFLOPS
  bool supportsSparsity;               // 희소성 지원?
  bool supportsDynamicShapes;          // 동적 모양 지원?

  TPUInfo()
    : name("TPUv4"), matmulUnits(128), vectorUnits(128),
      hbmMemory(32LL * 1024 * 1024 * 1024), peakTFLOPS(275),
      supportsSparsity(true), supportsDynamicShapes(false) {}
};

//===----------------------------------------------------------------------===//
// Kernel Configuration
//===----------------------------------------------------------------------===//

struct KernelConfig {
  std::vector<int> blockDim;           // 블록 크기 (x, y, z)
  std::vector<int> gridDim;            // 그리드 크기
  int64_t sharedMemoryBytes;           // Shared memory 사용량
  std::string kernelName;              // 커널 이름
  int occupancy;                       // Occupancy (%)

  KernelConfig() : sharedMemoryBytes(0), occupancy(50) {}
};

//===----------------------------------------------------------------------===//
// BackendPass - GPU/TPU Code Generation
//===----------------------------------------------------------------------===//

class BackendPass
    : public PassWrapper<BackendPass, OperationPass<func::FuncOp>> {
public:
  BackendPass() = default;
  explicit BackendPass(BackendTarget target) : targetBackend(target) {}

  StringRef getArgument() const final { return "aiacccel-backend"; }
  StringRef getDescription() const final {
    return "Generate GPU/TPU optimized code for target accelerator";
  }

  void runOnOperation() override;

private:
  BackendTarget targetBackend = BackendTarget::GPU_NVIDIA;

  // 타겟 정보 조회
  struct TargetInfo {
    // GPU info for different architectures
    GPUInfo getNVIDIAGPUInfo(int computeCapability);
    GPUInfo getAMDGPUInfo(std::string chipName);

    // TPU info
    TPUInfo getTPUInfo(std::string version);

    // 장치 정보 자동 감지
    BackendTarget detectTargetDevice();
  };

  // 커널 구성 최적화
  KernelConfig optimizeKernelConfig(Operation* op, int64_t workSize);

  // Block/Grid 차원 결정
  void determineBlockGridDims(const KernelConfig& config,
                              std::vector<int>& blockDim,
                              std::vector<int>& gridDim);

  // 메모리 배치 분석
  MemoryLayout analyzeMemoryPlacement(func::FuncOp func);

  // 메모리 전송 최적화
  LogicalResult optimizeMemoryTransfers(func::FuncOp func,
                                        const MemoryLayout& layout);

  // GPU 커널 코드 생성
  LogicalResult generateGPUKernel(Operation* op, const KernelConfig& config);

  // TPU 프로그램 생성
  LogicalResult generateTPUProgram(func::FuncOp func);

  // 동기화 및 이벤트 삽입
  LogicalResult insertSynchronization(func::FuncOp func,
                                      ComputeMode mode);

  // Occupancy 계산
  double calculateOccupancy(const KernelConfig& config, const GPUInfo& gpu);

  // 성능 예측
  double predictExecutionTime(Operation* op, const KernelConfig& config,
                             const GPUInfo& gpu);

  // Async 작업 파이프라인
  LogicalResult createAsyncPipeline(func::FuncOp func);
};

//===----------------------------------------------------------------------===//
// Pass 등록
//===----------------------------------------------------------------------===//

#define GEN_PASS_REGISTRATION
#include "AIAccel/AIAccelPasses.h.inc"

} // namespace aiacccel
} // namespace mlir

#endif // AIACCCEL_BACKEND_PASS_H
