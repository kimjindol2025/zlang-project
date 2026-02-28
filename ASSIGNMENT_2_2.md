# 📋 Assignment 2.2: 구문 분석(Parsing)과 AST 설계

> **기한**: 2026-03-23 (일)
> **난이도**: ⭐⭐⭐⭐⭐ (최고급)
> **핵심**: 토큰을 AST로 변환하는 파서 설계 및 구현

---

## 🎯 과제 개요

이 과제에서는 **렉싱이 생성한 토큰들을 문법 규칙에 따라 검증**하고, **추상 구문 트리(AST) 형태로 변환**하는 파서를 설계합니다.

### 목표
- ✅ AST 노드 구조 정의
- ✅ 재귀 강하 파서 구현
- ✅ 연산자 우선순위 처리
- ✅ 에러 복구 메커니즘
- ✅ 아키텍처 철학 수립

---

## 📝 Task 1: AST 노드 계층 설계

### 개요

Z-Lang이 지원할 **모든 구문 요소**를 AST 노드로 표현합니다.

### 요구사항

다음 **9개 범주의 AST 노드**를 Zig의 `union(enum)`으로 정의하세요:

```
【 정의할 노드 범주 】
1. 리터럴 (Literal)
   - 정수: Integer(i64)
   - 부동소수점: Float(f64)
   - 문자열: String([]const u8)
   - 부울: Boolean(bool)

2. 변수 (Variable)
   - 식별자: Identifier([]const u8)

3. 연산 (Operations)
   - 이항: BinaryOp { op, left, right }
   - 단항: UnaryOp { op, operand }

4. 함수 호출 (Function Call)
   - 호출: Call { callee, args }

5. 함수 정의 (Function Definition)
   - 정의: Function { name, params, body }

6. 변수 선언 (Variable Declaration)
   - 선언: VarDecl { name, type, init }

7. 제어 흐름 (Control Flow)
   - if-else: IfStmt { condition, then_branch, else_branch }
   - while: WhileStmt { condition, body }

8. 반환문 (Return)
   - 반환: ReturnStmt { value }

9. 블록 (Block)
   - 블록: Block { statements }
```

### 작성 내용

**파일 이름**: `src/parser/ASTNode.zig`

```zig
pub const Node = union(enum) {
    // 【 리터럴 】

    // 【 변수 】

    // 【 연산 】

    // 【 함수 호출 】

    // 【 함수 정의 】

    // 【 변수 선언 】

    // 【 제어 흐름 】

    // 【 반환문 】

    // 【 블록 】
};

pub const BinaryOperator = enum {
    // 산술 연산
    // 비교 연산
    // 논리 연산
};

pub const UnaryOperator = enum {
    // 단항 연산
};
```

### 기대 결과물

**체크리스트:**
- [ ] Node union(enum) 정의 (9개 변형)
- [ ] BinaryOperator enum (15+ 개 연산자)
- [ ] UnaryOperator enum (4+ 개 연산자)
- [ ] Parameter, ParseError 등 보조 구조체
- [ ] ASTBuilder 헬퍼 함수 (allocNode, number, identifier, binaryOp 등)
- [ ] 메모리 안전성 (allocator 사용)

**제출물:**
```
zlang-project/
└── src/
    └── parser/
        ├── ASTNode.zig      ← 여기!
        └── Parser.zig       (Task 2에서)
```

---

## 💻 Task 2: 재귀 강하 파서 구현

### 개요

**렉싱 단계(2.1)에서 생성된 토큰 배열**을 입력받아 **AST를 생성**하는 파서를 구현합니다.

### 요구사항

다음 **11개의 파싱 함수**를 구현하세요:

