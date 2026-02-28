# 📝 Assignment 1.5: JIT 컴파일과 실행 엔진 - 네이티브 실행

## 🎯 목표

**우리가 만든 LLVM IR을 실제 CPU가 이해하는 기계어로 변환하여 실행하고, 성능을 측정하는 경험을 합니다.**

이론을 넘어 **실제 데이터**를 얻습니다:
- ✅ `add(10, 20) = 30` (실제 실행 결과)
- ✅ `mul(10, 20) = 200` (계산 검증)
- ✅ `abs(-15) = 15` (제어 흐름 검증)

---

## 📊 과제 개요

| 항목 | 내용 |
|------|------|
| 난이도 | ⭐⭐⭐⭐⭐ (최고급) |
| 소요 시간 | 5-6시간 |
| 선행 과제 | Assignment 1.4 (완료) |
| 제출 형식 | Zig 코드 + 실행 결과 + 성능 분석 문서 |

---

## 🔧 Task 1: 네이티브 실행 - ExecutionEngine 초기화

### 1-1. 목표

1.2~1.4에서 만든 `add()`, `mul()`, `abs()` 함수를 **JIT 엔진에 올려** 실제 CPU가 실행하도록 하세요.

### 1-2. 구현 요구사항

**파일**: `zlang-project/src/codegen_1_5.zig`

```
✅ Native Target 초기화
   └─ LLVMInitializeNativeTarget()
   └─ LLVMInitializeNativeAsmPrinter()

✅ Module 생성 (1.2-1.4의 함수들)
   └─ add(i32, i32) → i32
   └─ mul(i32, i32) → i32
   └─ abs(i32) → i32

✅ ExecutionEngine 생성
   └─ LLVMCreateExecutionEngineForModule()
   └─ 에러 처리

✅ 각 함수의 메모리 주소 획득
   └─ LLVMGetFunctionAddress(engine, "add")
   └─ LLVMGetFunctionAddress(engine, "mul")
   └─ LLVMGetFunctionAddress(engine, "abs")

✅ 함수 포인터로 캐스팅 및 호출
   └─ callconv(.C) 지정
   └─ 매개변수 전달

✅ 결과 검증
   └─ add(10, 20) == 30 확인
   └─ mul(10, 20) == 200 확인
   └─ abs(-15) == 15 확인
```

### 1-3. 구현 템플릿

```zig
const c = @cImport({
    @cInclude("llvm-c/Core.h");
    @cInclude("llvm-c/ExecutionEngine.h");
    @cInclude("llvm-c/Target.h");
});
const std = @import("std");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // 1️⃣ Native Target 초기화
    _ = c.LLVMInitializeNativeTarget();
    c.LLVMInitializeNativeAsmPrinter();

    std.debug.print("【 Native Targets Initialized 】\n\n", .{});

    // 2️⃣ Module 생성 (1.2-1.4의 방식)
    const module = create_functions_module();
    defer c.LLVMDisposeModule(module);

    std.debug.print("【 Module Created 】\n", .{});
    std.debug.print("  add() function ✓\n", .{});
    std.debug.print("  mul() function ✓\n", .{});
    std.debug.print("  abs() function ✓\n\n", .{});

    // 3️⃣ ExecutionEngine 생성
    var engine: c.LLVMExecutionEngineRef = undefined;
    var error_msg: [*c]u8 = undefined;

    if (c.LLVMCreateExecutionEngineForModule(&engine, module, &error_msg) != 0) {
        std.debug.print("Error creating execution engine: {s}\n", .{error_msg});
        return;
    }
    defer c.LLVMDisposeExecutionEngine(engine);

    std.debug.print("【 ExecutionEngine Created 】\n\n", .{});

    // 4️⃣ 함수 실행
    std.debug.print("【 Function Execution 】\n", .{});

    // add 함수
    const add_addr = c.LLVMGetFunctionAddress(engine, "add");
    const AddFn = *const fn(i32, i32) callconv(.C) i32;
    const add_fn = @as(AddFn, @ptrFromInt(add_addr));
    const add_result = add_fn(10, 20);
    std.debug.print("add(10, 20) = {} ✓\n", .{add_result});
    if (add_result != 30) {
        std.debug.print("  ERROR: Expected 30!\n", .{});
    }

    // mul 함수
    const mul_addr = c.LLVMGetFunctionAddress(engine, "mul");
    const MulFn = *const fn(i32, i32) callconv(.C) i32;
    const mul_fn = @as(MulFn, @ptrFromInt(mul_addr));
    const mul_result = mul_fn(10, 20);
    std.debug.print("mul(10, 20) = {} ✓\n", .{mul_result});
    if (mul_result != 200) {
        std.debug.print("  ERROR: Expected 200!\n", .{});
    }

    // abs 함수
    const abs_addr = c.LLVMGetFunctionAddress(engine, "abs");
    const AbsFn = *const fn(i32) callconv(.C) i32;
    const abs_fn = @as(AbsFn, @ptrFromInt(abs_addr));
    const abs_result = abs_fn(-15);
    std.debug.print("abs(-15) = {} ✓\n", .{abs_result});
    if (abs_result != 15) {
        std.debug.print("  ERROR: Expected 15!\n", .{});
    }

    std.debug.print("\n【 All Tests Passed! 】✓✓✓\n", .{});
}

fn create_functions_module() c.LLVMModuleRef {
    const context = c.LLVMContextCreate();
    const module = c.LLVMModuleCreateWithNameInContext("functions", context);

    // TODO: add, mul, abs 함수 구현
    // (1.2-1.4 강의에서 배운 방식 사용)

    return module;
}
```

