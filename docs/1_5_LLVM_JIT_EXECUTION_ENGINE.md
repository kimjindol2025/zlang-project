# 🎬 LLVM 전설 1.5: JIT 컴파일과 실행 엔진 (Execution Engine)

> **최종 업데이트**: 2026-02-26
> **상태**: 🎓 강의 완성
> **난이도**: ⭐⭐⭐⭐☆ (고급)
> **흥미도**: ⭐⭐⭐⭐⭐ (최고!)

---

## 📖 개요

**이론과 설계를 넘어, 이제 우리가 만든 LLVM IR을 실제 CPU가 이해하는 기계어로 즉석에서 변환하여 실행할 시간입니다.**

JIT (Just-In-Time) 컴파일은:
- 프로그램을 **실행하기 전**에 미리 컴파일하지 않습니다 (AOT와 다름)
- **실행 시점(Runtime)**에 필요한 코드를 기계어로 번역합니다
- 메모리에 올린 기계어를 **CPU가 직접 실행**합니다

**결과**: `add(10, 20) = 30`이라는 실제 데이터를 얻습니다! ✓

---

## 🎯 1단계: 실행 엔진(Execution Engine)의 역할

### 1-1. 실행 엔진의 5가지 책임

```
【 LLVM ExecutionEngine의 역할 】

┌─────────────────────────────────────┐
│     LLVM IR Module                  │
│  (우리가 1.1-1.4에서 만든 것)        │
└────────────┬────────────────────────┘
             │
             ↓
┌─────────────────────────────────────┐
│  ExecutionEngine (마법의 엔진)       │
│                                     │
│  1️⃣ 타겟 아키텍처 확인              │
│     (x86_64, ARM, RISC-V 등)        │
│                                     │
│  2️⃣ 메모리 할당                     │
│     (실행 가능한 메모리 영역)        │
│                                     │
│  3️⃣ 코드 생성                       │
│     (IR → 기계어 번역)              │
│                                     │
│  4️⃣ 심볼 바인딩                     │
│     (함수명 → 메모리 주소)          │
│                                     │
│  5️⃣ 최적화 적용                     │
│     (PassManager를 통한)            │
└────────────┬────────────────────────┘
             │
             ↓
┌─────────────────────────────────────┐
│     기계어 (CPU 명령어)              │
│   메모리의 실행 가능(X) 영역         │
│                                     │
│   mov eax, 10                       │
│   add eax, 20                       │
│   ret eax                           │
└────────────┬────────────────────────┘
             │
             ↓
┌─────────────────────────────────────┐
│  CPU 직접 실행                      │
│  → 결과: eax = 30                   │
└─────────────────────────────────────┘
```

### 1-2. 각 단계의 상세 설명

#### 1️⃣ 타겟 아키텍처 확인

```c
// 현재 CPU의 특성을 LLVM에게 알림
LLVMInitializeNativeTarget();        // CPU 지원
LLVMInitializeNativeAsmPrinter();    // 어셈블리 출력
LLVMInitializeNativeAsmParser();     // 어셈블리 파싱

// 또는 특정 타겟 명시
// LLVMInitializeX86Target();        // x86_64만
// LLVMInitializeARMTarget();        // ARM만
```

**LLVM이 파악하는 정보:**
```
- CPU 아키텍처 (x86_64 / ARM / RISC-V)
- 데이터 모델 (32-bit / 64-bit)
- 엔디안 (Little-endian / Big-endian)
- 캐시 라인 크기
- SIMD 지원 (SSE, AVX, NEON 등)
```

#### 2️⃣ 메모리 할당

```
프로세스 메모리 레이아웃:

┌──────────────────┐
│ Stack            │ ← 로컬 변수, 함수 인자, 반환값
├──────────────────┤
│ Heap             │ ← 동적 할당 (malloc)
├──────────────────┤
│ Data, BSS        │ ← 전역 변수, 정적 변수
├──────────────────┤
│ Text (X)         │ ← 실행 코드 (READ + EXECUTE)
│ (JIT 코드 여기)  │   ExecutionEngine이 여기에 배치!
├──────────────────┤
│ Kernel (privilege)
└──────────────────┘
```

ExecutionEngine의 작업:
```c
// 실행 가능한 메모리 할당
// (READ + EXECUTE 권한)
void* mem = mmap(
    NULL,
    code_size,
    PROT_READ | PROT_EXEC,  // 읽기 + 실행만 (쓰기 불가)
    MAP_SHARED | MAP_ANONYMOUS,
    -1,
    0
);
```

