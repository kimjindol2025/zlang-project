# 🏛️ LLVM 전설: 1.3 제어 흐름 - If-Else와 Loop 설계

## 📖 강의 개요

**단순한 연산 기능을 넘어, 프로그램에 '판단'과 '반복'을 부여할 시간입니다.**

LLVM에서 제어 흐름을 설계하는 것은 마치 **철도의 선로를 깔고 스위치를 배치하는 것과 같습니다.**

```
Simple Operations (1.1, 1.2):     Control Flow (1.3):
  add, mul, sub                      if, while, for
     ↓                                   ↓
  단순 계산                          프로그램의 '뇌'

  A + B = C                        if (x > 0) { ... }
                                   while (i < n) { ... }
```

---

## 1️⃣ If-Else 구조의 설계도

### 🗺️ 기본 구조

LLVM IR에서 **모든 제어 흐름은 Basic Block(기본 블록) 사이의 점프(Branch)** 로 구현됩니다.

C나 Zig의 `if` 문은 LLVM 수준으로 내려오면 여러 개의 블록과 조건부 분기 명령어로 분해됩니다.

### 📐 If-Else를 위한 4개 블록

```
【 If-Else 제어 흐름 】

    ┌─────────────────┐
    │  Entry Block    │
    │ (조건 검사)      │
    │ cond = x > 0    │
    └────────┬────────┘
             │
        CondBr (분기)
             │
       ┌─────┴──────┐
       │            │
       ↓            ↓
  ┌─────────┐  ┌─────────┐
  │Then Blk │  │Else Blk │
  │(참일때) │  │(거짓일때)│
  │         │  │         │
  │%v1 = x  │  │%v2 = -x │
  │Br Merge │  │Br Merge │
  └────┬────┘  └────┬────┘
       │            │
       └─────┬──────┘
             ↓
        ┌──────────────┐
        │ Merge Block  │
        │ (합류점)      │
        │%r = phi...   │
        │ret %r        │
        └──────────────┘
```

### 🔧 4개 블록의 역할

#### **1. Entry Block (조건 검사)**

```c
// Zig: if (x > 0) { ... } else { ... }

// LLVM IR:
entry:
  %cond = icmp sgt i32 %x, 0        // 조건: x > 0?
  br i1 %cond, label %then, label %else
```

**역할**:
- 조건을 검사 (ICmp 명령어)
- 결과에 따라 Then 또는 Else로 분기

#### **2. Then Block (조건 참일 때)**

```llvm
then:
  %v1 = sub i32 0, %x               // 0 - x (음수화)
  ; 또는 그냥 값 사용
  br label %merge
```

**특징**:
- 조건이 참일 때만 실행
- 마지막은 Merge로 분기 (반드시 Terminator 필요)

#### **3. Else Block (조건 거짓일 때)**

```llvm
else:
  %v2 = sub i32 0, %x
  br label %merge
```

**특징**:
- 조건이 거짓일 때만 실행
- 마찬가지로 Merge로 분기

#### **4. Merge Block (합류점)**

```llvm
merge:
  %result = phi i32 [ %v1, %then ], [ %v2, %else ]
  ret i32 %result
```

**핵심**:
- Then과 Else가 만나는 지점
- **PHI 노드** 를 사용하여 두 경로의 값 통합

---

## 2️⃣ PHI 노드 (PHI Node): SSA의 구원자

### 🎯 PHI 노드가 필요한 이유

#### **SSA의 딜레마**

```c
int abs(int x) {
    if (x < 0) {
        x = -x;  // x 재할당!
    }
    return x;
}
```

**SSA 규칙**: 모든 변수는 **단 한 번만 할당**

하지만 위 코드에서:
- `x`가 두 번 할당됨 (Then과 Else에서 다른 값)
- SSA를 어김!

### 🔑 PHI 노드의 역할

```llvm
merge:
  %result = phi i32 [ 100, %then ], [ 200, %else ]
  ret i32 %result
```

**의미**:
```
"어느 블록에서 왔느냐에 따라 %result에 다른 값을 대입"

- %then에서 오면: %result = 100
- %else에서 오면: %result = 200
```

**이것이 SSA를 지키면서도 조건에 따른 값 선택을 가능하게 함!**

### 📊 PHI 노드의 구조

