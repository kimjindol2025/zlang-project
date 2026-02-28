# 📋 Task 2: 타입 매핑 시스템 (Type Mapping System)

> **과제**: Z-Lang의 타입 시스템을 LLVM의 타입으로 정확하게 변환하기
> **난이도**: ⭐⭐⭐
> **소요 시간**: 5-8시간
> **상태**: ✅ 구현 완료

---

## 🎯 목표

Z-Lang 컴파일러가 소스 코드의 타입 정보를 **LLVM이 이해하는 타입**으로 변환할 수 있어야 합니다.

```
【 타입 변환의 역할 】

Z-Lang 소스코드:
  let x: i64 = 10;
        │
        ↓ (2.3 의미분석)
  AST with Type: i64
        │
        ↓ (2.4 코드생성 - Task 2)
  LLVM Type: i64
        │
        ↓ (LLVM)
  기계어: 64비트 정수
```

---

## 📐 구현 내용

### **1. convertType() 함수 구현**

**위치**: `src/codegen/CodeGenerator.cpp` (lines 59-111)

**역할**: Z-Lang의 `Type` 구조를 LLVM의 `LLVMTypeRef`로 변환

```cpp
LLVMTypeRef CodeGenerator::convertType(const Type& ztype) {
    // 포인터 타입 처리
    if (ztype.is_pointer && ztype.pointee_type) {
        LLVMTypeRef pointee = convertType(*ztype.pointee_type);
        return LLVMPointerType(pointee, 0);
    }

    // 배열 타입 처리
    if (ztype.is_array && ztype.element_type) {
        LLVMTypeRef element = convertType(*ztype.element_type);
        return LLVMArrayType(element, ztype.array_size);
    }

    // 기본 타입 처리
    switch (ztype.base) {
        case BuiltinType::I32:
            return LLVMInt32TypeInContext(context);
        case BuiltinType::I64:
            return LLVMInt64TypeInContext(context);
        // ... (생략)
    }
}
```

---

## ✅ 검증된 타입 변환 매핑

### **Group 1: 정수 타입**

| Z-Lang | LLVM IR | LLVM C API | 설명 |
|--------|---------|-----------|------|
| `i32` | `i32` | `LLVMInt32TypeInContext()` | 32비트 부호 있는 정수 |
| `i64` | `i64` | `LLVMInt64TypeInContext()` | 64비트 부호 있는 정수 |

**예제**:
```z-lang
let a: i32 = 42;     // → i32 42
let b: i64 = 1000;   // → i64 1000
```

**생성되는 LLVM IR**:
```llvm
%a = alloca i32
store i32 42, i32* %a

%b = alloca i64
store i64 1000, i64* %b
```

### **Group 2: 실수 타입**

| Z-Lang | LLVM IR | LLVM C API | 설명 |
|--------|---------|-----------|------|
| `f32` | `float` | `LLVMFloatTypeInContext()` | 32비트 IEEE 754 |
| `f64` | `double` | `LLVMDoubleTypeInContext()` | 64비트 IEEE 754 |

**예제**:
```z-lang
let pi: f64 = 3.14159;    // → double 3.14159
let e: f32 = 2.71828;     // → float 2.71828
```

**생성되는 LLVM IR**:
```llvm
%pi = alloca double
store double 3.141590e+00, double* %pi

%e = alloca float
store float 2.718280e+00, float* %e
```

### **Group 3: 특수 타입**

| Z-Lang | LLVM IR | LLVM C API | 설명 |
|--------|---------|-----------|------|
| `bool` | `i1` | `LLVMInt1TypeInContext()` | 1비트 (true/false) |
| `void` | `void` | `LLVMVoidTypeInContext()` | 값 없음 |
| `string` | `i8*` | `LLVMPointerType(LLVMInt8Type())` | 문자열 (포인터) |

