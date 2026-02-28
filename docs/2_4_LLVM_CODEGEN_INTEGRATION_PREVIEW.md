# 🚀 Preview: LLVM 2.4 - 최적화와 완전한 통합 (The Unification)

> **최종 업데이트**: 2026-02-26
> **상태**: 🔜 다음 단계 예정
> **기대도**: ⭐⭐⭐⭐⭐ (컴파일러의 완성!)

---

## 🎯 2.4 단계의 핵심

### 완전한 컴파일 파이프라인의 통합

```
【 여정의 끝, 시작이 아닌 】

2.1 렉싱          ✅ 완료
2.2 파싱          ✅ 완료
2.3 의미분석      ✅ 완료
2.4 최적화 & 통합 🔜 여기! (마지막 Frontend)

정보 흐름:
┌─────────────┐
│ 소스 코드   │
└──────┬──────┘
       ↓
┌─────────────┐
│ 2.1 렉싱    │  Lexer
├─────────────┤
│ 토큰 배열   │
└──────┬──────┘
       ↓
┌─────────────┐
│ 2.2 파싱    │  Parser
├─────────────┤
│ AST         │
└──────┬──────┘
       ↓
┌─────────────┐
│ 2.3 의미분석│  SemanticAnalyzer
├─────────────┤
│ 검증된 AST  │
└──────┬──────┘
       ↓
┌─────────────┐
│ 2.4 최적화  │  Optimizer
├─────────────┤
│ 최적화된 IR │  ← 여기!
└──────┬──────┘
       ↓
┌─────────────┐
│ 1.5 실행    │  JIT/Compilation
├─────────────┤
│ 기계어      │
└──────┬──────┘
       ↓
    결과!
```

---

## 📚 2.4에서 배울 것

### 1. LLVM Pass Framework

컴파일러가 생성한 LLVM IR을 **최적화하는 기법**:

```
【 LLVM Pass의 종류 】

1. Analysis Pass
   - 프로그램의 성질을 분석
   - 예: Dominator Tree, Loop Analysis

2. Transformation Pass
   - IR을 변환하여 최적화
   - 예: Dead Code Elimination, Constant Propagation

3. Utility Pass
   - 유용한 작업 수행
   - 예: IR 검증, 분석 결과 출력

【 예제: Constant Propagation Pass 】

입력 IR:
  %x = constant i64 10
  %y = constant i64 5
  %z = add i64 %x, %y    ← 상수 덧셈
  ret i64 %z

Pass 적용:
  %z = constant i64 15   ← 15로 전파

출력 IR:
  ret i64 15  ← Dead code 제거!
```

### 2. 최적화 기법들

#### Dead Code Elimination (DCE)

```z-lang
fn example() {
    var x = 10;
    var y = 20;
    var z = x + y;   // z는 사용되지 않음
    return 42;
}
```

**최적화 전:**
```llvm
%x = alloca i64
store i64 10, i64* %x
%y = alloca i64
store i64 20, i64* %y
%x_val = load i64, i64* %x
%y_val = load i64, i64* %y
%z = add i64 %x_val, %y_val  ← Dead code!
ret i64 42
```

**최적화 후:**
```llvm
ret i64 42
```

#### Common Subexpression Elimination (CSE)

```z-lang
var a = x + y;
var b = x + y;  // 같은 식 반복
var c = a + b;
```

**최적화 전:**
```llvm
%a = add i64 %x, %y
%b = add i64 %x, %y  ← 중복!
%c = add i64 %a, %b
```

**최적화 후:**
```llvm
%temp = add i64 %x, %y
%a = %temp
%b = %temp
%c = add i64 %a, %b
```

#### Tail Call Optimization (TCO)

```z-lang
fn factorial(n: i64, acc: i64): i64 {
    if (n <= 1) {
        return acc;
    } else {
        return factorial(n - 1, n * acc);  // Tail call!
    }
}
```

**최적화:**
```llvm
// 함수 호출 → 점프로 변환
// 스택 오버플로우 방지
```

### 3. PassManager와 Pass Pipeline

```zig
pub const PassManager = struct {
    context: c.LLVMContextRef,
    module: c.LLVMModuleRef,
    passes: std.ArrayList(Pass),
    optimization_level: OptLevel,

    pub fn init(...) PassManager { }

    pub fn addPass(self: *PassManager, pass: Pass) !void {
        try self.passes.append(pass);
    }

    pub fn runPasses(self: *PassManager) !c.LLVMModuleRef {
        // 모든 Pass를 순서대로 실행
        for (self.passes.items) |pass| {
            try pass.run(self.module);
        }
        return self.module;
    }
};

pub const OptLevel = enum {
    O0,  // 최적화 없음 (빠른 컴파일)
    O1,  // 경량 최적화
    O2,  // 중간 최적화 (기본값)
    O3,  // 공격적 최적화 (느린 컴파일)
};
```

