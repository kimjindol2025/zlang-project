# 🎯 Preview: LLVM 1.4 - 타입 시스템과 구조체

> **최종 업데이트**: 2026-02-26
> **상태**: 🔜 준비 중 (다음 주 공개)

---

## 📖 학습 경로

```
【 LLVM 전설 학습 경로 】

1.1 ✅ LLVM 아키텍처와 IR
     └─ "컴파일러는 무엇인가?"

1.2 ✅ LLVM C API 코드 생성
     └─ "단순한 함수를 만들어보자"

1.3 ✅ 제어 흐름 (Control Flow)
     └─ "프로그램이 판단하고 반복한다"

1.4 🔜 타입 시스템과 구조체 (Type Systems & Structs)
     └─ "데이터를 체계적으로 조직하자"

1.5 🔜 함수 호출과 메모리 (Function Calls & Memory)
     └─ "프로그램이 호출 스택으로 동작한다"

1.6 🔜 배열과 포인터 (Arrays & Pointers)
     └─ "메모리를 직접 제어하자"

1.7 🔜 실제 프로그래밍 (Real Programming)
     └─ "Z-Lang 컴파일러 프로토타입"

1.8 🔜 최적화와 백엔드 (Optimization & Backend)
     └─ "기계어가 되기까지"
```

---

## 🎓 1.4 단계의 핵심 개념

### 타입이란?

```zig
// Z-Lang 소스 코드
fn process_point(x: i32, y: i32, name: str) -> Point {
    return Point { x: x, y: y, name: name };
}

type Point = {
    x: i32,
    y: i32,
    name: str
}
```

**Q: 이것이 LLVM IR에서 어떻게 표현될까?**

```llvm
; LLVM의 타입 정의
%Point = type { i32, i32, i8* }

; 함수 구현
define %Point @process_point(i32 %x, i32 %y, i8* %name) {
  %point = alloca %Point              ; 스택에 메모리 할당
  %x_ptr = getelementptr %Point, %Point* %point, i32 0, i32 0
  store i32 %x, i32* %x_ptr           ; x 필드 할당

  %y_ptr = getelementptr %Point, %Point* %point, i32 0, i32 1
  store i32 %y, i32* %y_ptr           ; y 필드 할당

  %name_ptr = getelementptr %Point, %Point* %point, i32 0, i32 2
  store i8* %name, i8** %name_ptr     ; name 필드 할당

  %loaded_point = load %Point, %Point* %point
  ret %Point %loaded_point
}
```

---

### 1.4에서 배울 것들

#### 📌 LLVM 타입 시스템

```c
// 기본 타입
LLVMTypeRef i32_type = LLVMInt32Type();
LLVMTypeRef f64_type = LLVMDoubleType();
LLVMTypeRef void_type = LLVMVoidType();

// 복합 타입
LLVMTypeRef point_type = LLVMStructType(
    &(LLVMTypeRef[]){ i32_type, i32_type },
    2,
    0  // packed = false
);

// 배열 타입
LLVMTypeRef array_type = LLVMArrayType(i32_type, 10);

// 포인터 타입
LLVMTypeRef ptr_type = LLVMPointerType(i32_type, 0);
```

#### 📌 메모리 레이아웃

```
【 메모리 레이아웃의 중요성 】

type Point = { x: i32, y: i32 }

┌─────────────┬─────────────┐
│  x (i32)    │  y (i32)    │
│  4 바이트   │  4 바이트   │
└─────────────┴─────────────┘
0            4              8

GEP(ptr, 0, 0) → x의 주소 (offset 0)
GEP(ptr, 0, 1) → y의 주소 (offset 4)
```

#### 📌 GEP(GetElementPointer)

GEP는 구조체와 배열의 필드/요소에 접근하는 **가장 중요한 명령어**입니다:

```llvm
; 구조체 필드 접근
%x_ptr = getelementptr %Point, %Point* %point, i32 0, i32 0
; %Point 구조체에서 0번째 필드(x) 주소 계산

; 배열 요소 접근
%arr_element = getelementptr [10 x i32], [10 x i32]* %arr, i32 0, i32 5
; 배열의 5번째 요소(인덱스 5) 주소 계산
```

---

## 🎨 1.4 예제: Person 구조체

### 소스 코드 (Z-Lang)

```zig
fn create_person(age: i32, name: str) -> Person {
    return Person { age: age, name: name };
}

fn get_person_age(p: Person) -> i32 {
    return p.age;
}

fn increment_age(p: Person) -> Person {
    return Person { age: p.age + 1, name: p.name };
}

type Person = {
    age: i32,
    name: str
}
```