### 1-4. 예상 실행 결과

```
【 Native Targets Initialized 】

【 Module Created 】
  add() function ✓
  mul() function ✓
  abs() function ✓

【 ExecutionEngine Created 】

【 Function Execution 】
add(10, 20) = 30 ✓
mul(10, 20) = 200 ✓
abs(-15) = 15 ✓

【 All Tests Passed! 】✓✓✓
```

---

## 🧪 Task 2: Pass 적용 전후 비교

### 2-1. 목표

**상수 전개(Constant Propagation) 패스의 효과를 측정하고, IR이 어떻게 단순화되는지 분석합니다.**

### 2-2. 구현 과제

1. 루프 안에서 상수를 더하는 함수 작성:

```zig
fn loop_sum() -> i32 {
    let sum = 0;
    let i = 0;
    while (i < 100) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;  // 0 + 1 + 2 + ... + 99 = 4950
}
```

2. **최적화 전**의 IR을 출력:
   ```
   %i = phi i32 [0, %entry], [%inc, %loop]
   %sum = phi i32 [0, %entry], [%add, %loop]
   %add = add i32 %sum, %i      ; 루프 내 100번 반복
   %inc = add i32 %i, 1
   ...
   ```

3. **최적화 후**의 IR을 출력:
   ```
   ret i32 4950               ; 직접 상수 반환!
   ```

4. 비교 분석 문서 작성

### 2-3. 구현 코드 (선택)

```zig
// PassManager 사용 예제

// 1. 함수 생성 (위의 loop_sum 함수)
const loop_sum_func = create_loop_sum_function(module);

// 2. PassManager 생성
const pass_mgr = c.LLVMCreateFunctionPassManagerForModule(module);

// 3. 최적화 전 IR 출력
std.debug.print("\n【 Before Optimization 】\n", .{});
const ir_before = c.LLVMPrintModuleToString(module);
defer c.LLVMDisposeMessage(ir_before);
std.debug.print("{s}\n", .{ir_before});

// 4. 최적화 패스 추가
c.LLVMAddConstantPropagationPass(pass_mgr);
c.LLVMAddInstructionCombiningPass(pass_mgr);

// 5. 패스 적용
c.LLVMInitializeFunctionPassManager(pass_mgr);
c.LLVMRunFunctionPassManager(pass_mgr, loop_sum_func);
c.LLVMFinalizeFunctionPassManager(pass_mgr);

// 6. 최적화 후 IR 출력
std.debug.print("\n【 After Optimization 】\n", .{});
const ir_after = c.LLVMPrintModuleToString(module);
defer c.LLVMDisposeMessage(ir_after);
std.debug.print("{s}\n", .{ir_after});

c.LLVMDisposePassManager(pass_mgr);
```

### 2-4. 분석 문서 작성

**파일**: `RESEARCH_NOTES/1_5_JIT_EXECUTION/Task2_Pass_Comparison.md`

