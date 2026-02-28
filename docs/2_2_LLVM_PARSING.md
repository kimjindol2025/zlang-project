# 🏛️ LLVM 전설: 2.2 구문 분석(Parsing) - 코드의 계보, AST 설계

> **최종 업데이트**: 2026-02-26
> **상태**: ✅ 완성 (2.1 렉싱 → 2.2 파싱)
> **난이도**: ⭐⭐⭐⭐⭐ (최고급 - 컴파일러의 영혼!)

---

## 🎯 2.2 단계의 핵심 개념

### 렉싱 vs 파싱: 언어의 두 가지 계층

```
【 언어 이해의 두 단계 】

1단계: 렉싱 (2.1 완료)
  "fn add(a, b) { return a + b; }"
         ↓
  [keyword_fn, identifier(add), lparen, identifier(a), comma, identifier(b), ...]
  └─ 단어(Token)의 나열만 파악

2단계: 파싱 (2.2 - 여기!)
  [keyword_fn, identifier(add), lparen, ...]
         ↓
  Program {
    FunctionDef {
      name: "add",
      params: [Parameter("a"), Parameter("b")],
      body: Block {
        Statement::Return {
          expr: BinaryOp {
            op: Add,
            left: Variable("a"),
            right: Variable("b")
          }
        }
      }
    }
  }
  └─ **구조(Structure)와 의미(Semantics)** 파악
```

### 파서의 책임

파서는 **문법(Grammar) 규칙**을 적용하여:
- ✅ 토큰이 올바른 순서로 배열되었는가?
- ✅ 기대하는 토큰이 실제로 나타났는가?
- ✅ 중첩된 구조(블록, 함수, 호출)가 올바른가?
- ✅ 연산자의 우선순위가 정확한가?

를 검증하고, **트리 구조(AST)**로 변환합니다.

---

## 📚 1. 추상 구문 트리(AST, Abstract Syntax Tree)

### AST의 정의

> **추상 구문 트리(AST)**는 소스 코드의 **의미를 계층적으로 표현**한 나무 구조입니다.
> - **추상(Abstract)**: 세미콜론, 괄호 등 문법적 디테일은 제거
> - **구문(Syntax)**: 코드의 구조적 형태
> - **트리(Tree)**: 부모-자식 관계로 표현

### 예제: `a + b * c` 파싱

일반인 관점:
```
a + b * c = ?
```

토큰 관점 (2.1 렉싱):
```
[identifier(a), plus, identifier(b), mul, identifier(c)]
```

AST 관점 (2.2 파싱) - **핵심은 우선순위!**:
```
        +
       / \
      a   *
         / \
        b   c
```

**왜 이 구조?** 곱하기가 더하기보다 아래에 있으므로, 계산 시 곱하기가 먼저 실행됩니다:
```
a + (b * c)  ← 올바른 해석!
```

### AST 노드의 종류

```
【 AST 노드 분류 】

1. 리터럴 (Literal)
   └─ 숫자: 42, 3.14
   └─ 문자열: "hello"
   └─ 부울: true, false

2. 변수/식별자 (Variable)
   └─ 이름: x, count, result

3. 이항 연산 (Binary Operation)
   └─ +, -, *, /, %, ==, !=, <, >, ...

4. 단항 연산 (Unary Operation)
   └─ !, -, +

5. 함수 호출 (Function Call)
   └─ foo(x, y, z)
   └─ print("result")

6. 함수 정의 (Function Definition)
   └─ fn add(a, b) { return a + b; }

7. 변수 선언 (Variable Declaration)
   └─ var x = 10;
   └─ let name = "Alice";

8. 제어 흐름 (Control Flow)
   └─ if-else, while, for

9. 블록 (Block)
   └─ { statement1; statement2; ... }
```

---

## 🔧 2. Zig를 이용한 AST 노드 설계

### 2.1 AST 노드 기본 구조

Zig의 `union(enum)`은 서로 다른 형태의 노드를 하나의 타입으로 묶는 완벽한 도구입니다.

