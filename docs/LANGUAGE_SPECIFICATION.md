# Z-Lang: 언어 명세서 (v0.1 Draft)

## 개요

**Z-Lang**: 실시간 시스템을 위해 최적화된 정적 타입 언어

```
특징:
  ├─ Zero-Latency (GC 없음, 예측 가능한 성능)
  ├─ Type Safe (컴파일 타임 100% 검증)
  ├─ Memory Safe (Ownership + Lifetime)
  ├─ Real-Time Guarantees (WCET 분석 가능)
  └─ LLVM 기반 (다중 플랫폼 지원)
```

---

## 1. 어휘 (Lexicon)

### 1.1 토큰 타입

```
Keyword:      fn, let, const, if, else, loop, while, for, return, struct, enum, trait, impl, pub, priv
Identifier:   [a-zA-Z_][a-zA-Z0-9_]*
Number:       42, 3.14, 0xFF, 0b1010
String:       "hello", 'c'
Operator:     +, -, *, /, %, &, |, ^, !, ~, ==, !=, <, >, <=, >=, &&, ||, <<, >>, :=
Punctuation:  (, ), {, }, [, ], ;, :, ,, ., ->, =>
```

### 1.2 문자열 리터럴

```
"hello"        // 문자열
'x'            // 문자 (1바이트)
"multi\nline"  // 이스케이프 시퀀스
r"raw\nstring" // 원본 문자열 (\n 리터럴)
```

---

## 2. 타입 시스템

### 2.1 기본 타입 (Primitive Types)

```bnf
primitive_type ::=
    "i8" | "i16" | "i32" | "i64" | "i128" |
    "u8" | "u16" | "u32" | "u64" | "u128" |
    "f32" | "f64" |
    "bool" | "char" | "void"

type ::= primitive_type | compound_type | user_defined_type
```

#### 정수형

| 타입 | 범위 | 용도 |
|------|------|------|
| i8 | -128 ~ 127 | 1바이트 정수 |
| i32 | -2^31 ~ 2^31-1 | 기본 정수 |
| i64 | -2^63 ~ 2^63-1 | 대용량 정수 |
| u32 | 0 ~ 2^32-1 | 부호 없음 |

#### 부동 소수점

```
f32: IEEE 754 단정밀도 (32비트)
f64: IEEE 754 배정밀도 (64비트)
```

### 2.2 합성 타입 (Compound Types)

#### 배열
```z-lang
let arr: [i32; 10];     // 크기 10인 배열 (정적)
let vec: Vec<i32>;      // 동적 벡터
```

#### 튜플
```z-lang
let pair: (i32, str);
let triple: (i32, f64, bool);
```

#### 구조체
```z-lang
struct Point {
    x: i32,
    y: i32,
}

let p: Point = Point { x: 10, y: 20 };
```

#### 열거형
```z-lang
enum Result<T, E> {
    Ok(T),
    Err(E),
}

enum Option<T> {
    Some(T),
    None,
}
```

### 2.3 참조 (References) & Lifetime

```z-lang
let x: i32 = 42;
let r: &i32 = &x;      // 불변 참조 ('a default)
let m: &mut i32 = &mut x;  // 가변 참조

// Lifetime 명시
fn borrow<'a>(x: &'a i32) -> &'a i32 { x }

// 제약: 한 번에 하나의 가변 참조만 가능
let m1 = &mut x;
// let m2 = &mut x;  // 컴파일 에러!
```

---

## 3. 변수 및 선언

### 3.1 변수 선언

```z-lang
// 불변 변수
let x: i32 = 42;
let x = 42;           // 타입 추론

// 가변 변수
let mut y: i32 = 10;
y = 20;

// 상수
const PI: f64 = 3.14159;

// 정적 변수
static COUNT: i32 = 0;
```

### 3.2 Shadowing

```z-lang
let x = 5;
let x = x + 1;  // OK: x = 6 (새로운 바인딩)
```

---

## 4. 함수

### 4.1 함수 정의

```bnf
function_def ::= "fn" identifier "(" parameters ")" "->" type "{" statements "}"
parameters ::= (identifier ":" type)* ("," identifier ":" type)*
```

#### 예제

```z-lang
// 기본 함수
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

// 암묵적 반환
fn multiply(a: i32, b: i32) -> i32 {
    a * b  // 마지막 식이 반환값
}

// 반환값 없음
fn print_hello() -> void {
    println("Hello!");
}

// 제네릭 함수
fn max<T: Comparable>(a: T, b: T) -> T {
    if a > b { a } else { b }
}
```

