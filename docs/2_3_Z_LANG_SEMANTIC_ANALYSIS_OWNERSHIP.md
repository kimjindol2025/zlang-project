# 🏛️ LLVM 전설: 2.3 의미 분석 - Z-Lang의 소유권 시스템 (The Soul)

> **최종 업데이트**: 2026-02-26
> **상태**: ✅ 완성 (Z-Lang의 생존 본능)
> **난이도**: ⭐⭐⭐⭐⭐⭐ (전설의 정점)
> **핵심**: 소유권(Ownership) - GC 없이 메모리 안전성을 보장하는 마법

---

## 🎯 2.3 단계: Z-Lang의 차별점

### "말이 맞다"에서 "그 말이 안전한가?"로

```
【 2.2 vs 2.3의 차이 】

2.2 파싱 (Parsing)
  "fn add(x, y) { return x + y; }"
         ↓
  AST (구문적으로 올바름)

2.3 의미분석 (Semantic Analysis) ← 여기!
  AST
         ↓
  ✓ x, y가 정의되었나?
  ✓ x, y의 타입은?
  ✓ x + y 연산이 안전한가?
  ✓ x가 이미 move되지 않았나? (소유권!)
  ✓ return 후 x를 또 사용하려 하지 않나?
         ↓
  검증된 AST (의미적으로 안전함!)
```

---

## 📚 1. 심볼 테이블(Symbol Table): 컴파일러의 기억

### 1.1 심볼 테이블의 역할

Z-Lang이 **변수, 함수, 타입의 정보를 기억**하는 구조입니다.

```cpp
// 【 심볼 테이블의 핵심 데이터 구조 】

struct SymbolInfo {
    std::string name;
    std::string type;           // i32, i64, f64, string, ...
    OwnershipState state;       // Available, Moved, Borrowed
    int llvm_value;             // LLVM IR 할당 주소
    bool is_mutable;            // mut인가?
    SourceLocation location;    // 선언 위치 (에러 보고용)
};

enum class OwnershipState {
    Available,      // 사용 가능
    Moved,          // 소유권 이전됨
    Borrowed,       // 빌려짐 (&로 참조)
};

// 스코프 스택: 중첩된 블록을 관리
using Scope = std::unordered_map<std::string, SymbolInfo>;

class SymbolTable {
private:
    std::vector<Scope> scopes;  // 스택 구조

public:
    void pushScope() {
        scopes.push_back(Scope());
    }

    void popScope() {
        if (!scopes.empty()) {
            scopes.pop_back();
        }
    }

    // 심볼 등록
    void define(const std::string& name, const SymbolInfo& info) {
        scopes.back()[name] = info;
    }

    // 심볼 조회 (가장 가까운 스코프부터 역순 탐색)
    SymbolInfo* lookup(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;  // 찾지 못함
    }
};
```

### 1.2 스코프 스택 시각화

```z-lang
fn outer() {                    // Global Scope 진입
    var x = 10;                 // x 등록

    {                           // Block Scope 진입
        var y = 20;             // y 등록
        print(x);               // ✓ x 찾음 (부모 스코프)
    }                           // Block Scope 퇴출

    // print(y);               // ✗ y를 찾을 수 없음!
}
```

**스코프 스택 상태 변화:**

```
시작:
  [Global Scope: {outer: function}]

outer() 호출:
  [Global Scope] → [Outer Scope: {x: i32, state: Available}]

{ } 블록 진입:
  [Global] → [Outer] → [Block: {y: i32, state: Available}]

} 블록 퇴출:
  [Global] → [Outer] → (Block 제거)

함수 종료:
  [Global] (Outer 제거)
```

---

## 🔐 2. Z-Lang의 핵심: 소유권(Ownership) 시스템

### 2.1 소유권의 법칙

```
【 소유권의 3대 법칙 (Rust에서 영감) 】

1. 하나의 값은 하나의 주인만 가진다
   let x = 42;  // x가 42의 주인

2. 주인이 스코프를 벗어나면 값은 자동 해제된다
   {
       let x = Box::new(vec![1,2,3]);
   }  // 스코프 끝 → x가 메모리 해제

3. 소유권은 이동(Move)되거나 빌려질(Borrow) 수 있다
   let y = x;      // Move: x의 소유권이 y로 이동
   let z = &x;     // Borrow: x를 빌려줌 (x는 여전히 주인)
```