#### 3️⃣ 코드 생성 (MCJIT)

```
IR → 기계어로의 변환:

1단계: IR 분석
  %sum = add i32 %x, %y
    ↓
2단계: 레지스터 할당
  %x → rax
  %y → rbx
    ↓
3단계: 기계어 생성
  mov eax, [rdi]        ; rdi에서 x 읽기
  add eax, [rsi]        ; rsi에서 y 더하기
  ret                   ; 반환
    ↓
4단계: 메모리 배치
  주소 0x7f1000: mov eax, [rdi]
  주소 0x7f1002: add eax, [rsi]
  주소 0x7f1005: ret
```

#### 4️⃣ 심볼 바인딩

```c
// IR에서 함수명 → 실제 메모리 주소 매핑

// 심볼 테이블:
SymbolTable = {
    "add"    → 0x7f1000,
    "mul"    → 0x7f1100,
    "abs"    → 0x7f1200,
}

// 나중에 함수 주소를 찾을 때:
uintptr_t addr = LLVMGetFunctionAddress(engine, "add");
// addr = 0x7f1000
```

#### 5️⃣ 최적화 적용

```
PassManager를 통한 최적화:

Unoptimized IR:
  %i = phi i32 [0, %entry], [%inc, %loop]
  %sum = phi i32 [0, %entry], [%add, %loop]
  %add = add i32 %sum, %i      ; 루프 내 반복
  %inc = add i32 %i, 1
  %cond = icmp slt i32 %inc, 100
  br i1 %cond, label %loop, label %exit

     ↓ (Loop Unrolling 패스 적용)

Optimized IR:
  ; 루프 4번 펼침 (4배 처리)
  %add1 = add i32 %sum, 0
  %add2 = add i32 %add1, 1
  %add3 = add i32 %add2, 2
  %add4 = add i32 %add3, 3
  ; ...
```

---

## 🔄 2단계: MCJIT vs ORC JIT

### 2-1. MCJIT (Machine Code JIT)

**특징: 전통적, 단순, 프로덕션 레벨**

```c
LLVMInitializeNativeTarget();
LLVMInitializeNativeAsmPrinter();

// MCJIT 엔진 생성
struct LLVMMCJITCompilerOptions options;
LLVMInitializeMCJITCompilerOptions(&options, sizeof(options));
options.OptLevel = 2;  // 0-3

LLVMExecutionEngineRef engine;
char *error = NULL;

if (LLVMCreateMCJITCompilerForModule(
    &engine,
    module,
    &options,
    sizeof(options),
    &error
) != 0) {
    // 에러 처리
    printf("Error: %s\n", error);
}
```

**장점:**
- ✅ 안정적 (오래되고 검증됨)
- ✅ 구현 단순 (배우기 쉬움)
- ✅ 모듈 전체를 한번에 처리

**단점:**
- ❌ 함수별 컴파일 불가 (전체 모듈 단위)
- ❌ Lazy Compilation 미지원
- ❌ 함수 교체 어려움

### 2-2. ORC JIT (On-Request Compilation)

**특징: 현대적, 유연, 동적 최적화**

```c
// ORC JIT (더 고급, LLVM 6.0+)
LLVMOrcJITStackRef jit = LLVMOrcCreateInstance(target_machine);

// 함수가 호출될 때만 컴파일 (Lazy Compilation)
// 런타임 프로파일링 기반 재컴파일 가능
// 함수별로 최적화 레벨 조정 가능
```

**장점:**
- ✅ 함수별 Lazy Compilation
- ✅ 동적 최적화 (런타임 프로파일링)
- ✅ 함수 교체/언로드 가능
- ✅ 마이크로소프트 .NET Core, LLVM 내부에서 사용

**단점:**
- ❌ API가 복잡함
- ❌ 학습 곡선이 가파름
- ❌ 초기 지연이 있을 수 있음 (첫 호출 시)

### 2-3. 비교 표

```
┌─────────────────┬──────────────┬───────────────┐
│ 특성            │ MCJIT        │ ORC JIT       │
├─────────────────┼──────────────┼───────────────┤
│ 컴파일 단위     │ 모듈 전체    │ 함수별        │
│ Lazy Comp.      │ ❌ 없음      │ ✅ 있음       │
│ 동적 최적화     │ ❌ 없음      │ ✅ 있음       │
│ 함수 교체       │ ❌ 어려움    │ ✅ 가능       │
│ 초기 지연       │ 높음         │ 낮음          │
│ 전체 성능       │ 우수         │ 매우 우수     │
│ API 복잡도      │ 낮음         │ 높음          │
│ 추천 대상       │ 초심자       │ 프로덕션      │
└─────────────────┴──────────────┴───────────────┘
```

