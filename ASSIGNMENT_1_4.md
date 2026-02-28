# 📝 Assignment 1.4: 타입 시스템과 복합 구조체(Struct) 설계

## 🎯 목표

**LLVM의 타입 시스템을 사용하여 복잡한 데이터 구조를 메모리에 배치하고, GEP 명령어로 필드에 접근하는 경험을 합니다.**

메모리의 물리적 레이아웃부터 타입 기반 최적화까지,
**데이터 설계의 완전한 이해**를 목표로 합니다.

---

## 📊 과제 개요

| 항목 | 내용 |
|------|------|
| 난이도 | ⭐⭐⭐⭐⭐ (최고급) |
| 소요 시간 | 4-5시간 |
| 선행 과제 | Assignment 1.3 (완료) |
| 제출 형식 | Zig 코드 + 생성된 IR + 분석 문서 |

---

## 🔧 Task 1: 좌표 구조체 구현

### 1-1. Point 구조체 정의

**파일**: `zlang-project/src/codegen_1_4.zig`

Point 구조체를 정의하세요:

```zig
// 구조체 정의: { i32, i32 }
// 필드 0: x (i32)
// 필드 1: y (i32)
```

**구현 요구사항:**

```
✅ Context 생성/해제
✅ Module 생성/해제
✅ Builder 생성/해제
✅ i32 타입 정의
✅ Point 구조체 정의 (LLVMStructType)
   - 2개 필드
   - i32, i32
   - packed = 0 (정렬 있음)
✅ 함수 생성 (create_point)
✅ Entry 블록 생성
✅ Builder 배치
✅ 스택에 Point 할당 (alloca)
✅ IR 출력
```

### 1-2. 구현 템플릿

```zig
const c = @cImport({
    @cInclude("llvm-c/Core.h");
});
const std = @import("std");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // 1. Context, Module, Builder
    const context = c.LLVMContextCreate();
    defer c.LLVMContextDispose(context);

    const module = c.LLVMModuleCreateWithNameInContext("struct_demo", context);
    defer c.LLVMDisposeModule(module);

    const builder = c.LLVMCreateBuilderInContext(context);
    defer c.LLVMDisposeBuilder(builder);

    // 2. i32 타입
    const i32_type = c.LLVMInt32TypeInContext(context);

    // 3. Point 구조체 정의: { i32, i32 }
    var field_types = [_]c.LLVMTypeRef{ i32_type, i32_type };
    const point_type = c.LLVMStructType(&field_types, 2, 0);

    // 4. create_point 함수: void create_point()
    const func_type = c.LLVMFunctionType(
        c.LLVMVoidTypeInContext(context),
        null,  // 인자 없음
        0,
        0
    );
    const create_point_func = c.LLVMAddFunction(module, "create_point", func_type);

    // 5. Entry 블록
    const entry = c.LLVMAppendBasicBlockInContext(context, create_point_func, "entry");
    c.LLVMPositionBuilderAtEnd(builder, entry);

    // 6. 스택에 Point 할당
    const point = c.LLVMBuildAlloca(builder, point_type, "point");

    // 7. void 반환
    _ = c.LLVMBuildRetVoid(builder);

    // 8. IR 출력
    std.debug.print("【 Generated LLVM IR 】\n\n", .{});
    const ir_string = c.LLVMPrintModuleToString(module);
    defer c.LLVMDisposeMessage(ir_string);
    std.debug.print("{s}\n", .{ir_string});
}
```

### 1-3. 예상 결과 IR

```llvm
%Point = type { i32, i32 }

define void @create_point() {
entry:
  %point = alloca %Point
  ret void
}
```

---

## 🎯 Task 2: 필드 값 변경

### 2-1. GEP를 사용한 필드 접근

Task 1의 `create_point` 함수에 **필드 값 설정 로직**을 추가하세요:

```
✅ x 필드 (index 0) 주소 계산 (GEP)
✅ x 필드에 값 저장 (store)
✅ y 필드 (index 1) 주소 계산 (GEP)
✅ y 필드에 값 저장 (store)
✅ 최종 구조체 로드 (load)
✅ 반환
```

