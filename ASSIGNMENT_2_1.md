# 📝 Assignment 2.1: 어휘 분석(Lexing) - 토큰 설계와 구현

## 🎯 목표

**소스 코드 문자열을 의미 있는 토큰 배열로 변환하는 렉서(Lexer)를 설계하고 구현합니다.**

이제 우리는:
- 1.1-1.5: **IR을 생성하고 실행** (Backend)
- 2.1+: **소스 코드를 읽고 분석** (Frontend) ← 시작!

---

## 📊 과제 개요

| 항목 | 내용 |
|------|------|
| 난이도 | ⭐⭐⭐⭐☆ (고급) |
| 소요 시간 | 5-6시간 |
| 선행 과제 | LLVM 1.1-1.5 완료 |
| 제출 형식 | Zig 코드 + 토큰 출력 + 분석 문서 |

---

## 🔧 Task 1: 토큰(Token) 정의

### 1-1. 목표

본인이 설계하는 Z-Lang의 **최소 토큰 10개 이상**을 enum으로 정의하세요.

### 1-2. 요구사항

```
✅ 기본 토큰 타입 정의
   └─ Keyword, Identifier, Number, String
   └─ Operator, Punctuation, EOF

✅ Z-Lang에 필요한 토큰 10개 이상
   예: fn, let, if, while, +, -, =, (, ), ;

✅ Token 구조체 정의
   └─ tag: TokenType
   └─ lexeme: 원본 텍스트
   └─ line, column: 위치 정보
   └─ literal: 해석된 값 (선택)

✅ 기본 메서드
   └─ display() - 토큰 출력
   └─ debug_print() - 디버그 정보
```

### 1-3. 구현 템플릿

**파일**: `zlang-project/src/lexer_2_1.zig`

```zig
const std = @import("std");

pub const TokenType = enum {
    // 리터럴
    number,
    identifier,
    string,

    // 키워드 (최소 10개)
    keyword_fn,      // function
    keyword_let,     // variable
    keyword_const,   // constant
    keyword_if,      // if
    keyword_else,    // else
    keyword_while,   // while
    keyword_for,     // for
    keyword_return,  // return
    keyword_true,    // true
    keyword_false,   // false

    // 연산자
    plus,       // +
    minus,      // -
    star,       // *
    slash,      // /
    percent,    // %
    equal,      // =
    equal_equal,  // ==
    bang_equal,   // !=
    less,       // <
    greater,    // >

    // 구두점
    lparen,     // (
    rparen,     // )
    lbrace,     // {
    rbrace,     // }
    comma,      // ,
    semicolon,  // ;
    dot,        // .

    // 기타
    eof,
    newline,
    unknown,
};

pub const Token = struct {
    tag: TokenType,
    lexeme: []const u8,
    line: u32,
    column: u32,
    literal: ?[]const u8 = null,

    pub fn display(self: Token) void {
        std.debug.print("{}:{} {:?} '{s}'", .{
            self.line,
            self.column,
            self.tag,
            self.lexeme
        });

        if (self.literal) |lit| {
            std.debug.print(" [{}]", .{lit});
        }

        std.debug.print("\n", .{});
    }
};

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // 토큰 정의 테스트
    const test_token = Token{
        .tag = .keyword_fn,
        .lexeme = "fn",
        .line = 1,
        .column = 1,
    };

    test_token.display();
    // 출력: 1:1 keyword_fn 'fn'
}
```

---

## 🧪 Task 2: 렉서(Lexer) 구현

### 2-1. 목표

다음 소스 코드를 올바른 토큰 배열로 변환하는 렉서를 구현하세요:

```
"fn add(a, b) { return a + b; }"
```

### 2-2. 요구사항

```
✅ Lexer 구조체 정의
   └─ source: []const u8
   └─ current: usize (현재 위치)
   └─ line, column: 위치 추적
   └─ tokens: ArrayList(Token)

✅ tokenize() 함수
   └─ 소스 코드를 토큰 배열로 변환
   └─ 공백과 주석 무시

✅ 기본 토큰 인식
   └─ 키워드 식별
   └─ 식별자 수집
   └─ 연산자 감지
   └─ 구두점 처리

✅ 에러 처리
   └─ 알 수 없는 문자 감지
   └─ 위치 정보와 함께 보고
```