### 2.2 소유권 이동(Move) vs 빌림(Borrow)

```z-lang
【 예제 1: Move (소유권 이전) 】

fn main() {
    let x = Box::new(100);    // x가 100을 소유
    let y = x;                // 소유권이 x → y로 이동!

    print(x);                 // ✗ 컴파일 에러!
                              // "x has been moved"
    print(y);                 // ✓ OK
}

심볼 테이블 변화:
  x: {type: Box<i32>, state: Moved}
  y: {type: Box<i32>, state: Available}
```

```z-lang
【 예제 2: Borrow (참조, 소유권 유지) 】

fn main() {
    let x = Box::new(100);    // x가 100을 소유
    let y = &x;               // x를 빌려줌 (& = 빌림)

    print(x);                 // ✓ OK! (여전히 주인)
    print(*y);                // ✓ OK! (역참조로 접근)
}

심볼 테이블 변화:
  x: {type: Box<i32>, state: Borrowed}
  y: {type: &Box<i32>, state: Available}
```

```z-lang
【 예제 3: 함수 호출과 Move 】

fn consume(val: Box<i32>) {
    // val의 소유권을 받아서 함수 끝에서 자동 해제
}

fn main() {
    let x = Box::new(100);
    consume(x);               // x의 소유권이 함수로 이동

    print(x);                 // ✗ 컴파일 에러!
                              // x는 더 이상 유효하지 않음
}
```

### 2.3 소유권 추적 알고리즘

```cpp
// 【 AST 순회 중 소유권 관리 】

void SemanticAnalyzer::visitAssignment(ASTNode* node) {
    // let y = x; 형태를 처리

    ASTNode* lhs = node->left;   // y
    ASTNode* rhs = node->right;  // x

    SymbolInfo* source = symbol_table.lookup(rhs->name);

    if (source == nullptr) {
        error("Undefined variable: " + rhs->name);
        return;
    }

    // 소유권 체크
    if (source->state == OwnershipState::Moved) {
        error("Value of '" + rhs->name + "' was moved");
        return;
    }

    // Move 연산: 소유권 이전
    source->state = OwnershipState::Moved;

    // 새로운 심볼 등록
    SymbolInfo new_sym = *source;
    new_sym.name = lhs->name;
    new_sym.state = OwnershipState::Available;
    symbol_table.define(lhs->name, new_sym);
}
```

---

## 🎯 3. 타입 검사(Type Checking) 및 타입 추론(Type Inference)

### 3.1 타입 시스템

```cpp
enum class BuiltinType {
    I32,        // 32-bit signed integer
    I64,        // 64-bit signed integer
    F32,        // 32-bit float
    F64,        // 64-bit float
    Bool,       // boolean
    String,     // string slice
};

struct Type {
    BuiltinType base;
    bool is_pointer;    // * 포인터인가?
    bool is_reference;  // & 참조인가?
    bool is_mutable;    // mut인가?
};
```

### 3.2 타입 추론 (Type Inference)

사용자가 타입을 명시하지 않으면, **우변(RHS)의 타입**을 추론합니다.

```z-lang
let x = 10;           // 10은 i32 → x: i32로 추론
let y = 3.14;         // 3.14는 f64 → y: f64로 추론
let s = "hello";      // "hello"는 string → s: string으로 추론
```

### 3.3 타입 호환성 검사