### 2-2. 구현 코드

Task 1의 코드에서 다음을 추가하세요:

```zig
// 이전: Point 할당
const point = c.LLVMBuildAlloca(builder, point_type, "point");

// TODO: x 필드 주소 계산
// LLVMBuildStructGEP2(builder, point_type, point, 0, "x_ptr")

// TODO: 상수 10 생성
// c.LLVMConstInt(i32_type, 10, 0)

// TODO: x 필드에 10 저장
// c.LLVMBuildStore(builder, ten_val, x_ptr)

// TODO: y 필드 주소 계산 (인덱스 1)

// TODO: 상수 42 생성

// TODO: y 필드에 42 저장

// TODO: 전체 구조체 로드
// c.LLVMBuildLoad2(builder, point_type, point, "result")

// 반환
_ = c.LLVMBuildRetVoid(builder);
```

### 2-3. 예상 결과 IR

```llvm
define void @create_point() {
entry:
  %point = alloca %Point

  ; x 필드에 10 저장
  %x_ptr = getelementptr %Point, ptr %point, i32 0, i32 0
  store i32 10, ptr %x_ptr

  ; y 필드에 42 저장
  %y_ptr = getelementptr %Point, ptr %point, i32 0, i32 1
  store i32 42, ptr %y_ptr

  ; 전체 구조체 로드
  %result = load %Point, ptr %point

  ret void
}
```

### 2-4. 중요: GEP 인덱스 해석

```
getelementptr %Point, ptr %point, i32 0, i32 1
                                   ↑       ↑
                                   포인터  구조체
                                   오프셋  필드
                                           인덱스

i32 0  → 포인터가 이미 Point의 시작을 가리키므로 오프셋 0
i32 1  → 구조체의 1번째 필드(y 필드)
```

---

## 📦 Task 3: 배열 설계와 다중 인덱싱

### 3-1. Point 배열 정의

새로운 함수 `initialize_points`를 추가하세요:

```
✅ [5 x %Point] 배열 타입 정의
✅ initialize_points() 함수 생성
✅ 배열을 스택에 할당
✅ points[2].x 주소 계산 (GEP 다중 인덱싱)
✅ points[2].x에 99 저장
✅ 결과 로드 및 반환
```

### 3-2. 구현 템플릿

```zig
// Point 구조체 정의 이후...

// 배열 타입: [5 x %Point]
const point_array_type = c.LLVMArrayType(point_type, 5);

// initialize_points 함수
const init_func = c.LLVMAddFunction(module, "initialize_points", func_type);
const init_entry = c.LLVMAppendBasicBlockInContext(context, init_func, "entry");
c.LLVMPositionBuilderAtEnd(builder, init_entry);

// 배열 할당
const points = c.LLVMBuildAlloca(builder, point_array_type, "points");

// TODO: points[2]의 주소 계산 (첫 번째 GEP)
// indices: [0, 2]
// - 0: 배열 포인터 오프셋
// - 2: 배열 내 인덱스

// TODO: points[2].x 필드 주소 계산 (두 번째 GEP)
// LLVMBuildStructGEP2(builder, point_type, point_ptr, 0, "x_ptr")

// TODO: 99 저장
const ninety_nine = c.LLVMConstInt(i32_type, 99, 0);
// store 99를 x_ptr에

// 반환
_ = c.LLVMBuildRetVoid(builder);
```

### 3-3. GEP 다중 인덱싱 해석

```
points[2].x의 주소 계산:

【 첫 번째 GEP: 배열 인덱싱 】
getelementptr [5 x %Point], ptr %points, i32 0, i32 2
메모리 계산: base + (2 * sizeof(Point))
           = base + (2 * 8)
           = base + 16 바이트

【 두 번째 GEP: 구조체 필드 】
getelementptr %Point, ptr %point_ptr, i32 0, i32 0
메모리 계산: point_ptr + (0 * 4) = point_ptr
           (x 필드는 첫 번째 필드이므로 오프셋 0)

【 최종 오프셋 】
points[2].x = base + 16 + 0 = base + 16 바이트
```