```
【 파싱 함수 계층 】

1. parse()                    ← 진입점
   └─ parseFunction()         (함수 정의)
      └─ parseBlock()         (블록)
         └─ parseStatement()  (각 문장)
            ├─ parseVarDecl()     (변수 선언)
            ├─ parseReturn()      (return 문)
            ├─ parseIf()          (if 문)
            ├─ parseWhile()       (while 루프)
            └─ parseExpression()  (식)
               └─ parseEquality() (== != 우선순위)
                  └─ parseComparison() (< > <= >= 우선순위)
                     └─ parseAdditive() (+ - 우선순위)
                        └─ parseMultiplicative() (* / % 우선순위)
                           └─ parseUnary() (! - + 우선순위)
                              └─ parsePostfix() (함수 호출)
                                 └─ parsePrimary() (숫자, 식별자, 괄호)
```

### 작성 내용

**파일 이름**: `src/parser/Parser.zig`

```zig
pub const Parser = struct {
    tokens: []const Token,
    current: usize = 0,
    allocator: Allocator,
    builder: ASTBuilder,
    errors: std.ArrayList(ParseError),

    pub fn init(allocator: Allocator, tokens: []const Token) Parser {
        return .{
            .tokens = tokens,
            .allocator = allocator,
            .builder = ASTBuilder.init(allocator),
            .errors = std.ArrayList(ParseError).init(allocator),
        };
    }

    // 【 핵심 메서드 】
    fn current_token(self: *const Parser) Token { }
    fn peek_token(self: *const Parser, offset: usize) Token { }
    fn advance(self: *Parser) Token { }
    fn expect(self: *Parser, expected: TokenType) !Token { }

    // 【 파싱 함수 】
    pub fn parse(self: *Parser) !*Node { }
    fn parseFunction(self: *Parser) !*Node { }
    fn parseBlock(self: *Parser) !*Node { }
    fn parseStatement(self: *Parser) !*Node { }
    fn parseVarDecl(self: *Parser) !*Node { }
    fn parseReturn(self: *Parser) !*Node { }
    fn parseIf(self: *Parser) !*Node { }
    fn parseWhile(self: *Parser) !*Node { }
    fn parseExpression(self: *Parser) !*Node { }
    fn parseEquality(self: *Parser) !*Node { }
    fn parseComparison(self: *Parser) !*Node { }
    fn parseAdditive(self: *Parser) !*Node { }
    fn parseMultiplicative(self: *Parser) !*Node { }
    fn parseUnary(self: *Parser) !*Node { }
    fn parsePostfix(self: *Parser) !*Node { }
    fn parsePrimary(self: *Parser) !*Node { }
};
```

### 작성 체크리스트

- [ ] Parser 구조체 및 초기화
- [ ] current_token(), peek_token(), advance()
- [ ] expect() - 토큰 검증
- [ ] parse() - 진입점
- [ ] parseFunction() - fn keyword 처리
- [ ] parseBlock() - { } 블록 처리
- [ ] parseStatement() - 문장 타입 분기
- [ ] parseVarDecl() - var/let 선언
- [ ] parseReturn() - return 문
- [ ] parseIf() - if-else 문
- [ ] parseWhile() - while 루프
- [ ] parseExpression() - 식 파싱 (최상위)
- [ ] parseEquality() - == != (우선순위 처리)
- [ ] parseComparison() - < > <= >=
- [ ] parseAdditive() - + -
- [ ] parseMultiplicative() - * / %
- [ ] parseUnary() - ! - +
- [ ] parsePostfix() - 함수 호출
- [ ] parsePrimary() - 기본 식

### 기대 결과물

**테스트 입력**: `fn result(x) { return x * 2 + 1; }`

**토큰 배열** (2.1 렉싱 결과):
```
[keyword_fn, identifier(result), lparen, identifier(x), rparen,
 lbrace, keyword_return, identifier(x), star, number(2), plus,
 number(1), semicolon, rbrace]
```

**생성되는 AST**:
```
Function {
    name: "result",
    params: [Parameter("x", ...)],
    body: Block {
        statements: [
            ReturnStmt {
                value: BinaryOp {
                    op: Add,
                    left: BinaryOp {
                        op: Mul,
                        left: Identifier("x"),
                        right: Number(2)
                    },
                    right: Number(1)
                }
            }
        ]
    }
}
```

