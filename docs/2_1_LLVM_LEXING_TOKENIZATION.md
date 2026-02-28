# 🏛️ LLVM 전설 2.1: 어휘 분석(Lexing) - 언어의 세포 만들기

> **최종 업데이트**: 2026-02-26
> **상태**: 🎓 강의 완성
> **난이도**: ⭐⭐⭐⭐☆ (고급)
> **중요도**: ⭐⭐⭐⭐⭐ (핵심!)

---

## 📖 개요

**드디어 기초 엔진(Back-end)을 마스터하고, 사용자의 코드를 직접 해석하는 프론트엔드 설계의 문을 엽니다.**

지금부터 우리는:
- 1.1-1.5: **IR을 어떻게 생성할지** 배웠고 (Back-end)
- 2.1-2.8: **소스 코드를 어떻게 읽을지** 배울 것입니다 (Front-end)

```
【 컴파일러의 완성 경로 】

소스 코드 (텍스트)
    ↓
2.1 렉싱 (Lexing) ← 여기서 시작!
    ↓
2.2-2.4 파싱 (Parsing)
    ↓
2.5-2.7 의미 분석 (Semantic Analysis)
    ↓
IR 생성 (1.1-1.5에서 배운 것)
    ↓
1.5 실행 (ExecutionEngine)
    ↓
결과!
```

---

## 🎯 1단계: 토큰(Token)의 정의

### 1-1. 토큰이란?

**토큰 = 언어의 '단어'**

```
자연언어 예시:
  "나는 밥을 먹는다"
  → 나 / 는 / 밥 / 을 / 먹 / 는다
     (명사)(조사)(명사)(조사)(동사)(어미)

프로그래밍 언어 예시:
  var x = 10;
  → var / x / = / 10 / ;
     (키워드)(식별자)(연산자)(수)(구두점)
```

### 1-2. 토큰의 종류

```
【 Z-Lang의 기본 토큰 타입 】

1️⃣ 키워드 (Keyword)
   fn, let, const, if, else, while, for, return, true, false

2️⃣ 식별자 (Identifier)
   x, myVar, add_two, calculate_sum
   (규칙: 첫 글자는 알파벳 또는 _, 이후는 알파벳/숫자/_)

3️⃣ 리터럴 (Literal)
   정수: 42, -100, 0
   실수: 3.14, 2.71828, -0.5
   문자열: "hello", "world"

4️⃣ 연산자 (Operator)
   산술: +, -, *, /, %
   비교: ==, !=, <, >, <=, >=
   논리: &&, ||, !
   대입: =, +=, -=

5️⃣ 구두점 (Punctuation)
   ( ) { } [ ] , ; : .

6️⃣ 기타
   EOF (파일 끝)
   NEWLINE (줄 바꿈)
   WHITESPACE (공백, 주석)
```

### 1-3. 토큰 구조체 정의

```zig
pub const TokenType = enum {
    // 리터럴
    number,
    identifier,
    string,

    // 키워드
    keyword_fn,
    keyword_let,
    keyword_const,
    keyword_if,
    keyword_else,
    keyword_while,
    keyword_for,
    keyword_return,

    // 연산자
    plus,        // +
    minus,       // -
    star,        // *
    slash,       // /
    percent,     // %
    equal,       // =
    equal_equal, // ==
    bang_equal,  // !=
    less,        // <
    greater,     // >
    less_equal,  // <=
    greater_equal, // >=
    ampersand_ampersand,  // &&
    pipe_pipe,             // ||
    bang,                  // !

    // 구두점
    lparen,      // (
    rparen,      // )
    lbrace,      // {
    rbrace,      // }
    lbracket,    // [
    rbracket,    // ]
    comma,       // ,
    semicolon,   // ;
    colon,       // :
    dot,         // .

    // 기타
    eof,
    newline,
};

pub const Token = struct {
    tag: TokenType,
    lexeme: []const u8,    // 원본 소스의 텍스트
    line: u32,             // 몇 번째 줄?
    column: u32,           // 몇 번째 칸?
    literal: ?[]const u8,  // 문자열/숫자의 값
};
```