```zig
const std = @import("std");
const Allocator = std.mem.Allocator;

// 【 AST 노드 타입 정의 】
pub const Node = union(enum) {
    // 1. 리터럴
    number: f64,
    string: []const u8,
    boolean: bool,

    // 2. 변수
    identifier: []const u8,

    // 3. 이항 연산
    binary_op: struct {
        op: BinaryOperator,
        left: *Node,
        right: *Node,
    },

    // 4. 단항 연산
    unary_op: struct {
        op: UnaryOperator,
        operand: *Node,
    },

    // 5. 함수 호출
    call: struct {
        callee: []const u8,
        args: std.ArrayList(*Node),
    },

    // 6. 함수 정의
    function: struct {
        name: []const u8,
        params: std.ArrayList(Parameter),
        body: *Node,
    },

    // 7. 변수 선언
    var_decl: struct {
        name: []const u8,
        var_type: ?[]const u8,  // null = type inference
        init_expr: ?*Node,       // null = uninitialized
    },

    // 8. 블록
    block: struct {
        statements: std.ArrayList(*Node),
    },

    // 9. if 문
    if_stmt: struct {
        condition: *Node,
        then_branch: *Node,
        else_branch: ?*Node,
    },

    // 10. while 루프
    while_stmt: struct {
        condition: *Node,
        body: *Node,
    },

    // 11. return 문
    return_stmt: struct {
        value: ?*Node,  // null = return;
    },
};

pub const Parameter = struct {
    name: []const u8,
    param_type: []const u8,
};

pub const BinaryOperator = enum {
    // 산술 연산
    add,        // +
    sub,        // -
    mul,        // *
    div,        // /
    mod,        // %
    pow,        // ^

    // 비교 연산
    eq,         // ==
    ne,         // !=
    lt,         // <
    le,         // <=
    gt,         // >
    ge,         // >=

    // 논리 연산
    @"and",     // &&
    @"or",      // ||

    // 비트 연산
    bit_and,    // &
    bit_or,     // |
    bit_xor,    // ^
};

pub const UnaryOperator = enum {
    not,        // !
    neg,        // - (음수)
    pos,        // + (양수)
    bit_not,    // ~
};
```

### 2.2 AST 생성 헬퍼 함수

메모리 관리와 노드 생성을 간편하게 하기 위한 헬퍼:

```zig
pub const ASTBuilder = struct {
    allocator: Allocator,

    pub fn init(allocator: Allocator) ASTBuilder {
        return .{ .allocator = allocator };
    }

    // 노드 메모리 할당
    pub fn allocNode(self: *const ASTBuilder) !*Node {
        return try self.allocator.create(Node);
    }

    // 리터럴 노드
    pub fn number(self: *const ASTBuilder, value: f64) !*Node {
        var node = try self.allocNode();
        node.* = .{ .number = value };
        return node;
    }

    pub fn string(self: *const ASTBuilder, value: []const u8) !*Node {
        var node = try self.allocNode();
        node.* = .{ .string = value };
        return node;
    }

    pub fn identifier(self: *const ASTBuilder, name: []const u8) !*Node {
        var node = try self.allocNode();
        node.* = .{ .identifier = name };
        return node;
    }

    // 이항 연산 노드
    pub fn binaryOp(
        self: *const ASTBuilder,
        op: BinaryOperator,
        left: *Node,
        right: *Node,
    ) !*Node {
        var node = try self.allocNode();
        node.* = .{
            .binary_op = .{
                .op = op,
                .left = left,
                .right = right,
            },
        };
        return node;
    }

    // 함수 호출 노드
    pub fn call(
        self: *const ASTBuilder,
        callee: []const u8,
        args: std.ArrayList(*Node),
    ) !*Node {
        var node = try self.allocNode();
        node.* = .{
            .call = .{
                .callee = callee,
                .args = args,
            },
        };
        return node;
    }

    // 블록 노드
    pub fn block(self: *const ASTBuilder) !*Node {
        var node = try self.allocNode();
        node.* = .{
            .block = .{
                .statements = std.ArrayList(*Node).init(self.allocator),
            },
        };
        return node;
    }
};
```

---

## 🎬 3. 재귀 강하 파서(Recursive Descent Parser) 구현

