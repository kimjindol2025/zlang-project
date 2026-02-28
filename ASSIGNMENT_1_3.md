# 📝 Assignment 1.3: LLVM 제어 흐름(Control Flow) 설계

## 🎯 목표

**LLVM C API를 사용하여 조건문과 반복문이 포함된 실제 프로그래밍 로직 생성하기**

이전에는 단순 연산 함수만 생성했다면,
이제는 **분기와 루프가 있는 실제 프로그램의 제어 흐름을 설계하는 경험**을 합니다!

---

## 📊 과제 개요

| 항목 | 내용 |
|------|------|
| 난이도 | ⭐⭐⭐⭐☆ (고급) |
| 소요 시간 | 4-5시간 |
| 선행 과제 | Assignment 1.2 (완료) |
| 제출 형식 | Zig 코드 + 생성된 IR + 분석 문서 |

---

## 🔧 Task 1: 절댓값(abs) 함수 구현

### 1-1. If-Else 구조 이해

절댓값 함수는 **분기(branching)**의 기본 예제입니다:

```zig
fn abs(x: i32) -> i32 {
    if (x < 0) {
        return -x;      // then block
    } else {
        return x;       // else block
    }
}
```

**LLVM 제어 흐름 구조:**
```
┌─────────────┐
│   Entry     │
│  x < 0?     │
└──────┬──────┘
       │
   ┌───┴────┐
   │        │
   ▼        ▼
┌──────┐ ┌──────┐
│ Then │ │ Else │
│ -x   │ │  x   │
└──────┘ └──────┘
   │        │
   └───┬────┘
       │
       ▼
   ┌────────┐
   │ Merge  │
   │ Return │
   └────────┘
```

### 1-2. 구현 요구사항

**파일**: `zlang-project/src/codegen_1_3.zig`

다음을 포함하여 abs 함수를 완전히 구현하세요:

**확인 사항:**
```
✅ Context, Module, Builder 생성
✅ 함수 타입 정의 (i32 → i32)
✅ abs 함수 추가
✅ Entry block 생성 및 Builder 배치
✅ 인자 획득 (x)
✅ 조건 비교: LLVMBuildICmp (x < 0)
   - Predicate: LLVMIntSLT (signed less than)
   - 비교 대상: x와 0
✅ 조건 분기: LLVMBuildCondBr
   - true 시: then_block으로 이동
   - false 시: else_block으로 이동
✅ Then block: -x 계산
   - LLVMBuildNeg(builder, x, "neg")
   - LLVMBuildRet(builder, neg_result)
✅ Else block: x 반환
   - LLVMBuildRet(builder, x)
✅ 검증 및 IR 출력
```

### 1-3. 구현 템플릿