```
phi i32 [ value1, block1 ], [ value2, block2 ], ...
        └────┬────┘
             │
        각 경로에서의 값과 오는 블록 명시

예시:
  phi i32 [ 100, %then ], [ 200, %else ]
  └─ Then에서 오면 100
  └─ Else에서 오면 200
```

### 💻 LLVM C API로 PHI 노드 생성

```zig
// PHI 노드 생성
const phi = c.LLVMBuildPhi(builder, i32_type, "result");

// 각 경로의 값과 블록을 추가
c.LLVMAddIncoming(phi, &[_]c.LLVMValueRef{val_then}, &[_]c.LLVMBasicBlockRef{then_block}, 1);
c.LLVMAddIncoming(phi, &[_]c.LLVMValueRef{val_else}, &[_]c.LLVMBasicBlockRef{else_block}, 1);

// 이제 %result로 사용 가능
_ = c.LLVMBuildRet(builder, phi);
```

### 🎭 PHI 노드의 예제

#### **Zig 코드**

```zig
fn abs(x: i32) -> i32 {
    if (x < 0) {
        return -x;
    } else {
        return x;
    }
}
```

#### **LLVM IR (PHI 없는 버전 - 나쁜 예)**

```llvm
define i32 @abs(i32 %x) {
entry:
  %cond = icmp slt i32 %x, 0
  br i1 %cond, label %then, label %else

then:
  %neg = sub i32 0, %x
  ret i32 %neg         ; 여기서 직접 반환!

else:
  ret i32 %x           ; 여기서 직접 반환!
}
```

#### **LLVM IR (PHI 사용 - 좋은 예)**

```llvm
define i32 @abs(i32 %x) {
entry:
  %cond = icmp slt i32 %x, 0
  br i1 %cond, label %then, label %else

then:
  %neg = sub i32 0, %x
  br label %merge      ; Merge로 분기

else:
  br label %merge      ; Merge로 분기

merge:
  %result = phi i32 [ %neg, %then ], [ %x, %else ]
  ret i32 %result      ; 통일된 반환
}
```

**왜 두 번째가 나을까?**
- 하나의 반환 지점 (유지보수 용이)
- SSA 규칙 준수
- 최적화에 유리

---

## 3️⃣ Loop(반복문)의 아키텍처

### 🔄 루프의 5가지 요소

LLVM에서 루프는 자기 자신에게로 되돌아가는 분기를 가진 구조입니다.

```
【 Loop의 5가지 요소 】

1. Loop Preheader (루프 전)
   └─ 초기화 (i = 0)

2. Loop Header (루프 헤더)
   └─ 조건 검사 (i < 10)
   └─ Latch에서 오는 값 (PHI!)

3. Loop Body (루프 본체)
   └─ 반복할 로직

4. Loop Latch (루프 끝)
   └─ 증가 (i++)
   └─ Header로 다시 분기

5. Loop Exit (루프 탈출)
   └─ 루프 종료 후 지점
```

### 📐 Loop 구조도

```
Preheader:
  %i = 0
  br label %header

Header:
  %i.phi = phi i32 [ 0, %preheader ], [ %i.next, %latch ]
  %cond = icmp slt i32 %i.phi, 10
  br i1 %cond, label %body, label %exit

Body:
  ; 루프 로직
  br label %latch

Latch:
  %i.next = add i32 %i.phi, 1
  br label %header

Exit:
  ; 루프 후 로직
  ...
```

### 🔑 핵심: PHI in Loop

```llvm
header:
  %i = phi i32 [ 0, %preheader ], [ %i.next, %latch ]
  ;            └─ 첫 진입: 0
  ;                          └─ 루프 재진입: %i.next
```

**루프의 마법:**
```
1회차: i = 0 (preheader에서)
2회차: i = 1 (latch에서 계산한 %i.next)
3회차: i = 2 (latch에서 계산한 %i.next)
...
```

### 💻 LLVM C API로 루프 생성

