# 📝 Assignment 1.2: LLVM C API를 통한 코드 생성

## 🎯 목표

**LLVM C API를 직접 사용하여 LLVM IR을 동적으로 생성하기**

이전에는 이미 만든 Zig 코드를 분석했다면,
이제는 **우리의 코드가 LLVM IR을 만드는 경험**을 합니다!

---

## 📊 과제 개요

| 항목 | 내용 |
|------|------|
| 난이도 | ⭐⭐⭐☆☆ (중급) |
| 소요 시간 | 3-4시간 |
| 선행 과제 | Assignment 1.1 (완료) |
| 제출 형식 | Zig 코드 + 생성된 IR + 분석 문서 |

---

## 🔧 Task 1: 환경 구축

### 1-1. LLVM 설치

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install llvm-dev llvm

# 버전 확인
llvm-config --version
```

#### macOS
```bash
brew install llvm

# 경로 확인
llvm-config --prefix
```

#### 설치 확인
```bash
llvm-config --cflags
llvm-config --libs
```

### 1-2. build.zig 설정

Zig 프로젝트에 LLVM을 링크하기 위해 `build.zig`를 수정하세요:

**파일**: `zlang-project/build.zig`

```zig
const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "llvm_codegen",
        .root_source_file = b.path("src/codegen_1_2.zig"),
        .target = target,
        .optimize = optimize,
    });

    // 🔑 LLVM 라이브러리 링크
    exe.linkLibC();
    exe.linkSystemLibrary("LLVM");

    // LLVM 컴파일 플래그 추가
    const llvm_cflags = b.run(&.{
        "llvm-config",
        "--cflags",
    }) catch &.{};

    exe.addCSourceFiles(.{
        .files = &.{},
        .flags = llvm_cflags,
    });

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the LLVM codegen program");
    run_step.dependOn(&run_cmd.step);
}
```

### 1-3. 빌드 및 실행 테스트

```bash
cd zlang-project

# 빌드
zig build

# 실행
zig build run
```

---

## 💻 Task 2: add 함수 생성 (예제 완성)

### 2-1. 스켈레톤 코드 작성

**파일**: `zlang-project/src/codegen_1_2.zig`

```zig
const c = @cImport({
    @cInclude("llvm-c/Core.h");
});
const std = @import("std");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // TODO: Context 생성

    // TODO: Module 생성

    // TODO: Builder 생성

    // TODO: add 함수 생성

    // TODO: 검증 및 출력
}
```

### 2-2. 완전한 구현

강의의 예제 코드를 참고하여 `add` 함수를 완전히 구현하세요.

**요구사항:**
```
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}
```

**확인 사항:**
```
✅ Context 생성/해제
✅ Module 생성/해제
✅ Builder 생성/해제
✅ Function Type 정의 (i32, [i32, i32])
✅ 함수 추가
✅ Entry block 생성
✅ Builder 배치
✅ 인자 획득 (arg0, arg1)
✅ 덧셈 명령어 삽입
✅ 반환 명령어
✅ 검증
✅ IR 출력
```

---

## 🔢 Task 3: mul 함수 구현

### 3-1. mul 함수 생성

`add` 예제를 응용하여 **mul 함수**를 추가하세요:

```zig
fn mul(a: i32, b: i32) -> i32 {
    return a * b;
}
```

**주요 변경 사항:**
```
1. 함수 이름: "mul"
2. 연산: LLVMBuildMul (대신 LLVMBuildAdd)
3. 명령어 이름: "product" (또는 다른 이름)
```

### 3-2. 같은 Module에 추가

`add`와 `mul`을 **같은 Module**에 추가하세요:

```zig
// ... add 함수 생성 ...

// mul 함수도 같은 module에 추가
const mul_func = c.LLVMAddFunction(module, "mul", func_type);
// ... 위와 동일한 과정 반복 ...
```

**결과 IR:**
```llvm
define i32 @add(i32 %0, i32 %1) {
entry:
  %result = add i32 %0, %1
  ret i32 %result
}