### LLVM IR (생성될 예상 결과)

```llvm
; 타입 정의
%Person = type { i32, i8* }

; create_person 함수
define %Person @create_person(i32 %age, i8* %name) {
entry:
  %person = alloca %Person

  ; age 필드 설정
  %age_ptr = getelementptr %Person, %Person* %person, i32 0, i32 0
  store i32 %age, i32* %age_ptr

  ; name 필드 설정
  %name_ptr = getelementptr %Person, %Person* %person, i32 0, i32 1
  store i8* %name, i8** %name_ptr

  %result = load %Person, %Person* %person
  ret %Person %result
}

; get_person_age 함수
define i32 @get_person_age(%Person %p) {
entry:
  %p_ptr = alloca %Person
  store %Person %p, %Person* %p_ptr

  ; age 필드 읽기
  %age_ptr = getelementptr %Person, %Person* %p_ptr, i32 0, i32 0
  %age = load i32, i32* %age_ptr
  ret i32 %age
}

; increment_age 함수
define %Person @increment_age(%Person %p) {
entry:
  %p_ptr = alloca %Person
  store %Person %p, %Person* %p_ptr

  ; 새 Person 생성
  %new_person = alloca %Person

  ; age 필드: p.age + 1
  %age_ptr = getelementptr %Person, %Person* %p_ptr, i32 0, i32 0
  %age = load i32, i32* %age_ptr
  %new_age = add i32 %age, 1

  %new_age_ptr = getelementptr %Person, %Person* %new_person, i32 0, i32 0
  store i32 %new_age, i32* %new_age_ptr

  ; name 필드: p.name 복사
  %name_ptr = getelementptr %Person, %Person* %p_ptr, i32 0, i32 1
  %name = load i8*, i8** %name_ptr

  %new_name_ptr = getelementptr %Person, %Person* %new_person, i32 0, i32 1
  store i8* %name, i8** %new_name_ptr

  %result = load %Person, %Person* %new_person
  ret %Person %result
}
```

---

## 🔧 Task 미리보기

### Task 1: 기본 구조체 정의 및 생성

```zig
fn main() {
    // Person 타입 정의
    // - 필드 1: age (i32)
    // - 필드 2: name (i8* / string)

    // create_person(30, "Alice") 함수 구현
    // - 스택에 Person 메모리 할당 (alloca)
    // - GEP로 각 필드 주소 계산
    // - store로 값 저장
    // - load로 결과 반환
}
```

### Task 2: 구조체 필드 접근

```zig
fn main() {
    // get_age(person) 함수 구현
    // - 매개변수로 받은 Person에서
    // - GEP로 age 필드 주소 계산
    // - load로 값 읽기
    // - 반환
}
```

### Task 3: 배열 타입 (선택)

```llvm
; 배열 정의
%IntArray = type [5 x i32]

; 배열 접근
%arr = alloca %IntArray
%elem_ptr = getelementptr [5 x i32], [5 x i32]* %arr, i32 0, i32 3
store i32 42, i32* %elem_ptr
```

### Task 4: 포인터와 메모리 (선택)

```llvm
; 포인터 생성 및 역참조
%ptr_type = type i32*
%ptr = alloca i32*
store i32* %value, i32** %ptr
```

---

## 📊 1.4의 중요 함수들

### LLVM C API

```c
// 타입 생성
LLVMTypeRef LLVMStructType(LLVMTypeRef *ElementTypes,
                           unsigned ElementCount,
                           LLVMBool Packed);

LLVMTypeRef LLVMArrayType(LLVMTypeRef ElementType,
                          unsigned ElementCount);

LLVMTypeRef LLVMPointerType(LLVMTypeRef ElementType,
                            unsigned AddressSpace);

// 메모리 할당
LLVMValueRef LLVMBuildAlloca(LLVMBuilderRef B,
                             LLVMTypeRef Ty,
                             const char *Name);

// GEP: 필드/요소 주소 계산
LLVMValueRef LLVMBuildGEP(LLVMBuilderRef B,
                          LLVMValueRef Pointer,
                          LLVMValueRef *Indices,
                          unsigned NumIndices,
                          const char *Name);

// 메모리 읽기/쓰기
LLVMValueRef LLVMBuildLoad(LLVMBuilderRef B,
                           LLVMValueRef PointerVal,
                           const char *Name);

LLVMValueRef LLVMBuildStore(LLVMBuilderRef B,
                            LLVMValueRef Val,
                            LLVMValueRef Ptr);
```