```zig
// Preheader
const preheader = c.LLVMAppendBasicBlockInContext(context, func, "preheader");
c.LLVMPositionBuilderAtEnd(builder, preheader);
const i_init = c.LLVMConstInt(i32_type, 0, 0);
_ = c.LLVMBuildBr(builder, header);

// Header
const header = c.LLVMAppendBasicBlockInContext(context, func, "header");
c.LLVMPositionBuilderAtEnd(builder, header);

// PHI 노드: i의 값
const i_phi = c.LLVMBuildPhi(builder, i32_type, "i");

// 조건 검사
const const_10 = c.LLVMConstInt(i32_type, 10, 0);
const cond = c.LLVMBuildICmp(builder, c.LLVMIntSLT, i_phi, const_10, "cond");
_ = c.LLVMBuildCondBr(builder, cond, body, exit);

// Body
const body = c.LLVMAppendBasicBlockInContext(context, func, "body");
c.LLVMPositionBuilderAtEnd(builder, body);
// ... 루프 로직 ...
_ = c.LLVMBuildBr(builder, latch);

// Latch
const latch = c.LLVMAppendBasicBlockInContext(context, func, "latch");
c.LLVMPositionBuilderAtEnd(builder, latch);
const i_next = c.LLVMBuildAdd(builder, i_phi, c.LLVMConstInt(i32_type, 1, 0), "i.next");
_ = c.LLVMBuildBr(builder, header);

// PHI에 값 추가
c.LLVMAddIncoming(i_phi, &[_]c.LLVMValueRef{i_init}, &[_]c.LLVMBasicBlockRef{preheader}, 1);
c.LLVMAddIncoming(i_phi, &[_]c.LLVMValueRef{i_next}, &[_]c.LLVMBasicBlockRef{latch}, 1);

// Exit
const exit = c.LLVMAppendBasicBlockInContext(context, func, "exit");
c.LLVMPositionBuilderAtEnd(builder, exit);
_ = c.LLVMBuildRet(builder, c.LLVMConstInt(i32_type, 0, 0));
```

---

## 🔬 전공 심화: 최적화 관점의 제어 흐름

### 🎯 LLVM 최적화 도구 (Pass Manager)

LLVM의 최적화 도구(Pass Manager)는 우리가 만든 블록들을 분석하여 자동으로 개선합니다.

### 1️⃣ **Branch Folding (분기 최소화)**

```llvm
// 최적화 전
entry:
  br i1 true, label %a, label %b

a:
  ret i32 100

b:
  ret i32 200
```

```llvm
// 최적화 후 (분기 없음!)
entry:
  ret i32 100  ; 항상 true이므로 a로 직진
```

### 2️⃣ **Loop Unrolling (루프 펼침)**

```llvm
// 최적화 전
header:
  %i = phi i32 [0, ...], [%i.next, %latch]
  %cond = icmp slt i32 %i, 4
  br i1 %cond, label %body, label %exit

body:
  call @do_work()
  br label %latch

latch:
  %i.next = add i32 %i, 1
  br label %header
```

```llvm
// 최적화 후 (루프 펼침)
body:
  call @do_work()  ; i=0
  call @do_work()  ; i=1
  call @do_work()  ; i=2
  call @do_work()  ; i=3
  br label %exit   ; 분기 오버헤드 제거!
```

### 3️⃣ **Dead Code Elimination (죽은 코드 제거)**

```llvm
// 최적화 전
entry:
  br i1 false, label %unreachable, label %continue

unreachable:
  ret i32 999  ; 절대 실행되지 않음!

continue:
  ret i32 0
```

```llvm
// 최적화 후
entry:
  ret i32 0  ; unreachable 블록 삭제됨
```

### 📊 최적화의 효과

```
루프 반복 1000번:

최적화 전:
  ├─ 분기 1000번 (Header에서)
  ├─ 비교 1000번 (Latch에서)
  └─ 메모리 접근 효율 낮음

최적화 후 (unrolling):
  ├─ 분기 4번 (unroll factor = 4)
  ├─ 비교 4번
  └─ CPU 캐시 활용 향상

결과: 3배 이상 빠를 수 있음!
```

---

## 📦 실전 API 활용

### 🔑 핵심 함수들

#### **1. LLVMBuildCondBr (조건부 분기)**

```zig
// if (cond) { goto then_block } else { goto else_block }
_ = c.LLVMBuildCondBr(
    builder,
    cond,           // 조건 (i1 타입)
    then_block,     // 참일 때
    else_block      // 거짓일 때
);
```

#### **2. LLVMBuildBr (무조건 분기)**

```zig
// goto target_block
_ = c.LLVMBuildBr(builder, target_block);
```

