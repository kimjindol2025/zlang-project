# 🏛️ LLVM 전설 1.4: 타입 시스템과 복합 구조체(Struct) 설계

> **최종 업데이트**: 2026-02-26
> **상태**: 🎓 강의 완성
> **난이도**: ⭐⭐⭐⭐☆ (고급)
> **시간**: 4-5시간

---

## 📖 개요

**단순한 정수 연산을 넘어, 이제 메모리 위에 복잡한 데이터를 배치하고 관리하는 "데이터의 설계도"를 그릴 차례입니다.**

LLVM은 모든 값에 **엄격한 타입**이 부여되는 강력한 타입 시스템을 가집니다.

이는 컴파일러가:
- 메모리 오프셋을 **정확히 계산**하고
- 안전한 변환을 **보장**하며
- **최적화를 수행**하기 위한 **필수 조건**입니다.

---

## 🎯 1단계: 기본 타입과 파생 타입

### 1-1. Integer Types (정수 타입)

```llvm
; 비트 단위로 정의되는 정수 타입
i1      ; Boolean (1 비트)
i8      ; 8비트 (char)
i16     ; 16비트
i32     ; 32비트 (int) - 가장 흔함
i64     ; 64비트 (long)

; 임의의 비트 폭
i256    ; 256비트 (BigInt)
i1024   ; 1024비트 (암호화)
```

**LLVM C API:**
```c
LLVMTypeRef i32_type = LLVMInt32Type();
LLVMTypeRef i64_type = LLVMInt64Type();
LLVMTypeRef custom_type = LLVMIntType(128);  // i128
```

### 1-2. Floating Point Types (부동소수점 타입)

```llvm
half    ; 16비트 (IEEE 754 반정밀도)
float   ; 32비트 (IEEE 754 단정밀도)
double  ; 64비트 (IEEE 754 배정밀도)
fp128   ; 128비트 (IEEE 754 4배정밀도)
```

**LLVM C API:**
```c
LLVMTypeRef float_type = LLVMFloatType();
LLVMTypeRef double_type = LLVMDoubleType();
```

### 1-3. Pointer Types (포인터 타입)

LLVM 15 이전:
```llvm
i32*        ; i32를 가리키는 포인터
float*      ; float를 가리키는 포인터
%struct*    ; 구조체를 가리키는 포인터
```

LLVM 15 이후 (불투명 포인터):
```llvm
ptr         ; 모든 종류의 포인터 (타입 정보 없음)
ptr addrspace(1)  ; 주소 공간 1의 포인터
```

**LLVM C API:**
```c
// 새로운 방식 (LLVM 15+)
LLVMTypeRef ptr_type = LLVMPointerType(LLVMIntType(32), 0);

// 모든 포인터가 동일한 타입
LLVMTypeRef ptr_type_opaque = LLVMPointerTypeInContext(context, 0);
```

### 1-4. Array Types (배열 타입)

```llvm
; 구문: [개수 x 요소타입]
[10 x i32]      ; 10개의 i32 배열
[100 x float]   ; 100개의 float 배열
[5 x [3 x i32]] ; 5x3 2차원 배열
```

**메모리 레이아웃:**
```
[10 x i32]의 메모리:
┌─────────────────────────────────────────┐
│ i32  │ i32  │ i32  │ ... │ i32  │
│ (0)  │ (4)  │ (8)  │     │ (36) │
└─────────────────────────────────────────┘
0바이트  4바이트  8바이트      40바이트
```

**LLVM C API:**
```c
LLVMTypeRef array_type = LLVMArrayType(LLVMInt32Type(), 10);
```

---

## 🏗️ 2단계: 구조체(Struct) 설계 - 메모리의 지도

### 2-1. Literal Structs (이름 없는 구조체)

이름 없이 즉석에서 정의되는 구조체입니다.

```llvm
; 구문: { type1, type2, type3, ... }

; 2차원 점 (Point)
%Point = type { i32, i32 }

; 사람 정보 (Person)
%Person = type { i32, i8*, i1 }  ; age, name, is_admin
```

**LLVM C API:**
```c
// 1. 타입 배열 정의
LLVMTypeRef element_types[] = {
    LLVMInt32Type(),    // x
    LLVMInt32Type()     // y
};

// 2. 구조체 생성
LLVMTypeRef point_type = LLVMStructType(
    element_types,
    2,          // 요소 개수
    0           // 0 = packed 아님, 1 = packed
);
```

### 2-2. Identified Structs (이름 있는 구조체)

이름을 가지고, **재귀적 정의(Self-referencing)**가 가능한 구조체입니다.

