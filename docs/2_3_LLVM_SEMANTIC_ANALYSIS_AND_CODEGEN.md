# 🏛️ LLVM 전설: 2.3 의미 분석과 IR 생성 통합 (The Bridge)

> **최종 업데이트**: 2026-02-26
> **상태**: ✅ 완성 (2.2 파싱 → 2.3 의미분석 & 생성)
> **난이도**: ⭐⭐⭐⭐⭐ (최고급 - 컴파일러의 영혼!)

---

## 🎯 2.3 단계의 핵심: 추상에서 실제로

### 문법에서 의미로, 의미에서 실행으로

```
【 컴파일러의 여정 】

입력: 소스 코드
  ↓
2.1 렉싱 (Lexing)
  "var x = 10; x + 5"
         ↓
  [keyword_var, identifier(x), assign, number(10), semicolon, ...]
         (단어의 나열 - 의미 없음)
  ↓
2.2 파싱 (Parsing)
  [tokens]
         ↓
  VarDecl { name: "x", init: 10 }
  BinaryOp { +, Identifier("x"), 5 }
         (구조는 있는데 타입은?)
  ↓
2.3 의미 분석 & 코드 생성 (여기!)
  ✓ x는 정의되었나? → YES
  ✓ x의 타입은? → i64
  ✓ i64 + i64 가능한가? → YES

  LLVM IR 생성:
    %0 = alloca i64
    store i64 10, i64* %0
    %1 = load i64, i64* %0
    %2 = add i64 %1, 5
         ↓
1.5 실행 (JIT/컴파일)
  기계어 실행
         ↓
결과: 15
```

---

## 📚 1. 심볼 테이블(Symbol Table): 이름의 장부

### 1.1 심볼 테이블의 역할

컴파일러는 **모든 변수, 함수, 타입의 정보를 기억**해야 합니다.

```
【 심볼 테이블이 기억하는 것 】

변수 x:
  - 이름: "x"
  - 타입: i64
  - 스코프: main 함수의 로컬
  - LLVM 할당: %x = alloca i64
  - 초기화 여부: true

함수 foo:
  - 이름: "foo"
  - 매개변수: [(name: "a", type: i64), (name: "b", type: i64)]
  - 반환 타입: i64
  - LLVM 함수: LLVMValueRef (foo 함수)
```

### 1.2 스코프(Scope)와 스택 구조

```z-lang
fn outer() {                    // Scope Level 1
    var x = 10;                 // x ∈ outer scope

    fn inner() {                // Scope Level 2
        var y = 20;             // y ∈ inner scope
        {                       // Scope Level 3
            var z = 30;         // z ∈ block scope
            print(x);           // ✓ x는 parent scope에서 찾음
            print(y);           // ✓ y는 현재 스코프에서 찾음
            print(z);           // ✓ z는 현재 스코프에서 찾음
        }                       // Scope Level 3 끝
        print(z);               // ✗ 에러! z는 블록 밖
    }
}
```

**스코프 스택 시각화:**

```
┌─────────────────────┐
│  Block Scope        │  ← z만 존재
├─────────────────────┤
│  inner() Scope      │  ← y, x (parent), z (현재)
├─────────────────────┤
│  outer() Scope      │  ← x, inner() 함수
├─────────────────────┤
│  Global Scope       │  ← outer() 함수, 전역 함수들
└─────────────────────┘
```

### 1.3 Zig로 심볼 테이블 구현