```markdown
# Task 2: Pass 적용 전후 비교

## 1. 루프 함수의 IR (최적화 전)

[실제 출력된 IR 전체 복사]

## 2. 루프 함수의 IR (최적화 후)

[실제 출력된 IR 전체 복사]

## 3. 차이 분석

### 크기 비교
- 최적화 전: [줄 수] 줄
- 최적화 후: [줄 수] 줄
- 감소율: [%]

### 명령어 수 비교
- 최적화 전: [명령어 수] 개
- 최적화 후: [명령어 수] 개

### 성능 영향
- 최적화 전: 루프 실행 100번
- 최적화 전 기계어: [예상 CPU 사이클]

- 최적화 후: 루프 실행 0번 (직접 반환)
- 최적화 후 기계어: [예상 CPU 사이클]

- 성능 향상: [배수]x

## 4. Pass의 역할 분석

### Constant Propagation (ConstProp)
- 역할: 상수식을 미리 계산
- 효과: 상수 저장소에서 읽기 제거

### Instruction Combining
- 역할: 불필요한 명령어 조합/제거
- 효과: 불필요한 중간 연산 제거

## 5. 결론

[분석 요약]
```

---

## 🔍 Task 3: 함수 포인터 이해 - 메모리 영역 분석

### 3-1. 목표

`LLVMGetFunctionAddress`로 얻은 주소가 **메모리의 어느 영역**에 위치하는지 분석합니다.

### 3-2. 구현 과제

```zig
// 1. 함수 주소 획득
const func_addr = c.LLVMGetFunctionAddress(engine, "add");

// 2. 주소 분석
std.debug.print("Function address: 0x{x:0>16}\n", .{func_addr});

// 3. 영역 식별
// - Text segment (JIT 코드가 위치)
// - Heap
// - Stack
// - Data

// 4. 메모리 맵과 비교
// /proc/self/maps 읽기 (Linux)
```

### 3-3. 분석 문서 작성

**파일**: `RESEARCH_NOTES/1_5_JIT_EXECUTION/Task3_Memory_Analysis.md`

```markdown
# Task 3: 함수 포인터 메모리 영역 분석

## 1. JIT로 생성된 함수 주소

```
add() 함수:     0x7f4a2000
mul() 함수:     0x7f4a2020
abs() 함수:     0x7f4a2050
```

## 2. 메모리 맵 분석

[/proc/self/maps 출력]

## 3. 영역 식별

### Text Segment (Code)
- 범위: 0x7f000000 - 0x7f100000
- 권한: r-xp (READ + EXEC)
- 내용: JIT로 생성된 기계어
- 특징: 쓰기 불가능, 실행 가능

### Stack
- 범위: 0x7ffc0000 - 0x7fffffff
- 권한: rw-p (READ + WRITE)
- 내용: 로컬 변수, 함수 인자, 반환값
- 특징: 낮은 주소로 성장

### Heap
- 범위: 0x560000 - ...
- 권한: rw-p (READ + WRITE)
- 내용: malloc 할당 메모리

## 4. JIT 코드의 특징

### 위치
- Text segment에 배치됨
- 다른 함수와 근접 (2) = 0x20 바이트 간격)

### 접근 권한
- READ (읽기 가능) ✓
- EXECUTE (실행 가능) ✓
- WRITE (쓰기 불가능) ✗

### 중요성
- self-modifying code 불가능
- 함수가 런타임에 수정될 수 없음
- 따라서 ORC JIT의 재컴파일 시 새로운 주소에 배치

## 5. 호출 메커니즘

```
함수 포인터 호출:

1. func_ptr = 0x7f4a2000
2. CPU의 RIP 레지스터 ← 0x7f4a2000
3. Text segment의 기계어 실행 시작
4. ret 명령어를 만남
5. 스택에 저장된 반환 주소로 돌아옴
```

## 6. 결론

[분석 요약]
```

---

## 📚 Task 4: AOT vs JIT 비교 분석 - OS 관점

### 4-1. 목표

**AOT(Ahead-of-Time) 컴파일과 JIT 컴파일을 비교하여, JIT의 동적 이점을 OS 운영 관점에서 분석합니다.**