```zig
const c = @cImport({
    @cInclude("llvm-c/Core.h");
});
const std = @import("std");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // 1. Context, Module, Builder 생성
    const context = c.LLVMContextCreate();
    defer c.LLVMContextDispose(context);

    const module = c.LLVMModuleCreateWithNameInContext("abs_demo", context);
    defer c.LLVMDisposeModule(module);

    const builder = c.LLVMCreateBuilderInContext(context);
    defer c.LLVMDisposeBuilder(builder);

    // 2. 함수 타입 정의: i32 (i32) -> i32
    const i32_type = c.LLVMInt32TypeInContext(context);
    var param_types = [_]c.LLVMTypeRef{i32_type};
    const func_type = c.LLVMFunctionType(i32_type, &param_types, 1, 0);

    // 3. abs 함수 추가
    const abs_func = c.LLVMAddFunction(module, "abs", func_type);

    // 4. Entry block 생성
    const entry_block = c.LLVMAppendBasicBlockInContext(context, abs_func, "entry");

    // TODO: then_block, else_block, merge_block 생성

    // 5. Entry block에서 Builder 배치
    c.LLVMPositionBuilderAtEnd(builder, entry_block);

    // 6. 인자 획득
    const x = c.LLVMGetParam(abs_func, 0);

    // 7. 조건 비교: x < 0
    const zero = c.LLVMConstInt(i32_type, 0, 0);
    const cond = c.LLVMBuildICmp(
        builder,
        c.LLVMIntSLT,    // signed less than
        x,
        zero,
        "x_lt_0"
    );

    // TODO: 조건 분기 (then_block, else_block으로)

    // 8. Then block: -x 계산
    // TODO: Builder를 then_block에 배치
    // TODO: neg = -x 계산
    // TODO: return neg

    // 9. Else block: x 반환
    // TODO: Builder를 else_block에 배치
    // TODO: return x

    // 10. Merge block: PHI 노드와 반환
    // TODO: Builder를 merge_block에 배치
    // TODO: PHI 노드 생성
    // TODO: phi.addIncoming() - then_block에서의 값
    // TODO: phi.addIncoming() - else_block에서의 값
    // TODO: return phi

    // 11. 검증
    var error_msg: [*c]u8 = null;
    const verify_result = c.LLVMVerifyModule(
        module,
        c.LLVMPrintMessageAction,
        &error_msg
    );

    if (verify_result != 0) {
        std.debug.print("Verification failed: {s}\n", .{error_msg});
        c.LLVMDisposeMessage(error_msg);
    }

    // 12. IR 출력
    std.debug.print("【 Generated LLVM IR 】\n\n", .{});
    const ir_string = c.LLVMPrintModuleToString(module);
    defer c.LLVMDisposeMessage(ir_string);
    std.debug.print("{s}\n", .{ir_string});
}
```

### 1-4. 예상 결과 IR

완전히 구현하면 다음과 같은 IR이 생성됩니다:

```llvm
define i32 @abs(i32 %0) {
entry:
  %x_lt_0 = icmp slt i32 %0, 0
  br i1 %x_lt_0, label %then, label %else

then:
  %neg = sub i32 0, %0
  br label %merge

else:
  br label %merge

merge:
  %result = phi i32 [ %neg, %then ], [ %0, %else ]
  ret i32 %result
}
```

---

## 📍 Task 2: PHI 노드 분석

### 2-1. PHI 노드의 역할

위의 abs 함수 IR에서:

```llvm
%result = phi i32 [ %neg, %then ], [ %0, %else ]
```

이 PHI 노드는 **SSA(Static Single Assignment)의 핵심**입니다.

**문제 상황:**
```llvm
// ❌ 이것은 불가능 (SSA 위반)
then:
  %result = sub i32 0, %0
else:
  %result = %0

// 하나의 변수 %result가 두 번 할당됨!
```

**PHI 노드 해결책:**
```llvm
// ✅ SSA 준수
then:
  %neg = sub i32 0, %0
else:
  (아무것도 없음)

merge:
  %result = phi i32 [ %neg, %then ], [ %0, %else ]
  // %result는 정확히 한 번만 할당됨!
```

### 2-2. PHI 노드 상세 분석 문서 작성

**파일**: `RESEARCH_NOTES/1_3_CONTROL_FLOW/Task2_PHI_Analysis.md`

다음을 포함하여 2-3페이지의 분석 문서를 작성하세요:

```markdown
# Task 2: PHI 노드와 SSA

## 1. PHI 노드란?

PHI 노드는...

## 2. abs 함수에서의 PHI 노드

### 실행 경로별 값 추적

abs(-5) 호출 시:
- Entry: x = -5
- x < 0? → true
- then 블록 실행: neg = -(-5) = 5
- merge 블록: %result = phi i32 [ 5, %then ], [ -5, %else ]
  → then에서 왔으므로 5 선택!

abs(5) 호출 시:
- Entry: x = 5
- x < 0? → false
- else 블록 실행 (아무 계산 없음)
- merge 블록: %result = phi i32 [ 5, %then ], [ 5, %else ]
  → else에서 왔으므로 5 선택!

## 3. LLVM C API의 PHI 노드

코드:
```zig
const phi = c.LLVMBuildPhi(builder, i32_type, "result");
c.LLVMAddIncoming(phi, &[_]c.LLVMValueRef{neg}, &[_]c.LLVMBasicBlockRef{then_block}, 1);
c.LLVMAddIncoming(phi, &[_]c.LLVMValueRef{x}, &[_]c.LLVMBasicBlockRef{else_block}, 1);
```

## 4. 왜 PHI가 필요한가?

- SSA 원칙: 각 변수는 정확히 한 번만 할당됨
- 이전 접근법: 메모리 스택 사용 (느림)
- PHI 노드: 레지스터 레벨에서 직접 선택 (빠름)

## 5. 다른 경우의 PHI 노드

Loop의 경우...

## 6. 최적화 관점

- 값 추적 가능: 데이터 흐름 분석
- 데드 코드 제거: 사용되지 않는 값 감지
- 루프 언롤링: 반복 구조 인식
```