```zig
pub const Symbol = struct {
    name: []const u8,
    symbol_type: SymbolType,  // Variable, Function, Type
    data_type: Type,          // i64, f64, string, ...
    llvm_value: ?c.LLVMValueRef = null,  // 할당된 메모리/함수
    initialized: bool = false,
};

pub const SymbolType = enum {
    variable,
    function,
    type_def,
};

pub const Type = union(enum) {
    integer: struct {
        bits: u32,  // 8, 16, 32, 64
    },
    floating: struct {
        bits: u32,  // 32 (float), 64 (double)
    },
    string,
    boolean,
    function: struct {
        param_types: []Type,
        return_type: *Type,
    },
};

pub const SymbolTable = struct {
    allocator: Allocator,
    scopes: std.ArrayList(Scope),  // 스코프 스택
    current_scope: usize,
    context: c.LLVMContextRef,
    module: c.LLVMModuleRef,

    pub fn init(allocator: Allocator, context: c.LLVMContextRef, module: c.LLVMModuleRef) SymbolTable {
        return .{
            .allocator = allocator,
            .scopes = std.ArrayList(Scope).init(allocator),
            .current_scope = 0,
            .context = context,
            .module = module,
        };
    }

    // 【 스코프 관리 】
    pub fn pushScope(self: *SymbolTable) !void {
        try self.scopes.append(Scope{
            .symbols = std.StringHashMap(Symbol).init(self.allocator),
        });
        self.current_scope += 1;
    }

    pub fn popScope(self: *SymbolTable) void {
        if (self.current_scope > 0) {
            var scope = self.scopes.pop();
            scope.symbols.deinit();
            self.current_scope -= 1;
        }
    }

    // 【 심볼 정의 】
    pub fn define(self: *SymbolTable, symbol: Symbol) !void {
        var scope = &self.scopes.items[self.scopes.items.len - 1];
        try scope.symbols.put(symbol.name, symbol);
    }

    // 【 심볼 조회 】
    pub fn lookup(self: *const SymbolTable, name: []const u8) ?Symbol {
        // 현재 스코프부터 역순으로 탐색 (가장 가까운 정의부터)
        var i: i32 = @intCast(i32, self.scopes.items.len - 1);
        while (i >= 0) : (i -= 1) {
            if (self.scopes.items[@intCast(usize, i)].symbols.get(name)) |symbol| {
                return symbol;
            }
        }
        return null;
    }

    // 【 타입 호환성 검사 】
    pub fn types_compatible(self: *const SymbolTable, left: Type, right: Type) bool {
        switch (left) {
            .integer => |l| {
                if (right == .integer) {
                    return l.bits == right.integer.bits;
                }
            },
            .floating => |l| {
                if (right == .floating) {
                    return l.bits == right.floating.bits;
                }
            },
            .string => {
                return right == .string;
            },
            .boolean => {
                return right == .boolean;
            },
            else => {},
        }
        return false;
    }
};

pub const Scope = struct {
    symbols: std.StringHashMap(Symbol),
};
```

---

## 🧠 2. 시맨틱 분석(Semantic Analysis)

### 2.1 의미 체크: AST 순회

```zig
pub const SemanticAnalyzer = struct {
    allocator: Allocator,
    symbol_table: SymbolTable,
    errors: std.ArrayList(SemanticError),
    current_function: ?[]const u8,
    context: c.LLVMContextRef,
    module: c.LLVMModuleRef,
    builder: c.LLVMBuilderRef,

    pub fn init(
        allocator: Allocator,
        context: c.LLVMContextRef,
        module: c.LLVMModuleRef,
        builder: c.LLVMBuilderRef,
    ) SemanticAnalyzer {
        return .{
            .allocator = allocator,
            .symbol_table = SymbolTable.init(allocator, context, module),
            .errors = std.ArrayList(SemanticError).init(allocator),
            .current_function = null,
            .context = context,
            .module = module,
            .builder = builder,
        };
    }

    // 【 AST 순회 메인 함수 】
    pub fn analyze(self: *SemanticAnalyzer, ast: *Node) !?c.LLVMValueRef {
        return self.visitNode(ast);
    }

    fn visitNode(self: *SemanticAnalyzer, node: *Node) !?c.LLVMValueRef {
        switch (node.*) {
            .block => try self.visitBlock(node),
            .function => try self.visitFunction(node),
            .var_decl => try self.visitVarDecl(node),
            .return_stmt => try self.visitReturn(node),
            .if_stmt => try self.visitIf(node),
            .while_stmt => try self.visitWhile(node),
            .binary_op => try self.visitBinaryOp(node),
            .call => try self.visitCall(node),
            .identifier => try self.visitIdentifier(node),
            .number => try self.visitNumber(node),
            .string => try self.visitString(node),
        }
    }
};
```

### 2.2 구체적 의미 검사

#### Variable Resolution (변수 선언 확인)

```zig
fn visitIdentifier(self: *SemanticAnalyzer, node: *Node) !?c.LLVMValueRef {
    const name = node.identifier;

    // 심볼 테이블에서 변수를 찾는다
    if (self.symbol_table.lookup(name)) |symbol| {
        return symbol.llvm_value;
    } else {
        // 정의되지 않은 변수!
        try self.addError("Undefined variable: {}", name);
        return null;
    }
}
```