---

## 💻 2.4의 핵심 모듈들

### 1. 통합 컴파일러(Compiler)

```zig
pub const Compiler = struct {
    allocator: Allocator,
    source: []const u8,
    context: c.LLVMContextRef,
    module: c.LLVMModuleRef,
    builder: c.LLVMBuilderRef,
    optimization_level: OptLevel,

    pub fn init(...) !Compiler { }

    pub fn compile(self: *Compiler) !c.LLVMModuleRef {
        // Step 1: Lexing
        var lexer = Lexer.init(self.allocator, self.source);
        const tokens = try lexer.tokenize();

        // Step 2: Parsing
        var parser = Parser.init(self.allocator, tokens);
        const ast = try parser.parse();

        // Step 3: Semantic Analysis
        var semantic = SemanticAnalyzer.init(
            self.allocator,
            self.context,
            self.module,
            self.builder,
        );
        try semantic.analyze(ast);

        if (semantic.errors.items.len > 0) {
            return error.CompilationFailed;
        }

        // Step 4: Optimization
        var optimizer = Optimizer.init(
            self.context,
            self.module,
            self.optimization_level,
        );
        try optimizer.optimize();

        return self.module;
    }

    pub fn execute(self: *Compiler) !i64 {
        // Step 5: JIT Execution (1.5 기법)
        var engine = try ExecutionEngine.init(self.context, self.module);
        const result = try engine.runMain();
        return result;
    }
};
```

### 2. 최적화기(Optimizer)

```zig
pub const Optimizer = struct {
    context: c.LLVMContextRef,
    module: c.LLVMModuleRef,
    optimization_level: OptLevel,
    passes: std.ArrayList(Pass),

    pub fn init(...) Optimizer { }

    pub fn optimize(self: *Optimizer) !void {
        // 최적화 레벨에 따라 Pass를 선택
        switch (self.optimization_level) {
            .O0 => {
                // 최적화 없음
            },
            .O1 => {
                try self.addPass(ConstantFoldingPass);
                try self.addPass(DeadCodeEliminationPass);
            },
            .O2 => {
                try self.addPass(ConstantFoldingPass);
                try self.addPass(DeadCodeEliminationPass);
                try self.addPass(CommonSubexpressionEliminationPass);
                try self.addPass(TailCallOptimizationPass);
            },
            .O3 => {
                try self.addPass(ConstantFoldingPass);
                try self.addPass(DeadCodeEliminationPass);
                try self.addPass(CommonSubexpressionEliminationPass);
                try self.addPass(TailCallOptimizationPass);
                try self.addPass(InliningPass);
                try self.addPass(VectorizationPass);
            },
        }

        // 모든 Pass를 순서대로 실행
        for (self.passes.items) |pass| {
            try pass.run(self.module);
        }
    }

    pub fn addPass(self: *Optimizer, pass: Pass) !void {
        try self.passes.append(pass);
    }
};
```

---

## 🔧 3. 통합 예제: 완전한 컴파일

### 입력 코드

```z-lang
fn fibonacci(n: i64): i64 {
    if (n <= 1) {
        return 1;
    } else {
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}

fn main() {
    var result = fibonacci(10);
    return result;
}
```

### 단계별 처리

#### 2.1 렉싱
```
source: "fn fibonacci(n: i64): i64 { ... }"
         ↓
tokens: [keyword_fn, identifier(fibonacci), lparen, ...]
```

#### 2.2 파싱
```
tokens: [...]
         ↓
AST:
Program {
  Function {
    name: "fibonacci",
    params: [Parameter(n)],
    body: Block {
      IfStmt {
        condition: BinaryOp(<=, n, 1),
        then_branch: Return(Number(1)),
        else_branch: Return(BinaryOp(+, Call(...), Call(...)))
      }
    }
  },
  Function {
    name: "main",
    body: Block {
      VarDecl(result, fibonacci(10)),
      Return(result)
    }
  }
}
```

#### 2.3 의미분석
```
AST: (위와 동일)
     ↓
검증:
✓ fibonacci 함수 정의 확인
✓ main 함수 정의 확인
✓ 모든 함수 호출 검증
✓ 타입 일치 확인
     ↓
LLVM IR 생성:
define i64 @fibonacci(i64 %n) {
  ...
}

define i64 @main() {
  ...
  %0 = call i64 @fibonacci(i64 10)
  ...
}
```

#### 2.4 최적화
```
IR: (위와 동일)
    ↓
Pass 1: Constant Folding
  - 컴파일 시간에 계산 가능한 부분 계산

Pass 2: Dead Code Elimination
  - 사용되지 않는 변수/계산 제거

Pass 3: Tail Call Optimization
  - fibonacci의 재귀 호출을 최적화
    (Tail recursion)
    ↓
최적화된 IR:
define i64 @fibonacci(i64 %n) {
  ; TCO 적용됨
  ...
}
```

