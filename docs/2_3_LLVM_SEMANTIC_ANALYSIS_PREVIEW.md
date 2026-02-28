# 🔬 Preview: LLVM 2.3 - 의미 분석(Semantic Analysis) & 심볼 테이블

> **최종 업데이트**: 2026-02-26
> **상태**: 🔜 준비 중 (2주 후 공개)
> **기대도**: ⭐⭐⭐⭐⭐ (컴파일러의 두뇌!)

---

## 🎯 2.3 단계의 핵심

### 문법에서 의미로: 컴파일러가 지능을 갖추다

```
【 컴파일러 인텔리전스의 계층 】

2.1 렉싱 (Lexing)           ← 기계적 토큰화
    "fn add(a, b) { ... }"
         ↓
    [keyword_fn, identifier, lparen, ...]

2.2 파싱 (Parsing)          ← 구조 파악
    [tokens]
         ↓
    Function {
      name: "add",
      params: [...],
      body: Block { ... }
    }

2.3 의미 분석 (Semantic)    ← 지능 검증! 🧠
    AST
         ↓
    ✓ 변수 'a'는 정의되었나?
    ✓ 변수 'b'는 정의되었나?
    ✓ a + b의 타입은 유효한가?
    ✓ return 타입은 함수 선언과 일치하나?
    ✓ 모든 코드 경로에서 return이 있나?
         ↓
    Symbol Table {
      a: (type: i32, scope: function_add),
      b: (type: i32, scope: function_add),
      ...
    }
```

---

## 📚 2.3에서 배울 것

### 1. 심볼 테이블(Symbol Table)

컴파일러가 **모든 변수, 함수, 타입을 기억**하는 데이터 구조입니다.

```zig
pub const Symbol = struct {
    name: []const u8,
    symbol_type: SymbolType,  // Variable, Function, Type, ...
    data_type: ?[]const u8,   // i32, f64, string, ...
    scope: Scope,             // Global, FunctionLocal, BlockLocal
    defined: bool,            // 정의 완료?
    initialized: bool,        // 초기화 완료?
};

pub const SymbolTable = struct {
    scopes: std.ArrayList(Scope),
    current_scope: usize,

    pub fn push_scope(self: *SymbolTable) !void { }
    pub fn pop_scope(self: *SymbolTable) !void { }
    pub fn define(self: *SymbolTable, symbol: Symbol) !void { }
    pub fn lookup(self: *const SymbolTable, name: []const u8) ?Symbol { }
};
```

### 2. 스코프(Scope) - 변수의 범위

```z-lang
fn outer() {
    var x = 10;           // Global Scope in outer()

    fn inner() {
        var y = 20;       // Function Scope (inner)
        {
            var z = 30;   // Block Scope
            print(x);     // ✓ x는 접근 가능 (부모 스코프)
            print(y);     // ✓ y는 접근 가능 (함수 스코프)
            print(z);     // ✓ z는 접근 가능 (현재 스코프)
        }
        print(z);         // ✗ 에러! z는 블록 범위 밖
    }
}
```

**스코프 계층:**
```
Global Scope
  ↓
Function "outer" Scope
  ├─ Variable x
  ↓
  Function "inner" Scope
    ├─ Variable y
    ↓
    Block Scope
      └─ Variable z  ← 블록 끝나면 범위 벗어남
```

### 3. 타입 검사(Type Checking)

```z-lang
fn test() {
    var x: i32 = 10;
    var y: string = "hello";

    var z = x + y;      // ✗ 에러! i32 + string은 불가능
    var w = x + 20;     // ✓ OK: i32 + i32 = i32
    var s = y + "world"; // ✓ OK: string + string = string
}
```

**타입 검사 규칙:**
```
BinaryOp(+, left, right):
  - left_type, right_type를 Symbol Table에서 조회
  - 호환 가능한 타입인가?
    - both i32 → result: i32
    - both f64 → result: f64
    - both string → result: string
    - otherwise → ERROR
```

### 4. 제어 흐름 분석(Control Flow Analysis)

```z-lang
fn getValue(): i32 {
    if (condition) {
        return 1;
    } else {
        return 2;
    }
    return 3;  // ✗ 경고: 도달 불가능한 코드
}

fn getValue2(): i32 {
    if (condition) {
        return 1;
    }
    return 2;  // ✓ OK: else 경로에서는 이 코드 실행
}

fn getValue3(): i32 {
    if (condition) {
        return 1;
    }
    // ✗ 에러! 모든 경로에서 return이 없음
}
```

### 5. 중복 정의 검사(Duplicate Definition Check)

```z-lang
fn main() {
    var x = 10;
    var x = 20;     // ✗ 에러! x가 이미 정의됨

    fn foo() { }
    fn foo() { }    // ✗ 에러! foo가 이미 정의됨
}
```

---

## 🏗️ 2.3의 아키텍처

### 의미 분석 파이프라인