#### Type Mismatch (타입 불일치 검사)

```zig
fn visitBinaryOp(self: *SemanticAnalyzer, node: *Node) !?c.LLVMValueRef {
    const left_val = try self.visitNode(node.binary_op.left);
    const right_val = try self.visitNode(node.binary_op.right);

    if (left_val == null or right_val == null) {
        return null;  // 에러 이미 보고됨
    }

    // 양쪽 타입 추론
    const left_type = try self.inferType(node.binary_op.left);
    const right_type = try self.inferType(node.binary_op.right);

    // 타입 검사
    switch (node.binary_op.op) {
        .add, .sub, .mul, .div => {
            // 산술 연산: 양쪽 타입이 호환되어야 함
            if (!self.symbol_table.types_compatible(left_type, right_type)) {
                try self.addError(
                    "Type mismatch in binary operation",
                    node
                );
                return null;
            }

            // 산술 연산 생성 (1.3 제어 흐름에서 배운 기법)
            return switch (left_type) {
                .integer => self.generateIntegerBinaryOp(
                    node.binary_op.op,
                    left_val.?,
                    right_val.?
                ),
                .floating => self.generateFloatingBinaryOp(
                    node.binary_op.op,
                    left_val.?,
                    right_val.?
                ),
                else => error.InvalidOperandType,
            };
        },
        else => return null,
    }
}

fn generateIntegerBinaryOp(
    self: *SemanticAnalyzer,
    op: BinaryOperator,
    left: c.LLVMValueRef,
    right: c.LLVMValueRef,
) c.LLVMValueRef {
    return switch (op) {
        .add => c.LLVMBuildAdd(self.builder, left, right, "addtmp"),
        .sub => c.LLVMBuildSub(self.builder, left, right, "subtmp"),
        .mul => c.LLVMBuildMul(self.builder, left, right, "multmp"),
        .div => c.LLVMBuildSDiv(self.builder, left, right, "divtmp"),
        else => unreachable,
    };
}

fn generateFloatingBinaryOp(
    self: *SemanticAnalyzer,
    op: BinaryOperator,
    left: c.LLVMValueRef,
    right: c.LLVMValueRef,
) c.LLVMValueRef {
    return switch (op) {
        .add => c.LLVMBuildFAdd(self.builder, left, right, "faddtmp"),
        .sub => c.LLVMBuildFSub(self.builder, left, right, "fsubtmp"),
        .mul => c.LLVMBuildFMul(self.builder, left, right, "fmultmp"),
        .div => c.LLVMBuildFDiv(self.builder, left, right, "fdivtmp"),
        else => unreachable,
    };
}
```

#### Function Signature Checking (함수 시그니처 검사)

```zig
fn visitCall(self: *SemanticAnalyzer, node: *Node) !?c.LLVMValueRef {
    const func_name = node.call.callee;

    // 함수가 정의되었는가?
    const func_symbol = self.symbol_table.lookup(func_name) orelse {
        try self.addError("Undefined function: {}", func_name);
        return null;
    };

    // 함수인가?
    if (func_symbol.symbol_type != .function) {
        try self.addError("{} is not a function", func_name);
        return null;
    }

    // 인자 개수 확인
    if (node.call.args.items.len != func_symbol.data_type.function.param_types.len) {
        try self.addError(
            "Function {} expects {} arguments, got {}",
            func_name,
            func_symbol.data_type.function.param_types.len,
            node.call.args.items.len,
        );
        return null;
    }

    // 각 인자의 타입 확인
    var llvm_args = try self.allocator.alloc(c.LLVMValueRef, node.call.args.items.len);
    for (node.call.args.items) |arg, i| {
        const arg_val = try self.visitNode(arg);
        const arg_type = try self.inferType(arg);
        const param_type = func_symbol.data_type.function.param_types[i];

        if (!self.symbol_table.types_compatible(arg_type, param_type)) {
            try self.addError(
                "Parameter {} type mismatch",
                i,
            );
            return null;
        }

        llvm_args[i] = arg_val.?;
    }

    // LLVM 함수 호출 (1.5에서 배운 기법)
    return c.LLVMBuildCall(
        self.builder,
        func_symbol.llvm_value.?,
        llvm_args.ptr,
        @intCast(c_uint, llvm_args.len),
        "calltmp",
    );
}
```