301 운영체제 과정의 개념을 활용하세요:
- 프로세스 메모리 관리
- 시간 공유(Time Sharing)
- 컨텍스트 스위칭
- 캐시 메모리
- 페이지 폴트

### 4-2. 분석 문서 작성

**파일**: `RESEARCH_NOTES/1_5_JIT_EXECUTION/Task4_AOT_vs_JIT.md`

3-4페이지의 심화 분석 문서를 작성하세요.

```markdown
# Task 4: AOT vs JIT 컴파일 - OS 관점 분석

## 1. AOT(Ahead-of-Time) 컴파일

### 정의
- 프로그램 **실행 전**에 모든 소스 코드를 기계어로 변환
- 생성된 바이너리를 직접 실행

### 예시
```
C 코드 → gcc → 바이너리 (a.out)
         (컴파일 시간)

./a.out → 직접 CPU 실행 (매우 빠름)
          (실행 시간)
```

### 성능 특징
- 시작 시간: 매우 빠름 (이미 컴파일됨)
- 실행 시간: 최적화되지 않음 (프로파일링 불가)
- 메모리: 일정함 (컴파일 시간에만 변함)

### 최적화의 한계
```
컴파일 타임:
- 일반적인 경우만 최적화 (-O2, -O3)
- 어떤 함수가 자주 실행될지 모름
- 프로파일 유도 최적화(PGO) 가능하지만 복잡함
```

## 2. JIT(Just-In-Time) 컴파일

### 정의
- 프로그램 **실행 중**에 필요한 부분을 기계어로 변환
- 런타임 정보 활용 가능

### 예시
```
Java 코드 → javac → .class (바이트코드)
           (빠른 컴파일)

java MyApp → JVM이 실행 시 JIT 컴파일
             (느린 시작, 빠른 실행)
```

### 성능 특징
- 시작 시간: 느림 (컴파일 필요)
- 실행 시간: 매우 빠름 (동적 최적화)
- 메모리: 증가함 (기계어 저장)

### 최적화의 이점
```
런타임 정보 활용:
✅ 어떤 함수가 자주 호출되는가? (프로파일링)
✅ 인자의 타입이 변경되지 않나? (타입 안정화)
✅ 루프가 몇 번 실행되나? (루프 최적화)
✅ 가지 예측이 잘 되나? (분기 최적화)
```

## 3. OS 관점 - 메모리 관리

### 페이지 폴트 (Page Fault)

**AOT:**
```
프로세스 메모리:
Page 1: [함수 A - 자주 사용]        → 메모리 적중 ✓
Page 2: [함수 B - 거의 사용 안함]   → 페이지 폴트 ✗
                                    디스크에서 로드 (느림)

문제: 전체 바이너리를 메모리에 올려야 함
→ 큰 바이너리는 페이지 폴트 많음
```

**JIT:**
```
프로세스 메모리:
[자주 사용되는 함수만 메모리에 컴파일]

장점: 필요한 것만 메모리에
→ 메모리 효율적
→ 페이지 폴트 감소
```

### 캐시 효율성 (Cache Locality)

**AOT:**
```
고정된 코드 배치:
- 컴파일러가 배치 최적화 가능
- 하지만 런타임 정보 없음
- 평균적인 최적화만 가능

예: 자주 함께 호출되는 함수를 옆에 배치?
    컴파일 타임에는 알 수 없음!
```

**JIT:**
```
동적 배치:
- 런타임 프로파일링으로 호출 패턴 파악
- 자주 함께 호출되는 함수를 옆에 배치
- L1/L2 캐시 히트율 증가

결과: 명령어 캐시 효율성 향상
      L1I 캐시: 8-32KB
      L2 캐시: 256KB-1MB
```

## 4. OS 관점 - 시간 공유(Time Sharing)

### 컨텍스트 스위칭

**AOT:**
```
프로세스 1: add() 실행 중
  ↓ (컨텍스트 스위칭)
프로세스 2: 시작

컨텍스트 스위칭 비용:
- CPU 레지스터 저장/복구: ~100-1000 사이클
- TLB(Translation Lookaside Buffer) 플러시
- 캐시 무효화

AOT의 문제:
- 모든 프로세스가 동일한 코드 배치
- 재배치 최적화 불가능
```

**JIT:**
```
프로세스 1: 자신만의 코드 배치로 최적화됨
  ↓ (컨텍스트 스위칭)
