# 📋 Assignment 2.3: 의미 분석과 IR 생성 통합

> **기한**: 2026-03-30 (일)
> **난이도**: ⭐⭐⭐⭐⭐ (최고급 - 컴파일러의 정점!)
> **핵심**: AST를 검증하고 LLVM IR로 변환하는 통합 시스템

---

## 🎯 과제 개요

이 과제는 컴파일러 제작의 **가장 중요한 순간**입니다.

- 2.1 렉싱: 단어를 읽었고
- 2.2 파싱: 구조를 파악했으니
- **2.3: 이제 의미를 검증하고 실행 가능한 LLVM IR로 변환합니다** ⚡

### 목표
- ✅ 심볼 테이블(Symbol Table) 구현
- ✅ 시맨틱 검증(Semantic Checking) 구현
- ✅ 방문자 패턴(Visitor Pattern) 기반 코드 생성
- ✅ 상수 폴딩(Constant Folding) 최적화
- ✅ 아키텍처 철학 정립

---

## 📝 Task 1: 심볼 테이블 구현

### 개요

변수, 함수, 타입 정보를 저장하고 관리하는 **심볼 테이블**을 구현합니다.

### 요구사항

```
【 심볼 테이블이 관리할 것 】

1. 변수 정보
   - 이름, 타입, 초기화 여부
   - LLVM 할당 주소 (%x = alloca i64)

2. 함수 정보
   - 이름, 매개변수 타입, 반환 타입
   - LLVM 함수 참조

3. 스코프 스택
   - Global Scope
   - Function Scope
   - Block Scope { }
```

### 작성 내용

**파일 이름**: `src/semantic/SymbolTable.zig`

```zig
pub const Symbol = struct {
    // 【 정의할 필드 】
    // - name: 이름
    // - symbol_type: 변수인가 함수인가
    // - data_type: 타입 (i64, f64, string, ...)
    // - llvm_value: LLVM ValueRef
    // - initialized: 초기화됐나
};

pub const Type = union(enum) {
    // 【 지원할 타입 】
    // - integer { bits: 8, 16, 32, 64 }
    // - floating { bits: 32, 64 }
    // - string
    // - boolean
    // - function { param_types, return_type }
};

pub const SymbolTable = struct {
    // 【 구현할 메서드 】
    pub fn init(...) SymbolTable { }
    pub fn pushScope(self: *SymbolTable) !void { }
    pub fn popScope(self: *SymbolTable) void { }
    pub fn define(self: *SymbolTable, symbol: Symbol) !void { }
    pub fn lookup(self: *const SymbolTable, name: []const u8) ?Symbol { }
    pub fn types_compatible(self: *const SymbolTable, left: Type, right: Type) bool { }
};
```

### 기대 결과물

**테스트 시나리오:**

```z-lang
// 전역 변수
var x: i64 = 10;

// 함수
fn add(a: i64, b: i64): i64 {
    return a + b;
}

// 함수 내 변수
fn test() {
    var y = 20;           // 로컬 변수
    var z = add(x, y);   // 전역 x 접근, 함수 호출
    {
        var w = 30;      // 블록 스코프
    }
    // print(w);         // ✗ 에러! w는 블록 범위 밖
}
```

**심볼 테이블 상태:**

```
Global Scope:
  - x: (type: i64, llvm_value: %0)
  - add: (type: function, llvm_value: @add)
  - test: (type: function, llvm_value: @test)

Function "test" Scope:
  - y: (type: i64, llvm_value: %1)
  - z: (type: i64, llvm_value: %2)

Block Scope { }:
  - w: (type: i64, llvm_value: %3)
```

### 체크리스트

- [ ] Symbol 구조체 (이름, 타입, LLVM 값)
- [ ] Type union(enum) (정수, 부동소수, 문자열, 함수)
- [ ] SymbolTable 구조체
- [ ] pushScope() - 스코프 진입
- [ ] popScope() - 스코프 퇴출
- [ ] define() - 심볼 등록
- [ ] lookup() - 심볼 조회 (스코프 스택 역순 탐색)
- [ ] types_compatible() - 타입 호환성 검사
- [ ] 메모리 안전성 (allocator 사용)

**예상 줄 수**: 400-500줄

**제출물**:
```
zlang-project/
└── src/
    └── semantic/
        └── SymbolTable.zig  ← 여기!
```

---

## 🧠 Task 2: 시맨틱 검증 로직

### 개요

AST의 각 노드를 방문하며 **의미적 검증**을 수행합니다.

### 요구사항

다음 **5가지 검증 시나리오**를 처리하세요:

```
【 검증할 시나리오 】

1. Variable Resolution (변수 선언 확인)
   var y = x + 1;  ← x가 선언되었나?

2. Type Mismatch (타입 불일치)
   var x: i64 = "hello";
   var y: string = 10;

3. Function Signature (함수 시그니처)
   fn add(a: i64, b: i64): i64 { return a + b; }
   var x = add(1);              ← 인자 1개만? (2개 필요)
   var y = add(1, "2");         ← 두 번째 인자가 문자열?

4. Undefined Function (함수 미정의)
   var x = foo(10);  ← foo가 정의되지 않음

5. Binary Operation Type Check (이항 연산 타입)
   var x: i64 = 10;
   var y: string = "hello";
   var z = x + y;  ← i64 + string은 불가능
```

### 작성 내용

**파일 이름**: `src/semantic/SemanticAnalyzer.zig`

```zig
pub const SemanticAnalyzer = struct {
    allocator: Allocator,
    symbol_table: SymbolTable,
    errors: std.ArrayList(SemanticError),
    current_function: ?[]const u8,

    pub fn init(...) SemanticAnalyzer { }
    pub fn analyze(self: *SemanticAnalyzer, ast: *Node) !void { }

    // 【 노드 방문 메서드 】
    fn visitNode(self: *SemanticAnalyzer, node: *Node) !void { }
    fn visitVarDecl(self: *SemanticAnalyzer, node: *Node) !void { }
    fn visitFunction(self: *SemanticAnalyzer, node: *Node) !void { }
    fn visitBinaryOp(self: *SemanticAnalyzer, node: *Node) !void { }
    fn visitCall(self: *SemanticAnalyzer, node: *Node) !void { }
    fn visitIdentifier(self: *SemanticAnalyzer, node: *Node) !void { }
};
```

### 구체적 요구사항

#### 1. Variable Resolution

```zig
fn visitIdentifier(self: *SemanticAnalyzer, node: *Node) !void {
    const name = node.identifier;

    if (self.symbol_table.lookup(name) == null) {
        try self.addError("Undefined variable: {}", name);
    }
}
```

#### 2. Type Mismatch in Variable Declaration

```zig
fn visitVarDecl(self: *SemanticAnalyzer, node: *Node) !void {
    if (node.var_decl.var_type) |explicit_type| {
        if (node.var_decl.init_expr) |init_expr| {
            const init_type = try self.inferType(init_expr);

            // 명시적 타입과 초기값 타입이 일치하나?
            if (!self.symbol_table.types_compatible(explicit_type, init_type)) {
                try self.addError(
                    "Type mismatch: expected {}, got {}",
                    explicit_type,
                    init_type,
                );
            }
        }
    }
}
```

#### 3. Function Signature Checking

```zig
fn visitCall(self: *SemanticAnalyzer, node: *Node) !void {
    const func_name = node.call.callee;

    // 함수가 정의되었나?
    const func_symbol = self.symbol_table.lookup(func_name) orelse {
        try self.addError("Undefined function: {}", func_name);
        return;
    };

    // 함수인가?
    if (func_symbol.symbol_type != .function) {
        try self.addError("{} is not a function", func_name);
        return;
    }

    // 인자 개수가 맞나?
    const expected_param_count = func_symbol.data_type.function.param_types.len;
    const actual_arg_count = node.call.args.items.len;

    if (expected_param_count != actual_arg_count) {
        try self.addError(
            "Function {} expects {} arguments, got {}",
            func_name,
            expected_param_count,
            actual_arg_count,
        );
        return;
    }

    // 각 인자의 타입이 맞나?
    for (node.call.args.items) |arg, i| {
        const arg_type = try self.inferType(arg);
        const param_type = func_symbol.data_type.function.param_types[i];

        if (!self.symbol_table.types_compatible(arg_type, param_type)) {
            try self.addError(
                "Parameter {} type mismatch: expected {}, got {}",
                i,
                param_type,
                arg_type,
            );
        }
    }
}
```

#### 4. Binary Operation Type Check

```zig
fn visitBinaryOp(self: *SemanticAnalyzer, node: *Node) !void {
    const left_type = try self.inferType(node.binary_op.left);
    const right_type = try self.inferType(node.binary_op.right);

    switch (node.binary_op.op) {
        .add, .sub, .mul, .div => {
            // 산술 연산: 양쪽 타입이 호환되어야 함
            if (!self.symbol_table.types_compatible(left_type, right_type)) {
                try self.addError(
                    "Type mismatch in binary operation: {} and {}",
                    left_type,
                    right_type,
                );
            }
        },
        else => {},
    }
}
```

