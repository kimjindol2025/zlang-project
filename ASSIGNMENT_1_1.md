# 📝 Assignment 1.1: IR 분석과 레지스터 추적

## 🎯 목표

LLVM Intermediate Representation(IR)의 **실제 동작**을 직접 경험하기

## 📋 과제 개요

| 항목 | 내용 |
|------|------|
| 난이도 | ⭐⭐☆☆☆ (초급) |
| 소요 시간 | 2-3시간 |
| 제출 형식 | Markdown 파일 + LLVM IR 파일 |
| 평가 기준 | 분석의 깊이와 정확도 |

---

## 🔧 준비 단계

### 필요한 도구
```bash
# Zig 설치 확인
zig version

# 간단한 테스트
zig build-obj --help | grep "femit-llvm"
```

### 작업 디렉토리 생성
```bash
mkdir -p ~/zlang-project/RESEARCH_NOTES/1_1_ASSIGNMENT
cd ~/zlang-project/RESEARCH_NOTES/1_1_ASSIGNMENT
```

---

## 📚 Task 1: IR 분석 (기본)

### 1-1. 첫 번째 LLVM IR 생성

**파일**: `simple_math.zig`

```zig
pub fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

pub fn multiply(a: i32, b: i32) -> i32 {
    return a * b;
}

pub fn subtract(a: i32, b: i32) -> i32 {
    return a - b;
}
```

**생성 명령**:
```bash
cd ~/zig-study
zig build-obj simple_math.zig -femit-llvm-ir

# 결과: simple_math.llvm.ir 또는 simple_math.ll
```

### 1-2. IR 파일 분석

생성된 `.ll` 파일을 열어서 다음을 기록하세요:

#### 📝 Task 1 답안: `Task1_IR_Analysis.md`

```markdown
# Task 1: IR 분석

## 1. @add 함수의 LLVM IR

```llvm
[여기에 생성된 @add 함수의 IR을 붙여넣기]
```

## 2. IR 코드의 의미 분석

### 매개변수 분석
- %0은 무엇인가? (첫 번째 매개변수 'a')
- %1은 무엇인가? (두 번째 매개변수 'b')

### 연산 분석
- add i32 %0, %1 의 의미?
  - 연산: ?
  - 피연산자: ?
  - 결과 저장: ?

### 반환값
- ret i32 %2 의 의미?

## 3. @multiply 함수와 @add의 차이점

[분석...]

## 4. 각 함수별 가상 레지스터 개수

- @add: ? 개
- @multiply: ? 개
- @subtract: ? 개

이 차이는 왜 발생할까?

## 5. 기관찰

[가장 놀라운 점은?]
```

---

## 📊 Task 2: 레지스터 추적 (중급)

### 2-1. 데이터 흐름 분석

더 복잡한 코드를 작성하고 분석하세요:

**파일**: `complex_calc.zig`

```zig
pub fn fma(a: i32, b: i32, c: i32) -> i32 {
    // Fused Multiply-Add: (a * b) + c
    return a * b + c;
}

pub fn chain_ops(x: i32) -> i32 {
    // x → x+1 → (x+1)*2 → (x+1)*2-3
    let step1 = x + 1;
    let step2 = step1 * 2;
    let step3 = step2 - 3;
    return step3;
}
```

생성 및 분석:
```bash
zig build-obj complex_calc.zig -femit-llvm-ir
```

### 2-2. 데이터 흐름도 그리기

#### 📝 Task 2 답안: `Task2_Register_Tracking.md`

```markdown
# Task 2: 레지스터 추적

## 1. @fma 함수의 데이터 흐름

### LLVM IR
```llvm
[생성된 IR 붙여넣기]
```

### 데이터 흐름도
```
%0 (a) ──┐
         ├──→ [mul] ──→ %2 ──┐
%1 (b) ──┘                   │
                             ├──→ [add] ──→ %3 ──→ [ret]
%2 (c) ───────────────────────┘
```

## 2. @chain_ops 함수 분석

### 초기 예상
```
x → x+1 → (x+1)*2 → (x+1)*2-3
```

### 실제 LLVM IR
```llvm
[생성된 IR]
```

### 레지스터 매핑
```
입력: %0 (x)
단계 1: %1 = x + 1
단계 2: %2 = %1 * 2
단계 3: %3 = %2 - 3
출력: %3
```

## 3. SSA 형식의 이점

Zig 코드에서 변수가 "재할당"되지만, LLVM IR에서는 새로운 %N을 사용한다. 왜?

[답변...]

## 4. 레지스터 이름의 의미

LLVM에서 %0, %1, %2, ... 는:
- 할당 순서인가?
- 중요도 순인가?
- 단순히 고유 ID인가?

[분석...]
```