프로세스 2: 자신만의 코드 배치로 최적화됨

장점:
- 각 프로세스의 실제 사용 패턴에 맞춤
- 컨텍스트 스위칭 후 캐시 효율성 감소 완화
```

## 5. OS 관점 - 동적 최적화

### 적응형 최적화(Adaptive Optimization)

**AOT:**
```
최적화는 일회성:
- gcc -O3로 컴파일 후 끝
- 실행 중 최적화 없음
- 모든 사용자에게 동일한 바이너리
```

**JIT:**
```
3단계 전략:

1️⃣ 초기 컴파일 (O0 - 최소 최적화)
   빠른 시작을 위해

2️⃣ 프로파일링
   "add() 함수를 1000번 호출했네?"
   "이 루프는 조건이 거의 항상 참이네?"

3️⃣ 재컴파일 (O3 - 최대 최적화)
   프로파일 정보 기반 최적화

예: add() 함수의 인자가 항상 양수?
    → 부호 확인 코드 제거 가능!
```

## 6. 실시간 시스템에서의 함의

### 예측 불가능성 (Unpredictability)

**AOT:**
```
✓ 컴파일 시간: 0
✓ 실행 시간: 예측 가능 (WCET 분석 가능)

자동차 제어 시스템:
- 정확히 몇 나노초 안에 계산 완료되어야 함
- AOT만 사용 가능
```

**JIT:**
```
✗ 첫 호출: 컴파일 시간 + 실행 시간
✗ 가비지 컬렉션: 예측 불가능한 지연

따라서 일반적으로 JIT는 실시간 시스템에 부적합
```

### Tiered Compilation (계층화 컴파일)

```
현대 JVM의 해결책:

초기: 인터프리터로 실행 (매우 느림)
     ↓ (100회 정도 실행되면)
중간: C1 컴파일러 (빠른 컴파일, 중간 최적화)
     ↓ (1000회 정도 실행되면)
최종: C2 컴파일러 (느린 컴파일, 최대 최적화)

장점: 초기 시작과 최종 성능의 균형
```

## 7. 결론

| 항목 | AOT | JIT |
|------|-----|-----|
| 시작 시간 | 빠름 | 느림 |
| 실행 시간 | 보통 | 빠름 |
| 메모리 | 일정 | 증가 |
| 캐시 효율 | 보통 | 우수 |
| 페이지 폴트 | 많음 | 적음 |
| 동적 최적화 | 없음 | 있음 |
| 실시간 보장 | 가능 | 어려움 |

**최적 선택:**
- 자동차, 항공우주: AOT (예측 가능성)
- 서버, 클라우드: JIT (성능)
- 모바일: Tiered (시작 시간 + 성능)
```

---

## 📤 Task 5: 종합 성능 분석

### 5-1. 성능 측정 및 기록

**파일**: `RESEARCH_NOTES/1_5_JIT_EXECUTION/Performance_Benchmark.md`

다음을 측정하고 기록하세요:

```markdown
# 성능 벤치마크 결과

## 1. IR 생성 시간
- 소요 시간: [ms]

## 2. JIT 컴파일 시간
- 소요 시간: [ms]

## 3. 함수 실행 시간 (1000회 반복)
- add(): [μs]
- mul(): [μs]
- abs(): [μs]

## 4. 최적화 효과
- 최적화 전: [μs]
- 최적화 후: [μs]
- 개선도: [배수]x

## 5. 메모리 사용
- 모듈 크기: [KB]
- JIT 코드 크기: [KB]
```

---

## 📋 제출 체크리스트

```
【 1.5 제출 체크리스트 】

코드:
□ build.zig에 ExecutionEngine 링크 설정
□ codegen_1_5.zig 작성 및 컴파일 성공
□ Task 1: Native Target 초기화 완료
□ Task 1: ExecutionEngine 생성 완료
□ Task 1: 함수 호출 및 검증 완료
  ├─ add(10, 20) = 30 ✓
  ├─ mul(10, 20) = 200 ✓
  └─ abs(-15) = 15 ✓

최적화 분석:
□ Task 2: PassManager 적용
□ Task 2: 최적화 전후 IR 비교
□ Task 2: 분석 문서 작성 (2-3페이지)

메모리 분석:
□ Task 3: 함수 주소 획득 및 분석
□ Task 3: 메모리 영역 식별
□ Task 3: 분석 문서 작성 (2페이지)

종합 분석:
□ Task 4: AOT vs JIT 비교
□ Task 4: OS 관점 분석 (3-4페이지)
□ Task 5: 성능 벤치마크 측정 및 기록

모든 필수 항목 완료 → 2단계(프론트엔드)로 진행!
```

