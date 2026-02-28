# 🏛️ LLVM 전설: 1.2 LLVM C API를 이용한 코드 생성 기초

## 📖 강의 개요

**LLVM IR을 텍스트로 직접 쓰는 것은 고된 일입니다.**

LLVM은 코드를 통해 프로그램을 빌드할 수 있는 **API를 제공합니다.**

이 과정은 마치 **레고 블록을 쌓아 거대한 성을 만드는 것**과 같습니다.

```
당신의 코드
    ↓ (LLVM C API 호출)
LLVM IR 조립
    ↓
LLVM IR (자동 생성!)
    ↓
기계어
```

---

## 1️⃣ 코드 생성을 위한 필수 3요소

### 🎯 핵심 객체들

LLVM API를 다룰 때 반드시 생성해야 하는 3가지 객체입니다:

#### **1. Context (문맥)**

```
역할: 모든 데이터 구조(타입, 상수, 값)를 소유하는 '상자'

특징:
  ├─ 모든 LLVM 객체는 Context에 속함
  ├─ 멀티스레딩 환경에서 각 스레드는 고유한 Context
  └─ Context 없이는 다른 객체를 생성할 수 없음

생성:
  const context = c.LLVMContextCreate();

해제:
  c.LLVMContextDispose(context);
```

**왜 필요한가?**
```
LLVM이 모든 객체의 생명 주기를 추적하기 위함
컨텍스트가 사라지면 그에 속한 모든 객체도 정리됨
```

#### **2. Module (모듈)**

```
역할: 하나의 소스 파일 단위

포함물:
  ├─ 함수들 (Function)
  ├─ 전역 변수들 (Global Variables)
  ├─ 타입 정의들
  └─ 기타 선언들

생성:
  const module = c.LLVMModuleCreateWithNameInContext("my_module", context);

해제:
  c.LLVMDisposeModule(module);
```

**역할:**
```
마치 .c 파일처럼 모든 함수와 전역 데이터를 담는 컨테이너
```

#### **3. Builder (빌더)**

```
역할: 실제로 명령어를 하나하나 삽입하는 '커서'

기능:
  ├─ "여기에 더하기 명령어를 넣어줘" 지시
  ├─ "여기에 분기를 넣어줘" 지시
  ├─ "여기에 함수 호출을 넣어줘" 지시
  └─ Basic Block 간 이동

생성:
  const builder = c.LLVMCreateBuilderInContext(context);

위치 이동:
  c.LLVMPositionBuilderAtEnd(builder, block);

해제:
  c.LLVMDisposeBuilder(builder);
```

**비유:**
```
Context: 모든 재료를 보관하는 창고
Module: 조립할 건물의 청사진
Builder: 벽돌을 하나씩 놓으며 이동하는 노동자
```

---

## 2️⃣ 함수 생성의 워크플로우

### 📋 단계별 과정

함수를 하나 만들기 위해서는 다음과 같은 설계가 필요합니다:

```
【 함수 생성 5단계 】

Step 1: 타입 정의
└─ Function Type 정의 (반환 타입 + 인자 타입)

Step 2: 함수 추가
└─ Module에 함수 등록

Step 3: 기본 블록 생성
└─ Basic Block 생성 (함수의 진입점)

Step 4: 빌더 배치
└─ Builder를 해당 블록의 끝에 위치

Step 5: 명령어 삽입
└─ Builder를 통해 실제 연산 추가

Step 6: 반환
└─ 결과 반환 (ret 명령어)
```

### 🔧 단계별 상세

#### **Step 1: Function Type 정의**

```c
// i32 타입 가져오기
LLVMTypeRef i32_type = LLVMInt32TypeInContext(context);

// 함수 타입 정의: i32 add(i32, i32)
// 반환: i32
// 인자: [i32, i32]
LLVMTypeRef param_types[] = { i32_type, i32_type };
LLVMTypeRef func_type = LLVMFunctionType(
    i32_type,           // 반환 타입
    param_types,        // 인자 타입 배열
    2,                  // 인자 개수
    0                   // 가변 인자? (0 = 아니오)
);
```