### 3-4. 예상 결과 IR

```llvm
%Point = type { i32, i32 }

define void @initialize_points() {
entry:
  %points = alloca [5 x %Point]

  ; points[2]의 주소
  %point_ptr = getelementptr [5 x %Point], ptr %points, i32 0, i32 2

  ; points[2].x의 주소
  %x_ptr = getelementptr %Point, ptr %point_ptr, i32 0, i32 0

  ; 99 저장
  store i32 99, ptr %x_ptr

  ret void
}
```

---

## 🔬 Task 4: GEP 인덱스 분석 - 왜 첫 번째는 0일까?

### 4-1. 분석 문서 작성

**파일**: `RESEARCH_NOTES/1_4_TYPE_SYSTEMS/Task4_GEP_Analysis.md`

3-4페이지의 심화 분석 문서를 작성하세요.

### 4-2. 다루어야 할 주제

#### 📌 포인터 산술의 기초

```c
// C에서의 포인터 산술
int arr[10];
int* ptr = arr;           // ptr = base address

int* p1 = ptr + 1;        // p1 = base + (1 * 4) = base + 4
int* p2 = ptr + 2;        // p2 = base + (2 * 4) = base + 8

// 복합 구조
struct Point {
    int x;
    int y;
};

Point pts[5];
Point* p = pts;           // p = base address
Point* p2 = p + 2;        // p2 = base + (2 * sizeof(Point))
                          //    = base + (2 * 8) = base + 16
```

#### 📌 LLVM의 GEP 철학

```llvm
; LLVM의 GEP는 C의 포인터 산술을 정확히 모델링합니다.

; 1. 배열 인덱싱
; arr[i] in C
; ↓
; getelementptr [N x T], ptr %arr, i32 0, i32 %i in LLVM

; 2. 구조체 필드
; s.field in C  (field = index 1)
; ↓
; getelementptr %S, ptr %s, i32 0, i32 1 in LLVM

; 3. 중첩된 접근
; arr[i].field in C
; ↓
; 첫 번째 GEP로 arr[i] 계산
; 두 번째 GEP로 field 계산
```

#### 📌 첫 번째 인덱스가 0인 이유

```
【 핵심: 포인터는 이미 올바른 위치를 가리키고 있다 】

// alloca로 할당한 경우
%arr = alloca [10 x i32]
; %arr은 배열의 시작 주소
; 추가 오프셋 불필요 → 0

// 함수 인자로 받은 경우
define void @foo(ptr %arr) {
  %p = getelementptr [10 x i32], ptr %arr, i32 0, i32 5
  ; %arr은 이미 배열의 시작 → 0
}

// 구조체 필드인 경우
%S = type { [10 x i32], ... }
%ptr = alloca %S
%arr_ptr = getelementptr %S, ptr %ptr, i32 0, i32 0
; 이제 %arr_ptr은 배열의 시작
%elem = getelementptr [10 x i32], ptr %arr_ptr, i32 0, i32 5
; 배열의 시작에서 5번째 → 0
```

#### 📌 포인터 오프셋 vs 요소 인덱스

```
getelementptr Type, ptr Base, i32 Offset, i32 Index1, ...

【 Offset (포인터 오프셋) 】
- Base 포인터 자체에 대한 배열 인덱싱
- 거의 항상 0 (Base가 이미 올바른 위치)
- 예외: 중첩된 배열의 경우 0이 아닐 수 있음

【 Index1, Index2, ... (요소 인덱스) 】
- Type 내부에서의 필드/요소 선택
- 배열이면 배열 인덱스
- 구조체면 필드 인덱스
```

#### 📌 실전 예제