---

## 🔢 Task 3: 간단한 반복(Loop) 구현 (선택)

### 3-1. 합계 함수

```zig
fn sum_to_n(n: i32) -> i32 {
    let sum = 0;
    let i = 0;
    while (i <= n) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
```

### 3-2. 루프의 5가지 블록

```
┌─────────────┐
│ Preheader   │  초기화: sum = 0, i = 0
└──────┬──────┘
       │
       ▼
   ┌────────┐
   │ Header │  조건 검사: i <= n?
   └──┬──┬──┘
      │  │
   진│  │거짓
      │  │
      ▼  ▼
   ┌──────┐ ┌──────────┐
   │ Body │ │   Exit   │
   │sum=  │ │ return   │
   │sum+i │ │  sum     │
   │i=i+1 │ └──────────┘
   └──┬───┘
      │
      └──→(루프 되돌리기)
```

### 3-3. 구현 (선택사항)

선택적으로 sum_to_n 함수를 구현하세요.

**핵심:**
- Preheader: 초기값 설정
- Header: PHI 노드로 반복되는 값 수신
- Body: 루프 로직
- Latch: 다음 반복 값 계산
- Exit: 루프 종료 후 반환

---

## 📊 Task 4: SSA와 PHI 노드 종합 분석

### 4-1. 분석 문서 작성

**파일**: `RESEARCH_NOTES/1_3_CONTROL_FLOW/Task4_SSA_PHI_Analysis.md`

3페이지 이상의 종합 분석 문서를 작성하세요:

```markdown
# Task 4: SSA와 PHI 노드의 종합 분석

## 1. SSA(Static Single Assignment)의 기본 원칙

### 1-1. SSA의 정의
SSA는...

### 1-2. SSA의 장점
1. 데이터 흐름 분석 용이
2. 최적화 기회 증대
3. 레지스터 할당 간단화

## 2. 전통적 접근법 vs SSA

### 전통적 접근법 (메모리)

```llvm
entry:
  %sum_addr = alloca i32      ; 스택에 메모리 할당
  store i32 0, i32* %sum_addr ; sum = 0
  %i_addr = alloca i32
  store i32 0, i32* %i_addr

loop_header:
  %i_load = load i32, i32* %i_addr    ; i 읽기
  %cond = icmp sle i32 %i_load, 10    ; i <= 10?
  br i1 %cond, label %loop_body, label %exit

loop_body:
  %sum_load = load i32, i32* %sum_addr ; sum 읽기
  %i_load2 = load i32, i32* %i_addr    ; i 읽기
  %add = add i32 %sum_load, %i_load2   ; sum + i
  store i32 %add, i32* %sum_addr       ; sum = ...
  (계속 메모리 접근)
```

**문제:** 메모리 접근이 많아서 느림

### SSA 접근법 (레지스터)

```llvm
preheader:
  br label %header

header:
  %sum = phi i32 [ 0, %preheader ], [ %next_sum, %body ]
  %i = phi i32 [ 0, %preheader ], [ %next_i, %body ]
  %cond = icmp sle i32 %i, 10
  br i1 %cond, label %body, label %exit

body:
  %next_sum = add i32 %sum, %i
  %next_i = add i32 %i, 1
  br label %header