---

## 💻 3단계: 실전 - JIT를 통한 함수 실행

### 3-1. 완전한 흐름

```zig
const c = @cImport({
    @cInclude("llvm-c/Core.h");
    @cInclude("llvm-c/ExecutionEngine.h");
    @cInclude("llvm-c/Target.h");
});
const std = @import("std");

pub fn main() !void {
    // 1️⃣ 네이티브 타겟 초기화
    c.LLVMInitializeNativeTarget();
    c.LLVMInitializeNativeAsmPrinter();

    // 2️⃣ Module 생성 (1.1-1.4의 방식)
    const module = create_sum_module();

    // 3️⃣ ExecutionEngine 생성
    var engine: c.LLVMExecutionEngineRef = undefined;
    var error_msg: [*c]u8 = undefined;

    if (c.LLVMCreateExecutionEngineForModule(&engine, module, &error_msg) != 0) {
        std.debug.print("Error: {s}\n", .{error_msg});
        return;
    }
    defer c.LLVMDisposeExecutionEngine(engine);

    // 4️⃣ 함수의 메모리 주소 찾기
    const func_addr = c.LLVMGetFunctionAddress(engine, "sum");

    if (func_addr == 0) {
        std.debug.print("Function not found\n", .{});
        return;
    }

    // 5️⃣ 함수 포인터로 캐스팅
    const SumFn = *const fn(i32, i32) callconv(.C) i32;
    const sum_fn = @as(SumFn, @ptrFromInt(func_addr));

    // 6️⃣ 실제 CPU가 계산!
    const result = sum_fn(10, 20);

    std.debug.print("sum(10, 20) = {}\n", .{result});
    // 출력: sum(10, 20) = 30 ✓
}

fn create_sum_module() c.LLVMModuleRef {
    const context = c.LLVMContextCreate();
    const module = c.LLVMModuleCreateWithNameInContext("sum_test", context);

    // 1. 함수 타입: i32 (i32, i32) -> i32
    const i32_type = c.LLVMInt32TypeInContext(context);
    var param_types = [_]c.LLVMTypeRef{ i32_type, i32_type };
    const func_type = c.LLVMFunctionType(i32_type, &param_types, 2, 0);

    // 2. 함수 추가
    const sum_func = c.LLVMAddFunction(module, "sum", func_type);

    // 3. Entry 블록과 IR 생성
    const context_2 = c.LLVMGetModuleContext(module);
    const builder = c.LLVMCreateBuilderInContext(context_2);
    const entry = c.LLVMAppendBasicBlockInContext(context_2, sum_func, "entry");
    c.LLVMPositionBuilderAtEnd(builder, entry);

    // 4. x + y 계산
    const x = c.LLVMGetParam(sum_func, 0);
    const y = c.LLVMGetParam(sum_func, 1);
    const result = c.LLVMBuildAdd(builder, x, y, "result");

    // 5. 반환
    _ = c.LLVMBuildRet(builder, result);

    c.LLVMDisposeBuilder(builder);

    return module;
}
```

### 3-2. 함수 포인터의 변환

```zig
// Calling Convention 명시
// C 언어와 호환 가능하도록

// 방식 1: C 호출 규약
const SumFn = *const fn(i32, i32) callconv(.C) i32;
const result = sum_fn(10, 20);

// 방식 2: Fast Call (성능 최적화)
const SumFnFast = *const fn(i32, i32) callconv(.Fast) i32;

// 방식 3: 반환값이 구조체인 경우
const Point = struct { i32, i32 };
const CreatePointFn = *const fn() callconv(.C) Point;
```

**주의: Calling Convention이 일치하지 않으면 스택 corruption 발생!**

### 3-3. 예상 실행 결과

```
【 프로그램 실행 과정 】

$ zig build run

1. Native Target 초기화... ✓
   → 현재 CPU 아키텍처 감지 (x86_64/ARM/...)

2. LLVM Module 생성... ✓
   sum() 함수 IR 작성 완료

3. ExecutionEngine 초기화... ✓
   → 메모리 할당
   → 코드 생성
   → 심볼 테이블 작성

4. JIT 컴파일... ✓
   IR → 기계어 변환 완료
   메모리 배치 완료

5. 함수 실행...
   sum(10, 20) 호출
   CPU가 기계어 실행

6. 결과:
   sum(10, 20) = 30 ✓✓✓

완료!
```