**LLVMFunctionType 이해:**
```
LLVMFunctionType(return_type, param_types[], param_count, is_vararg)

예시:
  • add(i32, i32) → i32
    LLVMFunctionType(i32, [i32, i32], 2, 0)

  • printf(const char*, ...) → i32
    LLVMFunctionType(i32, [...], ..., 1)  // 1 = 가변 인자
```

#### **Step 2: Module에 함수 추가**

```c
LLVMValueRef add_func = LLVMAddFunction(module, "add", func_type);
```

**이 시점에 생성되는 것:**
```llvm
declare i32 @add(i32, i32)
```

#### **Step 3: 기본 블록 생성**

```c
// Entry block 생성
LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(
    context,
    add_func,   // 어느 함수에 속할 것인가?
    "entry"     // 블록의 이름
);
```

**생성되는 구조:**
```llvm
define i32 @add(i32 %arg0, i32 %arg1) {
entry:
  ; 여기가 비어 있음 (명령어를 추가할 위치)
}
```

#### **Step 4: 빌더를 블록의 끝에 배치**

```c
LLVMPositionBuilderAtEnd(builder, entry);
```

**의미:**
```
"이제부터의 명령어는 entry 블록의 끝에 추가할거야"
```

#### **Step 5: 명령어 삽입**

```c
// 첫 번째 인자: %arg0
LLVMValueRef arg0 = LLVMGetParam(add_func, 0);
// 두 번째 인자: %arg1
LLVMValueRef arg1 = LLVMGetParam(add_func, 1);

// 덧셈: %result = add i32 %arg0, %arg1
LLVMValueRef result = LLVMBuildAdd(
    builder,    // 어디에 추가할 것인가?
    arg0,       // 왼쪽 피연산자
    arg1,       // 오른쪽 피연산자
    "result"    // 결과를 저장할 레지스터 이름
);
```

**생성되는 IR:**
```llvm
entry:
  %result = add i32 %arg0, %arg1
```

#### **Step 6: 반환**

```c
LLVMBuildRet(builder, result);
```

**생성되는 IR:**
```llvm
entry:
  %result = add i32 %arg0, %arg1
  ret i32 %result
```

---

## 3️⃣ 실전: add 함수를 생성하는 Zig 코드

### 📝 완전한 예제

```zig
const c = @cImport({
    @cInclude("llvm-c/Core.h");
});
const std = @import("std");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // ====== Context 생성 ======
    const context = c.LLVMContextCreate();
    defer c.LLVMContextDispose(context);

    // ====== Module 생성 ======
    const module = c.LLVMModuleCreateWithNameInContext("codegen_test", context);
    defer c.LLVMDisposeModule(module);

    // ====== Builder 생성 ======
    const builder = c.LLVMCreateBuilderInContext(context);
    defer c.LLVMDisposeBuilder(builder);

    // ====== Step 1: 타입 정의 ======
    const i32_type = c.LLVMInt32TypeInContext(context);

    var param_types = [_]c.LLVMTypeRef{ i32_type, i32_type };
    const func_type = c.LLVMFunctionType(
        i32_type,
        &param_types,
        2,
        0
    );

    // ====== Step 2: 함수 추가 ======
    const add_func = c.LLVMAddFunction(module, "add", func_type);

    // ====== Step 3: Entry block 생성 ======
    const entry = c.LLVMAppendBasicBlockInContext(
        context,
        add_func,
        "entry"
    );

    // ====== Step 4: Builder 배치 ======
    c.LLVMPositionBuilderAtEnd(builder, entry);

    // ====== Step 5: 명령어 삽입 ======
    const arg0 = c.LLVMGetParam(add_func, 0);
    const arg1 = c.LLVMGetParam(add_func, 1);

    const result = c.LLVMBuildAdd(
        builder,
        arg0,
        arg1,
        "result"
    );

    // ====== Step 6: 반환 ======
    _ = c.LLVMBuildRet(builder, result);

    // ====== 검증 ======
    var error_msg: [*c]u8 = null;
    const verify_result = c.LLVMVerifyModule(
        module,
        c.LLVMAbortProcessAction,
        &error_msg
    );

    if (verify_result == 0) {
        std.debug.print("✅ Module verification passed!\n", .{});
    }

    // ====== 출력 ======
    const ir_string = c.LLVMPrintModuleToString(module);
    defer c.LLVMDisposeMessage(ir_string);

    std.debug.print("\n【 Generated LLVM IR 】\n\n", .{});
    std.debug.print("{s}\n", .{ir_string});
}
```