```

**장점:** 메모리 접근 없음, 추적 용이, 빠름

## 3. 절댓값 함수의 SSA 분석

### abs 함수 재검토

```llvm
define i32 @abs(i32 %x) {
entry:
  %x_lt_0 = icmp slt i32 %x, 0
  br i1 %x_lt_0, label %then, label %else

then:
  %neg = sub i32 0, %x
  br label %merge

else:
  br label %merge

merge:
  %result = phi i32 [ %neg, %then ], [ %x, %else ]
  ret i32 %result
}
```

### SSA 검증

- [x] 각 값이 정확히 한 번만 정의됨
  - %x_lt_0은 entry에서만 정의
  - %neg는 then에서만 정의
  - %result는 merge에서만 정의

- [x] PHI 노드로 제어 흐름 병합
  - %result는 두 경로 모두에서 도달 가능한 merge에서 정의됨
  - 어디서 왔는지 명시: [ %neg, %then ], [ %x, %else ]

## 4. 최적화 기회

### 4-1. 불필요한 PHI 제거

만약 두 경로가 같은 값을 반환한다면?

```llvm
// Before
then:
  %val = add i32 %x, 1
  br label %merge

else:
  %val = add i32 %x, 1
  br label %merge

merge:
  %result = phi i32 [ %val, %then ], [ %val, %else ]
  ret i32 %result

// After (최적화)
%val = add i32 %x, 1  ; 블록 외부로 이동
ret i32 %val
```

### 4-2. 상수 폴딩

```llvm
// Before
entry:
  %a = add i32 2, 3
  ret i32 %a

// After (최적화)
entry:
  ret i32 5
```

## 5. PHI 노드의 한계

- 모든 선행 블록으로부터 정확히 하나의 값 필요
- 값이 없으면 undefined behavior
- 과도한 PHI는 성능 저하 가능성

## 6. 요약: 왜 SSA가 중요한가?

```
源 코드 (고수준)
  ↓ (컴파일러 프론트엔드)
SSA IR (LLVM 중간표현)
  ↓ (최적화 PASS들)
Optimized SSA
  ↓ (코드 생성)
기계어 (저수준)
```

SSA 덕분에 각 최적화 단계에서:
- 데이터 흐름이 명확함
- 별도의 분석 불필요
- 병렬 최적화 가능
- 증명 가능한 정확성 확보
```

---

## 📤 Task 5: IR 출력 및 파일 저장

### 5-1. 생성된 IR 저장

Task 1에서 생성한 abs 함수의 IR을:

```
RESEARCH_NOTES/1_3_CONTROL_FLOW/
├─ abs_generated.ll
└─ Task2_PHI_Analysis.md
└─ Task4_SSA_PHI_Analysis.md
```

에 저장하세요.

### 5-2. 빌드 및 실행

```bash
cd zlang-project

# 빌드 (build.zig 설정 필요)
zig build

# 실행
zig build run

# 출력을 파일로 저장
zig build run > RESEARCH_NOTES/1_3_CONTROL_FLOW/output.txt
```

---

## 📋 제출 체크리스트

```
【 1.3 제출 체크리스트 】

코드:
□ build.zig에 LLVM 링크 설정 확인
□ codegen_1_3.zig 작성 및 컴파일 성공
□ abs 함수 생성 코드 완전히 구현
□ 조건 비교 (ICmp) 작동 확인
□ 조건 분기 (CondBr) 작동 확인
□ PHI 노드 생성 및 동작 확인

IR 생성:
□ abs 함수의 IR 생성됨
□ Entry, Then, Else, Merge 블록 모두 존재
□ PHI 노드 확인
□ IR을 파일에 저장 (abs_generated.ll)

분석:
□ Task2_PHI_Analysis.md 작성 완료 (2-3페이지)
□ PHI 노드의 역할 명확히 이해
□ Task4_SSA_PHI_Analysis.md 작성 완료 (3페이지 이상)
□ SSA와 PHI의 관계 종합 분석

(선택) 심화:
□ Task 3: sum_to_n 루프 구현 (선택)

모든 필수 항목 완료 → 1.4로 진행!
```

---

## 🚀 다음 단계

### 1.4: 타입 시스템과 구조체 (Type Systems & Structs)