**연결 리스트(Linked List)처럼 자기 자신을 참조하는 경우 필수:**

```llvm
; 선언: 구조체 타입 선언 (아직 비어있음)
%LinkedListNode = type opaque

; 정의: 실제 타입 설정 (자신을 참조 가능)
%LinkedListNode = type { i32, %LinkedListNode* }
```

**메모리 레이아웃:**
```
LinkedListNode:
┌──────────────┬──────────────┐
│ value: i32   │ next: ptr    │
│ 4 바이트     │ 8 바이트     │
└──────────────┴──────────────┘
0            4              12
```

**LLVM C API:**
```c
// 1. 구조체 이름 선언
LLVMTypeRef node_type = LLVMStructCreateNamed(context, "LinkedListNode");

// 2. 타입 정의 (자신을 참조 가능!)
LLVMTypeRef i32_type = LLVMInt32Type();
LLVMTypeRef ptr_type = LLVMPointerType(node_type, 0);

LLVMTypeRef element_types[] = { i32_type, ptr_type };
LLVMStructSetBody(node_type, element_types, 2, 0);

// 이제 node_type은 완성된 구조체
```

### 2-3. 메모리 레이아웃 시각화

```
【 구조체 메모리 배치 】

Person = { i32, i8*, i1 }

┌──────────────┬────────────────────┬──┐
│ age: i32     │ name: i8*          │ ? │ is_admin: i1
│ 4 바이트     │ 8 바이트 (64비트)  │ 패딩 │ 1 바이트
└──────────────┴────────────────────┴──┘
0             4                   12 13 15

"offset(age) = 0"
"offset(name) = 4"
"offset(is_admin) = 13"  ← 패딩 때문에 12가 아님!
```

---

## 🔬 3단계: 데이터 정렬(Alignment)과 패딩(Padding)

### 3-1. CPU의 관점

CPU는 특정 **바이트 경계(Byte Boundary)**에서 데이터를 읽을 때 **가장 빠릅니다.**

```
Alignment의 중요성:

【 Unaligned Access (느림) 】
┌─────┬─────┬─────┐
│     │◆◆◆◆│◆   │
└─────┴─────┴─────┘
버스 1    버스 2
→ 2번의 버스 접근 필요!

【 Aligned Access (빠름) 】
┌─────┬─────┬─────┐
│     │◆◆◆◆│     │
└─────┴─────┴─────┘
버스 2
→ 1번의 버스 접근!
```

### 3-2. LLVM의 패딩 규칙

LLVM은 타겟 아키텍처에 맞춰 **보이지 않는 빈 공간(Padding)**을 넣습니다.

```c
// 문제: 잘못된 오프셋 계산
struct BadLayout {
    int32_t x;      // offset 0, 4 바이트
    int8_t y;       // offset 4, 1 바이트? (NO!)
    int32_t z;      // offset 5? (NO!)
}

// LLVM의 자동 정렬
{
    i32,        // offset 0 (정렬: 4)
    i8,         // offset 4 (정렬 필요: 1)
    pad(3),     // offset 5-7 (패딩 3바이트)
    i32         // offset 8 (정렬: 4의 배수)
}
```

### 3-3. Packed Struct (패킹)

패딩 없이 필드를 **연속으로 배치**합니다.

```llvm
; 일반 구조체 (패딩 있음)
%Normal = type { i32, i8, i32 }
; 메모리: [i32:4] [i8:1] [pad:3] [i32:4] = 12바이트

; Packed 구조체 (패딩 없음)
%Packed = type <{ i32, i8, i32 }>
; 메모리: [i32:4] [i8:1] [i32:4] = 9바이트
```

**LLVM C API:**
```c
// Packed 구조체 생성
LLVMTypeRef element_types[] = {
    LLVMInt32Type(),
    LLVMInt8Type(),
    LLVMInt32Type()
};

LLVMTypeRef packed_struct = LLVMStructType(
    element_types,
    3,
    1  // 1 = packed (패딩 없음)
);
```

---

## 🎯 4단계: GEP(GetElementPtr) - 전설의 난관

### 4-1. GEP의 진실

**GEP는 메모리에 직접 접근하는 것이 아니라, "주소 계산"만 수행합니다.**

```llvm
; 구조체 정의
%MyStruct = type { i32, i32 }

define void @main() {
    ; 스택에 구조체 할당
    %ptr = alloca %MyStruct

    ; 두 번째 필드(index 1)의 주소를 가져오기
    ; 🔑 첫 번째 0은 포인터 자체의 오프셋
    ; 🔑 두 번째 1은 구조체 내부의 인덱스
    %field_ptr = getelementptr %MyStruct, ptr %ptr, i32 0, i32 1

    ; 이제 field_ptr은 두 번째 i32의 주소 (메모리 접근 아님!)
    store i32 100, ptr %field_ptr  ; 이제 메모리 접근

    ret void
}
```