### 3.1 파서의 기본 구조

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

    // 1. 현재 토큰 조회
    fn current_token(self: *const Parser) Token {
        if (self.current < self.tokens.len) {
            return self.tokens[self.current];
        }
        return .{ .tag = .eof, .lexeme = "", .line = 0, .column = 0 };
    }

    // 2. 다음 토큰 미리보기 (Lookahead)
    fn peek_token(self: *const Parser, offset: usize) Token {
        if (self.current + offset < self.tokens.len) {
            return self.tokens[self.current + offset];
        }
        return .{ .tag = .eof, .lexeme = "", .line = 0, .column = 0 };
    }

    // 3. 토큰 소비 (Advance)
    fn advance(self: *Parser) Token {
        const token = self.current_token();
        if (self.current < self.tokens.len) {
            self.current += 1;
        }
        return token;
    }

    // 4. 기대하는 토큰 확인
    fn expect(self: *Parser, expected: TokenType) !Token {
        const token = self.current_token();
        if (token.tag != expected) {
            try self.error_recovery(expected, token);
            return error.UnexpectedToken;
        }
        return self.advance();
    }

    // 5. 에러 보고
    fn addError(self: *Parser, message: []const u8, token: Token) !void {
        try self.errors.append(.{
            .message = message,
            .line = token.line,
            .column = token.column,
        });
    }
};

pub const ParseError = struct {
    message: []const u8,
    line: u32,
    column: u32,
};
```

### 3.2 문법 규칙을 함수로 구현

파서의 핵심 아이디어: **문법의 각 규칙마다 하나의 함수를 작성**

```
【 Z-Lang 문법 (BNF 형태) 】