---

## 🔤 Task 3: 타입 대응표 (초급)

### 3-1. 다양한 타입 테스트

**파일**: `type_mapping.zig`

```zig
pub fn test_u8(x: u8) -> u8 {
    return x + 1;
}

pub fn test_i32(x: i32) -> i32 {
    return x * 2;
}

pub fn test_i64(x: i64) -> i64 {
    return x - 10;
}

pub fn test_f32(x: f32) -> f32 {
    return x * 3.14;
}

pub fn test_f64(x: f64) -> f64 {
    return x + 2.71;
}

pub fn test_bool(x: bool) -> bool {
    return x;  // Not x 를 하려 했지만 패스스루로 간단히
}
```

생성:
```bash
zig build-obj type_mapping.zig -femit-llvm-ir
```

### 3-2. 타입 매핑 표 작성

#### 📝 Task 3 답안: `Task3_Type_Mapping.md`

```markdown
# Task 3: 타입 대응표

## 1. Zig → LLVM 타입 매핑

| Zig 타입 | LLVM 타입 | 크기 | 비고 |
|---------|----------|------|------|
| u8 | i8 | 8비트 | ? |
| i32 | i32 | 32비트 | |
| i64 | i64 | 64비트 | |
| u64 | i64 | 64비트 | 부호 없음도 i64? |
| f32 | float | 32비트 | |
| f64 | double | 64비트 | |
| bool | i1 | 1비트 | true=1, false=0 |

## 2. 타입별 연산 분석

### u8 연산 (test_u8)
```llvm
[생성된 IR]
```

특징:
- 덧셈이 어떻게 표현되는가?
- 오버플로우 처리는?

### f32 연산 (test_f32)
```llvm
[생성된 IR]
```

특징:
- 정수 연산과 다른 점?
- 3.14는 어떻게 표현?

## 3. 흥미로운 발견

### f64의 상수 표현
```llvm
[f64 상수를 찾아서 어떻게 표현되는지 기록]
```

### 부호 있음/없음 타입
u8과 i8이 LLVM에서 같은 i8이 되는데, 이것이 가능한 이유는?

[답변...]

## 4. 타입 시스템의 관찰

- Zig의 bool은 LLVM의 i1인데, 왜 1비트만 사용할까?
- 정수 타입(i32)의 연산과 부동소수점(double) 연산이 다른가?
- 타입 정보가 최적화에 어떻게 활용될까?
```

---

## 🔍 Task 4: 효율성 분석 (심화)

### 4-1. 왜 LLVM IR을 거칠까?

#### 📝 Task 4 답안: `Task4_Efficiency_Analysis.md`

```markdown
# Task 4: LLVM IR을 거치는 효율성 분석

## 주제: "왜 모든 컴파일러가 직접 기계어를 만들지 않고 LLVM IR이라는 중간 단계를 거치는가?"

## 1. 언어 독립성 (Language Independence)

### 현실 시나리오

**직접 기계어 생성 방식:**
```
Zig → x86-64 기계어
C++ → x86-64 기계어
Python → x86-64 기계어
(각 언어마다 백엔드 개발 필요!)

만약 ARM64 지원을 추가하려면?
- Zig에 ARM64 백엔드 추가
- C++에 ARM64 백엔드 추가
- Python에 ARM64 백엔드 추가
(3배의 작업!)
```

**LLVM 사용 방식:**
```
Zig ──┐
C++  ├──→ LLVM IR ──→ x86-64 백엔드 ──→ x86-64 기계어
Python ──┘           ┌───────────────────┘
                     │
                     ├──→ ARM64 백엔드 ──→ ARM64 기계어
                     │
                     └──→ RISC-V 백엔드 ──→ RISC-V 기계어

