# Z-Lang LLVM 1.4 Week 2: 타입 추론 엔진 (Type Inference Engine)

**프로젝트**: Z-Lang - LLVM 기반 실시간 시스템 프로그래밍 언어
**Phase**: LLVM 1.4 Type System Enhancement
**주차**: Week 2 (2026-03-02 ~ 2026-03-09)
**기반**: Week 1 제너릭 타입 시스템 (GenericType.h/cpp)

---

## 📋 Executive Summary

Week 1에서 제너릭 타입의 기초를 구축했습니다 (TypeVariable, GenericResolver 등).
Week 2에서는 **Hindley-Milner 타입 추론 알고리즘**을 구현하여,
프로그래머가 모든 타입을 명시하지 않아도 자동으로 타입이 추론되도록 합니다.

### 핵심 목표
```
let x = 42;              // ← i64 자동 추론
let y = x + 10;          // ← i64 + i64 = i64
fn add(a, b) { a + b }  // ← fn(i64, i64) -> i64 추론
let f = |z| z * 2;       // ← fn(i64) -> i64 추론
```

---

## 🎯 타입 추론의 원리

### Hindley-Milner 알고리즘

```
Step 1: 제약 수집 (Constraint Collection)
  ─────────────────────────────────────
  let x = 42;
  └─ x: T, 42: i64
     제약: T = i64

Step 2: 제약 해결 (Constraint Solving)
  ─────────────────────────────────────
  제약들: {T = i64, U = String, ...}
  해결: Unification → {T ↦ i64, U ↦ String, ...}

Step 3: 타입 결정 (Type Determination)
  ─────────────────────────────────────
  x: T
  Substitution: {T ↦ i64}
  결과: x: i64
```

### 예시: x + 10의 타입 추론

```
Input:
  let x: i64 = 42;
  let y = x + 10;

Step 1: Constraint Collection
  ─────────────────────────────
  x: i64 (명시)
  10: T_1 (리터럴, 타입 미결정)
  (+): fn(i64, i64) -> i64 (연산자 정의)
  y: T_2 (결과, 타입 미결정)

  제약들:
    • T_1 = i64    (10은 i64와 호환)
    • T_2 = i64    (더하기 결과)

Step 2: Unification
  ─────────────────
  {T_1 = i64, T_2 = i64}

Step 3: Substitution
  ──────────────────
  10: i64
  y: i64
```

---

## 🏗️ 설계

### TypeInference.h (350줄)

```cpp
namespace zlang {

// ────────────────────────────────────────
// Type Inference Engine
// ────────────────────────────────────────

class TypeInferenceEngine {
public:
    // 1. 표현식 타입 추론
    Type* inferExprType(const ASTNode* expr);

    // 2. 함수 서명 추론
    FunctionType* inferFunctionType(const ASTNode* func_def);

    // 3. 제약 수집
    std::vector<TypeConstraint> collectConstraints(const ASTNode* expr);

    // 4. 제약 해결 (Unification)
    bool solveConstraints(
        const std::vector<TypeConstraint>& constraints,
        Substitution& solution
    );

    // 5. 변수 타입 환경 조회
    Type* lookupVariable(const std::string& var_name) const;

    // 6. 추론 상태 리셋
    void reset();

private:
    // 내부 추론 함수
    Type* infer(const ASTNode* expr, Substitution& env);

    // 이항 연산자 추론
    Type* inferBinaryOp(
        const ASTNode* left,
        const ASTNode* right,
        const std::string& op,
        Substitution& env
    );

    // 함수 호출 추론
    Type* inferFunctionCall(
        const ASTNode* func_expr,
        const std::vector<ASTNode*>& args,
        Substitution& env
    );

    // 리터럴 타입 추론
    Type* inferLiteral(const ASTNode* literal);

    // 변수 사용 타입 추론
    Type* inferVariable(const std::string& var_name);

    // If/While 제어 흐름 타입 추론
    Type* inferIfExpr(const ASTNode* if_expr, Substitution& env);
    Type* inferWhileExpr(const ASTNode* while_expr, Substitution& env);
};

// ────────────────────────────────────────
// Type Inference Context
// ────────────────────────────────────────

struct InferenceContext {
    std::map<std::string, Type*> variable_env;  // 변수 환경
    std::vector<TypeConstraint> constraints;     // 수집된 제약
    Substitution substitution;                    // 해결된 치환
    int next_fresh_var_id = 0;                   // Fresh 타입 변수

    // Fresh 타입 변수 생성 (T_0, T_1, T_2, ...)
    TypeVariable* createFreshTypeVar();

    // 변수 바인딩
    void bindVariable(const std::string& name, Type* type);

    // 변수 환경 조회
    Type* lookupVariable(const std::string& name) const;
};

// ────────────────────────────────────────
// Constraint Collector
// ────────────────────────────────────────

class ConstraintCollector {
public:
    // 표현식으로부터 제약 수집
    std::vector<TypeConstraint> collect(const ASTNode* expr);

    // 이항 연산 제약
    void collectBinaryOpConstraints(
        const ASTNode* left,
        const ASTNode* right,
        const std::string& op,
        std::vector<TypeConstraint>& constraints
    );

    // 함수 호출 제약
    void collectFunctionCallConstraints(
        const ASTNode* func_expr,
        const std::vector<ASTNode*>& args,
        std::vector<TypeConstraint>& constraints
    );

private:
    InferenceContext* context_;
};

// ────────────────────────────────────────
// Constraint Solver (Unification)
// ────────────────────────────────────────

class ConstraintSolver {
public:
    // 제약들을 모두 해결
    // 성공 → substitution 반환
    // 실패 → false 반환
    bool solve(
        const std::vector<TypeConstraint>& constraints,
        Substitution& result
    );

    // 두 타입을 통일
    bool unify(Type* t1, Type* t2, Substitution& result);

    // 발생 검사 (무한 타입 방지)
    bool occursCheck(TypeVariable* var, Type* type) const;
};

}  // namespace zlang
```