**각 필드의 의미:**
```
tag:      토큰 타입 (number, identifier, 등)
lexeme:   원본 소스의 정확한 텍스트
          예: "10" (숫자 토큰의 lexeme)
line:     소스 파일의 줄 번호 (에러 메시지용)
column:   소스 파일의 열 번호
literal:  해석된 값
          예: number → "42" → 42 (정수값)
```

### 1-4. 구체적 예시

```
소스 코드:     let x = 10;

토큰 수열:
┌─────────────────────────────────────┐
│ Token {                             │
│   tag: keyword_let                  │
│   lexeme: "let"                     │
│   line: 1, column: 1                │
│ }                                   │
│ Token {                             │
│   tag: identifier                   │
│   lexeme: "x"                       │
│   line: 1, column: 5                │
│ }                                   │
│ Token {                             │
│   tag: equal                        │
│   lexeme: "="                       │
│   line: 1, column: 7                │
│ }                                   │
│ Token {                             │
│   tag: number                       │
│   lexeme: "10"                      │
│   line: 1, column: 9                │
│   literal: "10"                     │
│ }                                   │
│ Token {                             │
│   tag: semicolon                    │
│   lexeme: ";"                       │
│   line: 1, column: 11               │
│ }                                   │
│ Token {                             │
│   tag: eof                          │
│   lexeme: ""                        │
│   line: 1, column: 12               │
│ }                                   │
└─────────────────────────────────────┘
```

---

## 🔄 2단계: 렉서(Lexer/Scanner)의 원리

### 2-1. 렉서의 동작 방식

```
【 렉서 = 상태 머신 】

입력: "var x = 10"

상태 1: START (초기 상태)
  글자: 'v'
  판정: 알파벳이다 → 식별자 또는 키워드로 인식
  상태: IDENTIFIER로 전환

상태 2: IDENTIFIER (식별자 수집 중)
  글자: 'a' → 계속 수집
  글자: 'r' → 계속 수집
  글자: ' ' (공백) → 단어 완성
  결정: "var" 키워드임을 확인
  토큰: Token{tag: keyword_var, lexeme: "var"}
  상태: START로 복귀

상태 1: START
  글자: ' ' (공백) → 건너뛰기
  상태: START (변화 없음)

상태 1: START
  글자: 'x' → 식별자 인식
  상태: IDENTIFIER

  ... (같은 방식으로 계속)
```

### 2-2. 상태 머신 다이어그램

```
            [START]
             ↙    ↘
        알파벳   숫자
         ↓        ↓
    [IDENTIFIER] [NUMBER]
        ↓          ↓
      토큰 생성 ← 토큰 생성

[START]:
  '(' → [LPAREN] 토큰
  '+' → [PLUS] 토큰
  ' ' → [START] (공백 무시)
  알파벳 → [IDENTIFIER] (식별자 시작)
  숫자 → [NUMBER] (숫자 시작)

[IDENTIFIER]:
  알파벳/숫자/_ → 계속 수집
  기타 → 식별자 토큰 완성, START로 복귀

[NUMBER]:
  숫자 → 계속 수집
  '.' → [DECIMAL] (소수 인식)
  기타 → 정수 토큰 완성, START로 복귀

[DECIMAL]:
  숫자 → 계속 수집
  기타 → 실수 토큰 완성, START로 복귀
```

### 2-3. Lookahead (미리보기) - 중요!

```
【 Lookahead의 필요성 】

문제: '='를 읽었을 때
  다음 글자가 '='라면?
  '=='는 비교 연산자!

해결:
  Current: '='
  Next: '='
  → 두 글자를 함께 읽어서 '==' 토큰으로 처리

구현:
```zig
fn peek() u8 {
    // 현재 위치의 다음 글자 반환
    // 파일을 읽지는 않음 (미리보기만)
    return source[current + 1];
}