### 2-3. 구현 요점

```zig
pub const Lexer = struct {
    source: []const u8,
    current: usize = 0,
    line: u32 = 1,
    column: u32 = 1,
    tokens: std.ArrayList(Token),
    allocator: std.mem.Allocator,

    pub fn init(allocator: std.mem.Allocator, source: []const u8) Lexer {
        return Lexer{
            .source = source,
            .tokens = std.ArrayList(Token).init(allocator),
            .allocator = allocator,
        };
    }

    pub fn tokenize(self: *Lexer) ![]Token {
        while (!self.is_at_end()) {
            // 1️⃣ 문자 읽기
            const c = self.advance();

            // 2️⃣ 분류 및 처리
            switch (c) {
                ' ', '\t', '\r' => {},    // 공백 무시
                '\n' => {
                    self.line += 1;
                    self.column = 0;
                },
                '(' => self.add_token(.lparen),
                ')' => self.add_token(.rparen),
                // ... (다른 글자들)
                else => {
                    if (std.ascii.isDigit(c)) {
                        self.number();
                    } else if (std.ascii.isAlpha(c)) {
                        self.identifier();
                    }
                }
            }
        }

        self.add_token(.eof);
        return self.tokens.items;
    }

    fn advance(self: *Lexer) u8 {
        const c = self.source[self.current];
        self.current += 1;
        self.column += 1;
        return c;
    }

    fn peek(self: *const Lexer) u8 {
        if (self.is_at_end()) return '\x00';
        return self.source[self.current];
    }

    fn identifier(self: *Lexer) void {
        // 이전에 알파벳을 이미 읽었으므로
        // 나머지 알파벳/숫자/_를 계속 읽기
        while (std.ascii.isAlphaNumeric(self.peek()) or
               self.peek() == '_') {
            _ = self.advance();
        }

        // TODO: 키워드 확인
        self.add_token(.identifier);
    }

    fn number(self: *Lexer) void {
        // TODO: 정수 부분 읽기
        // TODO: 소수 부분 읽기 (Task 3)
        self.add_token(.number);
    }

    fn add_token(self: *Lexer, tag: TokenType) void {
        const lexeme = self.source[self.current - 1..self.current];
        const token = Token{
            .tag = tag,
            .lexeme = lexeme,
            .line = self.line,
            .column = self.column,
        };
        self.tokens.append(token) catch unreachable;
    }

    fn is_at_end(self: *const Lexer) bool {
        return self.current >= self.source.len;
    }
};
```

### 2-4. 예상 실행 결과

```
입력: "fn add(a, b) { return a + b; }"

출력:
1:1 keyword_fn 'fn'
1:4 identifier 'add'
1:7 lparen '('
1:8 identifier 'a'
1:9 comma ','
1:11 identifier 'b'
1:12 rparen ')'
1:14 lbrace '{'
1:16 keyword_return 'return'
1:23 identifier 'a'
1:25 plus '+'
1:27 identifier 'b'
1:28 semicolon ';'
1:29 rbrace '}'
1:30 eof ''
```

---

## 🔢 Task 3: 숫자 처리 - 정수와 실수

### 3-1. 목표

정수뿐만 아니라 **소수점을 포함한 실수** 토큰도 인식하세요.

### 3-2. 요구사항

```
✅ 정수 인식
   └─ 1, 42, 1000

✅ 실수 인식
   └─ 3.14, 2.71, 0.5

✅ 엣지 케이스 처리
   └─ ".5" (0 생략) → 에러 또는 인식
   └─ "5." (소수점만) → 에러 또는 정수로 처리
   └─ "1.2.3" (중복 소수점) → 에러

✅ 상태 머신 설계
   └─ START → NUMBER → (dot?) → DECIMAL? → END
```

### 3-3. 구현 코드