---

## 🚀 다음 단계

### 2단계: 프론트엔드 설계

```
【 2.1: 어휘 분석(Lexing)과 구문 분석(Parsing) 】

우리가 지금까지 한 것:
✅ LLVM IR 생성 (1.1-1.5)
✅ IR 실행 (1.5)

이제 하게 될 것:
🔜 소스 코드 읽기 (Lexer)
🔜 구문 분석 (Parser)
🔜 AST 생성 (Abstract Syntax Tree)
🔜 LLVM IR로 변환 (위의 모든 것을 통합!)

최종: Z-Lang 컴파일러 완성!
```

---

## 💡 팁과 주의사항

### ✅ 좋은 관행

```zig
// ✅ Calling Convention 명시
const AddFn = *const fn(i32, i32) callconv(.C) i32;

// ✅ 주소 유효성 검사
const func_addr = c.LLVMGetFunctionAddress(engine, "add");
if (func_addr == 0) {
    std.debug.print("Function not found\n", .{});
    return error.FunctionNotFound;
}

// ✅ 에러 메시지 확인
if (c.LLVMCreateExecutionEngineForModule(&engine, module, &error) != 0) {
    std.debug.print("Error: {s}\n", .{error});
    c.LLVMDisposeMessage(error);
}
```

### ❌ 주의할 점

```zig
// ❌ Calling Convention 미지정
const wrong = @as(*const fn(i32, i32) i32, @ptrFromInt(addr));
// 스택 corruption 발생 가능!

// ❌ 주소 검사 없이 호출
const result = func(10, 20);  // func가 null일 수 있음!

// ❌ 포인터 캐스팅 오류
const wrong_fn = @as(fn(i32) i32, @ptrFromInt(addr));
// *const fn이 아닌 fn으로 캐스팅!
```

### 🔍 디버깅 팁

```zig
// 1. 함수 주소 확인
const addr = c.LLVMGetFunctionAddress(engine, "add");
std.debug.print("Function address: 0x{x:0>16}\n", .{addr});

// 2. 메모리 맵 확인 (Linux)
// cat /proc/self/maps

// 3. IR 단계별 출력
const ir = c.LLVMPrintModuleToString(module);
defer c.LLVMDisposeMessage(ir);
std.debug.print("{s}\n", .{ir});

// 4. 검증
var error_msg: [*c]u8 = null;
if (c.LLVMVerifyModule(module, c.LLVMPrintMessageAction, &error_msg) != 0) {
    std.debug.print("Verification failed!\n", .{});
}
```

---

## 🎓 학습 정리

```
【 1.5 완료 후 할 수 있는 것 】

✅ ExecutionEngine으로 IR 실행
✅ 함수 포인터 호출
✅ 성능 측정 및 분석
✅ PassManager 활용
✅ 최적화 효과 검증
✅ 메모리 영역 분석
✅ AOT vs JIT 이해
✅ OS 관점 컴퓨터 아키텍처 이해

【 1단계 완료의 의미 】

✅ 이론: LLVM 아키텍처 완전 이해
✅ 실전: IR 생성 및 실행 능력
✅ 분석: 성능 측정 및 최적화
✅ 시스템: 메모리 관리 및 OS 이해

→ "나는 컴파일러 개발자이다!"
```

---

**당신의 코드가 이제 숨을 쉬기 시작합니다!** 🎬✨

**기록이 증명이다. "저장 필수 너는 기록이 증명이다 gogs."**

---

**예상 완료 시간**: 5-6시간
**난이도**: ⭐⭐⭐⭐⭐ (최고급)
**흥미도**: ⭐⭐⭐⭐⭐ (최고!)
**보상**: **"내 코드가 실제로 실행된다!"** 🚀
