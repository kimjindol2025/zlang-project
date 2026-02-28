# 🎬 Preview: LLVM 1.5 - JIT 컴파일과 실행 엔진 설계

> **최종 업데이트**: 2026-02-26
> **상태**: 🔜 준비 중 (다음 주 공개)
> **기대도**: ⭐⭐⭐⭐⭐ (가장 흥미로운 단계!)

---

## 🎯 학습 경로

```
【 LLVM 전설 완성도 】

1.1 ✅ LLVM 아키텍처와 IR
     "컴파일러란 무엇인가?"
     └─ 이론 이해

1.2 ✅ LLVM C API 코드 생성
     "함수를 만드는 방법"
     └─ 기초 구현

1.3 ✅ 제어 흐름 (Control Flow)
     "프로그램이 판단하고 반복한다"
     └─ 고급 구현

1.4 ✅ 타입 시스템과 복합 구조체
     "데이터를 체계적으로 조직하자"
     └─ 메모리 설계

1.5 🔜 JIT 컴파일과 실행 엔진
     "우리 코드가 살아난다!"
     └─ 기계어 생성 및 실행 ← 당신은 여기서 지금까지의 모든 것을 확인!

1.6 🔜 함수 호출과 calling convention
1.7 🔜 고급 최적화와 리팩토링
1.8 🔜 실제 언어 컴파일러 설계
```

---

## 🔥 1.5가 특별한 이유

### 지금까지의 여정

```
1단계: IR 생성 (텍스트)
     create_point() 함수
     %point = alloca %Point
     store i32 10, ptr %x_ptr
     ...

↓ (보이지 않는 마법)

2단계: 기계어로 변환
     mov rax, rsp
     sub rsp, 16
     mov dword [rax], 10
     mov dword [rax+4], 42
     ...

↓

3단계: CPU에서 직접 실행!
     함수 호출 → 메모리 할당 → 값 저장 → 반환
     실제 데이터 흐름이 시작된다!
```

### 1.5에서 할 것

```
우리가 만든 IR이:

IR (텍스트) → LLVM Pass (최적화) → 기계어 (바이너리) → CPU 실행

이 모든 과정을 **프로그램 실행 중**에 수행합니다!
(Just-In-Time = 필요한 순간에 컴파일)
```

---

## 💡 핵심 개념

### ExecutionEngine

```c
// LLVM의 마법의 엔진
LLVMExecutionEngineRef engine = LLVMCreateExecutionEngineForModule(...);

// 함수 포인터를 C처럼 사용할 수 있게!
int (*func)(int, int) = (int (*)(int, int))
    LLVMGetFunctionAddress(engine, "add");

// 직접 호출!
int result = func(10, 20);  // 출력: 30
```

### JIT vs Ahead-of-Time (AOT)

```
【 Ahead-of-Time 】
프로그램 실행 전: 모든 코드 컴파일
장점: 빠른 시작
단점: 컴파일 시간 오래

【 Just-In-Time 】
프로그램 실행 중: 필요한 부분만 컴파일
장점: 최적화 시간 있음, 실행 중 적응
단점: 초기 지연 가능

【 LLVM의 혼합 전략 】
- Eager Compilation: 중요한 함수는 미리 컴파일
- Lazy Compilation: 가끔 사용되는 함수는 나중에
- Optimization: 런타임 프로파일링 기반 최적화
```

---

## 🎨 1.5 예제 미리보기

### 완전한 흐름

```zig
// 1. IR 생성 (1.1-1.4에서 배운 것)
const module = create_llvm_module();

// 2. ExecutionEngine 생성 (1.5에서 배울 것)
const engine = LLVMCreateExecutionEngineForModule(...);

// 3. 함수 포인터 얻기
const func_ptr = LLVMGetFunctionAddress(engine, "add");

// 4. C처럼 호출
const result = @intToPtr(fn(i32, i32) i32, func_ptr)(10, 20);

// 5. 결과 확인
std.debug.print("10 + 20 = {}\n", .{result});  // 출력: 30
```

### 예상 결과

```
【 프로그램 실행 】

$ zig build run

LLVM Module 생성...
add() 함수 생성...
mul() 함수 생성...
abs() 함수 생성...

ExecutionEngine 초기화...
JIT 컴파일 시작...

호출: add(10, 20) = 30 ✓
호출: mul(10, 20) = 200 ✓
호출: abs(-15) = 15 ✓

모든 함수 실행 완료!
```

---

## 📚 1.5의 주요 API

### ExecutionEngine 생성

```c
// 1. Module을 기반으로 Engine 생성
LLVMExecutionEngineRef LLVMCreateExecutionEngineForModule(
    LLVMModuleRef M,
    char **OutError
);

// 2. JIT 컴파일러 직접 생성 (고급)
LLVMExecutionEngineRef LLVMCreateJITCompilerForModule(
    LLVMModuleRef M,
    unsigned OptLevel,  // 0-3
    char **OutError
);

// 3. MCJit (Modern JIT, 권장)
LLVMExecutionEngineRef LLVMCreateMCJITCompilerForModule(
    LLVMModuleRef M,
    struct LLVMMCJITCompilerOptions *Options,
    size_t SizeOfOptions,
    char **OutError
);
```