program     ::= function*
function    ::= "fn" identifier "(" params ")" "{" block "}"
params      ::= (identifier ":" type ("," identifier ":" type)*)?
block       ::= statement*
statement   ::= var_decl | return_stmt | expr_stmt | if_stmt | while_stmt | block
var_decl    ::= ("var" | "let") identifier (":" type)? "=" expr ";"
return_stmt ::= "return" expr? ";"
expr_stmt   ::= expr ";"
if_stmt     ::= "if" expr "{" block "}" ("else" "{" block "}")?
while_stmt  ::= "while" expr "{" block "}"
expr        ::= assignment
assignment  ::= equality ("=" equality)*
equality    ::= comparison (("==" | "!=") comparison)*
comparison  ::= additive (("<" | "<=" | ">" | ">=") additive)*
additive    ::= multiplicative (("+" | "-") multiplicative)*
multiplicative ::= unary (("*" | "/" | "%") unary)*
unary       ::= ("!" | "-" | "+") unary | postfix
postfix     ::= primary (call)*
call        ::= "(" args ")"
args        ::= (expr ("," expr)*)?
primary     ::= number | string | identifier | "(" expr ")"
```

### 3.3 파서 함수 구현 (재귀 강하)

```zig
pub const Parser = struct {
    // ... (이전 필드들)

    // 【 Top-level 파싱 】
    pub fn parse(self: *Parser) !*Node {
        var statements = std.ArrayList(*Node).init(self.allocator);

        while (self.current_token().tag != .eof) {
            if (self.current_token().tag == .keyword_fn) {
                try statements.append(try self.parseFunction());
            } else {
                try self.addError("Expected function definition", self.current_token());
                self.advance();
            }
        }

        var root = try self.builder.block();
        root.block.statements = statements;
        return root;
    }

    // 【 함수 정의 파싱 】
    fn parseFunction(self: *Parser) !*Node {
        _ = try self.expect(.keyword_fn);

        const name_token = try self.expect(.identifier);
        const name = name_token.lexeme;

        _ = try self.expect(.lparen);

        // 파라미터 파싱
        var params = std.ArrayList(Parameter).init(self.allocator);

        while (self.current_token().tag != .rparen) {
            if (params.items.len > 0) {
                _ = try self.expect(.comma);
            }

            const param_name = try self.expect(.identifier);
            _ = try self.expect(.colon);
            const param_type = try self.expect(.identifier);

            try params.append(.{
                .name = param_name.lexeme,
                .param_type = param_type.lexeme,
            });
        }

        _ = try self.expect(.rparen);

        // 함수 본문 (블록) 파싱
        const body = try self.parseBlock();

        var node = try self.builder.allocNode();
        node.* = .{
            .function = .{
                .name = name,
                .params = params,
                .body = body,
            },
        };
        return node;
    }

    // 【 블록 파싱 】
    fn parseBlock(self: *Parser) !*Node {
        _ = try self.expect(.lbrace);

        var statements = std.ArrayList(*Node).init(self.allocator);

        while (self.current_token().tag != .rbrace and
               self.current_token().tag != .eof) {
            try statements.append(try self.parseStatement());
        }

        _ = try self.expect(.rbrace);

        var node = try self.builder.allocNode();
        node.* = .{
            .block = .{
                .statements = statements,
            },
        };
        return node;
    }

    // 【 문장 파싱 】
    fn parseStatement(self: *Parser) !*Node {
        return switch (self.current_token().tag) {
            .keyword_var, .keyword_let => try self.parseVarDecl(),
            .keyword_return => try self.parseReturn(),
            .keyword_if => try self.parseIf(),
            .keyword_while => try self.parseWhile(),
            .lbrace => try self.parseBlock(),
            else => blk: {
                const expr = try self.parseExpression();
                _ = try self.expect(.semicolon);
                break :blk expr;
            },
        };
    }

    // 【 변수 선언 파싱 】
    fn parseVarDecl(self: *Parser) !*Node {
        const is_let = self.current_token().tag == .keyword_let;
        _ = self.advance(); // 'var' 또는 'let' 소비

        const name_token = try self.expect(.identifier);

        var var_type: ?[]const u8 = null;
        if (self.current_token().tag == .colon) {
            _ = self.advance();
            const type_token = try self.expect(.identifier);
            var_type = type_token.lexeme;
        }

        _ = try self.expect(.assign);

        const init_expr = try self.parseExpression();
        _ = try self.expect(.semicolon);

        var node = try self.builder.allocNode();
        node.* = .{
            .var_decl = .{
                .name = name_token.lexeme,
                .var_type = var_type,
                .init_expr = init_expr,
            },
        };
        return node;
    }

    // 【 return 문 파싱 】
    fn parseReturn(self: *Parser) !*Node {
        _ = try self.expect(.keyword_return);

        var value: ?*Node = null;
        if (self.current_token().tag != .semicolon) {
            value = try self.parseExpression();
        }

        _ = try self.expect(.semicolon);

        var node = try self.builder.allocNode();
        node.* = .{
            .return_stmt = .{
                .value = value,
            },
        };
        return node;
    }

    // 【 if 문 파싱 】
    fn parseIf(self: *Parser) !*Node {
        _ = try self.expect(.keyword_if);

        const condition = try self.parseExpression();

        const then_branch = try self.parseBlock();

        var else_branch: ?*Node = null;
        if (self.current_token().tag == .keyword_else) {
            _ = self.advance();
            else_branch = try self.parseBlock();
        }

        var node = try self.builder.allocNode();
        node.* = .{
            .if_stmt = .{
                .condition = condition,
                .then_branch = then_branch,
                .else_branch = else_branch,
            },
        };
        return node;
    }

    // 【 while 루프 파싱 】
    fn parseWhile(self: *Parser) !*Node {
        _ = try self.expect(.keyword_while);

        const condition = try self.parseExpression();
        const body = try self.parseBlock();

        var node = try self.builder.allocNode();
        node.* = .{
            .while_stmt = .{
                .condition = condition,
                .body = body,
            },
        };
        return node;
    }

    // 【 식(Expression) 파싱 】
    fn parseExpression(self: *Parser) !*Node {
        return try self.parseEquality();
    }

    fn parseEquality(self: *Parser) !*Node {
        var node = try self.parseComparison();

        while (true) {
            switch (self.current_token().tag) {
                .eq => {
                    _ = self.advance();
                    const right = try self.parseComparison();
                    node = try self.builder.binaryOp(.eq, node, right);
                },
                .ne => {
                    _ = self.advance();
                    const right = try self.parseComparison();
                    node = try self.builder.binaryOp(.ne, node, right);
                },
                else => break,
            }
        }

        return node;
    }

    fn parseComparison(self: *Parser) !*Node {
        var node = try self.parseAdditive();

        while (true) {
            switch (self.current_token().tag) {
                .lt => {
                    _ = self.advance();
                    const right = try self.parseAdditive();
                    node = try self.builder.binaryOp(.lt, node, right);
                },
                .le => {
                    _ = self.advance();
                    const right = try self.parseAdditive();
                    node = try self.builder.binaryOp(.le, node, right);
                },
                .gt => {
                    _ = self.advance();
                    const right = try self.parseAdditive();
                    node = try self.builder.binaryOp(.gt, node, right);
                },
                .ge => {
                    _ = self.advance();
                    const right = try self.parseAdditive();
                    node = try self.builder.binaryOp(.ge, node, right);
                },
                else => break,
            }
        }

        return node;
    }

    fn parseAdditive(self: *Parser) !*Node {
        var node = try self.parseMultiplicative();

        while (true) {
            switch (self.current_token().tag) {
                .plus => {
                    _ = self.advance();
                    const right = try self.parseMultiplicative();
                    node = try self.builder.binaryOp(.add, node, right);
                },
                .minus => {
                    _ = self.advance();
                    const right = try self.parseMultiplicative();
                    node = try self.builder.binaryOp(.sub, node, right);
                },
                else => break,
            }
        }

        return node;
    }

    fn parseMultiplicative(self: *Parser) !*Node {
        var node = try self.parseUnary();

        while (true) {
            switch (self.current_token().tag) {
                .star => {
                    _ = self.advance();
                    const right = try self.parseUnary();
                    node = try self.builder.binaryOp(.mul, node, right);
                },
                .slash => {
                    _ = self.advance();
                    const right = try self.parseUnary();
                    node = try self.builder.binaryOp(.div, node, right);
                },
                .percent => {
                    _ = self.advance();
                    const right = try self.parseUnary();
                    node = try self.builder.binaryOp(.mod, node, right);
                },
                else => break,
            }
        }

        return node;
    }

    fn parseUnary(self: *Parser) !*Node {
        return switch (self.current_token().tag) {
            .bang => {
                _ = self.advance();
                const operand = try self.parseUnary();
                return try self.builder.allocNode();
                // node.* = .{ .unary_op = .{ .op = .not, .operand = operand } };
            },
            .minus => {
                _ = self.advance();
                const operand = try self.parseUnary();
                var node = try self.builder.allocNode();
                node.* = .{
                    .unary_op = .{
                        .op = .neg,
                        .operand = operand,
                    },
                };
                return node;
            },
            else => try self.parsePostfix(),
        };
    }

    fn parsePostfix(self: *Parser) !*Node {
        var node = try self.parsePrimary();

        while (self.current_token().tag == .lparen) {
            _ = self.advance(); // '(' 소비

            var args = std.ArrayList(*Node).init(self.allocator);

            while (self.current_token().tag != .rparen) {
                if (args.items.len > 0) {
                    _ = try self.expect(.comma);
                }
                try args.append(try self.parseExpression());
            }

            _ = try self.expect(.rparen);

            // identifier 추출 (postfix로는 함수 호출만)
            if (node.* == .identifier) {
                const func_name = node.identifier;
                var call_node = try self.builder.allocNode();
                call_node.* = .{
                    .call = .{
                        .callee = func_name,
                        .args = args,
                    },
                };
                node = call_node;
            }
        }

        return node;
    }

    fn parsePrimary(self: *Parser) !*Node {
        return switch (self.current_token().tag) {
            .number => blk: {
                const token = self.advance();
                const value = try std.fmt.parseFloat(f64, token.lexeme);
                break :blk try self.builder.number(value);
            },
            .string => blk: {
                const token = self.advance();
                break :blk try self.builder.string(token.lexeme);
            },
            .identifier => blk: {
                const token = self.advance();
                break :blk try self.builder.identifier(token.lexeme);
            },
            .lparen => blk: {
                _ = self.advance();
                const expr = try self.parseExpression();
                _ = try self.expect(.rparen);
                break :blk expr;
            },
            else => {
                try self.addError(
                    "Expected primary expression",
                    self.current_token(),
                );
                return error.UnexpectedToken;
            },
        };
    }
};
```

---

## 🔢 4. 연산자 우선순위(Operator Precedence)

### 4.1 연산자 우선순위 이해

```
【 수학적 우선순위 】

