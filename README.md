# 🌟 Z-Lang: 실시간 시스템을 위한 프로그래밍 언어

![Status](https://img.shields.io/badge/Status-Development-blue)
![Version](https://img.shields.io/badge/Version-0.1--Dev-green)
![License](https://img.shields.io/badge/License-MIT-orange)

## 소개 (Introduction)

**Z-Lang**은 LLVM을 기반으로 설계된 **새로운 프로그래밍 언어**입니다.

자동차, 의료기기, 항공우주 등 **실시간 시스템이 요구되는 분야**에 특화되어 있습니다.

```
특징:
  ✅ Zero-Latency (GC 없음)
  ✅ Type Safe (컴파일 타임 검증)
  ✅ Memory Safe (Ownership + Lifetime)
  ✅ Real-Time Guarantees (WCET 분석)
  ✅ Multi-Platform (x86-64, ARM, RISC-V)
```

---

## 🎯 프로젝트 목표

### 단기 (8주)
- ✅ 완전한 컴파일러 구현 (Lexer → Codegen)
- ✅ LLVM IR 생성
- ✅ 표준 라이브러리 제공
- ✅ 학술 논문 작성

### 장기
- 🎯 자율주행차 운영체제 기본 언어
- 🎯 의료기기 실시간 제어 언어
- 🎯 항공우주 안전-critical 시스템 언어
- 🎯 오픈소스 커뮤니티 주도

---

## 🚀 빠른 시작 (Quick Start)

### 사전 요구사항
```
- C++17 호환 컴파일러 (GCC 7+, Clang 5+)
- LLVM 15.0+
- CMake 3.16+
- Git
```

### 빌드 (구현 예정)
```bash
git clone https://gogs.dclub.kr/kim/zlang.git
cd zlang-project
mkdir build && cd build
cmake ..
make
./zlang --version
```

### 첫 프로그램
```z-lang
// hello.z
fn main() {
    println("Hello, Z-Lang!");
}
```

```bash
./zlang hello.z
./hello
# 출력: Hello, Z-Lang!
```

---

## 📚 문서

### 프로젝트 문서
| 문서 | 설명 |
|------|------|
| [PROJECT_PLAN.md](PROJECT_PLAN.md) | 8주 학습 계획 및 성공 기준 |
| [LANGUAGE_SPECIFICATION.md](docs/LANGUAGE_SPECIFICATION.md) | 완전한 언어 명세 (v0.1) |
| [COMPILER_DESIGN.md](docs/COMPILER_DESIGN.md) | 컴파일러 내부 구조 (개발 중) |
| [USER_GUIDE.md](docs/USER_GUIDE.md) | 사용자 가이드 (개발 중) |
| [PERFORMANCE_ANALYSIS.md](docs/PERFORMANCE_ANALYSIS.md) | 성능 분석 및 벤치마크 (개발 중) |

### LLVM 강의 시리즈 (LLVM Legend Course)

#### 1단계: Backend (IR 생성과 실행)
| 강의 | 과제 | 상태 |
|------|------|------|
| [1.1: LLVM 아키텍처와 IR](docs/1_1_LLVM_ARCHITECTURE_AND_IR.md) | [ASSIGNMENT_1_1.md](ASSIGNMENT_1_1.md) | ✅ 완료 |
| [1.2: LLVM C API 코드 생성](docs/1_2_LLVM_C_API_CODEGEN.md) | [ASSIGNMENT_1_2.md](ASSIGNMENT_1_2.md) | ✅ 완료 |
| [1.3: 제어 흐름 (Control Flow)](docs/1_3_LLVM_CONTROL_FLOW.md) | [ASSIGNMENT_1_3.md](ASSIGNMENT_1_3.md) | ✅ 완료 |
| [1.4: 타입 시스템과 복합 구조체](docs/1_4_LLVM_TYPE_SYSTEMS.md) | [ASSIGNMENT_1_4.md](ASSIGNMENT_1_4.md) | ✅ 완료 |
| [1.5: JIT 컴파일과 실행 엔진](docs/1_5_LLVM_JIT_EXECUTION_ENGINE.md) | [ASSIGNMENT_1_5.md](ASSIGNMENT_1_5.md) | ✅ 완료 |

#### 2단계: Frontend (코드 분석과 변환)
| 강의 | 과제 | 상태 |
|------|------|------|
| [2.1: 어휘 분석(Lexing)](docs/2_1_LLVM_LEXING_TOKENIZATION.md) | [ASSIGNMENT_2_1.md](ASSIGNMENT_2_1.md) | ✅ 완료 |
| [2.2: 구문 분석(Parsing)](docs/2_2_LLVM_PARSING.md) | [ASSIGNMENT_2_2.md](ASSIGNMENT_2_2.md) | ✅ 완료 |
| [2.3: 의미 분석(Semantic Analysis)](docs/2_3_LLVM_SEMANTIC_ANALYSIS_PREVIEW.md) | 예정 | 🔜 2026-03-16 |

---

## 🏗️ 프로젝트 구조

```
zlang-project/
├── src/
│   ├── main.cpp                 # 진입점
│   ├── lexer/
│   │   ├── Lexer.h
│   │   ├── Lexer.cpp
│   │   └── Token.h
│   ├── parser/
│   │   ├── Parser.h
│   │   ├── Parser.cpp
│   │   └── ASTNode.h
│   ├── semantic/
│   │   ├── TypeChecker.h
│   │   ├── TypeChecker.cpp
│   │   └── SymbolTable.h
│   └── codegen/
│       ├── CodeGenerator.h
│       ├── CodeGenerator.cpp
│       └── LLVMBackend.h
├── docs/
│   ├── LANGUAGE_SPECIFICATION.md
│   ├── COMPILER_DESIGN.md
│   ├── USER_GUIDE.md
│   ├── PERFORMANCE_ANALYSIS.md
│   └── RESEARCH_PAPER.md
├── examples/
│   ├── hello_world.z
│   ├── fibonacci.z
│   ├── rtos_example.z
│   └── automotive.z
├── tests/
│   ├── lexer_test.cpp
│   ├── parser_test.cpp
│   ├── codegen_test.cpp
│   └── integration_test.cpp
├── CMakeLists.txt
├── PROJECT_PLAN.md
└── README.md
```

---

## 📖 언어 특징

### 1. Zero-Latency 설계

```z-lang
// 실시간 시스템에 최적화
#[wcet_bound = "100_us"]  // WCET 자동 검증
fn sensor_read() -> i32 {
    return read_adc(0);
}
```

### 2. Type Safety

```z-lang
// Rust 스타일 Ownership
let x = Box::new(42);
let y = x;  // 소유권 이동
// x는 더 이상 사용 불가 (컴파일 에러)

// Lifetime 추적
fn borrow<'a>(x: &'a i32) -> &'a i32 {
    return x;
}
```

### 3. Real-Time Guarantees

```z-lang
// 메모리 할당 제약
#[no_alloc]
fn hardreal_time() {
    let arr: [i32; 100];  // 정적 할당만 OK
    // let vec = Vec::new();  // 컴파일 에러!
}
```

### 4. Multi-Platform Support

```z-lang
// 타겟 명시
#[target = "x86-64-linux"]
#[target = "arm64-bare-metal"]
#[target = "riscv64-imac"]

fn main() { }
```

---

## 🛠️ 개발 로드맵

### Week 1: LLVM 기초 + 언어 설계
```
├─ LLVM C++ API 학습
├─ LLVM IR 이해
├─ Z-Lang 문법 정의
└─ 렉서 기본 구조
```

### Week 2: Lexer & Parser
```
├─ 토큰화 (Tokenization)
├─ 구문 분석 (Parsing)
├─ AST 생성
└─ 파서 검증 테스트
```

### Week 3: 의미 분석
```
├─ 타입 검사 (Type Checking)
├─ 심볼 테이블 관리
├─ 에러 처리
└─ 의미 분석 테스트
```

### Week 4: LLVM 코드 생성
```
├─ AST → LLVM IR 변환
├─ 함수 생성
├─ 변수 관리
└─ 코드 생성 테스트
```

### Week 5: Real-Time 기능
```
├─ WCET 자동 계산
├─ 메모리 안전성 검증
├─ Safety-Critical 지원
└─ 실시간 검증 테스트
```

### Week 6: 표준 라이브러리
```
├─ I/O 함수 (println, read, write)
├─ Math 함수 (sqrt, sin, cos, ...)
├─ Collections (Vec, HashMap)
└─ 라이브러리 테스트
```

### Week 7: 최적화 & 성능
```
├─ LLVM Pass 작성
├─ 성능 벤치마킹
├─ 프로파일링
└─ 성능 최적화
```

### Week 8: 문서화 & 완성
```
├─ 최종 문서화
├─ 학술 논문 작성
├─ 오픈소스 준비
└─ 최종 발표
```

---

## 🧪 테스트

```bash
# (개발 진행 중)

# 모든 테스트 실행
cmake --build . --target test

# 특정 테스트
./lexer_test
./parser_test
./codegen_test
```

---

## 📊 성과 기대

### 코드 통계
```
총 줄 수:       5,000~10,000 줄 (C++)
구현 파일:      20~30개
테스트 케이스:  50+ 개
테스트 커버율:  > 80%
```

### 문서
```
언어 명세:      100+ 페이지
사용자 가이드:  50+ 페이지
연구 논문:      20~30 페이지
────────────────────────────
총 문서:        200+ 페이지
```

### 기대 효과
```
✅ 새로운 프로그래밍 언어 창조
✅ LLVM 전문가 수준 지식
✅ 컴파일러 설계 마스터
✅ Real-Time 시스템 최적화 경험
```

---

## 🎓 학습 배경

이 프로젝트는 **Zig 운영체제 전공**의 최종 프로젝트입니다.

```
【 학습 경로 】

Python University ✅ (완료)
  └─ 설계 패턴, 테스트 주도 개발

Zig 운영체제 전공 ✅ (완료)
  ├─ Lesson 1-2: 부팅 및 메모리
  ├─ Lesson 3-1~3-5: 단일 머신 커널
  ├─ Lesson 3-6~3-8: 완전한 마이크로커널
  └─ PostDoc Phase 1-4: 분산 시스템
       ├─ IPC 최적화 (10배)
       ├─ 분산 RPC (<1ms)
       ├─ 프로세스 마이그레이션 (1000+ 노드)
       └─ RTOS 검증 (ASIL D)

Z-Lang 컴파일러 설계 🚀 (현재)
  └─ LLVM 기반 새로운 언어
```

---

## 🏆 철학

> **"기록이 증명이다"**

모든 학습과 성과는:
- ✅ 실행 가능한 코드로 입증
- ✅ gogs 저장소에 영구 기록
- ✅ 학술 논문 형식으로 문서화
- ✅ 재현 가능한 벤치마크 포함

---

## 📚 참고 자료

### LLVM 공식 문서
- [LLVM Language Reference Manual](https://llvm.org/docs/LangRef/)
- [LLVM Programmer's Manual](https://llvm.org/docs/ProgrammersManual/)
- [LLVM Tutorial - Kaleidoscope](https://llvm.org/docs/tutorial/)

### 컴파일러 설계
- "Compilers: Principles, Techniques, and Tools" (Dragon Book)
- "Modern Compiler Implementation" (Andrew Appel)
- "Engineering a Compiler" (Cooper & Torczon)

### 실시간 시스템
- "Real-Time Systems: Design & Analysis" (Jane Liu)
- ISO 26262 (자동차 안전)
- IEC 61508 (일반 기능 안전)

---

## 🤝 기여 (Contributing)

이 프로젝트는 **개인 학습 프로젝트**이지만, 나중에 오픈소스로 전환될 예정입니다.

---

## 📄 라이선스

MIT License - 자세한 내용은 LICENSE 파일 참조

---

## 📞 연락처

- **저장소**: https://gogs.dclub.kr/kim/zlang.git
- **관련 프로젝트**: [ZigOS](https://gogs.dclub.kr/kim/zig-study.git)

---

## 📈 프로젝트 상태

### 핵심 마일스톤
| 항목 | 상태 | 진행도 |
|------|------|--------|
| 프로젝트 계획 | ✅ 완료 | 100% |
| 언어 명세 | ✅ v0.1 | 100% |
| LLVM Backend (1.1-1.5) | ✅ 완료 | 100% |
| LLVM Frontend - Lexing (2.1) | ✅ 완료 | 100% |
| LLVM Frontend - Parsing (2.2) | ✅ 완료 | 100% |
| LLVM Frontend - Semantic + Ownership (2.3) | ✅ 완료 | 100% |
| LLVM Frontend - IR Codegen (2.4) | ✅ 완료 | 100% |
| LLVM Frontend - Optimization & Verification (2.5) | ⏳ 계획 | 0% |
| Standard Library | ⏳ 계획 | 0% |
| Documentation | ✅ 진행 중 | 95% |

### LLVM 강의 진행 상황

#### Backend 단계 (완료)
| 강의 | 상태 |
|------|------|
| 1.1: LLVM 아키텍처와 IR | ✅ 완료 |
| 1.2: LLVM C API 코드 생성 | ✅ 완료 |
| 1.3: 제어 흐름 (Control Flow) | ✅ 완료 |
| 1.4: 타입 시스템과 복합 구조체 | ✅ 완료 |
| 1.5: JIT 컴파일과 실행 엔진 | ✅ 완료 |

#### Frontend 단계 (4/4 완료) ✨
| 강의 | 상태 | 공개 |
|------|------|------|
| 2.1: 어휘 분석 (Lexing) | ✅ 완료 | 2026-02-26 |
| 2.2: 구문 분석 (Parsing) | ✅ 완료 | 2026-02-26 |
| 2.3: 의미 분석 - 소유권 시스템 (Semantic + Ownership) | ✅ 완료 | 2026-02-26 |
| 2.4: LLVM IR 코드 생성 (Codegen) | ✅ 완료 | 2026-02-26 |
| 2.5: 최적화 및 검증 (Optimization & Verification) | 🔜 준비 중 | 2026-05-11 |

---

**최종 목표**: 2026년 4월 16일 완료 🎯

**좌우명**: "전설 되기" 🏆

---

## 🎓 Post-Doctoral Research: AI-Accelerator Compiler Stack

**프로젝트 시작일**: 2026-02-27
**상태**: Phase 1 준비 완료

### 📋 프로젝트 개요

이기종 메모리 계층을 가진 AI 가속기용 컴파일러 스택 구축

### 🏗️ 4-Layer Optimization Architecture

```
L1: Graph-Level (High-Level IR)
    └─ Dialect: linalg fusion, operator tiling
    
L2: Memory-Level (Mid-Level IR)
    └─ Bufferization, Double Buffering, SRAM optimization
    
L3: Vector-Level (Low-Level IR)
    └─ Vectorization, SIMD outer-product optimization
    
L4: Target-Level (Back-end)
    └─ CUDA, HIP, TPU, Custom ISA code generation
```

### 📚 Post-Doc Phase 1 Roadmap (8주)

| 주차 | Task | 산출물 | 검증 |
|------|------|--------|------|
| 1-2주 | AIAccel Dialect 정의 (TableGen) | AIAccel.td (200줄) | mlir-opt 파싱 |
| 3주 | Operation 구현 (MatMul, Conv2D) | AIAccelOps.cpp (500줄) | 20개 테스트 |
| 4주 | Fusion Pass | FusionPass.cpp (400줄) | 2x 성능 향상 |
| 5주 | Bufferization + Double Buffering | BufferizationPass.cpp (600줄) | 메모리 30% 감소 |
| 6주 | Polyhedral Loop Tiling | LoopTilingPass.cpp (700줄) | Affine 분석 |
| 7주 | GPU/TPU 백엔드 | GPUBackend.cpp (400줄) | CUDA/LLVM IR |
| 8주 | Integration & Paper | research_paper.md (3000줄) | End-to-End 벤치마크 |

### 🎯 최종 목표

**학술 논문**: "AI-Accelerator Compiler Stack: Multi-Level Optimization for Heterogeneous Memory Hierarchy"

- 목표 성능: **기존 대비 5-10배 향상**
- 투고 대상: IEEE/ACM TOCS, ASPLOS
- 핵심 기여: Custom Dialect + Polyhedral Optimization + Multi-Target Backend

### 💾 저장소 구조

```
zlang-project/
├── docs/
│   ├── PHASE_3_TEST_RESULTS.md
│   └── FINAL_EVALUATION.md
├── src/
│   ├── main.cpp (Frontend)
│   ├── lexer/, parser/, codegen/
│   └── analysis/ (WCET)
│
└── post-doc/ (NEW)
    ├── dialect/
    │   ├── AIAccel.td
    │   └── AIAccelOps.cpp
    ├── passes/
    │   ├── FusionPass.cpp
    │   ├── BufferizationPass.cpp
    │   ├── LoopTilingPass.cpp
    │   └── VectorizationPass.cpp
    ├── backends/
    │   ├── GPUBackend.cpp
    │   └── TPUBackend.cpp
    └── test/
        └── end_to_end_benchmarks.cpp
```

### 🔬 연구 철학

> "복잡함은 추상화로 제어하고, 성능은 수학으로 증명하며, 결과는 테스트로 신뢰를 얻는다."

**기록이 증명이다.** (Record is Your Proof)

---