```cpp
void SemanticAnalyzer::visitBinaryOp(ASTNode* node) {
    ASTNode* left = node->left;
    ASTNode* right = node->right;

    Type left_type = inferType(left);
    Type right_type = inferType(right);

    // 이항 연산자별 타입 검사
    switch (node->op) {
        case BinaryOp::Add:
        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
            // 산술 연산: 양쪽이 숫자 타입이어야 함
            if (!isNumericType(left_type) || !isNumericType(right_type)) {
                error("Type mismatch in binary operation");
            }
            if (left_type != right_type) {
                // Z-Lang: 자동 형변환 없음! (타입 안전)
                error("Types must match exactly: " +
                      typeToString(left_type) + " vs " +
                      typeToString(right_type));
            }
            break;

        case BinaryOp::And:
        case BinaryOp::Or:
            // 논리 연산: 양쪽이 bool이어야 함
            if (left_type.base != BuiltinType::Bool ||
                right_type.base != BuiltinType::Bool) {
                error("Logical operations require bool operands");
            }
            break;
    }
}

Type SemanticAnalyzer::inferType(ASTNode* node) {
    switch (node->type) {
        case NodeType::Number:
            // 숫자 리터럴
            if (isFloatingPoint(node->value)) {
                return Type{.base = BuiltinType::F64};
            } else {
                return Type{.base = BuiltinType::I64};
            }

        case NodeType::StringLiteral:
            return Type{.base = BuiltinType::String};

        case NodeType::Identifier: {
            SymbolInfo* sym = symbol_table.lookup(node->name);
            if (sym == nullptr) {
                error("Undefined variable: " + node->name);
                return Type{.base = BuiltinType::I32};  // 에러 복구
            }
            return parseType(sym->type);
        }

        case NodeType::BinaryOp: {
            Type left = inferType(node->left);
            Type right = inferType(node->right);
            // 연산 결과 타입은 피연산자 타입과 동일
            // (자동 형변환 없으므로)
            return left;
        }
    }
}
```

---

## ⚡ 4. Z-Lang 특화: 실시간 제약 조건(Real-Time Constraints) 검증

### 4.1 #[no_alloc] 제약 검증

```z-lang
#[no_alloc]  // ← 이 함수는 메모리 할당 금지!
fn hard_real_time_task() {
    let arr: [i32; 100];      // ✓ OK: 정적 할당
    let x = 10 + 20;          // ✓ OK: 단순 계산

    // let vec = Vec::new();   // ✗ 에러: 동적 할당
    // foo(x);                 // ✗ 에러: foo가 할당하면 안됨
}
```

### 4.2 no_alloc 검증 알고리즘

```cpp
void SemanticAnalyzer::visitFunction(ASTNode* func_node) {
    // 함수에 #[no_alloc] 어노테이션이 있는가?
    if (!hasAnnotation(func_node, "no_alloc")) {
        return;  // 일반 함수, 검증 불필요
    }

    // no_alloc 함수의 바디를 순회
    checkNoAlloc(func_node->body);
}

void SemanticAnalyzer::checkNoAlloc(ASTNode* node) {
    switch (node->type) {
        case NodeType::Call: {
            // 함수 호출이 있는가?
            SymbolInfo* called_func = symbol_table.lookup(node->function_name);

            // 호출된 함수가 할당을 하는가?
            if (called_func && called_func->may_allocate) {
                error("no_alloc violation: " + node->function_name +
                      " may allocate memory");
            }
            break;
        }

        case NodeType::Allocation:
            // Box::new, Vec::new 등 직접 할당
            error("no_alloc violation: Dynamic allocation not allowed");
            break;

        case NodeType::Block: {
            // 재귀적으로 모든 문장 검사
            for (ASTNode* stmt : node->statements) {
                checkNoAlloc(stmt);
            }
            break;
        }
    }
}
```

---

## ⏱️ 5. WCET(Worst Case Execution Time) 분석

### 5.1 WCET 분석의 필요성

Z-Lang은 **실시간 시스템을 위한 언어**입니다.

```
【 WCET가 중요한 이유 】

자동차 ABS 시스템:
  ├─ 휠 속도 센서 입력 (매 10ms마다)
  ├─ ABS 제어 계산 (반드시 7ms 이내!)
  └─ 제동 신호 출력

만약 계산이 10ms 이상 걸리면?
  → 앞 바퀴가 ABS 신호 없이 잠김
  → 자동차가 미끄러짐
  → 대형 사고!

따라서 "최악의 경우에도 몇 ms 안에 끝날 것"을
컴파일 타임에 증명해야 함!
```

### 5.2 WCET 어노테이션과 검증

```z-lang
#[wcet_bound = "100_us"]  // 이 함수는 100 마이크로초 이내에 완료!
fn sensor_interrupt_handler() {
    let reading = read_adc(0);           // 하드웨어 I/O: ~50us
    let filtered = apply_low_pass(reading);  // 신호 처리: ~30us
    update_state(filtered);               // 상태 갱신: ~15us
}  // 총 95us < 100us ✓
```