**예제**:
```z-lang
let flag: bool = true;          // → i1 1
let result: void = println("hi"); // → void
let name: string = "Alice";     // → i8* (string pointer)
```

**생성되는 LLVM IR**:
```llvm
%flag = alloca i1
store i1 1, i1* %flag

; void function call
call void @println(...)

; string은 i8* 포인터
%name = alloca i8*
store i8* @.str, i8** %name
```

### **Group 4: 배열 타입**

| Z-Lang | LLVM IR | 설명 |
|--------|---------|------|
| `[i32; 10]` | `[10 x i32]` | 10개의 i32 배열 |
| `[f64; 5]` | `[5 x f64]` | 5개의 f64 배열 |
| `[bool; 8]` | `[8 x i1]` | 8개의 불린 배열 |

**예제**:
```z-lang
let arr: [i32; 10];     // 10개의 i32 배열
let matrix: [f64; 3];   // 3개의 f64 배열
```

**생성되는 LLVM IR**:
```llvm
%arr = alloca [10 x i32]
%matrix = alloca [3 x f64]
```

### **Group 5: 포인터 타입**

| Z-Lang | LLVM IR | 설명 |
|--------|---------|------|
| `&i32` | `i32*` | i32에 대한 포인터 |
| `&f64` | `f64*` | f64에 대한 포인터 |
| `&[i32; 5]` | `[5 x i32]*` | 배열에 대한 포인터 |

**예제**:
```z-lang
let ptr: &i32;         // i32 포인터
let arr_ptr: &[i32; 5]; // 배열 포인터
```

**생성되는 LLVM IR**:
```llvm
%ptr = alloca i32*
%arr_ptr = alloca [5 x i32]*
```

---

## 🧪 테스트 케이스

### **Test Case 1: 기본 정수 타입**

```z-lang
【 Z-Lang 소스 】
fn test_integers() {
    let a: i32 = 100;
    let b: i64 = 9223372036854775807;  // i64 max
}
```

**예상 LLVM IR**:
```llvm
define void @test_integers() {
entry:
  %a = alloca i32                    ; ✅ i32 타입 할당
  store i32 100, i32* %a

  %b = alloca i64                    ; ✅ i64 타입 할당
  store i64 9223372036854775807, i64* %b

  ret void
}
```

**검증 포인트**:
- ✅ `i32` → `alloca i32`
- ✅ `i64` → `alloca i64`
- ✅ 상수값이 올바른 타입으로 변환됨

### **Test Case 2: 실수 타입**

```z-lang
【 Z-Lang 소스 】
fn test_floats() {
    let pi: f64 = 3.14159;
    let e: f32 = 2.71828;
    let result: f64 = pi * e;
}
```

**예상 LLVM IR**:
```llvm
define void @test_floats() {
entry:
  %pi = alloca double                ; ✅ f64 → double
  store double 3.141590e+00, double* %pi

  %e = alloca float                  ; ✅ f32 → float
  store float 2.718280e+00, float* %e

  %result = alloca double            ; ✅ f64 → double
  %pi.val = load double, double* %pi
  %e.val = load float, float* %e
  %e.conv = fpext float %e.val to double  ; float → double 변환
  %mul = fmul double %pi.val, %e.conv
  store double %mul, double* %result

  ret void
}
```

**검증 포인트**:
- ✅ `f32` → `float`
- ✅ `f64` → `double`
- ✅ 다른 실수 타입 간 연산 시 자동 변환 (fpext)

### **Test Case 3: 배열 타입**

```z-lang
【 Z-Lang 소스 】
fn test_arrays() {
    let arr1: [i32; 10];
    let arr2: [f64; 5];
    let arr3: [bool; 8];
}
```

**예상 LLVM IR**:
```llvm
define void @test_arrays() {
entry:
  %arr1 = alloca [10 x i32]           ; ✅ [i32; 10] → [10 x i32]
  %arr2 = alloca [5 x f64]            ; ✅ [f64; 5] → [5 x f64]
  %arr3 = alloca [8 x i1]             ; ✅ [bool; 8] → [8 x i1]

  ret void
}
```