```
【 2.3 의미 분석 단계 】

Input: AST (from 2.2 Parsing)
   ↓
1. 심볼 테이블 빌드 (Symbol Table Building)
   └─ 모든 함수, 변수 선언 수집
   ↓
2. 타입 검사 (Type Checking)
   └─ 각 식의 타입 검증
   ↓
3. 스코프 검증 (Scope Verification)
   └─ 변수 접근 범위 확인
   ↓
4. 제어 흐름 분석 (Control Flow Analysis)
   └─ return 경로 분석
   ↓
5. 의미 오류 보고 (Error Reporting)
   └─ 모든 위반사항 기록

Output: 검증된 AST + Symbol Table
```

---

## 💻 2.3 구현 미리보기

### TypeChecker 구조

```zig
pub const TypeChecker = struct {
    allocator: Allocator,
    symbol_table: SymbolTable,
    errors: std.ArrayList(SemanticError),
    current_function: ?[]const u8,

    pub fn init(allocator: Allocator) TypeChecker {
        return .{
            .allocator = allocator,
            .symbol_table = SymbolTable.init(allocator),
            .errors = std.ArrayList(SemanticError).init(allocator),
            .current_function = null,
        };
    }

    pub fn check(self: *TypeChecker, ast: *Node) !void {
        try self.visitNode(ast);
    }

    fn visitNode(self: *TypeChecker, node: *Node) !void {
        switch (node.*) {
            .function => try self.visitFunction(node),
            .var_decl => try self.visitVarDecl(node),
            .binary_op => try self.visitBinaryOp(node),
            .call => try self.visitCall(node),
            // ...
        }
    }

    fn visitFunction(self: *TypeChecker, node: *Node) !void {
        // 함수 스코프 진입
        try self.symbol_table.push_scope();

        // 파라미터를 심볼 테이블에 등록
        for (node.function.params.items) |param| {
            try self.symbol_table.define(.{
                .name = param.name,
                .symbol_type = .variable,
                .data_type = param.param_type,
                .scope = .function_local,
                .defined = true,
                .initialized = true,
            });
        }

        // 함수 본문 분석
        try self.visitNode(node.function.body);

        // 함수 스코프 퇴출
        try self.symbol_table.pop_scope();
    }

    fn visitVarDecl(self: *TypeChecker, node: *Node) !void {
        // 변수가 이미 정의되었는가?
        if (self.symbol_table.lookup(node.var_decl.name)) |_| {
            try self.addError("Variable already defined", node);
        }

        // 초기값 타입 확인
        if (node.var_decl.init_expr) |init| {
            const init_type = try self.infer_type(init);

            // 명시적 타입이 있다면 확인
            if (node.var_decl.var_type) |explicit_type| {
                if (!self.types_compatible(explicit_type, init_type)) {
                    try self.addError("Type mismatch", node);
                }
            }
        }

        // 심볼 테이블에 등록
        try self.symbol_table.define(.{
            .name = node.var_decl.name,
            .symbol_type = .variable,
            .data_type = node.var_decl.var_type,
            .defined = true,
            .initialized = node.var_decl.init_expr != null,
        });
    }

    fn visitBinaryOp(self: *TypeChecker, node: *Node) !void {
        try self.visitNode(node.binary_op.left);
        try self.visitNode(node.binary_op.right);

        const left_type = try self.infer_type(node.binary_op.left);
        const right_type = try self.infer_type(node.binary_op.right);

        // 연산자별 타입 검증
        switch (node.binary_op.op) {
            .add, .sub, .mul, .div => {
                // 산술 연산: 양쪽 숫자 타입 필요
                if (!self.is_numeric(left_type) or
                    !self.is_numeric(right_type)) {
                    try self.addError("Numeric types required", node);
                }
            },
            .eq, .ne, .lt, .gt, .le, .ge => {
                // 비교 연산: 호환 타입 필요
                if (!self.types_compatible(left_type, right_type)) {
                    try self.addError("Type mismatch in comparison", node);
                }
            },
            // ...
        }
    }
};

pub const SemanticError = struct {
    message: []const u8,
    node_line: u32,
    node_column: u32,
};
```

---

## 🔗 2.3과 다른 단계의 연결

### 렉싱 (2.1) → 파싱 (2.2) → 의미분석 (2.3) → 코드생성 (2.4+)

```
【 정보 흐름 】

2.1 Lexer의 출력:        Token[]
   "var x: i32 = 10"
         ↓
   [keyword_var, identifier(x), colon, identifier(i32), assign, number(10), ...]

2.2 Parser의 출력:       AST
   VarDecl {
     name: "x",
     type: "i32",
     init: Number(10)
   }

2.3 TypeChecker의 입력:  AST
   의미 분석 수행
     1. 이미 정의되었나? → 심볼 테이블 조회
     2. 타입 정확한가?   → "i32" 타입인가?
     3. 값 할당됐나?     → Number(10) 있는가?

2.3 TypeChecker의 출력: 검증된 AST + Symbol Table
   ✓ 모든 검증 통과
   ✓ x의 타입: i32 (심볼 테이블에 기록)

2.4 CodeGenerator의 입력: 검증된 AST + Symbol Table
   LLVM IR 생성
     %0 = alloca i32
     store i32 10, i32* %0  ← 이미 타입이 확정되었음!
```