### 체크리스트

- [ ] SemanticAnalyzer 구조체
- [ ] analyze() 메서드
- [ ] visitNode() - 디스패치 메서드
- [ ] visitVarDecl() - 변수 선언 검증
- [ ] visitFunction() - 함수 정의 검증
- [ ] visitBinaryOp() - 이항 연산 검증
- [ ] visitCall() - 함수 호출 검증
- [ ] visitIdentifier() - 변수 접근 검증
- [ ] inferType() - 타입 추론
- [ ] addError() - 에러 보고

**예상 줄 수**: 500-700줄

**제출물**:
```
zlang-project/
└── src/
    └── semantic/
        └── SemanticAnalyzer.zig  ← 여기!
```

---

## ⚡ Task 3: 코드 생성 통합 (Codegen)

### 개요

시맨틱 검증을 통과한 AST를 **LLVM IR로 변환**합니다.

### 요구사항

방문자 패턴을 이용해 AST의 각 노드를 LLVM 명령어로 변환하세요:

```
【 생성할 LLVM 명령어 】

1. 변수 선언
   AST: VarDecl { name: "x", type: i64, init: 10 }
   IR:  %x = alloca i64
        store i64 10, i64* %x

2. 함수 정의
   AST: Function { name: "add", params: [...], body: {...} }
   IR:  define i64 @add(i64 %a, i64 %b) { ... }

3. 이항 연산
   AST: BinaryOp { op: +, left: 5, right: 3 }
   IR:  %0 = add i64 5, 3

4. 함수 호출
   AST: Call { callee: "add", args: [5, 3] }
   IR:  %0 = call i64 @add(i64 5, i64 3)

5. 제어 흐름
   AST: IfStmt { condition, then, else }
   IR:  br i1 %cond, label %then, label %else
        then:
          ...
        br label %merge
        else:
          ...
        br label %merge
        merge:
          ...
```

### 작성 내용

**파일 이름**: `src/semantic/CodeGenerator.zig`

```zig
pub const CodeGenerator = struct {
    allocator: Allocator,
    symbol_table: SymbolTable,
    context: c.LLVMContextRef,
    module: c.LLVMModuleRef,
    builder: c.LLVMBuilderRef,
    errors: std.ArrayList(CodegenError),

    pub fn init(...) CodeGenerator { }
    pub fn generate(self: *CodeGenerator, ast: *Node) !c.LLVMModuleRef { }

    // 【 방문자 메서드 】
    fn visitNode(self: *CodeGenerator, node: *Node) !?c.LLVMValueRef { }
    fn visitVarDecl(self: *CodeGenerator, node: *Node) !?c.LLVMValueRef { }
    fn visitFunction(self: *CodeGenerator, node: *Node) !?c.LLVMValueRef { }
    fn visitBinaryOp(self: *CodeGenerator, node: *Node) !?c.LLVMValueRef { }
    fn visitCall(self: *CodeGenerator, node: *Node) !?c.LLVMValueRef { }
    fn visitNumber(self: *CodeGenerator, value: f64) !c.LLVMValueRef { }
};
```

### 구체적 요구사항

#### 변수 선언 코드 생성

```zig
fn visitVarDecl(self: *CodeGenerator, node: *Node) !?c.LLVMValueRef {
    const name = node.var_decl.name;
    const var_type_str = node.var_decl.var_type orelse "i64";

    // 1. LLVM 타입 결정
    const llvm_type = switch (var_type_str) {
        "i64" => c.LLVMInt64Type(self.context),
        "i32" => c.LLVMInt32Type(self.context),
        "f64" => c.LLVMDoubleType(self.context),
        else => return error.UnknownType,
    };

    // 2. 메모리 할당
    const alloca_inst = c.LLVMBuildAlloca(self.builder, llvm_type, name);

    // 3. 초기값이 있으면 저장
    if (node.var_decl.init_expr) |init| {
        const init_val = try self.visitNode(init);
        if (init_val != null) {
            _ = c.LLVMBuildStore(self.builder, init_val.?, alloca_inst);
        }
    }

    // 4. 심볼 테이블에 등록
    try self.symbol_table.define(.{
        .name = name,
        .symbol_type = .variable,
        .llvm_value = alloca_inst,
    });

    return null;
}
```

#### 이항 연산 코드 생성

