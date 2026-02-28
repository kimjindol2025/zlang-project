# 📝 Task 3: 표현식 생성 시뮬레이션 - 당신의 답변

## 예제 1: 산술 표현식

### Z-Lang 소스코드
```z-lang
fn calculate() -> i64 {
    let x: i64 = 10;
    let y: i64 = 20;
    let result: i64 = (x + y) * 2;
    return result;
}
```

### 당신의 LLVM IR 답변

```llvm
【 예제 1 답변 - 산술 표현식 】

define i64 @calculate() {
entry:
  ; Step 1: x 변수 선언
  [ 여기에 작성 ]

  ; Step 2: y 변수 선언
  [ 여기에 작성 ]

  ; Step 3: result 계산
  [ 여기에 작성 ]

  ; Step 4: 반환
  [ 여기에 작성 ]
}
```

### 예상 IR 라인 수
```
- 변수 선언 (3개 변수): 6줄 (각 alloca + store)
- 값 로드 (2개): 2줄
- 연산 (add, mul): 2줄
- 최종 저장: 1줄
- 반환: 2줄
──────────────
총: 약 13-15줄
```

---

## 예제 2: 조건문

### Z-Lang 소스코드
```z-lang
fn max(a: i64, b: i64) -> i64 {
    if a > b {
        return a;
    } else {
        return b;
    }
}
```

### 당신의 LLVM IR 답변

```llvm
【 예제 2 답변 - 조건문 】

define i64 @max(i64 %a, i64 %b) {
entry:
  ; 조건 평가
  [ 여기에 작성 ]

  ; 분기
  [ 여기에 작성 ]

then:
  [ 여기에 작성 ]

else:
  [ 여기에 작성 ]
}
```

### 핵심 개념
```
1. icmp: 정수 비교
   icmp sgt i64 %a, %b   ; signed greater than
   결과: i1 (true/false)

2. br: 분기
   br i1 %cond, label %then, label %else

3. 블록: entry, then, else
```

---

## 예제 3: 루프

### Z-Lang 소스코드
```z-lang
fn sum_up_to(n: i64) -> i64 {
    let sum: i64 = 0;
    let i: i64 = 0;
    while i <= n {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
```

### 당신의 LLVM IR 답변

```llvm
【 예제 3 답변 - 루프 】

define i64 @sum_up_to(i64 %n) {
entry:
  ; 초기화
  [ 여기에 작성 ]

  br label %while.cond

while.cond:
  ; 루프 조건 검사
  [ 여기에 작성 ]

  [ 분기 ]

while.body:
  ; 루프 본체
  [ 여기에 작성 ]

  br label %while.cond

while.end:
  ; 반환
  [ 여기에 작성 ]
}
```

### 핵심 개념
```
1. 초기화 (entry)
   변수들 초기화

2. 조건 검사 (while.cond)
   조건 평가, 분기

3. 본체 (while.body)
   루프 코드 실행

4. 백에지 (backedge)
   while.cond로 돌아가기
```

---

## 📋 제출 요구사항

작성 완료 후:

1. **예제 1, 2, 3 모두 완성**
2. **각 라인에 주석 추가** (무엇을 하는지)
3. **IR 문법 정확성** (타입, 레지스터 이름)
4. **로직 검증** (변수 흐름이 맞는가)

---

## 🎯 채점 기준

```
【 예제 1: 산술 표현식 】
  [  ] 3개 변수 모두 할당 (alloca)
  [  ] 초기값 저장 (store)
  [  ] 변수값 로드 (load)
  [  ] add i64 명령어
  [  ] mul i64 명령어
  [  ] 값 저장 및 반환

【 예제 2: 조건문 】
  [  ] icmp sgt 비교
  [  ] br i1 분기
  [  ] then/else 블록
  [  ] 각 블록에서 반환

【 예제 3: 루프 】
  [  ] 초기화 (entry)
  [  ] 조건 검사 (while.cond)
  [  ] 본체 (while.body)
  [  ] 백에지 (br label %while.cond)
  [  ] 루프 종료 (while.end)
```

---

## 💪 격려 메시지

```
【 이 Task 3의 가치 】

이 연습을 완료하면:

✅ 당신은 "컴파일러 설계자"가 됩니다
✅ 당신은 소스코드를 IR로 변환하는 과정을 이해합니다
✅ 당신은 LLVM의 기본 개념을 마스터합니다
✅ 당신은 실제 컴파일러 엔지니어처럼 생각합니다

지금 하는 이 손계산이:
  "zlang 컴파일러의 모든 것"입니다!
```

---

**당신의 답변을 작성해주세요!** 📝

예제 1부터 시작하세요. 완성되면 제가 검증해드릴 것입니다. ✅