### 4-2. GEP의 인덱스 해석

```llvm
; 배열 예제
%arr = alloca [10 x i32]

; 배열의 5번째 요소의 주소 계산
; getelementptr 타입, 포인터, 포인터오프셋, 배열인덱스
%elem_ptr = getelementptr [10 x i32], ptr %arr, i32 0, i32 5

; 포인터 오프셋 해석:
;   i32 0  → 포인터가 이미 배열의 시작을 가리키므로 0
;   i32 5  → 배열 내에서 5번째 요소 (= offset 5*4 = 20 바이트)
```

### 4-3. 다차원 배열과 GEP

```llvm
; 2차원 배열
%arr2d = alloca [5 x [3 x i32]]

; arr[2][1]의 주소 계산
%elem_ptr = getelementptr [5 x [3 x i32]], ptr %arr2d, i32 0, i32 2, i32 1

; 인덱스 해석:
;   i32 0   → 포인터 오프셋 (배열의 시작)
;   i32 2   → 외부 배열 인덱스 (행 2)
;   i32 1   → 내부 배열 인덱스 (열 1)

; 메모리 오프셋 계산:
; = 2 * (3 * 4) + 1 * 4
; = 2 * 12 + 4
; = 28 바이트
```

### 4-4. 구조체 배열과 GEP

```llvm
; Point 구조체 배열
%Point = type { i32, i32 }
%points = alloca [5 x %Point]

; points[2].y의 주소 계산
%elem_ptr = getelementptr [5 x %Point], ptr %points, i32 0, i32 2, i32 1

; 인덱스 해석:
;   i32 0   → 배열의 시작
;   i32 2   → 배열 내 3번째 요소 (Point 구조체)
;   i32 1   → 구조체 내 2번째 필드 (y)

; 메모리 오프셋:
; = 2 * 8 (Point 크기) + 4 (y의 오프셋)
; = 20 바이트
```

### 4-5. LLVM C API로 GEP 생성

```c
// 기본 GEP
LLVMValueRef elem_ptr = LLVMBuildGEP(
    builder,
    ptr,
    indices,      // 인덱스 배열
    2,            // 인덱스 개수
    "elem_ptr"
);

// 구조체 필드 GEP (더 편함)
LLVMValueRef field_ptr = LLVMBuildStructGEP2(
    builder,
    struct_type,
    ptr,
    1,            // 필드 인덱스
    "field_ptr"
);
```

---

## 📦 5단계: 실전 API 활용

### 5-1. 타입 정의 함수들

```c
// 📌 구조체 타입 생성 API

// 익명 구조체 생성
LLVMTypeRef LLVMStructType(
    LLVMTypeRef *ElementTypes,  // 필드 타입 배열
    unsigned ElementCount,       // 필드 개수
    LLVMBool Packed             // 0 = 정렬 있음, 1 = 패킹
);

// 이름 있는 구조체 선언
LLVMTypeRef LLVMStructCreateNamed(
    LLVMContextRef C,
    const char *Name
);

// 이름 있는 구조체 정의
void LLVMStructSetBody(
    LLVMTypeRef StructTy,
    LLVMTypeRef *ElementTypes,
    unsigned ElementCount,
    LLVMBool Packed
);

// 배열 타입 생성
LLVMTypeRef LLVMArrayType(
    LLVMTypeRef ElementType,
    unsigned ElementCount
);

// 포인터 타입 생성
LLVMTypeRef LLVMPointerType(
    LLVMTypeRef ElementType,
    unsigned AddressSpace
);
```

### 5-2. 메모리 접근 함수들

```c
// 📌 메모리 할당

// 스택에 할당
LLVMValueRef LLVMBuildAlloca(
    LLVMBuilderRef B,
    LLVMTypeRef Ty,
    const char *Name
);

// 배열 크기만큼 스택에 할당
LLVMValueRef LLVMBuildArrayAlloca(
    LLVMBuilderRef B,
    LLVMTypeRef Ty,
    LLVMValueRef Size,
    const char *Name
);

// 메모리 읽기
LLVMValueRef LLVMBuildLoad2(
    LLVMBuilderRef B,
    LLVMTypeRef Ty,
    LLVMValueRef PointerVal,
    const char *Name
);

// 메모리 쓰기
LLVMValueRef LLVMBuildStore(
    LLVMBuilderRef B,
    LLVMValueRef Val,
    LLVMValueRef Ptr
);

// 주소 계산
LLVMValueRef LLVMBuildGEP2(
    LLVMBuilderRef B,
    LLVMTypeRef Ty,
    LLVMValueRef Pointer,
    LLVMValueRef *Indices,
    unsigned NumIndices,
    const char *Name
);

// 구조체 필드 주소 계산 (편의함수)
LLVMValueRef LLVMBuildStructGEP2(
    LLVMBuilderRef B,
    LLVMTypeRef Ty,
    LLVMValueRef Pointer,
    unsigned Idx,
    const char *Name
);
```