우선순위 (높음 → 낮음):
  1. Unary       (-x, !x, +x)
  2. Power       (^)
  3. Multiply    (*, /, %)
  4. Add         (+, -)
  5. Compare     (<, >, <=, >=)
  6. Equality    (==, !=)
  7. Logical AND (&&)
  8. Logical OR  (||)
  9. Assignment  (=)

예제: 1 + 2 * 3 ^ 2
     = 1 + 2 * (3 ^ 2)
     = 1 + 2 * 9
     = 1 + 18
     = 19
```

### 4.2 재귀 강하 파서의 우선순위 처리

**핵심 원칙**: 낮은 우선순위의 연산자를 파싱하는 함수가 **더 높은 위치**에 있습니다!

```zig
// 우선순위가 낮을수록 상위 함수
parseExpression()      // 우선순위 0 (가장 낮음)
  └─ parseEquality()   // 우선순위 1
      └─ parseComparison()  // 우선순위 2
          └─ parseAdditive() // 우선순위 3
              └─ parseMultiplicative() // 우선순위 4
                  └─ parseUnary() // 우선순위 5
                      └─ parsePostfix() // 우선순위 6
                          └─ parsePrimary() // 우선순위 7 (가장 높음)
```

**이유?** 재귀 호출이 깊을수록 먼저 계산됩니다.

예: `1 + 2 * 3` 파싱 흐름
```
parseAdditive()
  → parsePrimary() = 1
  → 토큰 = "+"
  → parsePrimary() 호출... 아니, parseMultiplicative() 호출!
    → parseMultiplicative()
      → parsePrimary() = 2
      → 토큰 = "*"
      → parseUnary() = 3
      → BinaryOp(*, 2, 3) 완성 ← 곱하기가 먼저!
  → BinaryOp(+, 1, BinaryOp(*, 2, 3)) ← 올바른 구조!
