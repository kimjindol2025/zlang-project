# Post-Doctoral Research: AI-Accelerator Compiler Stack

**시작일**: 2026-02-27
**예상 기간**: 8주
**상태**: Phase 1 초기화 완료

---

## 📋 연구 개요

### 목표
이기종 메모리 계층 구조를 가진 AI 가속기(GPU, TPU, FPGA)용 **다층 최적화 컴파일러 스택** 구축

### 핵심 혁신
1. **Custom AIAccel Dialect**: 50개 이상의 AI-특화 연산 정의 (TableGen)
2. **자동 최적화 패스**: 폴리헤드럴 모델 기반 루프 타일링
3. **멀티 타겟 백엔드**: CUDA, HIP, TPU, 맞춤형 ISA 지원

---

## 🏗️ 4-Layer Architecture

### Layer 1: Graph-Level (High-Level IR)
**목표**: 연산자 간의 불필요한 데이터 이동 제거

**구현**:
- linalg Dialect 기반 Fusion Pass
- 연산자 병합 및 메모리 재사용 분석

**예상 성과**: 메모리 트래픽 30-50% 감소

### Layer 2: Memory-Level (Mid-Level IR)
**목표**: 제한된 로컬 메모리(SRAM) 효율적 관리

**구현**:
- Bufferization Pass: 텐서 → memref 변환
- Double Buffering: 계산-통신 오버래프

**예상 성과**: 메모리 대역폭 사용률 80% 이상

### Layer 3: Vector-Level (Low-Level IR)
**목표**: SIMD 유닛 최대 활용

**구현**:
- Vector Dialect 기반 Vectorization
- Outer-product 최적화

**예상 성과**: 벡터화율 95% 이상

### Layer 4: Target-Level (Back-end)
**목표**: 하드웨어 고유 명령어로의 최종 변환

**구현**:
- GPU: CUDA/HIP Intrinsic 호출
- TPU: TPU Instruction set 매핑
- 바이너리 생성

**예상 성과**: 네이티브 성능 달성

---

## 📅 8주 Roadmap

### Week 1-2: AIAccel Dialect 정의 (TableGen)
- [ ] AIAccel.td: 기본 Type/Op 정의
- [ ] 50개 Operation 정의 (MatMul, Conv2D, GEMM, etc)
- [ ] 테스트: mlir-opt 파싱 검증

**산출물**: AIAccel.td (200줄), AIAccelOps.cpp (300줄)

### Week 3: Operation 구현
- [ ] AIAccelOps.cpp: Operation 인터페이스 구현
- [ ] Builder, Verifier 작성
- [ ] 20개 기본 테스트 케이스

**산출물**: AIAccelOps.cpp (500줄), test suite (300줄)

### Week 4: Fusion Pass (연산자 통합)
- [ ] FusionPass: 2개 이상의 연산자 병합
- [ ] 데이터 의존성 분석
- [ ] 벤치마크: 2x 성능 향상 검증

**산출물**: FusionPass.cpp (400줄)

### Week 5: Bufferization + Double Buffering
- [ ] BufferizationPass: 텐서 → memref 변환
- [ ] SRAM 할당 최적화
- [ ] 메모리 30% 감소 검증

**산출물**: BufferizationPass.cpp (600줄)

### Week 6: Polyhedral Loop Tiling
- [ ] LoopTilingPass: 최적 타일 크기 자동 계산
- [ ] Affine 분석 기반 증명
- [ ] 캐시 히트율 분석

**산출물**: LoopTilingPass.cpp (700줄)

### Week 7: GPU/TPU 백엔드
- [ ] GPUBackend: CUDA/HIP 코드 생성
- [ ] TPUBackend: TPU instruction set 매핑
- [ ] 실제 하드웨어 벤치마크

**산출물**: GPUBackend.cpp (400줄), TPUBackend.cpp (300줄)

### Week 8: 통합 & 논문
- [ ] End-to-End 테스트
- [ ] 성능 벤치마크 (vs TVM, XLA)
- [ ] 논문 초안 작성

**산출물**: research_paper.md (3000줄)

---

## 🔬 검증 전략

### 1. 단위 테스트 (Unit Tests)
```cpp
// AIAccel Operations 테스트
TEST(AIAccelOps, MatMulShape) {
  // MatMul output shape 검증
}

TEST(FusionPass, ConvMatMulFusion) {
  // Conv2d + MatMul fusion 검증
}
```

### 2. 통합 테스트 (Integration Tests)
- mlir-opt으로 변환 검증
- FileCheck를 통한 IR 비교
- 예상 성능 대비 측정

### 3. 벤치마크 (Benchmarks)
```bash
# ResNet-50 컴파일 성능
# AlexNet 메모리 사용량
# Vision Transformer FLOPS
```

---

## 📊 성공 지표

| 지표 | 목표 | 측정 방법 |
|------|------|---------|
| **성능** | 5-10배 향상 | 실제 하드웨어 벤치마크 |
| **메모리** | 30% 감소 | SRAM 할당량 분석 |
| **컴파일 속도** | <1초 | ResNet-50 컴파일 시간 |
| **코드 품질** | 100% 테스트 커버리지 | Jest/pytest 검증 |

---

## 📚 학술 논문 계획

### 제목
"AI-Accelerator Compiler Stack: Multi-Level Optimization for Heterogeneous Memory Hierarchy"

### 구성
1. **Introduction** (왜 필요한가?)
2. **Related Work** (기존 연구 vs 우리의 혁신)
3. **Methodology** (4-Layer 설계 상세)
4. **Evaluation** (벤치마크 결과)
5. **Conclusion & Future Work**

### 투고 대상
- IEEE/ACM TOCS (Transactions on Computer Systems)
- ASPLOS (Architectural Support for Programming Languages and Operating Systems)
- PLDI (Programming Language Design and Implementation)

---

## 🎯 핵심 원칙

> **"복잡함은 추상화로 제어하고, 성능은 수학으로 증명하며, 결과는 테스트로 신뢰를 얻는다."**

1. **추상화**: AIAccel Dialect로 복잡한 AI 알고리즘 단순화
2. **증명**: Affine 분석으로 최적화 정확성 보증
3. **테스트**: FileCheck, 벤치마크로 모든 결과 검증

---

**기록이 증명이다.** (Record is Your Proof)

*작성자*: Claude Haiku 4.5 (Post-Doc Researcher)
*시작*: 2026-02-27