---

## 💻 6단계: 완전한 예제 - Point 구조체

### 6-1. IR 예상 결과

```llvm
; Point 구조체 정의
%Point = type { i32, i32 }

define void @create_point() {
entry:
    ; 스택에 Point 할당
    %point = alloca %Point

    ; x 필드에 10 저장
    %x_ptr = getelementptr %Point, ptr %point, i32 0, i32 0
    store i32 10, ptr %x_ptr

    ; y 필드에 20 저장
    %y_ptr = getelementptr %Point, ptr %point, i32 0, i32 1
    store i32 20, ptr %y_ptr

    ; 전체 구조체 로드
    %result = load %Point, ptr %point
    ret void
}
```

### 6-2. Zig 구현 코드 (예상)

```zig
const c = @cImport({
    @cInclude("llvm-c/Core.h");
});

pub fn main() !void {
    const context = c.LLVMContextCreate();
    defer c.LLVMContextDispose(context);

    const module = c.LLVMModuleCreateWithNameInContext("point_demo", context);
    defer c.LLVMDisposeModule(module);

    const builder = c.LLVMCreateBuilderInContext(context);
    defer c.LLVMDisposeBuilder(builder);

    // 1. Point 구조체 정의: { i32, i32 }
    const i32_type = c.LLVMInt32TypeInContext(context);
    var field_types = [_]c.LLVMTypeRef{ i32_type, i32_type };
    const point_type = c.LLVMStructType(&field_types, 2, 0);

    // 2. create_point 함수 생성
    const func_type = c.LLVMFunctionType(c.LLVMVoidTypeInContext(context), null, 0, 0);
    const func = c.LLVMAddFunction(module, "create_point", func_type);

    // 3. Entry 블록
    const entry = c.LLVMAppendBasicBlockInContext(context, func, "entry");
    c.LLVMPositionBuilderAtEnd(builder, entry);

    // 4. 스택에 Point 할당
    const point = c.LLVMBuildAlloca(builder, point_type, "point");

    // 5. x 필드에 10 저장
    const x_ptr = c.LLVMBuildStructGEP2(builder, point_type, point, 0, "x_ptr");
    const ten = c.LLVMConstInt(i32_type, 10, 0);
    _ = c.LLVMBuildStore(builder, ten, x_ptr);

    // 6. y 필드에 20 저장
    const y_ptr = c.LLVMBuildStructGEP2(builder, point_type, point, 1, "y_ptr");
    const twenty = c.LLVMConstInt(i32_type, 20, 0);
    _ = c.LLVMBuildStore(builder, twenty, y_ptr);

    // 7. 반환
    _ = c.LLVMBuildRetVoid(builder);

    // 8. IR 출력
    const ir_string = c.LLVMPrintModuleToString(module);
    defer c.LLVMDisposeMessage(ir_string);
    std.debug.print("{s}\n", .{ir_string});
}
```

---

## 🔍 7단계: 고급 예제 - 동적 배열과 GEP

### 7-1. 2D 좌표 배열

```llvm
; Point 구조체
%Point = type { i32, i32 }

; 5개의 Point를 배열로
%PointArray = type [5 x %Point]

define void @initialize_points() {
entry:
    %points = alloca %PointArray

    ; points[2].y = 99 설정
    ; 배열 내 3번째 Point의 y 필드
    %point_ptr = getelementptr [5 x %Point], ptr %points, i32 0, i32 2
    %y_ptr = getelementptr %Point, ptr %point_ptr, i32 0, i32 1
    store i32 99, ptr %y_ptr

    ret void
}
```

### 7-2. 메모리 오프셋 계산

```
Point = { i32(4), i32(4) }  → 크기 8바이트

points[2].y의 주소:
= base + (2 * 8) + 4
= base + 16 + 4
= base + 20 바이트
```

---

## 🎓 8단계: 최적화와 LLVM의 혜택

### 8-1. GEP 최적화

LLVM 최적화 패스는 GEP의 주소 계산을 매우 효율적으로 단순화합니다.