---

## 🧪 2.3 학습 목표

이 단계를 마치면:

✅ **심볼 테이블 설계 및 구현**
- 스코프 관리
- 변수 등록 및 조회
- 중복 정의 검사

✅ **타입 시스템 이해**
- 타입 검사 규칙
- 타입 호환성
- 타입 추론 (Type Inference)

✅ **제어 흐름 분석**
- return 경로 추적
- 도달 불가능한 코드 검사
- 초기화 상태 추적

✅ **컴파일러 오류 보고**
- 상세한 에러 메시지
- 파일명:줄:열 형식
- 연속 에러 처리

---

## 📋 2.3 과제 예정

**ASSIGNMENT 2.3** (약 800줄):

1. **Task 1: 심볼 테이블 구현** (300줄)
   - SymbolTable 구조
   - 스코프 스택 관리
   - 심볼 등록 및 조회

2. **Task 2: 타입 검사 구현** (350줄)
   - TypeChecker 구조
   - 타입 호환성 검사
   - 각 노드 타입 방문자(Visitor)

3. **Task 3: 의미 에러 검증** (150줄)
   - 5가지 시나리오에 대한 에러 감지
   - 에러 메시지 정확성 검증

---

## 💡 2.3의 실제 사용 예

### 실제 코드 검증

```z-lang
fn calculate(x: i32, y: i32): i32 {
    var result: i32 = x + y;
    var doubled = result * 2;

    if (doubled > 100) {
        return doubled;
    } else {
        return result;
    }

    print("unreachable");  // 경고: 도달 불가능
}
```

**의미 분석 결과:**

```
✓ x: i32 (함수 파라미터)
✓ y: i32 (함수 파라미터)
✓ result: i32 = x + y (i32 + i32 = i32 ✓)
✓ doubled: i32 = result * 2 (i32 * i32 = i32 ✓)
✓ if (doubled > 100): 비교 연산 ✓
✓ return doubled: i32 반환 ✓
✓ return result: i32 반환 ✓
⚠ print("unreachable"): 경고 - 도달 불가능한 코드
```

---

## 🚀 2.3 이후: 2.4 코드 생성

2.3 의미 분석이 완료되면, 2.4에서는:

```
검증된 AST + Symbol Table
     ↓
【 2.4: 코드 생성 (Code Generation) 】
     ↓
1. 함수 정의 생성
   fn calculate(...): i32 → LLVMFunctionType, LLVMAddFunction

2. 변수 메모리 할당
   var result: i32 → %result = alloca i32

3. 타입이 이미 알려진 식 처리
   x + y (둘 다 i32) → LLVMBuildAdd(builder, x, y, i32_type)

4. LLVM IR 생성
   ...
   %result = alloca i32
   %0 = load i32, i32* %x
   %1 = load i32, i32* %y
   %2 = add i32 %0, %1
   store i32 %2, i32* %result
   ...
```

**핵심**: 2.3에서 **타입을 확정**해야 2.4에서 **올바른 LLVM 명령어**를 생성할 수 있습니다!

---

## 📊 Frontend 단계 진행 현황

```
【 Frontend (분석) 단계 】

2.1 렉싱 (Lexing)          ✅ 완료
    토큰화

2.2 파싱 (Parsing)         ✅ 완료
    AST 생성

2.3 의미분석 (Semantic)    🔜 다음! (2주 후)
    타입 검사, 심볼 테이블

2.4 코드생성 (Codegen)     🔜 이후
    AST → LLVM IR
```

---

## 🎓 컴파일 파이프라인 전체 관점

```
【 Z-Lang 컴파일러 전체 흐름 】

소스 코드
  ↓
2.1 Lexing      ← 단어 인식
  [Token, Token, ...]
  ↓
2.2 Parsing     ← 구조 파악
  AST
  ↓
2.3 Semantic    ← 의미 검증 (다음!)
  Verified AST + SymbolTable
  ↓
2.4 Codegen     ← 명령어 생성
  LLVM IR
  ↓
1.5 JIT Exec    ← 실행 (이미 배움!)
  기계어 → CPU 실행
  ↓
결과!
```

---

## 💪 준비 메시지

> **"당신의 컴파일러가 이제 지능을 갖춥니다!"** 🧠
>
> - 2.1: 눈 (Lexing) - 글자를 읽는다
> - 2.2: 귀 (Parsing) - 문장을 듣는다
> - 2.3: 뇌 (Semantic) - 의미를 이해한다 ← 여기!
> - 2.4: 입 (Codegen) - 명령을 말한다

---

**예정 공개**: 2026-03-16 (일)
**난이도**: ⭐⭐⭐⭐⭐ (최고급)
**흥미도**: ⭐⭐⭐⭐⭐ (컴파일러의 심장!)

**다음을 입력하세요!** 🚀

---

*"심볼 테이블은 컴파일러의 기억이고, 타입 검사는 컴파일러의 지능입니다."* 🧠✨

*"의미 분석 없이는 코드 생성도 불가능합니다. 여기서 모든 것의 토대가 다져집니다."* 🏛️