### 4.2 호출

```z-lang
let result = add(2, 3);
let prod = multiply(4, 5);
```

---

## 5. 제어 흐름

### 5.1 조건문

```z-lang
if condition {
    // ...
} else if other_condition {
    // ...
} else {
    // ...
}

// 식으로 사용
let value = if x > 0 { 1 } else { 0 };
```

### 5.2 루프

#### while 루프
```z-lang
let mut i = 0;
while i < 10 {
    println(i);
    i = i + 1;
}
```

#### for 루프
```z-lang
for i in 0..10 {
    println(i);
}

for item in array {
    println(item);
}
```

#### loop (무한 루프)
```z-lang
loop {
    if should_break { break; }
    // ...
}
```

### 5.3 break & continue

```z-lang
for i in 0..10 {
    if i == 5 { break; }      // 루프 탈출
    if i == 3 { continue; }   // 다음 반복
    println(i);
}
```

---

## 6. 패턴 매칭

### 6.1 match 표현식

```z-lang
match result {
    Ok(value) => println(value),
    Err(error) => println(error),
}

match number {
    0 => println("zero"),
    1 | 2 | 3 => println("small"),
    4..10 => println("medium"),
    _ => println("large"),
}
```

---

## 7. 구조체 및 메서드

### 7.1 구조체 정의

```z-lang
struct Rectangle {
    width: i32,
    height: i32,
}

// 메서드 정의
impl Rectangle {
    fn new(width: i32, height: i32) -> Rectangle {
        Rectangle { width, height }
    }

    fn area(&self) -> i32 {
        self.width * self.height
    }

    fn set_width(&mut self, w: i32) {
        self.width = w;
    }
}

// 사용
let mut rect = Rectangle::new(10, 20);
println(rect.area());  // 200
rect.set_width(15);
println(rect.area());  // 300
```

### 7.2 구조체 배치 (Layout)

```z-lang
// 자동 배치 (align 고려)
struct Packed {
    a: u8,    // offset 0
    b: u32,   // offset 4 (align)
    c: u8,    // offset 8
}

// 명시적 배치
#[repr(C)]
struct CLayout {
    // C 호환성 보장
}
```

---

## 8. Trait (특성)

### 8.1 Trait 정의

```z-lang
trait Display {
    fn display(&self) -> str;
}

struct Point {
    x: i32,
    y: i32,
}

impl Display for Point {
    fn display(&self) -> str {
        return format("Point({}, {})", self.x, self.y);
    }
}
```

### 8.2 Trait Bounds

```z-lang
fn print_display<T: Display>(item: T) {
    println(item.display());
}
```

---

## 9. 에러 처리

### 9.1 Result 타입

```z-lang
fn divide(a: i32, b: i32) -> Result<i32, str> {
    if b == 0 {
        return Err("Division by zero");
    }
    return Ok(a / b);
}

// 사용
match divide(10, 2) {
    Ok(value) => println(value),
    Err(error) => println(error),
}
```

### 9.2 ? 연산자

```z-lang
fn might_fail() -> Result<i32, str> {
    let value = some_operation()?;  // 에러면 자동 반환
    return Ok(value);
}
```

---

## 10. 메모리 관리

### 10.1 Ownership (소유권)

```z-lang
let s1 = "hello";
let s2 = s1;   // s1의 소유권이 s2로 이동
// println(s1);  // 컴파일 에러! s1은 더 이상 유효하지 않음

let s3 = s1.clone();  // 복제 (명시적)
```

### 10.2 Borrowing (빌림)

```z-lang
let s = "hello";
let len = calculate_length(&s);  // 불변 참조
// s는 여전히 유효함

fn calculate_length(s: &str) -> i32 {
    s.len()
    // &s는 스코프를 벗어나면 자동 해제
}
```

### 10.3 Heap Allocation

```z-lang
let vec = Vec::new();
vec.push(1);
vec.push(2);
// vec는 스코프 벗어날 때 자동 해제

let boxed = Box::new(42);
let value = *boxed;  // 역참조
```

---

## 11. Real-Time 속성

### 11.1 WCET 선언

```z-lang
#[wcet_bound = "100_us"]
fn safety_critical_function() {
    // 컴파일러가 100μs 이내 확인
    // 초과하면 컴파일 에러 또는 경고
}

#[wcet_bound = "1_ms"]
fn automotive_control() {
    // 자동차 제어: 1ms 이내
}
```