```llvm
; 예제 1: 단순 배열
%arr = alloca [10 x i32]
%p = getelementptr [10 x i32], ptr %arr, i32 0, i32 5
; 0: %arr은 배열의 시작 (offset 불필요)
; 5: 배열의 5번째 요소

; 예제 2: 구조체 배열
%S = type { i32, [10 x i32] }
%arr = alloca [3 x %S]
%s_ptr = getelementptr [3 x %S], ptr %arr, i32 0, i32 2
; 0: %arr은 배열의 시작
; 2: 배열의 2번째 요소(구조체)

%inner_arr = getelementptr %S, ptr %s_ptr, i32 0, i32 1
; 0: %s_ptr은 구조체의 시작
; 1: 구조체의 1번째 필드([10 x i32])

%elem = getelementptr [10 x i32], ptr %inner_arr, i32 0, i32 7
; 0: %inner_arr은 배열의 시작
; 7: 배열의 7번째 요소
```

### 4-3. 분석 템플릿

```markdown
# Task 4: GEP 인덱스 분석 - 포인터 산술의 진실

## 1. 포인터 산술의 기초

C에서의 포인터 산술은...

## 2. alloca의 의미

alloca로 할당한 메모리는...

## 3. GEP의 첫 번째 인덱스 규칙

첫 번째 인덱스가 0인 이유는...

### 규칙 1: 이미 올바른 위치
```
alloca %Type → 포인터는 Type의 시작을 가리킴
GEP의 offset = 0
```

### 규칙 2: 함수 인자도 동일
```
함수 인자 ptr %T → 이미 T의 시작
GEP의 offset = 0
```

## 4. 예외 상황

offset이 0이 아닌 경우...

## 5. 최적화 관점

LLVM이 이렇게 설계한 이유...

## 6. 결론

포인터 산술은 수학적으로 엄밀하며,
LLVM은 이를 완벽하게 구현합니다.
```

---

## 📤 Task 5: 메모리 레이아웃 시각화

### 5-1. Point 구조체 메모리 맵

**파일**: `RESEARCH_NOTES/1_4_TYPE_SYSTEMS/Memory_Layout.md`

다음을 포함한 시각화 문서를 작성하세요:

```markdown
# Point 구조체의 메모리 레이아웃

## 1. 단순 Point 구조체

%Point = type { i32, i32 }

┌──────────────┬──────────────┐
│  x (i32)     │  y (i32)     │
│ 4 바이트     │ 4 바이트     │
└──────────────┴──────────────┘
offset 0      offset 4       offset 8

## 2. Point 배열

[5 x %Point]

┌─────────────────────┬─────────────────────┬─── ...
│ Point[0]            │ Point[1]            │
│ (8 바이트)          │ (8 바이트)          │
└─────────────────────┴─────────────────────┴─── ...
offset 0            offset 8            offset 16

## 3. GEP 계산 과정

points[2].x의 주소:

1. points 포인터: base address
2. GEP [5 x Point]: base + (2 * 8) = base + 16
3. GEP Point, field 0: base + 16 + 0 = base + 16

## 4. 패딩(Padding) 분석

만약 Point = { i32, i8, i32 }라면?

┌──────────────┬─┬───┬──────────────┐
│ x (i32)      │y│pad│  z (i32)     │
└──────────────┴─┴───┴──────────────┘
0              4 5 7 8             12
```

---

## 📋 제출 체크리스트

```
【 1.4 제출 체크리스트 】

코드:
□ build.zig에 LLVM 링크 설정 확인
□ codegen_1_4.zig 작성 및 컴파일 성공
□ Task 1: Point 구조체 정의 완료
□ Task 2: GEP를 사용한 필드 접근 완료
□ Task 3: 배열 및 다중 GEP 구현 완료

IR 생성:
□ create_point 함수의 IR 생성됨
□ initialize_points 함수의 IR 생성됨
□ GEP 명령어 확인
□ alloca, store, load 명령어 확인
□ IR을 파일에 저장

분석:
□ Task 4: GEP 분석 문서 (3-4페이지) 완료
□ Task 5: 메모리 레이아웃 시각화 완료
□ 포인터 산술 이해
□ 패딩과 정렬 개념 이해

모든 필수 항목 완료 → 1.5로 진행!
```