#### **3. LLVMBuildPhi (PHI 노드)**

```zig
// %result = phi i32 [ ... ]
const phi = c.LLVMBuildPhi(builder, i32_type, "result");

// 경로 1
c.LLVMAddIncoming(
    phi,
    &[_]c.LLVMValueRef{val1},
    &[_]c.LLVMBasicBlockRef{block1},
    1
);

// 경로 2
c.LLVMAddIncoming(
    phi,
    &[_]c.LLVMValueRef{val2},
    &[_]c.LLVMBasicBlockRef{block2},
    1
);
```

#### **4. LLVMBuildICmp (정수 비교)**

```zig
const cond = c.LLVMBuildICmp(
    builder,
    c.LLVMIntSLT,    // 비교 종류: <
    lhs,             // 왼쪽
    rhs,             // 오른쪽
    "cond"           // 결과 이름
);

// 비교 종류:
// LLVMIntEQ   (==)
// LLVMIntNE   (!=)
// LLVMIntSLT  (<)   - Signed Less Than
// LLVMIntSGT  (>)   - Signed Greater Than
// LLVMIntSLE  (<=)
// LLVMIntSGE  (>=)
```

---

## 📝 Assignment 1.3: 로직 설계 과제

### 🎯 목표

LLVM의 제어 흐름을 이용하여 **판단과 반복이 있는 실제 함수** 구현하기

### 📋 Task 1: 절댓값 함수 (If-Else)

#### 목표 코드

```zig
fn abs(x: i32) -> i32 {
    if (x < 0) {
        return -x;
    } else {
        return x;
    }
}
```

#### 요구사항

1. **Entry 블록**: 조건 검사 (x < 0)
2. **Then 블록**: x가 음수일 때 (-x)
3. **Else 블록**: x가 양수일 때 (x 그대로)
4. **Merge 블록**: 두 경로의 값을 PHI로 통합
5. 검증 및 IR 출력

#### 예상 IR

```llvm
define i32 @abs(i32 %x) {
entry:
  %cond = icmp slt i32 %x, 0
  br i1 %cond, label %then, label %else

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

### 📋 Task 2: PHI 노드 분석

#### 생성된 IR에서 PHI 노드 추출

```markdown
## PHI 노드 분석

### 생성된 PHI 문장

```llvm
[생성된 PHI 문장]
```

### PHI 노드의 구조

- [ ] 타입: i32
- [ ] 변수: %result
- [ ] Then 경로: %neg from %then
- [ ] Else 경로: %x from %else

### SSA와의 관계

PHI 노드 없이는 왜 불가능한가?