**AST 시각화**:
```
                  Function(result)
                       |
                     Block
                       |
                   ReturnStmt
                       |
                       +        ← 우선순위: +가 위
                      / \
                     *   1     ← 우선순위: *가 아래 (먼저 실행)
                    / \
                   x   2
```

**제출물:**
```
zlang-project/
└── src/
    └── parser/
        ├── ASTNode.zig
        └── Parser.zig       ← 여기!
```

---

## 🔍 Task 3: 연산자 우선순위 검증

### 개요

파서가 **올바른 우선순위**로 AST를 생성하는지 검증합니다.

### 요구사항

다음 **5개의 식(Expression)**을 파싱하고, 각각 **올바른 AST 구조**를 생성하는지 확인하세요:

```
【 테스트 식 】

1. a + b + c
   올바른 해석: (a + b) + c  (좌결합)
   AST 구조:
        +
       / \
      +   c
     / \
    a   b

2. a - b - c
   올바른 해석: (a - b) - c  (좌결합)
   AST 구조:
        -
       / \
      -   c
     / \
    a   b

3. a + b * c
   올바른 해석: a + (b * c)  (* 우선순위 높음)
   AST 구조:
        +
       / \
      a   *
         / \
        b   c

4. a * b + c
   올바른 해석: (a * b) + c
   AST 구조:
        +
       / \
      *   c
     / \
    a   b

5. a + b * c - d / e
   올바른 해석: (a + (b * c)) - (d / e)
   AST 구조:
          -
         / \
        +   /
       / \ / \
      a  * d e
        / \
       b   c
```

### 작성 내용

**파일 이름**: `test/parser_precedence_test.zig`

```zig
const std = @import("std");
const testing = std.testing;
const Lexer = @import("../src/lexer/Lexer.zig");
const Parser = @import("../src/parser/Parser.zig");

test "operator precedence: a + b + c (left associative)" {
    var gpa = std.testing.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();

    const source = "fn test() { return a + b + c; }";

    var lexer = Lexer.init(gpa.allocator(), source);
    const tokens = try lexer.tokenize();

    var parser = Parser.init(gpa.allocator(), tokens);
    const ast = try parser.parse();

    // ast.function.body.block.statements[0].return_stmt.value
    // 구조 검증: (a + b) + c

    // TODO: assert tree structure
}

test "operator precedence: a + b * c" {
    // a + (b * c) 구조 검증
}

test "operator precedence: a * b + c - d / e" {
    // 복잡한 식 우선순위 검증
}
```

### 기대 결과물

**체크리스트:**
- [ ] 각 5개 식에 대한 파싱 및 검증
- [ ] 우선순위가 올바르게 반영된 AST 구조
- [ ] 좌결합(Left Associative) 확인
- [ ] 모든 테스트 PASS

**제출물:**
```
zlang-project/
└── test/
    └── parser_precedence_test.zig  ← 여기!
```

---

## 🛡️ Task 4: 파싱 아키텍처와 철학 문서

### 개요

컴파일러 설계에서 가장 중요한 질문 중 하나:

> **파싱 단계에서 타입 검사를 함께 해야 하는가? 아니면 분리해야 하는가?**

이 질문에 대해 본인의 **아키텍처 철학**을 기술합니다.

### 요구사항

다음 **4개 섹션**으로 구성된 **2-3페이지 문서**를 작성하세요:

#### 섹션 1: 두 가지 설계 방식

**방식 A: 일체형(One-Pass Compiler)**
```
Parse + Type Check + Code Generation을 한 번에 처리

장점:
- 메모리 효율적 (AST 저장 불필요)
- 컴파일 속도 빠름

단점:
- 설계 복잡함 (파싱과 타입 검사 혼합)
- 재사용 어려움 (AST 재생성 불가)
- 에러 복구 어려움

예: Pascal, 오래된 C 컴파일러
```