---

## 🚀 다음 단계

### 1.5: JIT 컴파일과 실행 엔진 설계

```
우리가 만든 IR이 살아있는 기계어로 변하는 순간입니다!

【 LLVM 실행 엔진 】
├─ JIT 컴파일러 (Just-In-Time)
├─ ExecutionEngine 초기화
├─ 함수 포인터 얻기
├─ 함수 직접 호출
└─ 성능 벤치마킹

【 실습 목표 】
1. abs(x) 함수 JIT 컴파일
2. point_add(a, b) 함수 JIT 컴파일
3. 실제 값으로 호출하여 결과 확인
4. 성능 분석
```

---

## 💡 팁과 주의사항

### ✅ 좋은 관행

```zig
// ✅ 명확한 구조체 정의
var field_types = [_]c.LLVMTypeRef{
    c.LLVMInt32Type(),    // x
    c.LLVMInt32Type()     // y
};
const point_type = c.LLVMStructType(&field_types, 2, 0);

// ✅ 명확한 필드 인덱스
const x_ptr = c.LLVMBuildStructGEP2(builder, point_type, point, 0, "x_ptr");
const y_ptr = c.LLVMBuildStructGEP2(builder, point_type, point, 1, "y_ptr");

// ✅ 명확한 배열 타입
const array_type = c.LLVMArrayType(point_type, 5);
```

### ❌ 주의할 점

```zig
// ❌ 필드 인덱스 착각
const wrong_ptr = c.LLVMBuildStructGEP2(builder, point_type, point, 2, "wrong");
// Point는 2개 필드이므로 인덱스 2는 없음!

// ❌ 배열 크기 초과
const array_type = c.LLVMArrayType(point_type, 5);
// 하지만 index 10 접근 시도 (범위 외)

// ❌ 타입 불일치
store i32 100, ptr %x_ptr    // ✓
store i64 100, ptr %x_ptr    // ❌ i64를 i32* 포인터에 저장?
```

### 🔍 디버깅 팁

```zig
// 1. 구조체 필드 개수 확인
const field_count = c.LLVMCountStructElementTypes(point_type);
std.debug.print("Point has {} fields\n", .{field_count});

// 2. 구조체 크기 확인
const size = c.LLVMStoreSizeOfType(..., point_type);
std.debug.print("Point size: {} bytes\n", .{size});

// 3. IR 단계별 출력
const ir_string = c.LLVMPrintModuleToString(module);
std.debug.print("{s}\n", .{ir_string});

// 4. 검증
var error_msg: [*c]u8 = null;
const verify_result = c.LLVMVerifyModule(
    module,
    c.LLVMPrintMessageAction,
    &error_msg
);
if (verify_result != 0) {
    std.debug.print("Verification failed!\n", .{});
}
```

---

## 🎓 학습 정리

```
【 1.4 완료 후 할 수 있는 것 】

✅ LLVM 타입 시스템 완전 이해
✅ 구조체 정의 및 생성
✅ GEP로 필드/배열 요소 접근
✅ 메모리 레이아웃 계산
✅ 데이터 정렬과 패딩 이해
✅ 복잡한 중첩 구조체 처리
✅ 다차원 배열 설계

【 다음 1.5의 준비 】

✅ 실제 함수 호출 능력 준비
✅ JIT 컴파일러 이해
✅ 실행 가능한 기계어 생성
✅ 성능 측정 기술
```

---

**이제 당신은 'LLVM 메모리 설계자'입니다!** 🏛️

단순한 연산을 넘어,
**실제 프로그램의 데이터 구조를 설계하고 배치하는 능력**을 얻었습니다! ✨

Assignment 1.4 완료 후 **[1.5: JIT 컴파일과 실행 엔진 설계]** 으로 진행하세요!

---

**예상 완료 시간**: 4-5시간
**난이도**: ⭐⭐⭐⭐⭐ (최고급)
**보상**: LLVM의 진정한 메모리 관리자 도약! 🎉