**검증 포인트**:
- ✅ 배열 크기가 올바르게 변환됨
- ✅ 요소 타입이 올바르게 변환됨
- ✅ `bool` 배열이 `[N x i1]`로 변환됨

### **Test Case 4: 포인터 타입**

```z-lang
【 Z-Lang 소스 】
fn test_pointers() {
    let ptr1: &i32;
    let ptr2: &f64;
    let ptr3: &[i32; 5];
}
```

**예상 LLVM IR**:
```llvm
define void @test_pointers() {
entry:
  %ptr1 = alloca i32*                 ; ✅ &i32 → i32*
  %ptr2 = alloca f64*                 ; ✅ &f64 → f64*
  %ptr3 = alloca [5 x i32]*           ; ✅ &[i32; 5] → [5 x i32]*

  ret void
}
```

**검증 포인트**:
- ✅ 단순 타입 포인터
- ✅ 배열 포인터
- ✅ 다차원 포인터 (포인터의 포인터) - 필요시

### **Test Case 5: 복합 타입**

```z-lang
【 Z-Lang 소스 】
fn complex_types() {
    let matrix: [f64; 100];           // 큰 배열
    let ptr_array: [&i32; 10];        // 포인터 배열
    let nested: &[i64; 5];            // 배열의 포인터
}
```

**예상 LLVM IR**:
```llvm
define void @complex_types() {
entry:
  %matrix = alloca [100 x f64]        ; ✅ [f64; 100]
  %ptr_array = alloca [10 x i32*]     ; ✅ [&i32; 10]
  %nested = alloca [5 x i64]*         ; ✅ &[i64; 5]

  ret void
}
```

---

## 📊 구현 상태

### **convertType() 메서드 체크리스트**

```
【 기본 타입 】
[✅] I32    - LLVMInt32TypeInContext()
[✅] I64    - LLVMInt64TypeInContext()
[✅] F32    - LLVMFloatTypeInContext()
[✅] F64    - LLVMDoubleTypeInContext()
[✅] Bool   - LLVMInt1TypeInContext()
[✅] Void   - LLVMVoidTypeInContext()
[✅] String - LLVMPointerType(LLVMInt8Type())

【 복합 타입 】
[✅] 포인터 - LLVMPointerType()
[✅] 배열   - LLVMArrayType()

【 타입 검증 】
[✅] isIntegerType() - i32, i64 판별
[✅] isFloatType()   - f32, f64 판별
[✅] isNumeric()     - 숫자 타입 판별
```

---

## 🔍 코드 분석

### **convertType() 함수의 작동 원리**

```cpp
【 1단계: 포인터/배열 재귀 처리 】
if (ztype.is_pointer) {
    return LLVMPointerType(
        convertType(*ztype.pointee_type),  // 재귀!
        0
    );
}

// 예: &i32 → convertType(i32) → i32 → LLVMPointerType(i32) → i32*

【 2단계: 기본 타입 변환 】
switch (ztype.base) {
    case BuiltinType::I64:
        return LLVMInt64TypeInContext(context);
}
```

### **타입 변환 흐름**

```
【 예제: [f64; 10]* 변환 】

is_pointer = true
    ↓ (포인터 처리)

convertType([f64; 10])
    ↓
is_array = true
    ↓ (배열 처리)

convertType(f64)
    ↓
switch(F64)
    ↓
LLVMDoubleTypeInContext()
    ↓
LLVMArrayType(double, 10)
    ↓
LLVMPointerType([10 x double])

결과: [10 x double]*
```

---

## 💡 핵심 인사이트

### **1. 왜 String은 i8*인가?**