```
학습할 내용:

【 LLVM 타입 시스템 】
├─ 기본 타입: i32, f64, void
├─ 복합 타입: array, struct
├─ 포인터와 메모리 레이아웃
└─ 예제: 2D 포인트 구조체

【 구조체 생성 】
├─ Struct 타입 정의
├─ GEP: 구조체 필드 접근
├─ 메모리 할당 (alloca, malloc)
└─ 예제: Person 구조체

【 복합 제어 흐름 + 구조체 】
└─ 실제 사용 가능한 프로그램 구조 완성!
```

---

## 💡 팁과 주의사항

### ✅ 좋은 관행

```zig
// ✅ 명확한 블록 이름
const entry_block = c.LLVMAppendBasicBlockInContext(context, func, "entry");
const then_block = c.LLVMAppendBasicBlockInContext(context, func, "then");
const else_block = c.LLVMAppendBasicBlockInContext(context, func, "else");
const merge_block = c.LLVMAppendBasicBlockInContext(context, func, "merge");

// ✅ 명확한 변수 이름
const x_lt_0 = c.LLVMBuildICmp(builder, c.LLVMIntSLT, x, zero, "x_lt_0");
const neg_x = c.LLVMBuildNeg(builder, x, "neg_x");

// ✅ 블록 간 이동 명시
c.LLVMBuildCondBr(builder, cond, then_block, else_block);
c.LLVMBuildBr(builder, merge_block);
```

### ❌ 주의할 점

```zig
// ❌ Builder 위치 불일치
c.LLVMPositionBuilderAtEnd(builder, entry_block);
const cond = c.LLVMBuildICmp(...);  // 현재: entry_block
c.LLVMPositionBuilderAtEnd(builder, then_block);  // 위치 변경!
const neg = c.LLVMBuildNeg(...);  // 이제: then_block에 생성

// ❌ PHI 노드 불완전
const phi = c.LLVMBuildPhi(builder, i32_type, "result");
// AddIncoming 호출 없음 → undefined behavior!

// ❌ 모든 블록에서 return 필수
then_block:
  ret i32 %neg    // ✓
else_block:
  ret i32 %x      // ✓
merge_block:
  ret i32 %result // ✓ (merge에서도 필수)
```

### 🔍 디버깅 팁

```zig
// 1. 블록 확인
std.debug.print("Entry block: {*}\n", .{entry_block});
std.debug.print("Then block: {*}\n", .{then_block});

// 2. PHI 노드 확인
std.debug.print("PHI node created\n", .{});
c.LLVMAddIncoming(phi, &[_]c.LLVMValueRef{neg}, &[_]c.LLVMBasicBlockRef{then_block}, 1);
std.debug.print("PHI incoming added from then\n", .{});

// 3. IR 확인
const ir_string = c.LLVMPrintModuleToString(module);
std.debug.print("Generated IR:\n{s}\n", .{ir_string});
```

---

## 🎓 학습 정리

```
【 1.3 완료 후 할 수 있는 것 】

✅ 조건문 구현 (If-Else)
✅ 조건 비교 명령어 (ICmp)
✅ 조건 분기 (CondBr, Br)
✅ 다중 Basic Block 관리
✅ PHI 노드 생성 및 활용
✅ SSA 원칙 이해 및 적용
✅ 데이터 흐름 그래프 구성
✅ LLVM 최적화의 기초 이해

【 다음 학습을 위한 준비 】

✅ 타입 시스템 (타입 레이아웃, 포인터)
✅ 메모리 관리 (alloca, malloc, free)
✅ 구조체 필드 접근 (GEP)
✅ 실제 프로그래머 처럼 사용 가능한 언어 구현 준비
```

---

**이제 당신은 'LLVM 제어 흐름 마스터'입니다!** 🏗️

분기하고 반복하는 실제 프로그램의 제어 흐름을
LLVM IR로 정확히 설계하는 능력을 얻었습니다! ✨

Assignment 1.3 완료 후 **[1.4: 타입 시스템과 구조체]** 으로 진행하세요!

---

**예상 완료 시간**: 4-5시간
**난이도**: ⭐⭐⭐⭐☆ (고급)
**보상**: 실제 프로그래밍 로직 구현의 첫 걸음! 🎉