### 함수 실행

```c
// 함수 포인터 얻기
typedef int (*IntBinOpFn)(int, int);

IntBinOpFn func = (IntBinOpFn)
    LLVMGetFunctionAddress(engine, "add");

// 호출
int result = func(10, 20);  // 30

// Generic 호출 (모든 함수 타입)
LLVMGenericValueRef LLVMRunFunction(
    LLVMExecutionEngineRef EE,
    LLVMValueRef F,
    LLVMGenericValueRef *Args,
    unsigned NumArgs
);
```

---

## 🔬 1.5의 고급 기능

### 최적화 레벨

```c
// 최적화 없음 (빠른 컴파일)
LLVMCreateJITCompilerForModule(M, 0, &error);

// 일부 최적화 (권장)
LLVMCreateJITCompilerForModule(M, 2, &error);

// 최대 최적화 (느린 컴파일, 빠른 실행)
LLVMCreateJITCompilerForModule(M, 3, &error);
```

### 성능 측정

```zig
// 1. IR 생성 시간
const ir_start = std.time.nanoTimestamp();
create_complex_module();
const ir_time = std.time.nanoTimestamp() - ir_start;

// 2. JIT 컴파일 시간
const compile_start = std.time.nanoTimestamp();
const engine = LLVMCreateExecutionEngineForModule(...);
const compile_time = std.time.nanoTimestamp() - compile_start;

// 3. 함수 실행 시간
const exec_start = std.time.nanoTimestamp();
const result = func(10, 20);
const exec_time = std.time.nanoTimestamp() - exec_start;

std.debug.print("IR 생성: {} ns\n", .{ir_time});
std.debug.print("JIT 컴파일: {} ns\n", .{compile_time});
std.debug.print("함수 실행: {} ns\n", .{exec_time});
```

---

## 🎯 1.5 Task 미리보기

### Task 1: ExecutionEngine 초기화

```zig
fn main() {
    // 1. Module 생성 (1.4까지의 방식)
    const module = create_point_module();

    // 2. ExecutionEngine 생성
    var error_msg: [*c]u8 = null;
    const engine = c.LLVMCreateExecutionEngineForModule(
        module,
        &error_msg
    );

    if (engine == null) {
        // 에러 처리
    }

    // 3. 엔진 정리
    c.LLVMDisposeExecutionEngine(engine);
}
```

### Task 2: 함수 호출

```zig
// add(10, 20) 함수 호출
const add_fn = (fn(i32, i32) i32)@intToPtr(
    usize,
    c.LLVMGetFunctionAddress(engine, "add")
);

const result = add_fn(10, 20);
std.debug.print("add(10, 20) = {}\n", .{result});
```

### Task 3: 여러 함수 호출

```zig
// Point 구조체 반환 함수
// 데이터 타입 복잡성 처리

const create_point_fn = ...;
const point = create_point_fn();
std.debug.print("Point.x = {}, Point.y = {}\n", .{
    point.x,
    point.y
});
```

### Task 4: 성능 분석

```zig
// IR 생성 vs JIT 컴파일 vs 함수 실행
// 각 단계의 시간 측정 및 비교

// 최적화 레벨별 실행 시간 비교
const time_O0 = benchmark_with_optimization(0);
const time_O2 = benchmark_with_optimization(2);
const time_O3 = benchmark_with_optimization(3);

std.debug.print("O0: {} ns, O2: {} ns, O3: {} ns\n", .{
    time_O0,
    time_O2,
    time_O3
});
```

---

## 🚀 1.5 이후의 전망

### 컴파일러 파이프라인 완성

```
【 완전한 LLVM 컴파일러 파이프라인 】

소스 코드
    ↓
렉서 (Lexer) - 토큰화
    ↓
파서 (Parser) - 구문 분석
    ↓
의미 분석 (Semantic Analysis) - 타입 체크
    ↓
IR 생성 (LLVM IR) ← 1.1-1.4 배운 것
    ↓
최적화 (LLVM Passes)
    ↓
JIT 컴파일 (기계어 생성) ← 1.5에서 배울 것
    ↓
실행 (CPU 직접 실행)
    ↓
결과 출력
```

### 다음 단계들의 방향

```
1.6: Calling Convention
     - 함수 호출시 인자/반환값 전달 방식
     - 스택 레이아웃
     - 레지스터 할당

1.7: 고급 최적화
     - Dead Code Elimination
     - Loop Unrolling
     - Inlining
     - SIMD 벡터화

1.8: 실제 언어 구현
     - Z-Lang 컴파일러 완성
     - 5000+ 줄 C++ 코드
     - 실제 프로그램 컴파일 가능
```