---

## 💻 3. 방문자 패턴(Visitor Pattern)과 코드 생성

### 3.1 노드별 코드 생성

#### 변수 선언

```zig
fn visitVarDecl(self: *SemanticAnalyzer, node: *Node) !?c.LLVMValueRef {
    const name = node.var_decl.name;
    const var_type = node.var_decl.var_type orelse "i64";  // 기본값

    // 1. LLVM 타입 결정
    const llvm_type = switch (var_type) {
        "i64" => c.LLVMInt64Type(self.context),
        "f64" => c.LLVMDoubleType(self.context),
        "i32" => c.LLVMInt32Type(self.context),
        else => return error.UnknownType,
    };

    // 2. 메모리 할당 (alloca)
    const alloca_inst = c.LLVMBuildAlloca(
        self.builder,
        llvm_type,
        name,
    );

    // 3. 초기값이 있다면 저장
    if (node.var_decl.init_expr) |init| {
        const init_val = try self.visitNode(init);
        if (init_val == null) return null;

        _ = c.LLVMBuildStore(self.builder, init_val.?, alloca_inst);
    }

    // 4. 심볼 테이블에 등록
    try self.symbol_table.define(.{
        .name = name,
        .symbol_type = .variable,
        .data_type = .{ .integer = .{ .bits = 64 } },  // 단순화
        .llvm_value = alloca_inst,
        .initialized = node.var_decl.init_expr != null,
    });

    return null;  // 변수 선언은 값을 반환하지 않음
}
```

#### 함수 정의

```zig
fn visitFunction(self: *SemanticAnalyzer, node: *Node) !?c.LLVMValueRef {
    const func_name = node.function.name;

    // 1. 함수 타입 생성
    var param_types = try self.allocator.alloc(c.LLVMTypeRef, node.function.params.items.len);
    for (node.function.params.items) |param, i| {
        param_types[i] = c.LLVMInt64Type(self.context);
    }

    const func_type = c.LLVMFunctionType(
        c.LLVMInt64Type(self.context),  // 반환 타입
        param_types.ptr,
        @intCast(c_uint, param_types.len),
        0,
    );

    // 2. 함수 생성 (1.2에서 배운 기법)
    const llvm_func = c.LLVMAddFunction(self.module, func_name, func_type);

    // 3. 함수를 심볼 테이블에 등록
    try self.symbol_table.define(.{
        .name = func_name,
        .symbol_type = .function,
        .llvm_value = llvm_func,
    });

    // 4. 함수 본문 생성을 위해 기본 블록 생성
    const entry_block = c.LLVMAppendBasicBlock(llvm_func, "entry");
    c.LLVMPositionBuilderAtEnd(self.builder, entry_block);

    // 5. 함수 스코프 진입
    try self.symbol_table.pushScope();
    self.current_function = func_name;

    // 6. 파라미터를 지역 변수로 등록
    for (node.function.params.items) |param, i| {
        const param_value = c.LLVMGetParam(llvm_func, @intCast(c_uint, i));
        try self.symbol_table.define(.{
            .name = param.name,
            .symbol_type = .variable,
            .llvm_value = param_value,
            .initialized = true,
        });
    }

    // 7. 함수 본문 분석
    const body_result = try self.visitNode(node.function.body);

    // 8. 함수 스코프 퇴출
    self.symbol_table.popScope();
    self.current_function = null;

    return llvm_func;
}
```

#### if 문 (1.3 제어 흐름 기법 활용)

