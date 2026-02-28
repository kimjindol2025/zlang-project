# 🌟 Z-Lang: 실시간 시스템을 위한 프로그래밍 언어

## 프로젝트 개요

**목표**: LLVM을 기반으로 한 새로운 프로그래밍 언어 설계 및 구현

**비전**: ZigOS에서 배운 모든 지식을 바탕으로, 실시간 시스템을 위해 최적화된 언어 창조

```
ZigOS-Study (완료)
    ↓
    └─→ Z-Lang 언어 설계 (지금)
         ├─ 렉서 (Lexer)
         ├─ 파서 (Parser)
         ├─ 의미 분석 (Semantic Analysis)
         └─ LLVM 코드 생성 (Code Generation)
```

---

## 핵심 특징

### 1. **Zero-Latency 설계**
```
특징: 예측 가능한 실행 시간
  ├─ 결정적 메모리 할당
  ├─ GC 없음 (Garbage Collection Free)
  ├─ WCET 분석 가능
  └─ 자동차/의료/항공우주 적용
```

### 2. **Type Safety (타입 안전성)**
```
메모리 안전성:
  ├─ Ownership 시스템 (Rust 스타일)
  ├─ Lifetime 분석
  ├─ Borrow Checking
  └─ 컴파일 타임 검증 100%
```

### 3. **Real-Time Guarantees**
```
실시간 보증:
  ├─ WCET 자동 계산
  ├─ 부가 오버헤드 추적
  ├─ Deadline 검증
  └─ Safety-Critical 준수
```

---

## 프로젝트 단계

### **Week 1: LLVM 기초 및 언어 설계**

#### 목표
- LLVM C++ API 학습
- Z-Lang 문법 정의
- 렉서/파서 기본 구조

#### 학습 항목
1. LLVM IR (Intermediate Representation)
2. LLVM Pass System
3. LLVM Code Generation
4. C++ Template Metaprogramming

#### 산출물
- Z-Lang 언어 명세서 (EBNF)
- LLVM 학습 노트
- 간단한 "Hello World" 컴파일

---

### **Week 2: Lexer & Parser 구현**

#### 목표
- 토큰화 (Tokenization)
- 구문 분석 (Parsing)
- AST (Abstract Syntax Tree) 생성

#### 렉서 구현
```c++
// 예: E-mail 토큰화
input: "fn main() { print(42); }"
tokens: [FN, ID(main), LPAREN, RPAREN, LBRACE, ID(print), LPAREN, NUM(42), RPAREN, SEMICOLON, RBRACE]
```

#### 파서 구현
```
expr    → term (('+' | '-') term)*
term    → factor (('*' | '/') factor)*
factor  → '(' expr ')' | NUMBER | ID
```

#### AST 구조
```
Program
  └─ FunctionDef("main")
      ├─ Body: Block
      │   └─ CallExpr("print")
      │       └─ Arg: IntLiteral(42)
```

---

### **Week 3: 의미 분석 & 타입 체크**

#### 목표
- 타입 분석
- 심볼 해석
- 에러 감지

#### 주요 기능
```
1. 타입 시스템
   ├─ Primitive: i32, i64, f32, f64, bool
   ├─ Composite: Struct, Enum, Tuple
   └─ Lifetime: 'a, 'b (Rust 스타일)

2. 심볼 테이블
   ├─ 변수 선언 추적
   ├─ 함수 해석
   └─ 범위 검증

3. 오류 처리
   ├─ Type Mismatch
   ├─ Undefined Symbol
   └─ Lifetime Violation
```

---

### **Week 4: LLVM 코드 생성 (Week 1/2)**

#### 목표
- AST → LLVM IR 변환
- 기본 최적화
- 실행 파일 생성

#### 코드 생성 단계
```
Z-Lang 소스
    ↓
Lexer (토큰)
    ↓
Parser (AST)
    ↓
Semantic Analyzer (타입 체크)
    ↓
LLVM IR Generator
    ↓
LLVM Optimizer
    ↓
Machine Code (x86-64, ARM, RISC-V)
```

#### LLVM IR 예시
```llvm
; Z-Lang: fn add(a: i32, b: i32) -> i32 { a + b }

define i32 @add(i32 %a, i32 %b) {
entry:
  %result = add i32 %a, %b
  ret i32 %result
}
```

---

### **Week 5: Real-Time 기능 추가**