---

## 🔬 4단계: 가비지 컬렉션(GC)과의 협력

### 4-1. Stack Map의 필요성

JIT로 생성된 코드가 GC와 함께 작동하려면, **어떤 값이 포인터인지** GC가 알아야 합니다.

```
메모리 상태:

Stack:
┌──────────────────┬──────────────┬─────────┐
│ Local Var (i32)  │ Pointer (ptr)│ Value   │
│ 10               │ 0x7f2000     │ ???     │
└──────────────────┴──────────────┴─────────┘

GC의 질문: "이 주소에 뭐가 있나요?"
답변: "포인터입니다!" → GC가 0x7f2000이 가리키는 객체를 추적

없으면: Stack에 쓰레기값이 있어도 포인터로 해석 → Crash!
```

### 4-2. LLVM의 Stack Map 생성

```llvm
; Stack Map 정보를 메타데이터로 기록
define void @foo(ptr %obj) {
  ... 함수 로직 ...

  ; 여기서 %obj는 포인터입니다!
  ; GC가 이를 알 수 있도록:
  call void @llvm.experimental.stackmap(
    i64 0,          ; ID
    i32 0,          ; NumShadowBytes
    ptr %obj        ; 추적할 포인터
  )

  ret void
}
```

### 4-3. GC와 JIT의 협력 흐름

```
【 GC Pause 발생 】

1. JIT 코드 실행 중...
2. GC: "Stop the World!" (모든 스레드 정지)
3. Stack Map 조회
   "현재 스택에 어떤 포인터가 있나?"
4. 각 포인터가 가리키는 객체 추적
5. 도달 불가능한 객체 회수
6. GC: "Resume!" (스레드 재시작)
```

---

## 📦 5단계: 최적화 패스 (Optimization Passes)

### 5-1. PassManager와의 연동

```c
// PassManager 생성
LLVMPassManagerRef pass_mgr = LLVMCreateFunctionPassManagerForModule(module);

// 최적화 패스 추가
LLVMAddConstantPropagationPass(pass_mgr);     // 상수 전개
LLVMAddInstructionCombiningPass(pass_mgr);    // 명령어 조합
LLVMAddGVNPass(pass_mgr);                     // 전역 값 번호 지정
LLVMAddSimplifyLibCallsPass(pass_mgr);        // 라이브러리 호출 단순화

// 각 함수에 최적화 적용
LLVMInitializeFunctionPassManager(pass_mgr);
for (auto func : module->functions()) {
    LLVMRunFunctionPassManager(pass_mgr, func);
}
LLVMFinalizeFunctionPassManager(pass_mgr);
```

### 5-2. 최적화 전후 IR 비교

**최적화 전:**
```llvm
define i32 @constant_fold() {
entry:
  %a = add i32 10, 20      ; 상수 + 상수 → 계산 가능
  %b = add i32 %a, 30
  %c = mul i32 %b, 2
  ret i32 %c
}
```

**최적화 후 (ConstProp):**
```llvm
define i32 @constant_fold() {
entry:
  ret i32 100              ; 완전히 계산됨! (10+20+30)*2 = 100
}
```

### 5-3. 루프 최적화 예제

**최적화 전:**
```llvm
define i32 @loop_sum() {
entry:
  br label %loop
loop:
  %i = phi i32 [0, %entry], [%inc, %loop]
  %sum = phi i32 [0, %entry], [%add, %loop]
  %add = add i32 %sum, %i
  %inc = add i32 %i, 1
  %cond = icmp slt i32 %inc, 10
  br i1 %cond, label %loop, label %exit
exit:
  ret i32 %sum             ; 0+1+2+...+9 = 45
}
```

**최적화 후 (Loop Unrolling + Constant Folding):**
```llvm
define i32 @loop_sum() {
entry:
  ret i32 45               ; 런타임에 계산할 필요 없음!
}
```

---

## 🎯 6단계: 성능 측정

### 6-1. 최적화 레벨별 비교