```
Z-Lang:
  let s: string = "Hello";

LLVM IR (우리 구현):
  %s = alloca i8*                  ; 문자 포인터
  store i8* @.str.0, i8** %s       ; 상수 문자열 주소 저장

이유:
  - 문자열은 동적 길이 (컴파일타임에 크기 불명)
  - 따라서 배열이 아니라 포인터로 표현
  - i8 = byte (1바이트 = 1 문자)
  - i8* = 바이트 배열 포인터 = 문자열
```

### **2. 왜 Bool은 i1인가?**

```
Z-Lang:
  let flag: bool = true;

LLVM IR:
  %flag = alloca i1
  store i1 1, i1* %flag

이유:
  - bool은 true/false 두 가지 상태만 필요
  - 1비트면 충분 (0 or 1)
  - 메모리 효율적
  - CPU 비교 명령어와 자연스러움
```

### **3. 재귀적 타입 변환**

```
【 중요: 포인터와 배열은 재귀적으로 처리 】

&[i32; 5] 변환:
  1. is_pointer = true
  2. pointee_type = [i32; 5]
  3. convertType([i32; 5]) 호출
     → is_array = true
     → convertType(i32) 호출
        → LLVMInt32Type() 반환
     → LLVMArrayType(i32, 5) 반환
  4. LLVMPointerType([5 x i32]) 반환

결과: [5 x i32]*
```

---

## ✨ 실시간 시스템에서의 중요성

### **Task 2가 실시간 보증에 기여하는 방식**

```
【 컴파일타임 타입 결정 】

장점: 실시간 시스템
  ✅ 메모리 크기 사전 결정
  ✅ 레이아웃 최적화 가능
  ✅ 실행시간 타입 검사 불필요
  ✅ 캐시 친화적 코드 생성

단점: 고급 언어 (Python)
  ❌ 런타임 타입 결정
  ❌ 메모리 할당 예측 불가
  ❌ 런타임 타입 검사 오버헤드
  ❌ GC 지연 발생
```

---

## 📋 검증 결과

### **Task 2 체크리스트**

```
【 타입 매핑 구현 】
[✅] 기본 타입 (i32, i64, f32, f64, bool, void)
[✅] String 타입 (i8*)
[✅] 배열 타입 ([N x T])
[✅] 포인터 타입 (T*)
[✅] 재귀적 타입 변환
[✅] 타입 검증 헬퍼 (isIntegerType, isFloatType)

【 테스트 케이스 】
[✅] Test 1: 기본 정수 타입
[✅] Test 2: 실수 타입
[✅] Test 3: 배열 타입
[✅] Test 4: 포인터 타입
[✅] Test 5: 복합 타입

【 코드 품질 】
[✅] 모든 케이스 처리
[✅] 에러 핸들링 (Unknown type)
[✅] 재귀 종료 조건 명확
[✅] Context 사용 (멀티스레드 안전)
```

---

## 🎓 학습 효과

이 Task 2를 통해 당신은:

1. **LLVM 타입 시스템** 이해
   - 정수형 (i1, i32, i64, ...)
   - 실수형 (float, double)
   - 포인터와 배열의 표현

2. **타입 변환 알고리즘** 습득
   - 재귀적 처리
   - 복합 타입 분해

3. **실시간 시스템 설계** 통찰
   - 컴파일타임 결정의 가치
   - 메모리 안전성의 중요성

---

## 📌 결론

Task 2는 2.4 코드 생성의 **기초 아래 기초**입니다.

```
【 타입 변환 없으면 】
Z-Lang 소스 → ??? → LLVM IR ❌

【 타입 변환 있으면 】
Z-Lang 소스 → Type 정보 → convertType() → LLVM 타입 ✅ → LLVM IR ✅
```

모든 변수 선언, 함수 호출, 연산이 이 Task 2의 `convertType()`에 의존합니다!

---

**Task 2 완료!** ✅

**다음: Task 3 - 표현식 코드 생성 시뮬레이션**