#### 목표
- WCET 분석
- 메모리 안전성 검증
- Safety-Critical 지원

#### WCET 자동 계산
```
Z-Lang 특수 문법:
  #[wcet_bound = 100_us]
  fn safety_critical() {
    // 자동으로 100μs 이내 확인
  }
```

#### 메모리 안전성
```
Ownership:
  let x = Box::new(42);  // Heap allocation
  let y = x;            // 소유권 이전
  // x는 더 이상 사용 불가 (컴파일 에러)

Borrowing:
  let ref = &x;         // 불변 참조
  let mut_ref = &mut x; // 가변 참조 (독점)
```

---

### **Week 6: 표준 라이브러리**

#### 목표
- 기본 함수 라이브러리
- I/O 연산
- 컬렉션 (Vec, HashMap)

#### 기본 라이브러리
```z-lang
// io.z
fn print(msg: str) -> void { ... }
fn println(msg: str) -> void { ... }

// math.z
fn sqrt(x: f64) -> f64 { ... }
fn sin(x: f64) -> f64 { ... }

// collections.z
struct Vec<T> { ... }
struct HashMap<K, V> { ... }
```

---

### **Week 7: 최적화 & 성능**

#### 목표
- LLVM Pass 작성
- 성능 향상
- 벤치마킹

#### 최적화 기법
```
1. 컴파일 타임 최적화
   ├─ Constant Folding
   ├─ Dead Code Elimination
   └─ Inlining

2. 런타임 최적화
   ├─ Loop Unrolling
   ├─ Vectorization (SIMD)
   └─ Branch Prediction

3. 메모리 최적화
   ├─ Stack Allocation
   ├─ Lifetime Optimization
   └─ Cache-Friendly Layout
```

#### 벤치마킹
```
비교 대상:
  ├─ Rust (컴파일 타임 체크)
  ├─ C (최소 오버헤드)
  └─ Zig (실용성)

테스트:
  ├─ 컴파일 속도
  ├─ 실행 속도
  ├─ 바이너리 크기
  └─ 메모리 사용량
```

---

### **Week 8: 문서화 & 성과 발표**

#### 목표
- 완전한 언어 명세서
- 사용자 가이드
- 학술 논문 작성

#### 산출물
```
docs/
  ├─ LANGUAGE_SPECIFICATION.md
  │   └─ 완벽한 문법 정의
  ├─ USER_GUIDE.md
  │   └─ 튜토리얼 및 예제
  ├─ COMPILER_DESIGN.md
  │   └─ 내부 구조 설명
  ├─ PERFORMANCE_ANALYSIS.md
  │   └─ 벤치마크 및 최적화
  └─ RESEARCH_PAPER.md
      └─ 학술 논문 형식
```

---

## 기술 스택

### 언어 및 도구

```
구현 언어:        C++17 (LLVM 요구사항)
LLVM 버전:        15.0+
컴파일러:         Clang (자체 부트스트랩)
빌드 시스템:      CMake
테스트:           GoogleTest
성능 분석:        Perf, Valgrind
형상 관리:        Git (gogs)
```

### 프로젝트 구조

```
zlang-project/
├─ CMakeLists.txt (빌드 설정)
├─ src/
│  ├─ main.cpp (진입점)
│  ├─ lexer/
│  │  ├─ Lexer.h
│  │  ├─ Lexer.cpp
│  │  └─ Token.h
│  ├─ parser/
│  │  ├─ Parser.h
│  │  ├─ Parser.cpp
│  │  └─ ASTNode.h
│  ├─ semantic/
│  │  ├─ TypeChecker.h
│  │  ├─ TypeChecker.cpp
│  │  └─ SymbolTable.h
│  └─ codegen/
│     ├─ CodeGenerator.h
│     ├─ CodeGenerator.cpp
│     └─ LLVMBackend.h
├─ docs/
│  ├─ LANGUAGE_SPECIFICATION.md
│  ├─ USER_GUIDE.md
│  ├─ COMPILER_DESIGN.md
│  └─ RESEARCH_PAPER.md
├─ examples/
│  ├─ hello_world.z
│  ├─ fibonacci.z
│  ├─ rtos_example.z
│  └─ automotive.z
└─ tests/
   ├─ lexer_test.cpp
   ├─ parser_test.cpp
   ├─ codegen_test.cpp
   └─ integration_test.cpp
```