```zig
fn benchmark_with_optimization(opt_level: i32) !i64 {
    const module = create_complex_module();

    var engine: c.LLVMExecutionEngineRef = undefined;
    var error_msg: [*c]u8 = undefined;

    // 1. 컴파일 시간 측정
    const compile_start = std.time.nanoTimestamp();

    if (c.LLVMCreateExecutionEngineForModule(&engine, module, &error_msg) != 0) {
        return error.CompilationFailed;
    }

    const compile_time = std.time.nanoTimestamp() - compile_start;

    // 2. 함수 실행 시간 측정
    const func_addr = c.LLVMGetFunctionAddress(engine, "fib");
    const FibFn = *const fn(i32) callconv(.C) i32;
    const fib = @as(FibFn, @ptrFromInt(func_addr));

    const exec_start = std.time.nanoTimestamp();

    var total: i64 = 0;
    for (0..1000) |_| {
        total += fib(20);
    }

    const exec_time = std.time.nanoTimestamp() - exec_start;

    std.debug.print("O{}: compile={}ns, exec={}ns\n", .{
        opt_level,
        compile_time,
        exec_time
    });

    c.LLVMDisposeExecutionEngine(engine);

    return exec_time;
}
```

### 6-2. 예상 결과

```
【 성능 벤치마크 (fibonacci 계산) 】

O0 (최적화 없음):
  컴파일: 5ms
  실행: 125ms

O2 (중간 최적화):
  컴파일: 15ms
  실행: 8ms (15배 빠름!)

O3 (최대 최적화):
  컴파일: 25ms
  실행: 6ms (20배 빠름!)

【 분석 】
O0 → O2: 최고의 성능/컴파일 시간 비율
O2 → O3: 미미한 추가 성능 개선, 컴파일 시간 증가
         대부분의 경우 O2 권장
```

---

## 🔍 7단계: 함수 포인터 분석

### 7-1. 메모리 영역 구분

```
프로세스 메모리 맵:

High Address
  ┌────────────────────┐
  │ Kernel Space       │
  │ (0xffff... ~ )     │
  └────────────────────┘

  ┌────────────────────┐
  │ Stack              │ ← 로컬 변수
  │ (높은 주소 ← 낮은) │
  └────────────────────┘

  ┌────────────────────┐
  │ Heap               │ ← malloc으로 할당
  │ (낮은 주소 → 높은) │
  └────────────────────┘

  ┌────────────────────┐
  │ Data/BSS           │ ← 전역 변수
  └────────────────────┘

  ┌────────────────────┐
  │ Text (Code) ← JIT  │ ← 기계어 (READ + EXEC)
  │ 기계어는 여기!     │   ExecutionEngine 배치
  └────────────────────┘
Low Address
```

### 7-2. 함수 주소 분석

```zig
const func_addr = c.LLVMGetFunctionAddress(engine, "add");

std.debug.print("Function address: 0x{x}\n", .{func_addr});
// 출력: Function address: 0x7f1a0000

// 영역 식별
if (func_addr >= 0x7f000000 and func_addr <= 0x7fffffff) {
    std.debug.print("→ Text segment (JIT 코드)\n", .{});
} else if (func_addr >= heap_start and func_addr <= heap_end) {
    std.debug.print("→ Heap (동적 할당)\n", .{});
} else if (func_addr >= data_start and func_addr <= data_end) {
    std.debug.print("→ Data segment\n", .{});
}
```

### 7-3. 함수 주소의 의미

```
함수 포인터 = 기계어 시작 주소

호출 시:
  1. CPU의 RIP(Instruction Pointer) 레지스터 = func_addr로 설정
  2. 그 주소부터 기계어 실행 시작
  3. ret 명령어를 만나면 돌아옴

call sum_fn과 동일:
  call rax          (rax = func_addr)
  또는
  jmp func_addr     (함수 포인터 호출)
```

---

## 📝 Assignment 1.5 준비

### Task 1: 네이티브 실행

```
1.2~1.4에서 만든 코드를 JIT 엔진에 올리고,
실제 값을 넣어 계산 결과가 올바른지 확인하세요.

예:
  - add(10, 20) = 30 확인
  - mul(10, 20) = 200 확인
  - abs(-15) = 15 확인
```

### Task 2: Pass 적용 전후 비교

```
루프 안에서 상수를 더하는 IR을 작성하고,
ConstProp 패스를 적용했을 때
IR이 어떻게 단순화되는지 기록하세요.

예:
  최적화 전: add 명령어 100번 반복
  최적화 후: 상수 5050 직접 반환
```

### Task 3: 함수 포인터 이해