```

---

## ⚡ 5. Pratt Parsing: 고급 우선순위 처리

대규모 언어에서는 **Pratt Parsing**(또는 **Precedence Climbing**)을 사용합니다.

### 5.1 Pratt 파싱의 원리

각 토큰에 **결합력(Binding Power)** 숫자를 부여:

```zig
const BindingPower = struct {
    left: i32,
    right: i32,
};

const precedence = std.AutoHashMap([]const u8, BindingPower).init(allocator);

try precedence.put("||", .{ .left = 1, .right = 2 });
try precedence.put("&&", .{ .left = 3, .right = 4 });
try precedence.put("==", .{ .left = 5, .right = 6 });
try precedence.put("!=", .{ .left = 5, .right = 6 });
try precedence.put("<", .{ .left = 7, .right = 8 });
try precedence.put(">", .{ .left = 7, .right = 8 });
try precedence.put("+", .{ .left = 9, .right = 10 });
try precedence.put("-", .{ .left = 9, .right = 10 });
try precedence.put("*", .{ .left = 11, .right = 12 });
try precedence.put("/", .{ .left = 11, .right = 12 });
```

### 5.2 Pratt 파서 구현

```zig
fn parseExpression(self: *Parser, min_precedence: i32) !*Node {
    var left = try self.parsePrimary();

    while (true) {
        const op = self.current_token().lexeme;
        const bp = self.precedence.get(op) orelse break;

        if (bp.left < min_precedence) break;

        _ = self.advance(); // 연산자 소비

        const right = try self.parseExpression(bp.right);
        left = try self.builder.binaryOp(op, left, right);
    }

    return left;
}
```

**장점:**
- ✅ 더 유연함 (새 연산자 추가 용이)
- ✅ 좌 재귀 자동 처리
- ✅ 복잡한 언어에 적합

---

## 🛡️ 6. 에러 복구(Error Recovery)

파서가 에러를 만났을 때, **완전히 중단하지 않고** 계속 파싱하는 기법입니다.

### 6.1 에러 복구 전략

```zig
fn errorRecovery(self: *Parser, expected: TokenType) !void {
    try self.addError("Expected token", self.current_token());

    // 전략 1: 기대하는 토큰까지 건너뛰기
    while (self.current_token().tag != expected and
           self.current_token().tag != .eof) {
        self.advance();
    }

    // 전략 2: 동기화 포인트 찾기
    // (블록 끝, 문장 끝 등)
    if (self.current_token().tag == expected) {
        self.advance();
    }
}
```

### 6.2 실제 예제

```
입력:  fn add(a b) { return a + b; }
             ↑ 쉼표 빠짐!