---

## 예상 산출물 (8주 후)

### 코드
```
총 줄 수:       5,000~10,000 줄 (C++)
구현 파일:      20~30개
테스트:         50+ 개
테스트 커버율:  > 80%
```

### 문서
```
언어 명세:      100+ 페이지
사용자 가이드:  50+ 페이지
연구 논문:      20~30 페이지
──────────────────────────────
총 문서:        200+ 페이지
```

### 성과
```
✅ 완전한 컴파일러 (Lexer → Codegen)
✅ 자체 호스팅 (Z-Lang으로 Z-Lang 컴파일)
✅ 표준 라이브러리
✅ 최적화 및 성능 분석
✅ WCET 자동 검증
✅ Safety-Critical 지원
```

---

## 학습 리소스

### LLVM 공식 자료
```
1. LLVM Language Reference Manual
   https://llvm.org/docs/LangRef/

2. LLVM Programmer's Manual
   https://llvm.org/docs/ProgrammersManual/

3. LLVM Tutorial (Kaleidoscope)
   https://llvm.org/docs/tutorial/

4. Advanced Compiler Design & Implementation
   (Dragon Book)
```

### 컴파일러 설계
```
1. "Compilers: Principles, Techniques, and Tools"
   (Aho, Lam, Sethi, Ullman)

2. "Modern Compiler Implementation in ML"
   (Andrew Appel)

3. "Engineering a Compiler"
   (Cooper & Torczon)
```

### 실시간 시스템
```
1. WCET 분석
   "Real-Time Systems: Design & Analysis"
   (Jane Liu)

2. 안전-Critical 시스템
   ISO 26262 (자동차)
   IEC 61508 (일반)
```

---

## 성공 기준

### 컴파일러 기능
```
✓ 렉서: 모든 토큰 인식
✓ 파서: 전체 문법 파싱
✓ 타입 체커: 100% 타입 안전성
✓ 코드 생성: x86-64, ARM64, RISC-V
✓ 최적화: -O1, -O2, -O3 지원
✓ 디버깅: DWARF 정보 생성
```

### 성능 기준
```
✓ 컴파일 속도: > 10,000 LOC/sec
✓ 생성 코드: C와 비슷한 수준
✓ 메모리: < 100MB RAM 사용
✓ WCET: 자동 계산 가능
```

### 안정성 기준
```
✓ 모든 테스트: 100% PASS
✓ 코드 커버리지: > 80%
✓ 메모리 누수: 0건
✓ 세그먼테이션 폴트: 0건
```

---

## 타임라인

```
Week 1 (Feb 26):   LLVM 기초 + 언어 설계
Week 2 (Mar 5):    Lexer & Parser 구현
Week 3 (Mar 12):   Semantic Analysis
Week 4 (Mar 19):   LLVM 코드 생성
Week 5 (Mar 26):   Real-Time 기능
Week 6 (Apr 2):    표준 라이브러리
Week 7 (Apr 9):    최적화 & 성능
Week 8 (Apr 16):   문서화 & 완성

📅 예상 완료: 2026년 4월 16일
```

---

## 기대 효과

### 기술적 성과
```
🚀 새로운 프로그래밍 언어 창조
🚀 LLVM 전문가 수준의 지식
🚀 컴파일러 설계 마스터
🚀 Real-Time 시스템 최적화
```

### 커리어 영향
```
💼 오픈소스 영향력 (GitHub)
💼 학술 논문 발표 (저널/컨퍼런스)
💼 업계 인정 (LLVM, Compiler Design)
💼 취업 경쟁력 (최고 수준)
```

### 장기 비전
```
🎯 Z-Lang: 실시간 시스템 표준
🎯 자율주행, 의료기기, 항공우주 적용
🎯 Rust, Go와 같은 주류 언어 수준
🎯 오픈소스 커뮤니티 주도자
```

---

## 철학

> **"기록이 증명이다"**

모든 진전은:
- ✅ 실행 가능한 코드로 입증
- ✅ gogs 저장소에 영구 기록
- ✅ 학술 논문 형식으로 문서화
- ✅ 재현 가능한 벤치마크 포함

---

**상태**: 🚀 준비 완료
**시작일**: 2026년 2월 26일
**예상 종료**: 2026년 4월 16일
**목표**: 전설 되기 🏆