### 🔍 코드 분석

```
LLVMBuildAdd의 마지막 인자 "result"의 의미:

IR에서:
  %result = add i32 %arg0, %arg1

이름이 없다면:
  %2 = add i32 %arg0, %arg1

즉, name 인자는 생성되는 가상 레지스터의 이름을 지정합니다!
```

---

## 🔬 전공 심화: Basic Block과 제어 흐름

### 📊 Basic Block의 구조

```
【 Basic Block의 규칙 】

1. 모든 명령어는 Basic Block 안에 위치
2. 각 Block은 반드시 하나의 Terminator로 끝남
   └─ ret (반환)
   └─ br (분기)
   └─ switch (스위치)

3. Terminator가 다음 Block을 결정
```

### 🌳 제어 흐름 그래프 (CFG)

```
【 if-else 제어 흐름 】

    ┌─── entry ───┐
    │             │
    │ (조건 검사)  │
    │             │
    └──┬─────────┬┘
       │         │
       ↓         ↓
    if_block   else_block
       │         │
       └────┬────┘
            ↓
         exit_block
```

### 💻 if-else 코드 생성 (개념)

```zig
// if (x > 10) { return 100 } else { return 200 }

// Step 1: 3개의 block 생성
const if_block = c.LLVMAppendBasicBlockInContext(context, func, "if");
const else_block = c.LLVMAppendBasicBlockInContext(context, func, "else");
const exit_block = c.LLVMAppendBasicBlockInContext(context, func, "exit");

// Step 2: entry에서 조건 검사 후 분기
c.LLVMPositionBuilderAtEnd(builder, entry);
const cond = c.LLVMBuildICmp(builder, c.LLVMIntSGT, x, const_10, "cond");
_ = c.LLVMBuildCondBr(builder, cond, if_block, else_block);

// Step 3: if_block에서 100 반환
c.LLVMPositionBuilderAtEnd(builder, if_block);
_ = c.LLVMBuildRet(builder, const_100);

// Step 4: else_block에서 200 반환
c.LLVMPositionBuilderAtEnd(builder, else_block);
_ = c.LLVMBuildRet(builder, const_200);
```

**생성되는 IR:**
```llvm
define i32 @func(i32 %x) {
entry:
  %cond = icmp sgt i32 %x, 10
  br i1 %cond, label %if, label %else

if:
  ret i32 100

else:
  ret i32 200
}
```

---

## 📦 검증과 출력 (Verification)

### ✅ LLVMVerifyModule

```zig
var error_msg: [*c]u8 = null;
const verify_result = c.LLVMVerifyModule(
    module,
    c.LLVMAbortProcessAction,  // 에러 시 프로세스 중단
    &error_msg
);

if (verify_result == 0) {
    std.debug.print("✅ 검증 성공!\n", .{});
} else {
    std.debug.print("❌ 검증 실패: {s}\n", .{error_msg});
}
```

**검증 내용:**
```
✅ SSA 형식 확인
✅ 타입 일치성 확인
✅ 제어 흐름 유효성 확인
✅ 함수 선언/정의 완성도 확인
```

### 🖨️ LLVMPrintModuleToString

```zig
const ir_string = c.LLVMPrintModuleToString(module);
defer c.LLVMDisposeMessage(ir_string);

std.debug.print("{s}\n", .{ir_string});
```