```zig
fn visitBinaryOp(self: *CodeGenerator, node: *Node) !?c.LLVMValueRef {
    const left = try self.visitNode(node.binary_op.left);
    const right = try self.visitNode(node.binary_op.right);

    if (left == null or right == null) return null;

    return switch (node.binary_op.op) {
        .add => c.LLVMBuildAdd(self.builder, left.?, right.?, "addtmp"),
        .sub => c.LLVMBuildSub(self.builder, left.?, right.?, "subtmp"),
        .mul => c.LLVMBuildMul(self.builder, left.?, right.?, "multmp"),
        .div => c.LLVMBuildSDiv(self.builder, left.?, right.?, "divtmp"),
        else => null,
    };
}
```

### 체크리스트

- [ ] CodeGenerator 구조체
- [ ] generate() 메서드
- [ ] visitNode() - 디스패치
- [ ] visitVarDecl() - 변수 선언 IR
- [ ] visitFunction() - 함수 정의 IR
- [ ] visitBinaryOp() - 이항 연산 IR
- [ ] visitCall() - 함수 호출 IR
- [ ] visitNumber() - 상수 IR
- [ ] 상수 폴딩 최적화
- [ ] 메모리 관리

**예상 줄 수**: 600-800줄

**제출물**:
```
zlang-project/
└── src/
    └── semantic/
        └── CodeGenerator.zig  ← 여기!
```

---

## 📖 Task 4: 아키텍처 철학 문서

### 개요

컴파일러 설계의 가장 근본적인 질문:

> **"왜 시맨틱 분석이 파싱 단계보다 나중에 일어나는가?"**

### 요구사항

**3-4페이지 문서**를 작성하여 다음을 설명하세요:

#### 섹션 1: 순서의 이유 (파싱 → 의미분석)

```
【 왜 이 순서인가 】

파싱이 먼저인 이유:
  1. 구문적 정확성이 전제 조건
  2. AST 없이 의미 검증 불가능
  3. 토큰 수준에서는 의미 추론 불가

의미분석이 나중인 이유:
  1. AST라는 정보 구조가 필요
  2. 심볼 테이블 구축 가능
  3. 타입 추론 가능
```

#### 섹션 2: One-Pass vs Multi-Pass

```
【 두 가지 아키텍처 】

One-Pass Compiler (파싱 + 의미분석 + 코드생성)
  장점: 메모리 효율적
  단점: 복잡하고 최적화 어려움
  예: 오래된 Pascal, BASIC

Multi-Pass Compiler (파싱 → 의미분석 → 코드생성)
  장점: 명확한 단계 분리, 재사용 가능
  단점: 메모리 사용량 多
  예: GCC, Clang, Rust
```

#### 섹션 3: 실제 사례

다음 코드로 설명하세요:

```z-lang
fn process(x: i64) {
    var y = x + "10";  // ✗ 타입 에러!
}
```

**파싱 단계:**
- 문법이 올바른가? → YES
- AST 생성 가능한가? → YES

**의미분석 단계:**
- x의 타입은? → i64 (심볼 테이블)
- "10"의 타입은? → string (리터럴 추론)
- i64 + string 가능한가? → NO
- 에러 보고!

**당신의 관점:**
- 파싱에서 이 에러를 미리 감지할 수 있었나?
- 왜 또는 왜 안 되나?

#### 섹션 4: Z-Lang의 설계 원칙

```
【 Z-Lang이 택한 원칙 】

1. 관심사의 분리 (Separation of Concerns)
   - 파싱: 구문적 정확성만 담당
   - 의미분석: 의미적 정확성 담당
   - 코드생성: IR 생성 담당

2. 정보 흐름
   소스코드
    ↓ (렉싱)
   토큰 배열
    ↓ (파싱)
   AST
    ↓ (의미분석)
   검증된 AST + 심볼 테이블
    ↓ (코드생성)
   LLVM IR

3. 에러 처리 정책
   - 파싱 에러: 문법 위반 (즉시 보고)
   - 의미 에러: 타입/스코프 위반 (파싱 후 보고)
   - 생성 에러: IR 불가능 (생성 시점 보고)

4. 확장 가능성
   - 최적화 Pass 추가 가능 (의미분석과 코드생성 사이)
   - 새 타입 시스템 추가 가능
   - 새 타겟 백엔드 추가 가능 (IR 다음)
```

### 작성 내용

**파일 이름**: `docs/SEMANTIC_ARCHITECTURE_PHILOSOPHY.md`