**방식 B: 다단계(Multi-Pass Compiler)**
```
Pass 1: 파싱        → AST 생성
Pass 2: 의미 분석   → 타입 검사, 심볼 테이블
Pass 3: 코드 생성   → LLVM IR

장점:
- 설계 명확함 (각 단계 독립적)
- 재사용 가능 (AST 활용)
- 최적화 용이 (여러 Pass 가능)
- 에러 처리 깔끔함

단점:
- 메모리 사용량 多
- 여러 번 순회 필요

예: GCC, Clang, Rust, Go
```

#### 섹션 2: Z-Lang의 선택

현재 Z-Lang은 **다단계(Multi-Pass) 방식**을 선택했습니다:
- 2.2: 파싱 (AST 생성)
- 2.3: 의미 분석 (타입 검사)
- 2.4+: 코드 생성

**당신이 이 선택을 어떻게 평가하는가?**
- 🔹 찬성하는가? 왜?
- 🔹 반대하는가? 왜?
- 🔹 다른 방식을 제안하는가?

#### 섹션 3: 구체적 사례

다음 코드를 예로 들어 설명하세요:

```z-lang
fn main() {
    var x = "hello";  // 문자열
    var y = x + 10;   // 타입 에러!
    return y;
}
```

**파싱 단계(2.2)에서는:**
- 문법적으로 올바른가? → ✅ YES
- AST를 생성할 수 있는가? → ✅ YES

**의미 분석 단계(2.3)에서:**
- 문자열 + 숫자는 유효한 연산인가? → ❌ NO
- 에러를 보고해야 한다!

**당신의 관점:**
- 파싱 단계에서 이 에러를 잡아야 하는가?
- 아니면 의미 분석 단계에 맡기는가?
- 각 선택의 트레이드오프는?

#### 섹션 4: 최종 결론

다음 항목을 명확히 정의하세요:

```
【 Z-Lang의 컴파일 파이프라인 설계 원칙 】

1. 관심사의 분리 (Separation of Concerns)
   - 파싱의 책임: ___________________
   - 의미 분석의 책임: _______________
   - 코드 생성의 책임: _______________

2. 에러 처리 전략
   - 파싱 에러: 문법 오류 → 즉시 보고
   - 의미 에러: 타입 불일치 → ???
   - 생성 에러: IR 불가능 → ???

3. 재사용 가능성
   - AST를 여러 언어로 컴파일할 수 있는가?
   - 최적화 Pass를 추가하려면?

4. 추후 확장 계획
   - 마이크로 언어 추가?
   - DSL(Domain-Specific Language) 지원?
```

### 작성 내용

**파일 이름**: `docs/PARSING_ARCHITECTURE_PHILOSOPHY.md`

```markdown
# 파싱 아키텍처와 철학

## 1. 두 가지 설계 방식

### 방식 A: 일체형(One-Pass Compiler)

### 방식 B: 다단계(Multi-Pass Compiler)

## 2. Z-Lang의 선택

## 3. 구체적 사례 분석

## 4. 최종 결론: Z-Lang 컴파일 철학

【 핵심 원칙 】
1. 관심사의 분리
2. 에러 처리 전략
3. 재사용 가능성
4. 추후 확장 계획
```

### 기대 결과물

**체크리스트:**
- [ ] 두 가지 설계 방식 명확히 설명
- [ ] Z-Lang의 선택 정당화
- [ ] 구체적 사례 3개 분석
- [ ] 최종 결론: 명확한 아키텍처 원칙 수립
- [ ] 페이지 수: 2-3장
- [ ] 전문적인 기술 문서 형식

**제출물:**
```
zlang-project/
└── docs/
    └── PARSING_ARCHITECTURE_PHILOSOPHY.md  ← 여기!
```

---

## 📊 과제 완성 체크리스트

### Task 1: AST 노드 설계 ✅

- [ ] ASTNode.zig 파일 생성
- [ ] Node union(enum) 정의 (9개 변형)
- [ ] BinaryOperator enum (15+ 연산자)
- [ ] UnaryOperator enum (4+ 연산자)
- [ ] ASTBuilder 헬퍼 함수
- [ ] 메모리 안전성