```zig
fn visitIf(self: *SemanticAnalyzer, node: *Node) !?c.LLVMValueRef {
    // 조건 평가
    const cond = try self.visitNode(node.if_stmt.condition);
    if (cond == null) return null;

    // 현재 블록 저장
    const current_block = c.LLVMGetInsertBlock(self.builder);
    const parent_func = c.LLVMGetBasicBlockParent(current_block);

    // 1.3 제어 흐름 기법: 4-블록 구조
    // Entry → Then → Merge (else가 있으면 Else도)

    const then_block = c.LLVMAppendBasicBlock(parent_func, "then");
    const else_block = if (node.if_stmt.else_branch != null)
        c.LLVMAppendBasicBlock(parent_func, "else")
    else
        null;
    const merge_block = c.LLVMAppendBasicBlock(parent_func, "merge");

    // 분기문 생성
    _ = c.LLVMBuildCondBr(self.builder, cond.?, then_block, else_block orelse merge_block);

    // Then 블록 생성
    c.LLVMPositionBuilderAtEnd(self.builder, then_block);
    _ = try self.visitNode(node.if_stmt.then_branch);
    if (c.LLVMGetInsertBlock(self.builder) == then_block) {
        _ = c.LLVMBuildBr(self.builder, merge_block);
    }

    // Else 블록 생성 (있으면)
    if (node.if_stmt.else_branch) |else_branch| {
        c.LLVMPositionBuilderAtEnd(self.builder, else_block.?);
        _ = try self.visitNode(else_branch);
        if (c.LLVMGetInsertBlock(self.builder) == else_block) {
            _ = c.LLVMBuildBr(self.builder, merge_block);
        }
    } else {
        // else가 없으면 바로 merge로
        c.LLVMPositionBuilderAtEnd(self.builder, else_block orelse current_block);
        _ = c.LLVMBuildBr(self.builder, merge_block);
    }

    // Merge 블록에서 계속
    c.LLVMPositionBuilderAtEnd(self.builder, merge_block);

    return null;
}
```

---

## ⚡ 4. 고급 최적화: 지능적 코드 생성

### 4.1 상수 폴딩(Constant Folding)

```zig
fn visitBinaryOp(self: *SemanticAnalyzer, node: *Node) !?c.LLVMValueRef {
    // 상수 폴딩: 컴파일 시간에 계산 가능한 식은 즉시 계산

    const left = try self.visitNode(node.binary_op.left);
    const right = try self.visitNode(node.binary_op.right);

    if (left == null or right == null) return null;

    // 양쪽이 상수인가?
    if (c.LLVMIsConstant(left.?) != 0 and c.LLVMIsConstant(right.?) != 0) {
        // 컴파일 시간에 계산
        switch (node.binary_op.op) {
            .add => {
                const left_val = c.LLVMConstIntGetSExtValue(left.?);
                const right_val = c.LLVMConstIntGetSExtValue(right.?);
                const result = left_val + right_val;
                return c.LLVMConstInt(c.LLVMInt64Type(self.context), @intCast(c_ulonglong, result), 1);
            },
            .mul => {
                const left_val = c.LLVMConstIntGetSExtValue(left.?);
                const right_val = c.LLVMConstIntGetSExtValue(right.?);
                const result = left_val * right_val;
                return c.LLVMConstInt(c.LLVMInt64Type(self.context), @intCast(c_ulonglong, result), 1);
            },
            else => {},
        }
    }

    // 상수가 아니면 런타임 연산 생성
    return self.generateBinaryOp(node.binary_op.op, left.?, right.?);
}
```

**예제:**
```z-lang
var x = 1 + 2 + 3;  // 컴파일 시 6으로 계산됨
                    // IR: %x = alloca i64
                    //     store i64 6, i64* %x
```

### 4.2 단락 평가(Short-Circuiting)

```z-lang
if (a != null && a.value > 10) {
    // a가 null이면 a.value는 평가되지 않음
}
```

이를 1.3 제어 흐름 기법으로 구현:

```zig
fn visitBinaryOp_LogicalAnd(
    self: *SemanticAnalyzer,
    left: *Node,
    right: *Node,
) !c.LLVMValueRef {
    // 1.3 if 문과 유사한 구조

    // 왼쪽 조건 평가
    const left_val = try self.visitNode(left);

    // 현재 블록
    const current_block = c.LLVMGetInsertBlock(self.builder);
    const parent_func = c.LLVMGetBasicBlockParent(current_block);

    // 오른쪽 평가 블록과 결과 병합 블록
    const right_block = c.LLVMAppendBasicBlock(parent_func, "and_right");
    const merge_block = c.LLVMAppendBasicBlock(parent_func, "and_merge");

    // 왼쪽이 거짓이면 오른쪽 건너뜀
    _ = c.LLVMBuildCondBr(self.builder, left_val.?, right_block, merge_block);

    // 오른쪽 평가
    c.LLVMPositionBuilderAtEnd(self.builder, right_block);
    const right_val = try self.visitNode(right);
    _ = c.LLVMBuildBr(self.builder, merge_block);

    // 결과 병합
    c.LLVMPositionBuilderAtEnd(self.builder, merge_block);

    // PHI 노드로 결과 선택 (1.3 제어 흐름에서 배운 기법)
    const result_type = c.LLVMInt1Type(self.context);
    const phi = c.LLVMBuildPhi(self.builder, result_type, "and_result");

    var pred_blocks = [_]c.LLVMBasicBlockRef{ current_block, right_block };
    var values = [_]c.LLVMValueRef{
        c.LLVMConstInt(result_type, 0, 1),  // 왼쪽이 거짓
        right_val.?,                         // 오른쪽 결과
    };

    c.LLVMAddIncoming(phi, &values[0], &pred_blocks[0], 2);

    return phi;
}
```

---

## 🌳 5. 전체 파이프라인 통합

### 5.1 Lexer → Parser → Semantic → Codegen 완전 흐름

```zig
pub const Compiler = struct {
    allocator: Allocator,
    context: c.LLVMContextRef,
    module: c.LLVMModuleRef,
    builder: c.LLVMBuilderRef,

    pub fn compile(self: *Compiler, source: []const u8) !c.LLVMModuleRef {
        // 【 Step 1: Lexing (2.1) 】
        var lexer = Lexer.init(self.allocator, source);
        const tokens = try lexer.tokenize();
        defer self.allocator.free(tokens);

        // 【 Step 2: Parsing (2.2) 】
        var parser = Parser.init(self.allocator, tokens);
        const ast = try parser.parse();

        // 【 Step 3: Semantic Analysis & Codegen (2.3) 】
        var semantic = SemanticAnalyzer.init(
            self.allocator,
            self.context,
            self.module,
            self.builder,
        );
        _ = try semantic.analyze(ast);

        // 에러가 있으면 반환
        if (semantic.errors.items.len > 0) {
            for (semantic.errors.items) |err| {
                std.debug.print("Error: {}\n", .{err});
            }
            return error.CompilationFailed;
        }

        // 【 Step 4: JIT 실행 또는 저장 (1.5) 】
        return self.module;
    }
};
```

### 5.2 실행 예제

```z-lang
// 입력 코드
fn add(a: i64, b: i64): i64 {
    return a + b;
}

fn main() {
    var x = 10;
    var y = 20;
    var result = add(x, y);
    return result;
}
```

**단계별 처리:**

```
【 2.1 렉싱 】
source → [keyword_fn, identifier(add), lparen, identifier(a), ...]

【 2.2 파싱 】
tokens → AST:
         Program {
           Function {
             name: "add",
             params: [Parameter(a), Parameter(b)],
             body: Block {
               Return { BinaryOp(+, Identifier(a), Identifier(b)) }
             }
           },
           Function {
             name: "main",
             ...
           }
         }

【 2.3 의미분석 & 코드생성 】
AST → 심볼 테이블 구축:
      add: (function, i64(i64,i64) → i64)
      main: (function, void() → i64)

   → LLVM IR 생성:
      define i64 @add(i64 %a, i64 %b) {
      entry:
        %0 = add i64 %a, %b
        ret i64 %0
      }

      define i64 @main() {
      entry:
        %x = alloca i64
        store i64 10, i64* %x
        %y = alloca i64
        store i64 20, i64* %y
        %x_val = load i64, i64* %x
        %y_val = load i64, i64* %y
        %result = call i64 @add(i64 %x_val, i64 %y_val)
        ret i64 %result
      }

【 1.5 JIT 실행 】
LLVM IR → 기계어 → CPU 실행 → 결과: 30
```

---

## 📊 6. 에러 처리와 보고

### 6.1 의미 에러의 종류