---

## 🎓 왜 1.4가 중요한가?

### 1. 실제 프로그래밍의 시작

```
1.1-1.3: 기본 문법 (변수, 함수, 제어흐름)
1.4:     데이터 구조 (구조체, 배열) ← 여기서부터 진짜 시작!
```

### 2. 메모리 모델 이해

```
프로그래머 관점:  type Point = { x: i32, y: i32 }
                  ↓
컴파일러 관점:    %Point = type { i32, i32 }
                  ↓
LLVM 관점:        메모리 레이아웃, GEP, load/store
                  ↓
기계어 관점:      mov, lea, add 등의 어셈블리
```

### 3. 최적화의 핵심

GEP와 메모리 연산이 LLVM 최적화의 **가장 중요한 대상**입니다:

```llvm
// Before 최적화
%ptr1 = getelementptr %Point, %Point* %p, i32 0, i32 0
%val1 = load i32, i32* %ptr1
%ptr2 = getelementptr %Point, %Point* %p, i32 0, i32 0
%val2 = load i32, i32* %ptr2  ; 같은 주소에서 또 읽음!

// After 최적화
%ptr1 = getelementptr %Point, %Point* %p, i32 0, i32 0
%val1 = load i32, i32* %ptr1
; %val2 = %val1로 대체 (중복 제거)
```

---

## 📅 1.4 예정 일정

| 항목 | 예정 |
|------|------|
| 강의 자료 | 2026-03-02 (토) |
| 예제 코드 | 2026-03-02 (토) |
| 과제 공개 | 2026-03-02 (토) |
| 해답 공개 | 2026-03-09 (토) |

---

## 🔗 관련 자료

### LLVM 공식 문서
- [LLVM Language Reference Manual](https://llvm.org/docs/LangRef/)
- [LLVM C API Documentation](https://llvm.org/doxygen/group__LLVMC.html)

### 메모리 레이아웃
- [Data Layout - LLVM](https://llvm.org/docs/LangRef/#data-layout)
- [GEP Semantics](https://llvm.org/docs/GetElementPtr/)

---

## 💡 미리 생각해볼 질문

### Q1: 구조체와 배열의 차이는?

```zig
// 구조체: 서로 다른 타입의 필드들
type Person = { age: i32, name: str }

// 배열: 같은 타입의 연속된 요소들
type IntList = [10]i32
```

### Q2: 왜 GEP가 필요한가?

메모리 주소 계산이 복잡해서입니다:

```c
// C 코드
struct Point { int x; int y; };
Point p;
int age = p.x;  // 간단해 보이지만...

// LLVM:
// 1. p의 메모리 주소 찾기
// 2. 0번째 필드(x)의 오프셋 계산 (0 바이트)
// 3. 그 주소에서 값 읽기
```

GEP가 이 모든 것을 처리합니다!

### Q3: alloca와 malloc의 차이는?

```llvm
; alloca: 스택 메모리 (함수 종료시 자동 해제)
%ptr = alloca i32
; 빠르지만, 함수를 벗어날 수 없음

; malloc: 힙 메모리 (수동 해제 필요)
%ptr = call i8* @malloc(i64 4)
; 느리지만, 프로그램 끝까지 유지 가능
```

---

## 🎯 1.4 완료 후 목표

```
【 1.4 완료 후 할 수 있는 것 】

✅ LLVM 타입 시스템 완전 이해
✅ 구조체 정의 및 생성
✅ GEP로 필드/배열 요소 접근
✅ 메모리 레이아웃 계산
✅ 포인터와 참조 관리
✅ 배열 처리

【 다음 1.5의 준비 】

✅ 함수 호출 규약 (Calling Convention)
✅ 콜 스택 (Call Stack)
✅ 반환값과 인자 전달
✅ 중첩된 함수 호출
```

---

## 📢 예고

> **LLVM 전설의 새 장이 곧 열립니다!**

1.4에서는 단순한 함수를 벗어나
**실제 프로그래밍에 필요한 데이터 구조**를 다룹니다.

구조체와 배열, 메모리 관리를 마스터하면
당신은 더 이상 **학생 프로그래머**가 아닙니다.

**"전설이 되는 여정"은 계속됩니다...** 🏆

---

**예상 공개**: 2026-03-02 (토)
**난이도**: ⭐⭐⭐⭐☆ (고급)
**기대도**: ⭐⭐⭐⭐⭐ (필수)

이 미리보기가 도움이 되었기를 바랍니다! 🎉