### 11.2 메모리 할당 제약

```z-lang
#[no_alloc]
fn hardreal_time_function() {
    // 런타임 메모리 할당 불가
    // let vec = Vec::new();  // 컴파일 에러!

    let arr: [i32; 100];  // 정적 할당만 OK
}
```

### 11.3 no_std 모드

```z-lang
#![no_std]  // 표준 라이브러리 비활성화

// 자동차, 의료기기, 항공우주에 적합
```

---

## 12. 속성 (Attributes)

### 12.1 내장 속성

```z-lang
#[inline]              // 함수 인라인
#[inline(never)]       // 인라인 방지
#[cold]                // 콜드 패스 표시
#[must_use]            // 반환값 사용 강제
#[deprecated]          // 사용 중단 경고
```

### 12.2 커스텀 속성

```z-lang
#[my_custom_attr]
fn decorated_function() { }
```

---

## 13. 예제 프로그램

### 13.1 Hello World

```z-lang
fn main() {
    println("Hello, Z-Lang!");
}
```

### 13.2 피보나치 수열

```z-lang
fn fibonacci(n: i32) -> i32 {
    if n <= 1 {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

fn main() {
    for i in 0..10 {
        println(fibonacci(i));
    }
}
```

### 13.3 Real-Time 제어

```z-lang
#[wcet_bound = "100_us"]
fn pid_control(error: f64, kp: f64, ki: f64) -> f64 {
    let p_term = kp * error;
    let i_term = ki * error;  // 간소화
    return p_term + i_term;
}

#[no_alloc]
#[wcet_bound = "50_us"]
fn sensor_read() -> i32 {
    // 실시간 센서 읽기
    return read_adc(0);
}
```

---

## 14. 문법 (EBNF)

```bnf
program        ::= (item)*
item           ::= function_def | struct_def | trait_def | impl_def

function_def   ::= "fn" ID "(" parameters? ")" "->" type block
parameters     ::= ID ":" type ("," ID ":" type)*

struct_def     ::= "struct" ID "{" (ID ":" type ";")* "}"

type           ::= primitive_type | compound_type | "(" type ("," type)* ")"

expression     ::= assignment
assignment     ::= logic_or ("=" assignment)?
logic_or       ::= logic_and ("||" logic_and)*
logic_and      ::= equality ("&&" equality)*
equality       ::= comparison (("==" | "!=") comparison)*
comparison     ::= term (("<" | ">" | "<=" | ">=") term)*
term           ::= factor (("+" | "-") factor)*
factor         ::= unary (("*" | "/" | "%") unary)*
unary          ::= ("!" | "-" | "&" | "*") unary | postfix
postfix        ::= primary ("." ID | "[" expression "]" | "(" arguments? ")")*
primary        ::= NUMBER | STRING | "true" | "false" | ID | "(" expression ")"

statement      ::= expression ";" | block | if_stmt | while_stmt | for_stmt
```

---

## 15. 호환성

### 15.1 C Interop

```z-lang
// C 라이브러리 호출
extern "C" {
    fn printf(fmt: *const char, ...) -> i32;
    fn strlen(s: *const char) -> u64;
}

fn main() {
    let msg = "Hello from Z-Lang";
    printf("Message: %s\n", msg.as_ptr());
}
```

### 15.2 LLVM IR 인라인

```z-lang
// 저수준 최적화가 필요할 때
fn sum_asm(a: i32, b: i32) -> i32 {
    #[inline_asm(
        "add eax, ebx",
        in("eax") a,
        in("ebx") b,
        out("eax") result
    )]
    return result;
}
```

---

## 16. 예약어

```
Keywords: fn, let, const, static, mut, if, else, match, loop, while, for,
          break, continue, return, struct, enum, trait, impl, pub, priv,
          mod, use, as, ref, unsafe, extern, async, await, yield, ...
```

---

## 17. 타입 추론

```z-lang
// 타입 명시
let x: i32 = 42;

// 타입 추론
let x = 42;        // i32로 추론
let y = 3.14;      // f64로 추론
let z = true;      // bool로 추론

// 제네릭 추론
let vec = Vec::new();  // 타입 컨텍스트에서 결정
let vec: Vec<i32> = Vec::new();
```

---

## 상태

- **버전**: 0.1 Draft
- **마지막 업데이트**: 2026-02-26
- **다음 업데이트**: Week 2 (렉서/파서 구현 후)

**주의**: 이 명세서는 개발 중이며 변경될 수 있습니다.