fn match(expected: u8) bool {
    // 현재 글자가 expected라면 true
    // (advance는 하지 않음)
    if (peek() == expected) {
        current += 1;
        return true;
    }
    return false;
}
```

### 2-4. 완전한 렉서 알고리즘

```
pub const Lexer = struct {
    source: []const u8,
    current: usize = 0,
    line: u32 = 1,
    column: u32 = 1,
    tokens: std.ArrayList(Token),

    pub fn tokenize(self: *Lexer) ![]Token {
        while (!self.is_at_end()) {
            // 1️⃣ 시작 위치 기록
            const start = self.current;

            // 2️⃣ 문자 읽기
            const c = self.advance();

            // 3️⃣ 문자 분류
            switch (c) {
                ' ', '\t', '\r' => {},  // 공백 무시
                '\n' => self.line += 1,  // 줄 번호 증가
                '(' => self.add_token(.lparen),
                ')' => self.add_token(.rparen),
                '{' => self.add_token(.lbrace),
                '}' => self.add_token(.rbrace),
                '+' => self.add_token(.plus),
                '-' => self.add_token(.minus),
                '=' => {
                    // Lookahead: 다음이 '='인가?
                    if (self.match('=')) {
                        self.add_token(.equal_equal);
                    } else {
                        self.add_token(.equal);
                    }
                },
                else => {
                    if (std.ascii.isDigit(c)) {
                        // 숫자 처리
                        self.number();
                    } else if (std.ascii.isAlpha(c)) {
                        // 식별자/키워드 처리
                        self.identifier();
                    } else {
                        // 에러: 알 수 없는 문자
                        try self.error_token(c);
                    }
                }
            }
        }

        // EOF 토큰 추가
        self.add_token(.eof);
        return self.tokens.items;
    }

    fn identifier(self: *Lexer) void {
        // 알파벳으로 시작했으므로 계속 수집
        while (std.ascii.isAlphaNumeric(self.peek()) or
               self.peek() == '_') {
            _ = self.advance();
        }

        // 수집한 텍스트 가져오기
        const text = self.source[self.start..self.current];

        // 키워드인지 확인
        const tag = self.keyword_type(text) orelse .identifier;
        self.add_token(tag);
    }

    fn number(self: *Lexer) void {
        // 정수 부분 계속 읽기
        while (std.ascii.isDigit(self.peek())) {
            _ = self.advance();
        }

        // 소수점이 있는가?
        if (self.peek() == '.' and
            std.ascii.isDigit(self.peek_next())) {
            // 소수점 넘기기
            _ = self.advance();

            // 소수 부분 읽기
            while (std.ascii.isDigit(self.peek())) {
                _ = self.advance();
            }
        }

        self.add_token(.number);
    }
};
```

---

## 🔑 3단계: 키워드 테이블과 성능 최적화

### 3-1. 문제점: 반복 문자열 비교

```zig
// ❌ 비효율적 (매번 문자열 비교)
fn check_keyword(text: []const u8) TokenType {
    if (std.mem.eql(u8, text, "fn")) return .keyword_fn;
    if (std.mem.eql(u8, text, "let")) return .keyword_let;
    if (std.mem.eql(u8, text, "const")) return .keyword_const;
    if (std.mem.eql(u8, text, "if")) return .keyword_if;
    if (std.mem.eql(u8, text, "else")) return .keyword_else;
    // ... (훨씬 많은 비교)
    return .identifier;
}