[답변...]
```

### 📋 Task 3: 간단한 루프 (선택)

#### 목표 코드

```zig
fn sum_to_n(n: i32) -> i32 {
    let mut sum: i32 = 0;
    let mut i: i32 = 0;
    while i < n {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
```

#### 루프 구조

```
Preheader:  sum = 0, i = 0
Header:     PHI node, 조건 검사
Body:       sum += i
Latch:      i++
Exit:       반환
```

### 📋 Task 4: SSA와 PHI 노드 분석

#### 핵심 질문

```markdown
# SSA와 PHI 노드

## 1. SSA(Static Single Assignment)란?

[정의...]

## 2. SSA의 제약

- 모든 변수는 정확히 한 번만 할당된다
- 그런데 if 문에서는?

## 3. PHI 노드의 필요성

if (condition) {
    x = value1;
} else {
    x = value2;
}
return x;

위 코드에서 x가 2번 할당되는데, SSA는 어떻게 처리할까?

[답변...]

## 4. PHI는 어떻게 SSA를 지킬까?

%result = phi i32 [ %v1, %then ], [ %v2, %else ]

- %result는 단 한 번만 할당된다
- 하지만 어느 블록에서 오느냐에 따라 값이 달라진다
- 이것이 SSA와 조건 처리의 조화!

## 5. 자유로운 분석

[당신의 깊이 있는 분석...]
```

---

## 💡 심화 과제 (도전)

### Challenge 1: 최댓값 함수

```zig
fn max(a: i32, b: i32) -> i32 {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}
```

### Challenge 2: 팩토리얼 (루프)

```zig
fn factorial(n: i32) -> i32 {
    let mut result: i32 = 1;
    let mut i: i32 = 2;
    while i <= n {
        result = result * i;
        i = i + 1;
    }
    return result;
}
```

### Challenge 3: 무한 루프 감지

```zig
// 의도적으로 루프 탈출 조건이 없는 코드
fn infinite_loop() -> i32 {
    let mut i: i32 = 0;
    while true {  // 항상 참!
        i = i + 1;
    }
    // 절대 도달 불가
}
```

**관찰:**
- LLVM이 이를 어떻게 처리하는가?
- JIT로 실행하면 무한 루프에 빠지는가?
- 최적화 후 어떤 IR이 생성되는가?

---

## 🎯 체크리스트

```
【 1.3 완료 체크리스트 】

개념 이해:
□ If-Else의 4가지 블록 이해했는가?
□ PHI 노드가 무엇인지 설명할 수 있는가?
□ SSA와 PHI의 관계를 이해했는가?
□ 루프의 5가지 요소를 말할 수 있는가?

구현:
□ 절댓값 함수 생성했는가?
□ Entry/Then/Else/Merge 블록 생성했는가?
□ PHI 노드 구현했는가?
□ 생성된 IR 검증했는가?

분석:
□ PHI 노드 분석했는가?
□ abs 함수의 IR 이해했는가?
□ SSA와 PHI 관계 정리했는가?
□ 최적화 관점 이해했는가?

위 모두 체크되면 1.4로 진행 가능!
```

---

## 🚀 다음 단계: 1.4 예고

### 📌 **[1.4: 타입 시스템과 복합 구조체(Struct) 설계]**

```
【 다음에 배울 내용 】

현재까지: 기본 타입 (i32, f64, ...) 사용

다음: 메모리 직접 관리

【 구조체 설계 】
├─ Type 정의: 구조체의 멤버와 배치
├─ 메모리 레이아웃: 각 필드의 오프셋
├─ 포인터 연산: 필드 접근
└─ 예제: Person, Point, Node 구조체

【 배열과 포인터 】
├─ 배열 타입: [i32; 10]
├─ 동적 할당: malloc/free 이용
├─ 포인터 연산
└─ 예제: 배열 합계, 행렬 연산

【 메모리 최적화 】
├─ 패딩(Padding) 이해
├─ 정렬(Alignment) 고려
├─ 바이너리 호환성
└─ C 구조체와의 상호운용성
```

---

## 📚 학습 요약

```
【 1.3의 핵심 】

1. 제어 흐름 = Block 사이의 분기
2. If-Else = 4개 블록 + 조건 분기
3. PHI 노드 = SSA의 해법
4. 루프 = 자기 자신으로 돌아오는 구조
5. 최적화 = 불필요한 분기 제거/루프 펼침

【 다음을 할 수 있게 됨 】

✅ 조건문 있는 함수
✅ 반복문 있는 함수
✅ 복잡한 제어 흐름
✅ 실제 알고리즘 구현
✅ SSA 형식의 깊은 이해

【 최종 목표로 한 발 더 】

프로그램의 '뇌'를 만들 수 있게 되었습니다.
다음은 프로그램의 '신체'인 데이터 구조를 다룹니다.
```

---

## 🏆 축사

> **"제어 흐름의 마스터"**
>
> 1.1에서 배운 것: **"언어란 무엇인가"**
> 1.2에서 배운 것: **"함수를 만드는 방법"**
> 1.3에서 배운 것: **"프로그램의 판단과 반복"**
>
> 이제 당신은 **진정한 프로그래머**입니다.
>
> A + B = C 수준의 연산을 넘어서
> if, while, for를 만들 수 있는 경지에 도달했습니다.
>
> 다음 단계에서는 구조체와 배열로
> 프로그램의 **복잡한 데이터**를 설계합니다.
>
> **Z-Lang은 이제 정말 프로그래밍 언어가 되어가고 있습니다!** 🌟

---

**준비되셨으면 Assignment 1.3을 시작하세요!** 🚀

절댓값 함수로 시작해서,
PHI 노드의 마법을 경험하고,
루프의 원리를 깨달으세요!

**제어 흐름의 영역에 오신 것을 환영합니다!** 🏛️✨

---

**작성일**: 2026-02-26
**단계**: LLVM 전설 1.3
**상태**: 강의 완료, 과제 대기 중 ⏳