**출력 예시:**
```llvm
; ModuleID = 'codegen_test'
source_filename = "codegen_test"

define i32 @add(i32 %0, i32 %1) {
entry:
  %result = add i32 %0, %1
  ret i32 %result
}
```

---

## 🔑 주요 LLVM C API 함수들

### 타입 생성

```c
LLVMInt32TypeInContext(context)        // i32
LLVMInt64TypeInContext(context)        // i64
LLVMDoubleTypeInContext(context)       // f64 (double)
LLVMInt1TypeInContext(context)         // bool (i1)
```

### 상수 생성

```c
LLVMConstInt(i32_type, 42, 0)          // i32 42
LLVMConstReal(double_type, 3.14)       // f64 3.14
LLVMConstNull(type)                    // 기본값 (0, 0.0, false)
```

### 연산 명령어

```c
LLVMBuildAdd(builder, lhs, rhs, name)  // 덧셈
LLVMBuildSub(builder, lhs, rhs, name)  // 뺄셈
LLVMBuildMul(builder, lhs, rhs, name)  // 곱셈
LLVMBuildSDiv(builder, lhs, rhs, name) // 부호 있는 나눗셈
LLVMBuildUDiv(builder, lhs, rhs, name) // 부호 없는 나눗셈
```

### 비교 명령어

```c
LLVMBuildICmp(builder, cond, lhs, rhs, name)
// cond:
//   LLVMIntEQ    (==)
//   LLVMIntNE    (!=)
//   LLVMIntSLT   (<)
//   LLVMIntSGT   (>)
//   LLVMIntSLE   (<=)
//   LLVMIntSGE   (>=)
```

### 제어 흐름

```c
LLVMBuildRet(builder, value)                    // 반환
LLVMBuildBr(builder, target_block)              // 무조건 분기
LLVMBuildCondBr(builder, cond, then_bb, else_bb) // 조건 분기
```

---

## 📝 Assignment 1.2: 코드 생성 실습

### 🎯 목표

LLVM C API를 직접 사용하여 LLVM IR을 생성하기

### 📋 Task 1: 환경 구축

#### build.zig에 LLVM 링크

```zig
const exe = b.addExecutable(.{
    .name = "llvm_codegen",
    .root_source_file = b.path("src/main.zig"),
    .target = b.standardTargetOptions(.{}),
    .optimize = b.standardOptimizeOption(.{}),
});

// LLVM 라이브러리 링크
exe.linkLibC();
exe.linkSystemLibrary("LLVM");

b.installArtifact(exe);
```

#### LLVM 설치 확인

```bash
# Linux
apt-get install llvm-dev

# macOS
brew install llvm

# Verify
llvm-config --version
llvm-config --cflags
llvm-config --libs
```

### 📋 Task 2: mul 함수 구현

위 `add` 예제를 응용하여 **mul 함수**를 생성하세요:

```zig
// 생성할 함수
pub fn mul(a: i32, b: i32) -> i32 {
    return a * b;
}
```

**변경할 부분:**
1. 함수 이름: "add" → "mul"
2. 연산: `LLVMBuildAdd` → `LLVMBuildMul`
3. 결과 출력

### 📋 Task 3: IR 출력 및 분석

생성된 LLVM IR을 다음과 같이 기록하세요:

```markdown
## mul 함수의 생성된 IR

```llvm
[여기에 생성된 IR 붙여넣기]
```

## add와 mul의 차이점

### 연산 명령어의 차이

add 함수:
```llvm
%result = add i32 %0, %1
```

mul 함수:
```llvm
%result = mul i32 %0, %1
```

### 관찰

- 두 연산 모두 같은 형식을 가짐
- 타입은 동일 (i32)
- 실제 연산만 다름 (add vs mul)
```

### 📋 Task 4: 레지스터 이름의 역할 관찰

**코드에서:**
```zig
const result = c.LLVMBuildMul(builder, arg0, arg1, "result");
```