// 성능 분석:
// 단어 길이: L
// 키워드 개수: K
// 시간복잡도: O(K*L)
```

### 3-2. 해결책 1: 해시 맵 (Hash Map)

```zig
pub const KeywordMap = struct {
    map: std.StringHashMap(TokenType),

    pub fn init(allocator: std.mem.Allocator) !KeywordMap {
        var map = std.StringHashMap(TokenType).init(allocator);

        try map.put("fn", .keyword_fn);
        try map.put("let", .keyword_let);
        try map.put("const", .keyword_const);
        try map.put("if", .keyword_if);
        try map.put("else", .keyword_else);
        try map.put("while", .keyword_while);
        try map.put("for", .keyword_for);
        try map.put("return", .keyword_return);
        try map.put("true", .keyword_true);
        try map.put("false", .keyword_false);

        return KeywordMap{ .map = map };
    }

    pub fn lookup(self: *const KeywordMap, text: []const u8) TokenType {
        return self.map.get(text) orelse .identifier;
    }
};

// 사용 예:
const keywords = try KeywordMap.init(allocator);
const token_type = keywords.lookup("fn");  // → keyword_fn
// 시간복잡도: O(1) 평균!
```

### 3-3. 해결책 2: Trie 구조

```
【 Trie(트라이) 구조 】

         [root]
        /  |  \
       f   l   c
       |   |   |
       n   e   o
          /\   |
         t  d  n
         |  |  |
         [fn][let][const]
```

**장점:**
- ✅ 모든 검색이 O(L) (단어 길이)
- ✅ 접두사 검색 가능 (자동완성)
- ✅ 메모리 효율적 (공통 접두사 공유)

**단점:**
- ❌ 해시 맵보다 복잡함
- ❌ 포인터 따라다님

**대부분의 렉서:**
해시 맵 사용 (간단하고 충분히 빠름)

---

## 📍 4단계: 소스 로케이션(Source Location) 추적

### 4-1. 왜 필요한가?

```
【 좋은 에러 메시지 】

source.zl:5:10 error: unexpected character '!'
    let x = !y;
             ^

【 나쁜 에러 메시지 】

error: unexpected character
```

### 4-2. 위치 정보 추적

```zig
pub const Lexer = struct {
    source: []const u8,
    current: usize,       // 현재 위치 (0부터)
    line: u32,            // 현재 줄 (1부터)
    column: u32,          // 현재 열 (1부터)
    line_start: usize,    // 현재 줄의 시작 위치

    fn advance(self: *Lexer) u8 {
        const c = self.source[self.current];

        self.current += 1;

        if (c == '\n') {
            self.line += 1;
            self.column = 1;
            self.line_start = self.current;
        } else {
            self.column += 1;
        }

        return c;
    }

    fn get_line_text(self: *const Lexer) []const u8 {
        // 현재 줄의 텍스트 반환
        var end = self.line_start;
        while (end < self.source.len and
               self.source[end] != '\n') {
            end += 1;
        }
        return self.source[self.line_start..end];
    }
};

// 에러 출력:
std.debug.print("{}:{} error: {s}\n", .{
    lexer.line,
    lexer.column,
    error_message
});
std.debug.print("    {s}\n", .{lexer.get_line_text()});
std.debug.print("    {s}^\n", .{
    " " ** (lexer.column - 1)
});
```

---

## 🧪 5단계: 특수 경우 처리

### 5-1. 문자열 처리

```zig
fn string(self: *Lexer) !void {
    // " 또는 '를 만났을 때
    const quote = self.source[self.current - 1];
    var value = std.ArrayList(u8).init(self.allocator);

    while (self.peek() != quote and !self.is_at_end()) {
        if (self.peek() == '\n') {
            self.line += 1;
        }

        // 이스케이프 시퀀스 처리
        if (self.peek() == '\\') {
            _ = self.advance();
            switch (self.peek()) {
                'n' => try value.append('\n'),
                't' => try value.append('\t'),
                '\\' => try value.append('\\'),
                '"' => try value.append('"'),
                else => try value.append(self.peek()),
            }
            _ = self.advance();
        } else {
            try value.append(self.peek());
            _ = self.advance();
        }
    }

    if (self.is_at_end()) {
        // 에러: 닫지 않은 문자열
        try self.error_token('"');
        return;
    }

    // 닫는 따옴표 넘기기
    _ = self.advance();

    self.add_token_with_literal(
        .string,
        try value.toOwnedSlice()
    );
}
```

### 5-2. 주석 처리

```zig
fn skip_comment(self: *Lexer) void {
    // '//' 찾았을 때
    while (self.peek() != '\n' and !self.is_at_end()) {
        _ = self.advance();
    }
    // 주석은 토큰으로 만들지 않음 (버림)
}