```
【 2.3 단계에서 감지 가능한 에러들 】

1. Undefined Variable
   var y = x + 1;  ← x가 정의되지 않음

2. Type Mismatch
   var x: i64 = "hello";  ← 문자열을 정수형에 할당

3. Function Not Found
   result = foo(10);  ← foo 함수가 정의되지 않음

4. Argument Count Mismatch
   var x = add(1);  ← add(a, b)가 1개만 받음

5. Argument Type Mismatch
   var x = add(1, "2");  ← 두 번째 인자가 문자열

6. Return Type Mismatch
   fn getValue(): i64 {
       return "hello";  ← 함수는 i64 반환이지만 문자열 반환
   }

7. Missing Return
   fn getValue(): i64 {
       var x = 10;
       // return 문 없음!
   }
```

### 6.2 상세한 에러 메시지

```zig
pub const SemanticError = struct {
    message: []const u8,
    node_line: u32,
    node_column: u32,
    context: []const u8,
};

pub fn addError(
    self: *SemanticAnalyzer,
    message: []const u8,
    node: *Node,
) !void {
    try self.errors.append(.{
        .message = message,
        .node_line = node.line,
        .node_column = node.column,
        .context = "In function {}", // 함수 이름 포함
    });
}

// 출력 예:
// Error at line 5, column 12 (in function 'main'):
// Undefined variable 'x'
//
// 5 |     var y = x + 1;
//   |             ↑
```

---

## 🎯 7. 심볼 테이블과 코드 생성의 연결

### 7.1 변수 접근의 흐름

```
【 변수 x에 접근하는 과정 】

AST: Identifier("x")
  ↓
SemanticAnalyzer.visitIdentifier()
  ↓
symbol_table.lookup("x")
  ↓
Symbol {
  name: "x",
  type: i64,
  llvm_value: %x (alloca i64에 대한 포인터)
}
  ↓
c.LLVMBuildLoad(builder, %x, "x_val")
  ↓
LLVM IR: %x_val = load i64, i64* %x
```

### 7.2 타입 정보의 흐름

```
【 타입 정보로 올바른 명령어 선택 】

AST: BinaryOp(+, Identifier(a), Identifier(b))
  ↓
SemanticAnalyzer.visitBinaryOp()
  ↓
inferType(Identifier(a)) → i64
inferType(Identifier(b)) → i64
  ↓
types_compatible(i64, i64) → true
  ↓
switch (operand_type) {
  .integer => generateIntegerBinaryOp(...)
    → c.LLVMBuildAdd()  ← 정수 덧셈
  .floating => generateFloatingBinaryOp(...)
    → c.LLVMBuildFAdd()  ← 부동소수 덧셈
}
```

---

## 📋 요약: 2.3 의미분석의 네 가지 역할

| 역할 | 담당 | 예제 |
|------|------|------|
| **1. 기억** | Symbol Table | 변수 x가 i64 타입임을 기억 |
| **2. 검증** | Semantic Checker | x + y에서 타입 호환성 확인 |
| **3. 최적화** | Constant Folding | 1 + 2를 컴파일 시 3으로 계산 |
| **4. 생성** | Codegen | 검증된 정보로 LLVM IR 생성 |

---

## 🚀 2.3 완료 후: 실행 가능한 코드 탄생

```
【 2.3 완료 후의 흐름 】

검증된 AST + LLVM IR
         ↓
1.5 JIT Execution Engine
         ↓
기계어 생성
         ↓
CPU 위에서 실행
         ↓
결과!
```

**당신이 정의한 언어가 마침내 CPU 위에서 춤을 춥니다!** 🎉

---

## 💪 2.3의 완성도

```
【 컴파일러의 완성도 】

2.1 렉싱:           단어를 읽는다 (눈) 👀
2.2 파싱:           구조를 이해한다 (귀) 👂
2.3 의미분석:       의미를 검증한다 (뇌) 🧠 ← 여기!
2.4+ 코드생성:      명령을 내린다 (입) 👄
1.5 실행:           행동한다 (신체) 💪

당신의 컴파일러가 이제 완전한 지능을 갖춘 존재입니다!
```

---

**최종 메시지**:

> **"심볼 테이블은 컴파일러의 기억이고,**
> **의미 검증은 컴파일러의 지능이며,**
> **코드 생성은 컴파일러의 실행 능력입니다."** 🏛️
>
> **당신이 이 모든 것을 만들었습니다!** ⚡

---

*"추상에서 시작해, 의미를 거쳐, 마침내 실제가 된다. 이것이 컴파일의 마법입니다."* ✨

*"The Bridge between semantics and execution is where legends are born." 🎬*