---

## 💡 왜 1.5가 중요한가?

### 이론에서 현실로

```
1-4단계: "이렇게 생성되어야 한다"는 이론
1.5단계: "실제로 생성되고 실행된다"는 증명

우리가 만든 IR 코드가:
- 실제로 유효한가?
- 성능은 어떤가?
- 최적화는 제대로 동작하는가?

이 모든 것을 **실제 데이터**로 확인할 수 있습니다.
```

### 자신감 및 이해도 상승

```
【 학습 전 】
"LLVM IR이 뭔지 알지만, 실제로 동작하는지는?"

【 학습 후 】
"내가 만든 IR을 호출했고, 정답을 받았다!"
→ 완벽한 이해
→ 자신감 극상승
→ 컴파일러 설계자 수준으로 도약
```

---

## 📊 1.5의 기대 효과

### 측정 가능한 성과

```
【 실습 결과물 】
✅ add(), mul(), abs() 함수 JIT 실행
✅ Point 구조체 생성 및 필드 접근
✅ 5개 Point 배열 생성 및 조회

【 성능 데이터 】
✅ IR 생성 시간: ~1-5 ms
✅ JIT 컴파일 시간: ~10-50 ms (O2 기준)
✅ 함수 실행 시간: ~100 ns (나노초!)

【 최적화 비교 】
✅ O0 vs O2 vs O3 실행 시간 비교
✅ 복잡한 함수의 최적화 효과 측정
```

---

## 🎓 학습 경로 정리

```
【 LLVM 전설 학습 경로 】

Week 1: 기초 이론 + 기본 구현
  1.1: IR 개념, SSA, 3가지 형식
  1.2: 함수 생성, 단순 연산
  1.3: 제어 흐름, PHI 노드, 루프

Week 2: 데이터 구조 + 실행
  1.4: 타입 시스템, 구조체, GEP
  1.5: JIT 컴파일, 함수 실행 ← 이제부터 현실!

Week 3-4: 고급 주제
  1.6: Calling Convention, 함수 호출
  1.7: 최적화 Pass, 성능 튜닝
  1.8: 실제 언어 설계 (Z-Lang)
```

---

## 🎬 다음 주의 여정

```
【 1.5 강의 예정 내용 】

📖 강의 (약 3,000줄)
  ├─ ExecutionEngine 아키텍처
  ├─ JIT vs AOT 비교
  ├─ Calling Convention 소개
  ├─ 함수 호출 방식
  ├─ 최적화 레벨별 성능
  └─ 실행 중 에러 처리

💻 과제 (약 1,500줄)
  ├─ Task 1: ExecutionEngine 초기화
  ├─ Task 2: 단순 함수 호출
  ├─ Task 3: 구조체 반환 함수
  ├─ Task 4: 성능 벤치마킹
  └─ 심화: 동적 함수 체인

📊 분석
  ├─ JIT 컴파일 시간 분석
  ├─ 최적화 효과 측정
  ├─ 함수 호출 오버헤드
  └─ 메모리 사용량 프로파일링
```

---

## 🌟 1.5의 흥분 요소

### 실제 결과를 본다

```
프로그램 실행:
  add(10, 20) = 30 ✓
  mul(10, 20) = 200 ✓
  abs(-5) = 5 ✓

우리가 만든 IR 코드가:
- ✅ 문법적으로 올바르고
- ✅ 의미적으로 정확하고
- ✅ CPU에서 실행되고
- ✅ 정확한 결과를 낸다!

이보다 더 큰 보상이 있을까요?
```

### 자유도 상승

```
1.5 전:
"정해진 과제만 따라한다"

1.5 후:
"내가 원하는 함수를 만들고, 실행하고, 성능을 측정한다!"

예시:
- 피보나치 함수 JIT 컴파일
- 2D 변환 행렬 계산
- 소수 판정법 성능 비교
- Mandelbrot 집합 계산

모든 것이 가능해집니다!
```

---

## 📢 최종 메시지

> **당신은 지금 **컴파일러 개발자**의 문턱에 서 있습니다.**

1.5를 마치면:
- ✅ LLVM을 완전히 이해
- ✅ IR과 기계어의 연결고리 파악
- ✅ 실제 컴파일러 구현 가능

Z-Lang 컴파일러는 더 이상 꿈이 아닙니다.
**현실이 될 준비가 되어 있습니다!**

---

**예정 공개**: 2026-03-09 (일)
**난이도**: ⭐⭐⭐⭐☆ (고급)
**흥미도**: ⭐⭐⭐⭐⭐ (최고!)
**보상**: **"내 코드가 직접 실행된다!"** 🚀

---

**기록이 증명이다.**
**"저장 필수 너는 기록이 증명이다 gogs."**

우리의 여정이 계속됩니다...

당신의 **"다음"**을 기다리고 있습니다! ✨

---

*"전설 되기는 계속된다..."* 🏆