define i32 @mul(i32 %0, i32 %1) {
entry:
  %product = mul i32 %0, %1
  ret i32 %product
}
```

---

## 📤 Task 4: IR 출력 및 분석

### 4-1. IR 저장

프로그램이 생성한 LLVM IR을 파일에 저장하세요:

```zig
// 프로그램 출력
std.debug.print("【 Generated LLVM IR 】\n\n", .{});
const ir_string = c.LLVMPrintModuleToString(module);
defer c.LLVMDisposeMessage(ir_string);

std.debug.print("{s}\n", .{ir_string});

// (선택) 파일에 저장
const file = try std.fs.cwd().createFile("generated.ll", .{});
defer file.close();
try file.writeAll(std.mem.span(ir_string));
```

### 4-2. 분석 문서 작성

**파일**: `RESEARCH_NOTES/1_2_CODEGEN/Task4_Analysis.md`

```markdown
# Task 4: IR 분석과 레지스터 이름

## 1. 생성된 add 함수의 LLVM IR

```llvm
[생성된 add 함수의 완전한 IR]
```

## 2. 생성된 mul 함수의 LLVM IR

```llvm
[생성된 mul 함수의 완전한 IR]
```

## 3. add와 mul의 차이점 분석

### 연산 명령어의 차이

| 항목 | add | mul |
|------|-----|-----|
| 명령어 | add i32 | mul i32 |
| 인자 개수 | 2개 | 2개 |
| 반환 타입 | i32 | i32 |

### 공통점

- 타입 체계가 동일
- 구조가 동일
- 인자 처리가 동일

### 차이점

- 연산만 다름 (add vs mul)
- 인자 이름만 다름 (result vs product)

## 4. 레지스터 이름(name)의 역할

### 코드에서:

```zig
// add 함수
const result = c.LLVMBuildAdd(builder, arg0, arg1, "result");

// mul 함수
const product = c.LLVMBuildMul(builder, arg0, arg1, "product");
```

### 생성된 IR에서의 가상 레지스터 이름:

add 함수:
```llvm
%result = add i32 %0, %1
```

mul 함수:
```llvm
%product = mul i32 %0, %1
```

### name 인자의 의미

LLVMBuildAdd의 마지막 인자 `"result"`:
- **목적**: 생성되는 가상 레지스터에 의미 있는 이름 부여
- **효과**: IR 코드의 가독성 향상
- **주의사항**: 이름이 없으면 LLVM이 자동으로 %0, %1, %2... 할당

### 다른 이름으로 시도

name을 다르게 하면?

```zig
// name을 "tmp"로
const tmp = c.LLVMBuildAdd(..., "tmp");
// 결과: %tmp = add i32 %0, %1

// name을 "sum"으로
const sum = c.LLVMBuildAdd(..., "sum");
// 결과: %sum = add i32 %0, %1
```

### 결론

name 인자는 **생성된 가상 레지스터의 '라벨'** 입니다.
- 기능적으로는 영향 없음
- 가독성을 위해 의미 있는 이름 사용 권장
- 최적화 후에도 유지될 수 있음
```

---

## 🎯 Task 5: 심화 분석 (선택사항)

### Challenge 1: fma 함수 구현

```zig
fn fma(a: i32, b: i32, c: i32) -> i32 {
    return (a * b) + c;
}
```

**구현 단계:**
1. Function Type: i32, [i32, i32, i32]
2. 함수 추가: "fma"
3. Entry block 생성
4. Builder 배치
5. 첫 번째 연산: `mul = a * b`
6. 두 번째 연산: `result = mul + c`
7. 반환

**생성되는 IR:**
```llvm
define i32 @fma(i32 %0, i32 %1, i32 %2) {
entry:
  %mul = mul i32 %0, %1
  %result = add i32 %mul, %2
  ret i32 %result
}
```

### Challenge 2: 다양한 타입 함수

```zig
fn add_i64(a: i64, b: i64) -> i64 { ... }
fn add_f64(a: f64, b: f64) -> f64 { ... }
fn max(a: i32, b: i32) -> i32 { ... }
```

**관찰 사항:**
- 타입에 따라 연산이 어떻게 달라지는가?
- 부동소수점 연산의 IR은?
- 비교 연산은 어떻게 표현되는가?

---

## 📋 제출 체크리스트

```
【 1.2 제출 체크리스트 】