**예상 줄 수**: 300-400줄

### Task 2: 재귀 강하 파서 ✅

- [ ] Parser.zig 파일 생성
- [ ] 11개 파싱 함수 구현
- [ ] 에러 복구 메커니즘
- [ ] 우선순위 처리
- [ ] 메모리 관리 (allocator)

**예상 줄 수**: 800-1,000줄

### Task 3: 우선순위 검증 ✅

- [ ] 5개 테스트 식 파싱
- [ ] AST 구조 검증
- [ ] 모든 테스트 PASS

**예상 줄 수**: 200-300줄

### Task 4: 아키텍처 철학 ✅

- [ ] 두 가지 설계 방식 비교
- [ ] Z-Lang의 선택 정당화
- [ ] 구체적 사례 분석
- [ ] 최종 결론 및 원칙 수립

**예상 페이지 수**: 2-3장

---

## 🎓 학습 목표

이 과제를 완료하면:

✅ **파싱의 3대 핵심** 이해:
- AST 노드 설계 능력
- 재귀 강하 파서 구현 능력
- 연산자 우선순위 처리 능력

✅ **컴파일러 아키텍처** 설계 능력:
- 다단계 컴파일 파이프라인 이해
- 관심사 분리 원칙 적용
- 에러 처리 전략 수립

✅ **Zig 프로그래밍** 숙련도:
- union(enum) 활용
- 메모리 관리 (allocator)
- 재귀적 함수 설계

---

## 📚 참고 자료

### 파싱 관련 문서
- [Crafting Interpreters - Parsing](https://craftinginterpreters.com/parsing.html)
- [Pratt Parsing](https://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/)

### Zig 관련
- [Zig Documentation](https://ziglang.org/documentation/0.12.0/)
- [union(enum) in Zig](https://ziglang.org/documentation/0.12.0/#Tagged-union)

---

## 🚀 제출 요구사항

### 파일 구조

```
zlang-project/
├── src/
│   └── parser/
│       ├── ASTNode.zig            ← Task 1
│       └── Parser.zig              ← Task 2
├── test/
│   └── parser_precedence_test.zig  ← Task 3
└── docs/
    └── PARSING_ARCHITECTURE_PHILOSOPHY.md  ← Task 4
```

### 최소 요구사항

- [ ] 모든 4개 Task 완성
- [ ] 코드: 1,300+ 줄
- [ ] 문서: 2-3장
- [ ] 테스트: 모두 PASS
- [ ] gogs 저장소에 커밋

### 추가 가산점

- ⭐ 에러 메시지의 상세함 (파일명:줄:열 형식)
- ⭐ AST 시각화 함수 (pretty-print)
- ⭐ Pratt Parsing 추가 구현
- ⭐ 통합 테스트 (렉싱 → 파싱 → 검증)

---

## 💡 팁과 조언

### 디버깅

파서 디버깅 시 AST를 시각화하면 도움이 됩니다:

```
입력: fn test() { return a + b * c; }

토큰: [keyword_fn, identifier, lparen, ...]

파싱 과정:
  parseFunction()
    parseBlock()
      parseStatement()
        parseReturn()
          parseExpression()
            parseAdditive()
              → a (좌측)
              → +
              → parseMultiplicative()
                → b * c (우측)

결과:
        +
       / \
      a   *
         / \
        b   c
```

### 메모리 관리

```zig
// ✅ 올바른 사용
var builder = ASTBuilder.init(allocator);
var node = try builder.number(42);
// allocator가 메모리 관리

// ❌ 피해야 할 패턴
var node = std.ArrayList(Node).init(allocator);
// defer cleanup 빼먹으면 메모리 누수!
```

---

**행운을 빕니다! 당신의 언어가 깊이를 갖춥니다.** 🌳

*"구문의 기록이 곧 실행의 지침이 됩니다."* 🎯