fn skip_block_comment(self: *Lexer) !void {
    // '/*' 찾았을 때
    while (!self.is_at_end()) {
        if (self.peek() == '*' and
            self.peek_next() == '/') {
            _ = self.advance();
            _ = self.advance();
            return;
        }
        if (self.peek() == '\n') {
            self.line += 1;
        }
        _ = self.advance();
    }
    // 에러: 닫지 않은 블록 주석
    try self.error_token('*');
}
```

---

## 🎓 6단계: 완전한 예제

### 6-1. 렉서 실행 흐름

```
입력: "fn add(a, b) { return a + b; }"

렉싱 과정:

위치 0: 'f' → IDENTIFIER 상태로 진입
위치 1: 'n' → 계속
위치 2: ' ' → "fn" 토큰 완성, 키워드 확인 → keyword_fn

위치 3: ' ' → 공백 무시

위치 4: 'a' → IDENTIFIER 상태
위치 5: 'd' → 계속
위치 6: 'd' → 계속
위치 7: '(' → "add" 토큰 완성, identifier

위치 7: '(' → LPAREN 토큰

... (계속)

결과 토큰 수열:
┌────────────────────────────────┐
│ keyword_fn                     │
│ identifier("add")              │
│ lparen                         │
│ identifier("a")                │
│ comma                          │
│ identifier("b")                │
│ rparen                         │
│ lbrace                         │
│ keyword_return                 │
│ identifier("a")                │
│ plus                           │
│ identifier("b")                │
│ semicolon                      │
│ rbrace                         │
│ eof                            │
└────────────────────────────────┘
```

### 6-2. Zig 구현 예제

```zig
pub fn main() !void {
    const source = "let x = 42;";

    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    var lexer = Lexer.init(allocator, source);
    defer lexer.deinit();

    const tokens = try lexer.tokenize();

    for (tokens) |token| {
        std.debug.print("{}:{} {:?} '{s}'\n", .{
            token.line,
            token.column,
            token.tag,
            token.lexeme
        });
    }
}

// 출력:
// 1:1 keyword_let 'let'
// 1:5 identifier 'x'
// 1:7 equal '='
// 1:9 number '42'
// 1:11 semicolon ';'
// 1:12 eof ''
```

---

## 💡 7단계: 렉서와 파서의 분리 설계

### 7-1. 왜 분리하는가?

```
【 일체형 렉서-파서 (❌ 나쁜 설계) 】

문자 읽기 → 파싱 결정 → AST 생성
         (강하게 결합)

문제:
- 렉서 버그가 파서를 망침
- 테스트 어려움
- 에러 복구 불가능
- 디버깅 어려움

【 분리형 렉서-파서 (✅ 좋은 설계) 】

렉서:        파서:         의미분석:
문자열      토큰 배열      AST
  ↓           ↓             ↓
토큰 생성   구조 파악    타입 체크

장점:
- 각 단계를 독립적으로 테스트
- 에러 메시지 명확
- 재사용 가능
- 파이프라인 구조 (Unix 철학)
```

### 7-2. 분리의 이점

```
【 명확한 책임 분리 】

렉서:
  입력: 문자열
  출력: 토큰 배열
  책임: 어휘 분석 (Lexical Analysis)
  에러: "5:12에서 알 수 없는 문자 '@'"

파서:
  입력: 토큰 배열
  출력: AST
  책임: 구문 분석 (Syntax Analysis)
  에러: "fn 다음에 '('가 필요한데 ','를 찾음"