**마지막 인자 "result"의 역할을 관찰하세요:**

```markdown
## LLVMBuildMul의 name 인자

### 코드
LLVMBuildMul(builder, arg0, arg1, "result")

### 생성된 IR에서의 이름
[관찰된 레지스터 이름 기록]

### 다른 이름으로 시도
name을 "product"로 바꾸면?
name을 "tmp"로 바꾸면?

### 결론
name 인자는...
[당신의 분석]
```

---

## 💡 심화 과제 (선택사항)

### Challenge 1: fma 함수

```zig
// fma(a, b, c) = (a * b) + c
// Fused Multiply-Add
```

**구현 순서:**
1. mul: `%prod = mul i32 %a, %b`
2. add: `%result = add i32 %prod, %c`
3. ret: `ret i32 %result`

### Challenge 2: 함수 체이닝

두 개 이상의 함수를 생성하고, 한 함수가 다른 함수를 호출하도록 만들기:

```zig
fn add(a: i32, b: i32) -> i32 { ... }
fn double_add(a: i32, b: i32, c: i32) -> i32 {
    let step1 = add(a, b);
    let step2 = add(step1, c);
    return step2;
}
```

---

## 🎯 체크리스트

```
【 1.2 완료 체크리스트 】

□ build.zig에 LLVM 라이브러리 링크했는가?
□ add 함수 예제 컴파일 및 실행했는가?
□ mul 함수 구현했는가?
□ 생성된 IR을 출력했는가?
□ add와 mul의 IR을 비교 분석했는가?
□ 레지스터 이름(name)의 역할을 이해했는가?
□ 모든 기록을 Markdown으로 정리했는가?

위 모두 체크되면 1.3으로 진행 가능!
```

---

## 🚀 다음 단계: 1.3 예고

### 📌 **[1.3: 제어 흐름(Control Flow) - If-Else와 Loop 설계]**

이제 간단한 함수는 만들 수 있습니다.

하지만 진정한 언어는 **조건문과 반복문**이 있어야 합니다!

```
다음에 배울 내용:

【 If-Else 구현 】
├─ 조건 검사 (ICmp)
├─ 조건 분기 (CondBr)
├─ 다중 Basic Block
└─ 예제: 절댓값 함수

【 Loop 구현 】
├─ 무조건 분기 (Br)
├─ Phi 노드 (값 선택)
├─ Loop 종료 조건
└─ 예제: 팩토리얼, 합계

【 고급 제어 흐름 】
├─ Switch 문
├─ Nested 구조
└─ 복잡한 알고리즘
```

---

## 📚 참고 자료

### LLVM C API 문서
```
https://llvm.org/doxygen/group__LLVMC.html

주요 섹션:
  ├─ Core.h: 기본 객체 생성/관리
  ├─ ExecutionEngine.h: 코드 실행
  ├─ Target.h: 타겟 아키텍처
  └─ ...
```

### Zig에서 LLVM 사용
```
@cImport 문법
@cInclude 경로 찾기
FFI (Foreign Function Interface) 이해
```

---

## 🎓 학습 요약

```
【 1.2의 핵심 】

1. Context, Module, Builder의 역할 이해
2. Function Type 정의 방법
3. 함수 추가 및 Basic Block 생성
4. Builder를 통한 명령어 삽입
5. 검증 및 IR 출력

【 다음을 할 수 있게 됨 】

✅ 간단한 연산 함수 생성
✅ LLVM IR 자동 생성
✅ 생성된 IR 검증 및 확인
✅ 레지스터 이름 이해
```

---

**준비되셨으면 Assignment 1.2를 시작하세요!** 🔧

이제 당신은 **'LLVM 빌더'** 입니다.

한 명령어씩 쌓아올린 당신의 코드가
**LLVM의 손에서 기계어로 변환되는 마법**을 목격하게 될 것입니다! ✨

---

**작성일**: 2026-02-26
**단계**: LLVM 전설 1.2
**상태**: 강의 완료, 과제 대기 중 ⏳