새로운 CPU 추가 시: 백엔드만 1개 추가!
```

### 분석

같은 LLVM IR로부터 여러 CPU 타겟을 지원한다면:
- 코드 중복을 얼마나 줄일 수 있는가?
- 유지보수 비용은?

[당신의 분석...]

## 2. 최적화의 집중화 (Optimization Centralization)

### 최적화 기회

**직접 기계어 생성:**
- Zig: 자체 최적화 엔진 개발 필요
- C++: Clang 자체 최적화 개발
- Python: Python 자체 최적화 개발
(같은 최적화를 여러 번 개발!)

**LLVM 사용:**
- 모든 언어가 **LLVM의 통합 최적화** 사용
- 새로운 최적화가 추가되면 모든 언어가 즉시 이득

### 예시: Constant Folding

```zig
let x = 5 + 3;  // 이것은 8로 최적화 가능
```

Constant Folding은:
- IR 레벨에서 언어 무관하게 적용 가능
- LLVM이 한 번 구현하면 C, C++, Zig, Rust 모두 이득

### 분석

만약 새로운 최적화 기법(예: 루프 벡터화)을 개발하려면:
- 직접 기계어: 각 언어마다 구현 필요 (비효율)
- LLVM: 한 곳에서만 구현 (효율적)

얼마나 효율적인가?

[당신의 분석...]

## 3. 검증 가능성 (Verifiability)

### SSA 형식의 이점

LLVM IR은 SSA 형식입니다:
- 모든 변수는 정확히 한 번만 할당됨
- 이것이 최적화를 수학적으로 명확하게 만듦

```llvm
; 명확한 데이터 흐름
%0 = load i32, ptr %x
%1 = add i32 %0, 5
%2 = mul i32 %1, 2
ret i32 %2
```

이 코드를 보면:
- %1의 값이 무엇인지 명확함
- %2의 값이 무엇인지 명확함
- 각 계산의 결과가 정확하게 추적됨

### 직접 기계어의 어려움

기계어는 레지스터를 다시 사용합니다:
```asm
mov rax, [rbx]      ; rax = x
add rax, 5          ; rax = x + 5
imul rax, 2         ; rax = (x + 5) * 2
ret
```

이 코드에서:
- rax가 여러 값을 저장
- 각 단계의 데이터 흐름 추적이 어려움
- 최적화 알고리즘이 더 복잡

### 분석

SSA 형식이 최적화를 얼마나 더 쉽게 만드는가?

[당신의 분석...]

## 4. 성능 관점 (Performance Perspective)

### LLVM 최적화의 효과

Task 1에서 생성한 코드를 다시 보세요:

```zig
pub fn chain_ops(x: i32) -> i32 {
    let step1 = x + 1;
    let step2 = step1 * 2;
    let step3 = step2 - 3;
    return step3;
}
```

이론적으로 최적화된 형태:
```
step3 = ((x + 1) * 2) - 3
      = (2x + 2) - 3
      = 2x - 1
```

LLVM이 이런 대수적 최적화까지 할까?
- 생성된 IR을 확인해보세요
- 기계어는 더 간단할까?

### 분석

[당신의 발견...]

## 5. 결론: LLVM IR의 가치

### 직접 기계어 생성 vs LLVM 사용

| 측면 | 직접 기계어 | LLVM |
|------|-----------|------|
| 언어별 백엔드 개발 | N개 (N = 언어 수 × CPU 수) | 1개 (CPU당 1개) |
| 최적화 엔진 | 각 언어마다 | 통합 |
| 새 CPU 지원 | 모든 언어에 추가 | 백엔드만 추가 |
| 검증 난이도 | 높음 | 낮음 (SSA) |

### 최종 평가

LLVM IR을 거치는 것의 가장 큰 이점은?

[당신의 결론...]

---

## 📌 추가 질문 (보너스)

1. LLVM이 왜 "Low Level Virtual Machine"이라는 이름을 갖게 되었을까?

2. 만약 LLVM이 없다면 Zig는 어떤 구조를 가져야 할까?

3. LLVM IR은 기계어보다 높은 수준의 정보를 가지고 있다. 이 정보가 어떻게 최적화에 활용될까?

4. 새로운 프로그래밍 언어를 만든다면, 왜 LLVM을 기반으로 하는 것이 현명할까?
```

---

## 📤 제출

모든 Task 파일을 다음 구조로 정렬하세요:

```
~/zlang-project/RESEARCH_NOTES/1_1_ASSIGNMENT/
├── Task1_IR_Analysis.md
├── Task2_Register_Tracking.md
├── Task3_Type_Mapping.md
├── Task4_Efficiency_Analysis.md
└── (생성된 LLVM IR 파일들)
    ├── simple_math.ll
    ├── complex_calc.ll
    └── type_mapping.ll
```

---

## ✅ 평가 기준

| 기준 | 배점 |
|------|------|
| Task 1: IR 분석 정확도 | 20점 |
| Task 2: 데이터 흐름 이해 | 20점 |
| Task 3: 타입 대응 완성도 | 20점 |
| Task 4: 효율성 분석 깊이 | 30점 |
| 문서 작성 품질 | 10점 |
| **총점** | **100점** |

---

## 🎯 다음 단계

Assignment 1.1 완료 후:

1. 작성한 파일들을 gogs에 커밋
2. 1.2 강의 자료 준비
3. **1.2: LLVM C API 코드 생성기 기초** 시작!

---

**시작 가능할 때**: 강의 1.1을 읽은 후 언제든 시작 가능
**예상 완료 시간**: 2-3시간
**난이도**: ⭐⭐☆☆☆ (기초)

행운을 빕니다! 🚀