파서:  "쉼마를 기대했는데 identifier 'b'가 나왔다"
       → 에러 기록
       → b를 쉼마로 취급하고 계속
       → 파싱 완료, 에러 목록 반환

결과:  fn add(a, b) { return a + b; }  ← 수정된 것처럼 처리
```

---

## 🌳 7. AST 순회 및 출력

### 7.1 AST 프린터

```zig
pub const ASTPrinter = struct {
    allocator: Allocator,
    depth: usize = 0,

    pub fn init(allocator: Allocator) ASTPrinter {
        return .{ .allocator = allocator };
    }

    pub fn print(self: *ASTPrinter, node: *const Node) !void {
        const indent = try self.allocator.alloc(u8, self.depth * 2);
        defer self.allocator.free(indent);

        @memset(indent, ' ');

        switch (node.*) {
            .number => |n| {
                std.debug.print("{s}Number({})\n", .{ indent, n });
            },
            .identifier => |id| {
                std.debug.print("{s}Identifier({})\n", .{ indent, id });
            },
            .binary_op => |op| {
                std.debug.print("{s}BinaryOp({})\n", .{ indent, @tagName(op.op) });
                self.depth += 1;
                try self.print(op.left);
                try self.print(op.right);
                self.depth -= 1;
            },
            .function => |func| {
                std.debug.print("{s}Function({})\n", .{ indent, func.name });
                self.depth += 1;
                try self.print(func.body);
                self.depth -= 1;
            },
            .block => |blk| {
                std.debug.print("{s}Block\n", .{indent});
                self.depth += 1;
                for (blk.statements.items) |stmt| {
                    try self.print(stmt);
                }
                self.depth -= 1;
            },
            // ... 다른 노드 타입들
        }
    }
};
```

---

## 📊 8. 파싱 과정 시각화

### 8.1 완전한 파싱 예제: `result(x) { return x * 2 + 1; }`

**단계 1: 렉싱 (2.1 완료)**
```
[keyword_fn, identifier(result), lparen, identifier(x), rparen,
 lbrace, keyword_return, identifier(x), star, number(2), plus,
 number(1), semicolon, rbrace]
```

**단계 2: 파싱 (2.2 - 여기!)**
```
parseFunction()
  ├─ name = "result"
  ├─ params = ["x"]
  └─ body = parseBlock()
      └─ statements = [parseReturn()]
          └─ value = parseExpression()
              └─ parseAdditive()
                  ├─ left = parseMultiplicative()
                  │   ├─ left = identifier(x)
                  │   ├─ op = *
                  │   └─ right = number(2)
                  ├─ op = +
                  └─ right = number(1)
```

**단계 3: AST 구조**
```
Function("result")
  └─ Block
      └─ Return
          └─ BinaryOp(+)
              ├─ BinaryOp(*)
              │   ├─ Identifier(x)
              │   └─ Number(2)
              └─ Number(1)
```

**시각화:**
```
           result()
             │
           Block
             │
           Return
             │
             +         ← 덧셈이 위에 있음 = 나중에 실행
            / \
           *   1     ← 곱하기가 아래 = 먼저 실행
          / \
         x   2
```

---

## 💾 9. AST의 활용: 다음 단계로의 연결

### 9.1 AST → 의미 분석 (2.3)

```zig
// 파싱 단계 (2.2)에서 생성된 AST
let ast = try parser.parse();

// 의미 분석 (2.3)
let type_checker = TypeChecker.init(allocator);
try type_checker.check(ast);

// 타입 검증:
// - x의 타입은? (함수 매개변수 → 매개변수 타입 확인)
// - x * 2의 타입은? (곱하기 연산자의 양쪽이 숫자형인가?)
// - (x * 2) + 1의 타입은? (덧셈의 양쪽이 숫자형인가?)
```

### 9.2 AST → 코드 생성 (2.4)

```zig
// 파싱 단계 (2.2)에서 생성된 AST
let ast = try parser.parse();

// 의미 분석 (2.3)
try type_checker.check(ast);

// 코드 생성 (2.4)
let codegen = CodeGenerator.init(allocator);
let ir_module = try codegen.generate(ast);