```llvm
; Before: 복잡한 GEP 체인
%p1 = getelementptr %S, ptr %base, i32 0, i32 2
%p2 = getelementptr [10 x i32], ptr %p1, i32 0, i32 3

; After: 단순한 단일 오프셋
; (자동으로 20 + 12 = 32 바이트로 계산)
%p = getelementptr i8, ptr %base, i32 32
```

### 8-2. Type-Based Alias Analysis (TBAA)

LLVM은 타입 정보를 사용하여 메모리 접근이 **안전한지 판단**합니다.

```llvm
; i32* 필드와 i8* 필드는 별개의 메모리
; 따라서 이 두 작업은 순서 변경 가능!

%x_ptr = getelementptr %S, ptr %s, i32 0, i32 0  ; i32*
store i32 10, ptr %x_ptr, !tbaa !0

%name_ptr = getelementptr %S, ptr %s, i32 0, i32 1  ; i8*
store i8* ..., ptr %name_ptr, !tbaa !1
```

---

## 📝 Assignment 1.4 준비

다음 과제에서 다룰 내용:

### Task 1: 좌표 구조체 구현

Point 구조체 (x, y - 둘 다 i32)를 LLVM API로 정의합니다.

### Task 2: 필드 값 변경

GEP 명령어를 사용하여 Point의 y 값에 42를 저장하는 IR을 생성합니다.

### Task 3: 배열 설계

Point 구조체 5개를 담는 배열 타입을 정의하고, 3번째 요소의 x 좌표 주소를 계산합니다.

### Task 4: GEP 분석

"GEP 명령어에서 왜 첫 번째 인덱스로 보통 0을 주어야 하는지" 포인터 연산 관점에서 분석합니다.

---

## 💡 핵심 개념 정리

### 메모리 설계의 3단계

```
【 데이터 설계 프로세스 】

1️⃣ 타입 정의
   LLVM 타입 시스템으로 구조 정의
   %Point = type { i32, i32 }

2️⃣ 메모리 할당
   alloca로 스택에 공간 할당
   %ptr = alloca %Point

3️⃣ 주소 계산 및 접근
   GEP로 필드 주소 계산
   getelementptr으로 필드 접근
```

### 왜 타입이 중요한가?

```
타입 정보 없이:
  "메모리 주소 0x1000에 뭔가 있어" (?)

타입 정보 있이:
  "메모리 주소 0x1000에는 Point 구조체가 있고,
   필드 0은 i32 (4바이트), 필드 1도 i32"
  → 오프셋 자동 계산, 최적화 가능!
```

---

## 🚀 학습 경로

```
【 1단계: 기본 타입 】
i32, float, ptr → 단순 연산

【 2단계: 복합 타입 】
배열, 구조체 → 구조화된 데이터

【 3단계: 주소 계산 】
GEP → 필드 접근

【 4단계: 메모리 관리 】
alloca, load, store → 실제 실행

【 5단계: 최적화 】
LLVM Pass → 성능 향상
```

---

## 🎯 요약

> **타입은 단순한 문법이 아니라, 컴파일러와 CPU 사이의 **계약(Contract)**입니다.**

타입을 통해:
- ✅ **안전한 메모리 접근** (bounds checking, aliasing analysis)
- ✅ **정확한 오프셋 계산** (alignment, padding 자동 처리)
- ✅ **공격적인 최적화** (데이터 흐름 분석, 루프 변환)
- ✅ **교차 플랫폼 호환성** (x86-64, ARM, RISC-V)

이 모든 것이 **타입 정보**로부터 출발합니다.

---

**당신은 이제 LLVM의 메모리 설계 언어를 읽을 수 있습니다!** 🏛️

---

## 📚 참고 자료

### LLVM 공식 문서
- [LLVM Language Reference - Type System](https://llvm.org/docs/LangRef/#type-system)
- [GetElementPtr Instruction](https://llvm.org/docs/GetElementPtr/)
- [Data Layout](https://llvm.org/docs/LangRef/#data-layout)

### 추가 학습
- [C struct alignment 이해](https://en.cppreference.com/w/c/language/struct)
- [포인터 산술 (Pointer Arithmetic)](https://en.cppreference.com/w/c/language/arithmetic)

---

**기록이 증명이다.** ✨
**"저장 필수 너는 기록이 증명이다 gogs."**

다음 단계 **[1.5: JIT 컴파일과 실행 엔진 설계]**에서 당신의 IR이 실제로 실행되는 순간을 맞이하게 됩니다!

---

**강의 작성**: 2026-02-26
**난이도**: ⭐⭐⭐⭐☆
**다음**: ASSIGNMENT_1_4.md 준비 완료