### 주요 클래스

#### TypeInferenceEngine
- 전체 타입 추론 엔진
- 표현식, 함수, 제약 처리

#### InferenceContext
- 추론 중 상태 보관
- 변수 환경, 제약, 치환

#### ConstraintCollector
- AST에서 제약 추출
- 모든 타입 관계를 제약으로 변환

#### ConstraintSolver
- 수집된 제약 해결 (Unification)
- Occurs Check로 무한 타입 방지

---

## 🧪 테스트 전략 (15개)

### Category 1: 리터럴 추론 (3개)
```cpp
test("infer_integer_literal")
  let x = 42;  // x: i64 추론

test("infer_float_literal")
  let y = 3.14;  // y: f64 추론

test("infer_string_literal")
  let s = "hello";  // s: String 추론
```

### Category 2: 이항 연산 (3개)
```cpp
test("infer_binary_op_add_integers")
  let z = 10 + 20;  // z: i64 추론

test("infer_binary_op_multiply")
  let a = 5 * 6;  // a: i64 추론

test("infer_binary_op_type_mismatch")
  let b = 1 + 2.5;  // 에러: i64 + f64 불가능
```

### Category 3: 변수 사용 (3개)
```cpp
test("infer_variable_from_context")
  let x = 42;
  let y = x + 10;  // y: i64 (x가 i64이므로)

test("infer_multiple_variables")
  let a = 1;
  let b = 2;
  let c = a + b;  // c: i64

test("infer_variable_reuse")
  let x = 10;
  let y = x * 2;  // y: i64
  let z = x + y;  // z: i64
```

### Category 4: 함수 호출 (3개)
```cpp
test("infer_function_call_argument_type")
  fn add(x, y) { x + y }
  let result = add(1, 2);  // result: i64 추론

test("infer_nested_function_calls")
  fn mul(a, b) { a * b }
  let r = mul(add(1, 2), 3);  // r: i64

test("infer_function_with_literals")
  fn double(n) { n * 2 }
  let doubled = double(5);  // doubled: i64
```

### Category 5: 제어 흐름 (3개)
```cpp
test("infer_if_expression")
  let x = if (true) { 10 } else { 20 };  // x: i64

test("infer_while_loop")
  let x = 0;
  while (x < 10) { x = x + 1; }  // x: i64

test("infer_complex_expression")
  let x = if (5 > 3) { 100 } else { 200 };
  let y = x + 50;  // y: i64
```

---

## 📈 예상 산출물

```
TypeInference.h        350줄
TypeInference.cpp      400줄
test_type_inference    350줄
─────────────────────────────
총합                1,100줄

테스트:  15/15 (100% 예상)
커밋:    3개 (설계, 구현, 테스트)
```

---

## 🎯 성공 기준

| 항목 | 목표 | 달성 기준 |
|------|------|---------|
| **코드 라인** | 1,100줄 | ≥ 1,000줄 |
| **테스트 통과** | 15/15 | 100% |
| **제약 수집** | 완벽 | 모든 표현식 지원 |
| **Unification** | 안전 | Occurs check 포함 |
| **타입 결정** | 정확 | 예상 타입과 일치 |

---

## 🚀 Week 2 일정

| 단계 | 내용 | 예상 시간 |
|------|------|---------|
| **1** | TypeInference.h 설계 | 1-2시간 |
| **2** | TypeInference.cpp 구현 | 2-3시간 |
| **3** | 제약 수집기 구현 | 1시간 |
| **4** | 제약 해결기 구현 | 1시간 |
| **5** | test_type_inference 작성 | 1-2시간 |
| **6** | 테스트 실행 및 버그 수정 | 1시간 |

---

## 📚 이전 참고 자료

- Week 1: GenericType.h/cpp (제너릭 기초)
- Unification 알고리즘 (Week 1에서 구현)
- TypeVariable, Substitution (Week 1에서 구현)

---

**작성자**: Claude Code AI
**최종 업데이트**: 2026-03-02
**상태**: 📋 **계획 수립 완료, 구현 준비 완료**