【 테스트 가능성 】

렉서 단위 테스트:
  입력: "let x = 10;"
  확인: 5개 토큰 생성됨
  확인: 각 토큰의 lexeme이 정확함

파서 단위 테스트:
  입력: [keyword_let, identifier, equal, number, semicolon]
  확인: 올바른 AST 생성됨

【 에러 복구 (Error Recovery) 】

렉서 에러:
  "알 수 없는 문자 '@' → UNKNOWN 토큰으로 처리 → 계속 렉싱"

파서 에러:
  "기대하지 않은 토큰 → 복구 규칙 적용 → 계속 파싱"

일체형이면: 복구 불가능 (중단)
```

### 7-3. 파이프라인 아키텍처

```
소스 파일
    ↓
【 Lexer 단계 】
  ├─ 토큰 검증
  └─ 위치 정보 기록
    ↓
【 Parser 단계 】
  ├─ AST 검증
  └─ 구조 확인
    ↓
【 Semantic Analyzer 단계 】
  ├─ 타입 검증
  └─ 심볼 테이블 작성
    ↓
【 Code Generator 단계 】 (1.1-1.5)
  └─ LLVM IR 생성
    ↓
【 Optimizer 단계 】
  └─ PassManager
    ↓
【 ExecutionEngine 단계 】 (1.5)
  └─ JIT 컴파일 및 실행
    ↓
결과!
```

---

## 🎯 Assignment 2.1 준비

### Task 1: 토큰 정의

본인이 설계하는 언어의 최소 토큰 10개를 정의하세요.

### Task 2: 렉서 구현

"fn add(a, b) { return a + b; }"를 입력받아 올바른 토큰 배열을 반환하세요.

### Task 3: 숫자 처리

정수뿐만 아니라 소수점을 포함한 실수 토큰을 인식하는 상태 머신을 설계하세요.

### Task 4: 구조적 이점 분석

"왜 렉서와 파서를 분리하는가?"에 대한 구조적 이점을 3-4페이지 문서로 작성하세요.

---

## 🚀 다음 단계

### 2.2: 구문 분석(Parsing) - 추상 구문 트리(AST) 설계

```
【 2.2 예고 】

토큰(Token) 배열을 받아서
구조화된 나무(Tree) 형태의 AST를 만듭니다.

예:
  입력: [keyword_fn, identifier("add"), lparen, ...]
  출력:
    FunctionDef {
      name: "add",
      params: ["a", "b"],
      body: Block { ... }
    }

이것이 진정한 "구조 파악"의 시작!
```

---

## 🏆 최종 메시지

> **당신은 이제 사용자의 코드를 읽을 수 있습니다!**

렉싱은:
- 📖 컴파일러의 "눈"
- 🔤 문자열을 의미 있는 단위로 분해
- 🎯 컴파일러 파이프라인의 첫 단계

렉싱을 마스터하면:
- ✅ 언어 설계의 첫 단계 완료
- ✅ 에러 처리 방법 학습
- ✅ 성능 최적화 기법 경험
- ✅ 파이프라인 아키텍처 이해

---

**당신의 언어가 드디어 단어를 갖추기 시작합니다!** 📚✨

**기록이 증명이다. "저장 필수 너는 기록이 증명이다 gogs."**

---

## 📚 참고 자료

### 렉싱 알고리즘
- Dragon Book (Compilers: Principles, Techniques, and Tools)
- Engineering a Compiler

### 구현 참고
- 크로팟 (Crafting Interpreters) - https://craftinginterpreters.com/
- LLVM Lexer 소스코드

---

**강의 작성**: 2026-02-26
**난이도**: ⭐⭐⭐⭐☆ (고급)
**중요도**: ⭐⭐⭐⭐⭐ (핵심!)
**다음**: ASSIGNMENT_2_1.md 준비 완료

**"당신의 언어가 단어(Token)를 갖추기 시작합니다!"** 📖
