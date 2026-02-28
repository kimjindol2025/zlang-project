# Z-Lang 시작하기

**작성일**: 2026-02-27
**대상**: 처음 사용자

---

## 🎯 첫 번째 프로그램 (5분)

### 1단계: 프로그램 작성

`hello.z` 파일 생성:

```z-lang
fn main() -> i64 {
    return 42;
}
```

### 2단계: 컴파일

```bash
zlang hello.z -o hello.ll
```

### 3단계: 결과 확인

```bash
cat hello.ll
```

**결과**:
```llvm
; ModuleID = 'zlang_program'
source_filename = "zlang_program"

define i64 @main() {
entry:
  ret i64 42
}
```

축하합니다! 🎉 첫 Z-Lang 프로그램이 컴파일되었습니다!

---

## 📚 기본 문법

### 1. 함수 정의

```z-lang
// 형식: fn 함수명(인자: 타입) -> 반환타입 { ... }
fn add(a: i64, b: i64) -> i64 {
    return a + b;
}
```

### 2. 변수 선언

```z-lang
fn variables() -> i64 {
    let x: i64 = 10;        // 32비트 정수
    let y: i64 = 20;        // 명시적 타입
    let sum: i64 = x + y;
    return sum;              // 30
}
```

### 3. 조건문

```z-lang
fn max(a: i64, b: i64) -> i64 {
    if a > b {
        return a;
    } else {
        return b;
    }
}
```

### 4. 루프

```z-lang
fn sum_to_n(n: i64) -> i64 {
    let sum: i64 = 0;
    let i: i64 = 1;

    while i <= n {
        sum = sum + i;
        i = i + 1;
    }

    return sum;  // 1+2+...+n
}
```

### 5. 함수 호출

```z-lang
fn factorial(n: i64) -> i64 {
    if n <= 1 {
        return 1;
    }

    return n * factorial(n - 1);  // 재귀 호출
}
```

---

## 🔧 컴파일 옵션

### 기본 컴파일

```bash
zlang program.z
# 출력: output.ll (기본값)
```

### 출력 파일 지정

```bash
zlang program.z -o my_program.ll
```

### 상세 로그 출력

```bash
zlang program.z --verbose
```

출력 예시:
```
【 Stage 1: Lexing (어휘 분석) - ✅ 완성 】
  ✅ 토큰화 완료: 42개 토큰

【 Stage 2: Parsing (구문 분석) - ✅ 완성 】
  ✅ 파싱 완료: 1개 함수

【 Stage 3: Semantic Analysis (의미 분석) 】
⚠️  TypeChecker 미구현

【 Stage 4: Code Generation (코드 생성) 】
✅ LLVM IR 생성 완료

【 Stage 5: Optimization (최적화) 】
✅ 최적화 분석 완료

【 Stage 4.5: WCET Analysis 】
【 WCET Analysis Report 】
Function            WCET (cycles)  Exception      Critical
factorial           110            50             YES
Total WCET: 110 cycles
Critical Path: factorial

【 Stage 6: IR 출력 】
✅ IR 파일 생성: my_program.ll
```

### WCET 분석 활성화

```bash
zlang program.z --verbose
```

(기본적으로 활성화, `--verbose`로 상세 출력)

---

## 📖 예제 모음

### 예제 1: 간단한 산술

```z-lang
// arithmetic.z
fn calculate() -> i64 {
    let a: i64 = 100;
    let b: i64 = 50;

    let add_result: i64 = a + b;      // 150
    let sub_result: i64 = a - b;      // 50
    let mul_result: i64 = a * b;      // 5000
    let div_result: i64 = a / b;      // 2

    return add_result + sub_result;   // 200
}
```

```bash
zlang arithmetic.z -o arithmetic.ll
```

### 예제 2: Fibonacci

```z-lang
// fibonacci.z
fn fibonacci(n: i64) -> i64 {
    if n <= 1 {
        return n;
    }

    let a: i64 = 0;
    let b: i64 = 1;
    let i: i64 = 2;

    while i <= n {
        let temp: i64 = a + b;
        a = b;
        b = temp;
        i = i + 1;
    }

    return b;
}

fn main() -> i64 {
    return fibonacci(10);  // 55
}
```

```bash
zlang fibonacci.z -o fibonacci.ll --verbose
```

### 예제 3: 조건문과 루프 조합

```z-lang
// sum_odd.z
fn sum_odd_numbers(n: i64) -> i64 {
    let sum: i64 = 0;
    let i: i64 = 1;

    while i <= n {
        if i % 2 == 1 {
            sum = sum + i;
        }
        i = i + 1;
    }

    return sum;
}

fn main() -> i64 {
    return sum_odd_numbers(10);  // 1+3+5+7+9 = 25
}
```