```zig
fn number(self: *Lexer) void {
    // 이미 첫 숫자를 읽었음
    const start = self.current - 1;

    // 정수 부분 계속 읽기
    while (std.ascii.isDigit(self.peek())) {
        _ = self.advance();
    }

    // 소수점 확인?
    if (self.peek() == '.' and
        std.ascii.isDigit(self.peek_next())) {
        // 소수점 넘기기
        _ = self.advance();

        // 소수 부분 읽기
        while (std.ascii.isDigit(self.peek())) {
            _ = self.advance();
        }
    }

    const number_text = self.source[start..self.current];
    self.add_token_with_literal(.number, number_text);
}

fn peek_next(self: *const Lexer) u8 {
    if (self.current + 1 >= self.source.len) {
        return '\x00';
    }
    return self.source[self.current + 1];
}

fn add_token_with_literal(
    self: *Lexer,
    tag: TokenType,
    literal: []const u8
) void {
    // ... token 생성
    // literal 필드에 값 저장
}
```

### 3-4. 테스트 케이스

```
입력: "let x = 3.14; let y = 42; let z = 0.5;"

기대 출력:
- 3.14 → number 토큰 (literal: "3.14")
- 42 → number 토큰 (literal: "42")
- 0.5 → number 토큰 (literal: "0.5")
```

---

## 📚 Task 4: 구조적 이점 분석

### 4-1. 목표

**"왜 렉서와 파서를 분리하는가?"**라는 질문에 대해 구조적 이점을 분석하는 3-4페이지 문서를 작성하세요.

### 4-2. 작성 가이드

**파일**: `RESEARCH_NOTES/2_1_LEXING/Task4_Lexer_Parser_Separation.md`

```markdown
# Task 4: 렉서와 파서의 분리 설계

## 1. 배경: 일체형 vs 분리형

### 일체형 설계 (❌)
```
source code
    ↓
[문자 읽음] → [바로 파싱] → [에러 시 중단]
    (강하게 결합)
```

### 분리형 설계 (✅)
```
source code
    ↓
[렉서: 토큰화]
    ↓
[파서: 구문 분석]
    ↓
[의미분석: 타입 체크]
    (느슨하게 결합)
```

## 2. 구조적 이점

### 2-1. 책임의 명확화 (Single Responsibility)

렉서: "어떤 문자들이 의미 있는 단위(토큰)인가?"
파서: "어떤 토큰들이 올바른 구조인가?"

### 2-2. 테스트 가능성

렉서 단위 테스트 (Lexer Unit Test):
```
입력: "let x = 10;"
검증: 정확히 5개 토큰 생성
검증: 각 토큰의 lexeme이 정확
검증: 위치 정보(line, column)가 정확
```

파서 단위 테스트 (Parser Unit Test):
```
입력: [keyword_let, identifier, equal, number, semicolon]
검증: 올바른 AST 생성
검증: 변수 선언 노드 포함
```

### 2-3. 에러 처리와 복구

렉서 에러:
```
"알 수 없는 문자 '@' → UNKNOWN 토큰으로 표시"
→ 계속 렉싱 가능 (에러 복구)
```

파서 에러:
```
"기대하지 않은 토큰 → 복구 규칙 적용"
→ 계속 파싱 가능 (에러 복구)

일체형이면: 복구 불가능
```

### 2-4. 재사용성 (Reusability)

같은 렉서를 여러 파서가 사용 가능:
- C 문법 파서
- Python 문법 파서
- JavaScript 문법 파서
→ 모두 같은 렉서 사용!

### 2-5. 유지보수성 (Maintainability)

렉서 버그: "숫자 인식이 안 됨"
→ 렉서만 수정, 파서는 영향 없음

파서 버그: "if 구문이 안 파싱됨"
→ 파서만 수정, 렉서는 영향 없음

### 2-6. 디버깅 용이성

토큰 배열을 덤프(dump)하여 중간 결과 확인 가능:
```
fn main() {
    let tokens = lexer.tokenize(source);

    // 렉서 출력 확인
    for (tokens) |token| {
        print_token(token);
    }

    // 파서로 진행
    let ast = parser.parse(tokens);
}
```

## 3. 실제 컴파일러 사례

### 3-1. GCC (C 컴파일러)
```
C 소스
  ↓