// IR 생성:
// %0 = load i32, i32* %x
// %1 = mul i32 %0, 2
// %2 = add i32 %1, 1
// ret i32 %2
```

---

## 🧪 10. 파서 테스트 전략

### 10.1 단위 테스트

```zig
pub fn testBasicExpression() !void {
    var gpa = std.testing.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();

    const tokens = [_]Token{
        .{ .tag = .number, .lexeme = "1", .line = 1, .column = 1 },
        .{ .tag = .plus, .lexeme = "+", .line = 1, .column = 3 },
        .{ .tag = .number, .lexeme = "2", .lexeme = "2", .line = 1, .column = 5 },
        .{ .tag = .eof, .lexeme = "", .line = 1, .column = 6 },
    };

    var parser = Parser.init(gpa.allocator(), &tokens);
    const ast = try parser.parse();

    try std.testing.expect(ast.* == .binary_op);
    try std.testing.expect(ast.binary_op.op == .add);
}

pub fn testFunctionDefinition() !void {
    // fn add(a: i32, b: i32) { return a + b; }
    const tokens = [_]Token{
        .{ .tag = .keyword_fn, .lexeme = "fn", .line = 1, .column = 1 },
        .{ .tag = .identifier, .lexeme = "add", .line = 1, .column = 4 },
        // ... 나머지 토큰들
    };

    var parser = Parser.init(gpa.allocator(), &tokens);
    const ast = try parser.parse();

    try std.testing.expect(ast.* == .function);
    try std.testing.expectEqualSlices(u8, ast.function.name, "add");
}
```

### 10.2 통합 테스트

```zig
pub fn testFullProgram() !void {
    const source =
        \\ fn main() {
        \\   var x = 10;
        \\   return x * 2;
        \\ }
    ;

    // 렉싱 (2.1)
    var lexer = Lexer.init(allocator, source);
    const tokens = try lexer.tokenize();

    // 파싱 (2.2)
    var parser = Parser.init(allocator, tokens);
    const ast = try parser.parse();

    // 검증
    try std.testing.expect(ast.* == .function);
}
```

---

## 🎓 요약: 파싱의 세 가지 핵심

| 개념 | 설명 | 예제 |
|------|------|------|
| **AST** | 소스 코드의 의미를 계층적으로 표현 | `a + b * c` → `+(a, *(b, c))` |
| **재귀 강하** | 문법의 각 규칙을 함수로 구현하여 재귀적으로 파싱 | `parseExpression() → parseEquality() → ...` |
| **우선순위** | 연산자의 계산 순서를 함수의 깊이로 표현 | 낮은 우선순위 = 얕은 깊이 |

---

## 🚀 다음 단계: 2.3 의미 분석(Semantic Analysis)

이제 우리는:
- ✅ **단어(Token)**를 인식했고 (2.1)
- ✅ **구조(Structure)**를 파악했으며 (2.2)

다음에는:
- 🔜 **의미(Semantics)**를 검증합니다 (2.3)

타입 검사, 심볼 테이블, 변수 범위(Scope) 분석이 이루어집니다.

---

**지금까지의 여정**:
```
【 컴파일러 파이프라인 】

2.1 렉싱          ✅ 완료
  "fn add(a) {}" → [keyword_fn, identifier, ...]

2.2 파싱          ✅ 완료 (여기!)
  [tokens] → AST (나무 구조)

2.3 의미 분석    🔜 다음
  AST → 타입 검증, 심볼 테이블

2.4 코드 생성    🔜 이후
  AST → LLVM IR 명령어

실행 (1.5)       ✅ 완료
  LLVM IR → 기계어 → CPU 실행
```

---

**최종 메시지**:

> **"당신의 언어가 드디어 깊이를 갖춥니다!"** 🌳
>
> - 2.1: 단어 (Tokens)
> - 2.2: 구조 (Syntax Tree) ← 여기!
> - 2.3: 의미 (Semantics)
> - 2.4+: 실행 (Execution)

**철학**: *"구문의 기록이 곧 실행의 지침"* 🎯

---

*"AST는 컴파일러의 영혼입니다. 이 나무를 그리는 순간, 코드의 의도가 명확해집니다."* 🏛️