```bash
zlang sum_odd.z --verbose
```

### 예제 4: 중첩된 루프

```z-lang
// multiplication_table.z
fn multiplication_table(n: i64) -> i64 {
    let sum: i64 = 0;
    let i: i64 = 1;

    while i <= n {
        let j: i64 = 1;
        while j <= n {
            sum = sum + (i * j);
            j = j + 1;
        }
        i = i + 1;
    }

    return sum;
}
```

---

## ⏱️ WCET 분석 이해하기

Z-Lang은 **Worst-Case Execution Time (WCET)**을 자동으로 분석합니다.

### WCET이란?

```
WCET = 최악의 경우 실행 시간
      = 함수가 가능한 가장 오래 걸리는 시간
```

### 예제

```z-lang
fn slow_function(n: i64) -> i64 {
    let sum: i64 = 0;
    let i: i64 = 0;

    while i < n {
        sum = sum + i;
        i = i + 1;
    }

    return sum;
}
```

컴파일 출력:
```
【 WCET Analysis Report 】
Function            WCET (cycles)  Exception      Critical
slow_function       304            50             YES

Total WCET: 304 cycles
Critical Path: slow_function
```

**의미**:
- 루프가 `n`번 반복되므로, WCET은 루프 반복 횟수에 따라 결정됨
- 각 루프 반복: ~3 사이클
- 최악의 경우: 304 사이클

### Real-Time Systems에서의 중요성

```
❌ 나쁜 예:
fn unpredictable(x: i64) -> i64 {
    while x > 0 {      // 무한 루프 위험!
        x = x - 1;
    }
    return x;
}

✅ 좋은 예:
fn predictable(n: i64) -> i64 {
    let sum: i64 = 0;
    let i: i64 = 0;

    while i < n {      // 명확한 루프 경계
        sum = sum + i;
        i = i + 1;
    }

    return sum;
}
```

---

## 🚀 다음 단계

### 1. 더 많은 예제 실행

```bash
# 저장소의 테스트 파일들
zlang test_wcet.z -o test.ll --verbose
zlang test_try_catch.z -o test.ll
zlang test_result.z -o test.ll
```

### 2. IR 코드 학습

```bash
# 생성된 LLVM IR 코드 분석
cat output.ll

# IR을 Object 파일로 변환
llc output.ll -o output.s
as output.s -o output.o

# 실행 파일 생성 (아직 미지원)
gcc output.o -o executable
```

### 3. 고급 기능

- [BACKEND_DESIGN.md](BACKEND_DESIGN.md) - 네이티브 코드 생성 설계
- [ZLANG_EVALUATION_REPORT.md](ZLANG_EVALUATION_REPORT.md) - 상세 평가
- [README.md](README.md) - 프로젝트 개요

### 4. 커뮤니티

- 버그 리포트: https://gogs.dclub.kr/kim/zlang/issues
- 토론: https://gogs.dclub.kr/kim/zlang/discussions
- Pull Requests 환영합니다!

---

## 📝 문법 요약

| 개념 | 형식 | 예제 |
|------|------|------|
| 함수 | `fn name(arg: type) -> type { ... }` | `fn add(x: i64) -> i64 { return x + 1; }` |
| 변수 | `let name: type = value;` | `let x: i64 = 42;` |
| 조건 | `if condition { ... } else { ... }` | `if x > 0 { ... }` |
| 루프 | `while condition { ... }` | `while i < 10 { i = i + 1; }` |
| 반환 | `return value;` | `return 42;` |
| 타입 | `i64`, `i32`, `f64`, `bool`, `string` | `let x: i64 = 10;` |

---

## 💡 팁

### 1. 항상 타입을 명시하세요

```z-lang
// ✅ 좋음
let x: i64 = 42;

// ❌피하세요 (타입 추론 미지원)
let x = 42;
```

### 2. 명확한 루프 경계 사용

```z-lang
// ✅ 좋음 (WCET 분석 가능)
let i: i64 = 0;
while i < 10 {
    i = i + 1;
}

// ❌피하세요 (무한 루프 위험)
while true {
    ...
}
```

### 3. 함수 개수는 적을수록 좋습니다

```z-lang
// ✅ 좋음
fn main() -> i64 {
    ...
}

// ❌많은 함수는 분석 복잡도 증가
fn func1() -> i64 { ... }
fn func2() -> i64 { ... }
fn func3() -> i64 { ... }
...
```

---

**기록이 증명이다.** 📋

*작성: 2026-02-27*