```markdown
# 시맨틱 분석 아키텍처와 철학

## 1. 파싱 → 의미분석 순서의 이유

### 파싱이 먼저인 이유

### 의미분석이 나중인 이유

## 2. One-Pass vs Multi-Pass 아키텍처

### One-Pass Compiler
- 장점과 단점

### Multi-Pass Compiler
- 장점과 단점
- Z-Lang의 선택

## 3. 실제 사례 분석

### 예제 코드: 타입 에러

### 파싱 단계에서는?

### 의미분석 단계에서는?

## 4. Z-Lang의 설계 원칙

### 관심사의 분리
- 각 단계의 책임

### 정보 흐름
- 파이프라인 다이어그램

### 에러 처리 정책
- 단계별 에러 종류

### 확장 가능성
- 미래 개선 방향
```

### 기대 결과물

**체크리스트:**
- [ ] 파싱 → 의미분석 순서의 명확한 설명
- [ ] One-Pass vs Multi-Pass 비교
- [ ] 실제 코드 예제 3-4개
- [ ] Z-Lang의 설계 원칙 명확히 정의
- [ ] 다이어그램 (AST, 정보 흐름)
- [ ] 미래 확장 계획
- [ ] 페이지 수: 3-4장

**제출물**:
```
zlang-project/
└── docs/
    └── SEMANTIC_ARCHITECTURE_PHILOSOPHY.md  ← 여기!
```

---

## 📊 과제 완성 체크리스트

### Task 1: 심볼 테이블 ✅
- [ ] SymbolTable.zig (400-500줄)
- [ ] 스코프 스택 관리
- [ ] 심볼 등록/조회
- [ ] 타입 호환성 검사

### Task 2: 시맨틱 검증 ✅
- [ ] SemanticAnalyzer.zig (500-700줄)
- [ ] 5가지 검증 시나리오
- [ ] 상세한 에러 메시지
- [ ] 방문자 패턴 구현

### Task 3: 코드 생성 ✅
- [ ] CodeGenerator.zig (600-800줄)
- [ ] 변수 선언 IR
- [ ] 이항 연산 IR
- [ ] 함수 호출 IR
- [ ] 상수 폴딩 최적화

### Task 4: 아키텍처 철학 ✅
- [ ] 3-4페이지 문서
- [ ] 설계 원칙 명확화
- [ ] 실제 예제 분석
- [ ] 확장 계획 제시

---

## 🎯 학습 목표

이 과제를 완료하면:

✅ **심볼 테이블 설계 능력**
- 스코프 관리
- 심볼 등록/조회
- 타입 호환성 검사

✅ **시맨틱 검증 능력**
- 변수 해석(Variable Resolution)
- 타입 검사(Type Checking)
- 함수 시그니처 검증
- 제어 흐름 분석

✅ **코드 생성 능력**
- 방문자 패턴
- LLVM API 활용
- 최적화 기법

✅ **컴파일러 아키텍처 이해**
- 다단계 파이프라인
- 관심사 분리
- 정보 흐름

---

## 📚 참고 자료

### Semantic Analysis 관련
- [Crafting Interpreters - Visiting Nodes](https://craftinginterpreters.com/statements-and-state.html)
- [LLVM Programmer's Manual](https://llvm.org/docs/ProgrammersManual/)

### 패턴 관련
- [Visitor Pattern in Compiler Design](https://en.wikipedia.org/wiki/Visitor_pattern)

---

## 🏆 완성 후

이 과제를 마치면:
- 🎉 **완전한 컴파일러의 3/4이 완성됩니다**
- 🚀 **다음은 최적화와 최종 통합입니다**

---

## 💡 팁과 조언

### 디버깅 팁

심볼 테이블을 디버깅할 때:
```zig
pub fn dump(self: *const SymbolTable) void {
    for (self.scopes.items) |scope, i| {
        std.debug.print("Scope {}\n", .{i});
        var it = scope.symbols.iterator();
        while (it.next()) |entry| {
            std.debug.print("  {}: {}\n", .{entry.key, entry.value});
        }
    }
}
```

### 메모리 관리

```zig
// ✅ 올바른 사용
var semantic = SemanticAnalyzer.init(allocator, ...);
defer semantic.deinit();

// ❌ 피해야 할 패턴
// defer 빼먹으면 메모리 누수!
```

### IR 검증

생성된 LLVM IR을 검증하세요:
```bash
llvm-as -o program.bc program.ll   # IR 검증
llvm-dis program.bc -o checked.ll  # 역어셈블
```

---

**당신의 컴파일러가 이제 지능을 갖춥니다!** 🧠✨

*"심볼 테이블은 기억, 의미분석은 지능, 코드생성은 행동입니다."* 🏛️

준비되셨다면 과제를 시작하세요! 🚀