[Lexer] → 토큰
  ↓
[Parser] → AST
  ↓
[Semantic Analyzer] → 타입 체크
  ↓
[Code Generator] → 어셈블리
  ↓
[Assembler] → 기계어
```

### 3-2. Python (인터프리터)
```
.py 파일
  ↓
[Lexer] → 토큰
  ↓
[Parser] → AST
  ↓
[Bytecode Generator] → .pyc
  ↓
[VM] → 실행
```

## 4. 파이프라인 구조의 철학

Unix 철학:
- "각 프로그램은 한 가지 일을 잘하도록"
- "프로그램의 출력이 다른 프로그램의 입력이 되도록"

컴파일러도 동일:
- Lexer: 한 가지 일 (어휘 분석)
- Parser: 한 가지 일 (구문 분석)
- SemanticAnalyzer: 한 가지 일 (의미 분석)

→ 파이프라인으로 연결!

## 5. 성능 관점

### 메모리 효율:
분리형: 렉싱 완료 후 파싱 시작
→ 렉서 메모리 해제 가능

일체형: 메모리 계속 점유
→ 메모리 부족 문제 가능

### 병렬화 가능성:
```
여러 파일 동시 처리:
File 1: 렉싱 중 | (파싱 중) | (분석 중)
File 2: (렉싱 중) | 파싱 중 | (분석 중)
File 3: | (렉싱 중) | 파싱 중

파이프라인 병렬화 가능!
일체형: 불가능
```

## 6. 결론

렉서와 파서의 분리:
✅ 설계의 명확성 (각 단계의 책임이 명확)
✅ 테스트 용이성 (각 단계를 독립적으로 검증)
✅ 에러 처리 (각 단계에서 복구 가능)
✅ 재사용성 (같은 렉서를 여러 곳에서 사용)
✅ 유지보수성 (버그 수정이 국지화)
✅ 디버깅 (중간 결과 확인 가능)
✅ 성능 (병렬화, 메모리 효율)

**결론: 좋은 컴파일러는 파이프라인 구조를 선택한다.**
```

---

## 📋 제출 체크리스트

```
【 2.1 제출 체크리스트 】

Task 1: 토큰 정의
□ TokenType enum 정의 (10개 이상)
□ Token 구조체 정의 (tag, lexeme, line, column, literal)
□ display() 메서드 구현

Task 2: 렉서 구현
□ Lexer 구조체 정의
□ init() 함수 구현
□ tokenize() 함수 구현
□ advance(), peek() 함수 구현
□ "fn add(a, b) { return a + b; }" 올바르게 토큰화됨
  ├─ keyword_fn ✓
  ├─ identifier ✓
  ├─ lparen, rparen ✓
  └─ (나머지 모두)

Task 3: 숫자 처리
□ number() 함수 구현
□ 정수 인식 (42, 1000)
□ 실수 인식 (3.14, 0.5)
□ 엣지 케이스 처리 (선택)

Task 4: 구조적 이점 분석
□ "렉서와 파서 분리" 문서 작성 (3-4페이지)
□ 일체형 vs 분리형 비교
□ 책임 분리, 테스트, 에러 처리 등 6가지 이점 분석
□ 실제 컴파일러 사례 제시

모든 필수 항목 완료 → 2.2로 진행!
```

---

## 🚀 다음 단계

### 2.2: 구문 분석(Parsing) - AST 설계

```
토큰 배열을 받아서 구조화된 나무(AST) 생성!

예:
  입력: [keyword_fn, identifier("add"), lparen, ...]
  출력: FunctionDef { name: "add", params: ["a", "b"], ... }
```

---

**예상 완료 시간**: 5-6시간
**난이도**: ⭐⭐⭐⭐☆ (고급)
**보상**: **"당신의 언어가 단어를 갖춥니다!"** 📖