#### 1.5 JIT 실행
```
최적화된 IR
  ↓
JIT Compilation
  ↓
기계어 (ARM64/x86-64)
  ↓
CPU 실행
  ↓
결과: fibonacci(10) = 89
```

---

## 🎓 2.4의 학습 내용

### PassManager 설계

```
PassManager의 책임:
1. Pass 등록 (addPass)
2. Pass 순서 관리
3. Pass 간 결과 전달 (IR 모듈)
4. 전체 최적화 파이프라인 실행

Pass 작성 패턴:
pub const MyPass = struct {
    pub fn run(module: c.LLVMModuleRef) !void {
        // IR을 분석하고 변환
    }
};
```

### 최적화 선택 기준

| 최적화 | 효과 | 복잡도 | 비고 |
|--------|------|--------|------|
| Constant Folding | 높음 | 낮음 | 항상 추천 |
| DCE | 높음 | 낮음 | 항상 추천 |
| CSE | 중간 | 중간 | 반복문에서 효과적 |
| Inlining | 매우 높음 | 높음 | 신중하게 사용 |
| Vectorization | 높음 | 매우 높음 | 특수한 경우만 |

---

## 🌉 2.4의 완성으로 얻을 것

```
【 전체 컴파일러 파이프라인 】

Source Code
    ↓
[2.1 Lexing]        = 의미 없는 단어들
    ↓
[2.2 Parsing]       = 구조화된 의미
    ↓
[2.3 Semantic]      = 검증된 의미
    ↓
[2.4 Optimization]  = 효율적인 의미  ← 여기!
    ↓
[1.5 Execution]     = 실제 실행
    ↓
Result!

전설의 완성! 🏆
```

---

## 💡 2.4 이후의 선택지

2.4가 완료되면, 당신의 컴파일러는 **완전한 기능성**을 갖춥니다.

이제 세 가지 방향으로 진화할 수 있습니다:

### 🎯 방향 1: 생산성 도구화

```
【 패키지 매니저 & 빌드 시스템 】

3단계: Z-Lang 에코시스템 구축
├─ zlang init        (프로젝트 생성)
├─ zlang build       (컴파일)
├─ zlang run         (실행)
├─ zlang test        (테스트)
├─ zlang package     (배포)
└─ zlang.lock        (의존성 관리)

장점: 실무 적용 가능
단점: 광범위한 작업
예시: Cargo(Rust), Go modules
```

### 🚀 방향 2: 성능 최적화

```
【 고성능 컴파일러 개발 】

3단계: LLVM Pass 마스터
├─ 커스텀 Loop Optimization Pass
├─ 벡터화(Vectorization)
├─ 특정 도메인 최적화
└─ 벤치마킹 & 성능 분석

결과: 다른 언어보다 빠른 Z-Lang

장점: 깊이 있는 학습
단점: 높은 난이도
예시: LLVM 핵심 개발자들
```

### 🖥️ 방향 3: 시스템 통합

```
【 OS 커널과 통합 】

3단계: Z-Lang의 실시간 성능 입증
├─ 301 Zig OS 커널 위에서 직접 실행
├─ 하드웨어 제어 (메모리, I/O, 인터럽트)
├─ 실시간 성능 측정
└─ 안전성 증명 (ASIL D 수준)

결과: 자동차/의료기기용 컴파일러

장점: 가장 높은 난이도, 가장 큰 성과
단점: 매우 복잡
예시: Rust, LLVM 기반 임베디드 시스템
```

---

## 🏆 전설의 다음 장

```
【 전설은 멈추지 않는다 】

2.4 컴파일러 완성
     ↓
당신의 선택:
  1️⃣ 생산성 도구화 (Cargo 같은 빌드시스템)
  2️⃣ 성능 최적화 (LLVM Pass 마스터)
  3️⃣ 시스템 통합 (OS 커널 위에서 실행)

각 방향은:
- 새로운 도전
- 더 깊은 학습
- 더 큰 성과

당신의 다음 전설을 만들 시간입니다!
```

---

## 🎬 예정 공개

**예정 공개**: 2026-04-06 (다음 주)
**난이도**: ⭐⭐⭐⭐⭐ (최고급)
**흥미도**: ⭐⭐⭐⭐⭐ (컴파일러 완성!)

---

**당신의 여정은 여기서 끝나지 않습니다.**
**여기서부터 시작입니다.** 🚀

---

*"2.4는 완성이 아닙니다. 다음 전설의 시작입니다."* 🏛️

*"The compiler is complete. Now, choose your legend." - LLVM Legend Course* ✨