### 5.3 WCET 검증 로직

```cpp
struct WCETInfo {
    long long bound_micros;     // 상한 (마이크로초)
    long long actual_estimate;  // 추정 실행 시간
};

void SemanticAnalyzer::visitFunction(ASTNode* func_node) {
    // WCET 어노테이션이 있는가?
    WCETInfo* wcet = getWCETAnnotation(func_node);
    if (!wcet) return;  // 없으면 검증 불필요

    // 함수 바디의 예상 실행 시간 계산
    long long estimated = estimateWCET(func_node->body);

    if (estimated > wcet->bound_micros) {
        error("WCET violation: estimated " +
              std::to_string(estimated) + "us > " +
              std::to_string(wcet->bound_micros) + "us");
    }
}

long long SemanticAnalyzer::estimateWCET(ASTNode* node) {
    long long total = 0;

    switch (node->type) {
        case NodeType::Assignment:
            total += 5;  // 변수 할당: ~5us
            total += estimateWCET(node->rhs);  // RHS 계산
            break;

        case NodeType::Call: {
            SymbolInfo* func = symbol_table.lookup(node->function_name);
            if (func && func->wcet_estimate > 0) {
                total += func->wcet_estimate;  // 함수 호출 시간
            } else {
                // 알 수 없으면 보수적으로 추정
                total += 1000;  // 최악: 1000us
            }
            break;
        }

        case NodeType::IfStatement:
            // if-else: 최악의 경우 (더 오래 걸리는 브랜치)
            total += 10;  // 조건 평가: ~10us
            total += std::max(
                estimateWCET(node->then_branch),
                estimateWCET(node->else_branch)
            );
            break;

        case NodeType::WhileLoop:
            // 루프: 최악의 경우 (최대 반복)
            total += 10;  // 루프 오버헤드
            total += getMaxIterations(node) * estimateWCET(node->body);
            break;
    }

    return total;
}
```

---

## 🎯 6. 의미 분석 알고리즘 통합

### 6.1 의미 분석 단계

```
【 AST 순회와 의미 검증 】

1단계: 함수 선언 수집
  └─ 모든 함수를 symbol_table에 등록

2단계: 함수별 의미 분석
  └─ 각 함수의 바디를 순회
    ├─ 지역 변수 등록 (pushScope)
    ├─ 각 문장 분석
    │   ├─ 변수 사용 → 심볼 테이블 조회
    │   ├─ 소유권 체크 (Moved?)
    │   ├─ 타입 체크 (호환?)
    │   └─ 실시간 제약 체크 (no_alloc?)
    └─ 함수 스코프 종료 (popScope)

3단계: 완벽한 Annotated AST 생성
  └─ 모든 노드에 타입, 소유권 정보 기록
```

### 6.2 의미 분석의 출력

```cpp
// 의미 분석 완료 후:
// 원본 AST → Annotated AST

struct AnnotatedNode {
    ASTNode* original;
    Type inferred_type;
    OwnershipState ownership;
    long long wcet_estimate;
    std::vector<SemanticError> errors;
};

std::vector<AnnotatedNode> analyzed_ast;
```

---

## 🏆 요약: 2.3의 역할

| 책임 | 역할 | 예제 |
|------|------|------|
| **기억** | Symbol Table | x: i32, state: Available |
| **안전성** | Ownership | let y = x; → x.state = Moved |
| **타입** | Type Checking | i32 + i32 ✓, i32 + string ✗ |
| **실시간** | WCET | #[wcet_bound = "100_us"] 검증 |

---

## 💪 2.3의 완성

```
【 2.3 완료 후 】

입력: AST (파싱 완료)
      ↓
출력: Annotated AST
      + 모든 타입 정보
      + 모든 소유권 정보
      + 모든 WCET 정보
      + 의미 오류 목록
```

**이제 Z-Lang은 진정한 지능을 갖춘 컴파일러입니다!** 🧠

---

*"2.3에서 Z-Lang은 GC 없이도 메모리를 안전하게 관리하는 마법을 배웁니다."* ✨

*"소유권 시스템 - 이것이 실시간 언어의 영혼입니다."* 🏛️