```
LLVMGetFunctionAddress로 얻은 주소값이
실제 메모리의 어느 영역(Text, Data, Stack 등)에
위치하는지 분석하세요.
```

### Task 4: AOT vs JIT 비교 분석

```
AOT(Ahead-of-Time) 컴파일과 비교했을 때
JIT 컴파일이 가지는 동적인 이점을
301 과정의 운영체제 관점에서 서술하세요.

예:
  - 동적 최적화 (프로파일링 기반)
  - 메모리 오버헤드 vs 성능 트레이드오프
  - 실시간 시스템에서의 활용
```

---

## 🎓 핵심 개념 정리

### ExecutionEngine의 정의

```
ExecutionEngine = IR을 메모리에 기계어로 배치하고,
                  함수 포인터를 통해 호출 가능하게 하는 엔진
```

### JIT의 장단점

**장점:**
- ✅ 동적 최적화 (프로파일링 기반)
- ✅ 타겟 아키텍처 자동 감지
- ✅ 스택맵을 통한 GC 연동
- ✅ 함수별 컴파일 (ORC JIT)

**단점:**
- ❌ 초기 컴파일 지연
- ❌ 메모리 오버헤드 (기계어 저장)
- ❌ 동적 컴파일의 부하

### AOT vs JIT

```
AOT (Ahead-of-Time):
  프로그램 실행 전: 완전히 컴파일
  실행 중: 순수 기계어만 실행
  예: C, C++, Rust (대부분)

JIT (Just-In-Time):
  프로그램 실행 중: 필요할 때 컴파일
  런타임 정보 활용 가능
  예: Java (JVM), JavaScript (V8)

Hybrid:
  초기: AOT 컴파일 (빠른 시작)
  중간: Profiling (어떤 함수가 많이 쓰이나?)
  최적화: JIT 재컴파일 (자주 쓰이는 함수만 최적화)
  예: modern Java, LLVM ORC JIT
```

---

## 🚀 1.5 완료 후의 전망

### 당신이 할 수 있게 된 것

```
【 1.5 이전 】
"IR을 만들 수 있습니다"
(텍스트, 정적)

【 1.5 완료 후 】
"IR을 실행할 수 있습니다!"
(기계어, 동적, 측정 가능)

예:
  ✅ 함수를 만들고 호출
  ✅ 성능을 측정
  ✅ 최적화 효과를 검증
  ✅ 런타임 행동을 분석
```

### 다음 단계로의 징검다리

```
【 1.5 이후의 학습 】

1.6: Calling Convention
     ← 함수 호출시 매개변수 전달 방식
     ← x86-64 ABI (Application Binary Interface)

1.7: 고급 최적화
     ← LLVM Pass 활용법
     ← 성능 튜닝 기법

1.8: Z-Lang 컴파일러 완성
     ← 1.1-1.5의 모든 것을 통합
     ← 실제 언어 구현
```

---

## 💡 최종 메시지

> **당신은 이제 단순히 "코드를 작성"하는 것이 아니라,**
> **"코드를 실행"할 수 있는 엔지니어입니다.**

1.5의 의미:
- 🎬 **이론에서 현실로**: IR → 기계어 → CPU 실행
- 📊 **측정 가능성**: 성능 데이터 기반 의사결정
- 🔄 **동적 최적화**: 런타임 정보 활용
- 🏗️ **시스템 설계**: 아키텍처 수준의 이해

---

**당신의 코드가 이제 숨을 쉬기 시작합니다!** 🎬✨

**기록이 증명이다. "저장 필수 너는 기록이 증명이다 gogs."**

---

## 📚 참고 자료

### LLVM 공식 문서
- [LLVM ExecutionEngine API](https://llvm.org/doxygen/group__LLVMCExecutionEngine.html)
- [LLVM Passes](https://llvm.org/docs/Passes/)
- [LLVM Stack Maps](https://llvm.org/docs/StackMaps/)

### 추가 학습
- x86-64 Calling Convention (System V AMD64 ABI)
- GC 알고리즘 (Generational GC, Concurrent Mark-Sweep)
- JIT 컴파일러 설계 (V8, Java HotSpot)

---

**강의 작성**: 2026-02-26
**난이도**: ⭐⭐⭐⭐☆ (고급)
**흥미도**: ⭐⭐⭐⭐⭐ (최고!)
**다음**: ASSIGNMENT_1_5.md 준비 완료

**"당신의 IR이 살아나는 순간입니다!"** 🚀