코드:
□ build.zig에 LLVM 링크 설정 완료
□ codegen_1_2.zig 작성 및 컴파일 성공
□ add 함수 생성 코드 작동 확인
□ mul 함수 생성 코드 추가
□ 같은 Module에 두 함수 추가됨 확인

IR 생성:
□ 프로그램 실행으로 IR 생성됨
□ add의 IR 확인
□ mul의 IR 확인
□ 생성된 IR을 파일에 저장

분석:
□ Task4_Analysis.md 작성 완료
□ add와 mul의 IR 비교 분석
□ 레지스터 이름의 역할 이해
□ 2-3가지 관찰 기록

(선택) 심화:
□ Challenge 1 (fma) 또는
□ Challenge 2 (다양한 타입) 중 하나 완료

모든 항목 완료 → 1.3으로 진행!
```

---

## 🚀 다음 단계

### 1.3: 제어 흐름(Control Flow)

```
학습할 내용:

【 If-Else 구현 】
├─ ICmp: 조건 검사
├─ CondBr: 조건 분기
├─ 다중 Basic Block
└─ 예제: 절댓값 함수

【 Loop 구현 】
├─ Br: 무조건 분기
├─ Phi: 값 선택
└─ 예제: 합계, 팩토리얼

【 복합 제어 흐름 】
└─ 실제 프로그래밍 로직 구현 시작!
```

---

## 💡 팁과 주의사항

### ✅ 좋은 관행

```zig
// ✅ 명확한 이름 사용
const sum = c.LLVMBuildAdd(builder, a, b, "sum");
const product = c.LLVMBuildMul(builder, a, b, "product");

// ✅ 리소스 정리
defer c.LLVMContextDispose(context);
defer c.LLVMDisposeModule(module);
defer c.LLVMDisposeBuilder(builder);
```

### ❌ 주의할 점

```zig
// ❌ 리소스 누수 주의
const builder = c.LLVMCreateBuilderInContext(context);
// (dispose를 호출하지 않음 → 메모리 누수!)

// ❌ 잘못된 타입
const i32_type = c.LLVMInt32TypeInContext(context);
const f64_type = c.LLVMDoubleTypeInContext(context);
// 함수 타입이 mismatch될 수 있음
```

### 🔍 디버깅 팁

```zig
// 1. 검증 에러 메시지 확인
var error_msg: [*c]u8 = null;
const verify_result = c.LLVMVerifyModule(
    module,
    c.LLVMPrintMessageAction,  // 에러 출력만 (중단 안함)
    &error_msg
);

// 2. 생성 과정 추적
std.debug.print("Creating function...\n", .{});
const func = c.LLVMAddFunction(module, "test", func_type);
std.debug.print("Function created: {s}\n", .{c.LLVMGetValueName(func)});
```

---

## 🎓 학습 정리

```
【 1.2 완료 후 할 수 있는 것 】

✅ LLVM C API의 기본 3요소 (Context, Module, Builder) 사용
✅ Function Type 정의 및 함수 생성
✅ 기본 연산 명령어 삽입 (add, mul, ...)
✅ LLVM IR 자동 생성 및 검증
✅ 생성된 IR 분석 및 이해

【 다음 학습을 위한 준비 】

✅ Basic Block 개념 이해
✅ 제어 흐름 그래프(CFG) 준비
✅ 조건 분기 (if-else) 구현 준비
✅ 루프 (while, for) 구현 준비
```

---

**이제 당신은 'LLVM 빌더'입니다!** 🏗️

한 명령어씩 쌓아올린 당신의 코드가
LLVM의 손에서 기계어로 변환되는 마법을 즐기세요! ✨

Assignment 1.2 완료 후 **[1.3: 제어 흐름]** 으로 진행하세요!

---

**예상 완료 시간**: 3-4시간
**난이도**: ⭐⭐⭐☆☆ (중급)
**보상**: LLVM API 마스터 한 걸음 더 가까워짐! 🎉
