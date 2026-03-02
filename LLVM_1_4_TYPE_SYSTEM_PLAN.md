# Z-Lang LLVM 1.4: 고급 타입 시스템 설계 및 구현 계획

**프로젝트**: Z-Lang LLVM 컴파일러
**Phase**: LLVM 1.4 (Type System Enhancement)
**시작일**: 2026-03-02
**목표 완료일**: 2026-03-16 (2주)
**목표 평가**: 95% 이상 달성

---

## 📋 Executive Summary

Z-Lang Phase 3 (91% 달성)에서 기본 타입 시스템을 갖추었습니다. LLVM 1.4에서는 **제너릭 타입, 타입 추론, 복합 타입** 등을 추가하여 **현대적 프로그래밍 언어** 수준으로 발전시킵니다.

### 핵심 목표
- ✅ 제너릭 타입 (Generic Type) 지원
- ✅ 타입 추론 (Type Inference) 구현
- ✅ 복합 타입 (Struct/Enum) 지원
- ✅ 라이프타임 (Lifetime) 추적
- ✅ 타입 호환성 검사 (Type Compatibility)
- ✅ 제너릭 함수 (Generic Function)
- ✅ 타입 캐스팅 (Implicit/Explicit)

### 예상 산출물
- 3개 신규 헤더: `GenericType.h`, `TypeInference.h`, `ComplexTypes.h`
- 2개 신규 구현: `TypeChecker2.cpp`, `TypeResolver.cpp`
- 150+ 테스트 케이스
- 2,000+ 줄 신규 코드

---

## 🎯 Phase 분석

### 현재 타입 시스템 (Type.h)

**강점**:
```cpp
✅ 기본 타입 정의 (i32, i64, f32, f64, bool, void)
✅ 포인터 & 배열 타입
✅ Result<T, E> & Option<T> 기본 구조
✅ 소유권 상태 추적 (Available/Moved/Borrowed)
```

**약점**:
```cpp
❌ 제너릭 타입 미지원
❌ 타입 추론 불가능 (모든 타입 명시 필요)
❌ 복합 타입 (Struct/Enum) 정의 불가
❌ 라이프타임 추적 없음
❌ 타입 호환성 검사 기본적임
❌ 제너릭 함수 미지원
```

**문제점 분석**:

| 문제 | 현 상황 | 예시 | 해결 방안 |
|------|--------|------|---------|
| **제너릭 부재** | 모든 타입을 미리 정의해야 함 | `Vec<i64>`, `Vec<String>` 별도 구현 | `Generic<T>` 메커니즘 도입 |
| **타입 추론 불가** | 모든 변수에 타입 명시 필요 | `let x = 42;` 불가능, `let x: i64 = 42;` 필수 | HM 타입 추론 알고리즘 |
| **복합 타입 미지원** | 기본 타입만 사용 가능 | `struct Point { x: i64, y: i64 }` 불가 | AST 확장 + 새 타입 클래스 |
| **라이프타임 부재** | 참조의 유효 기간 미추적 | 댕글링 포인터 가능 | 라이프타임 어노테이션 시스템 |

---

## 🏗️ LLVM 1.4 설계

### 아키텍처

```
┌─────────────────────────────────────────────┐
│     Z-Lang LLVM 1.4 Type System             │
├─────────────────────────────────────────────┤
│                                             │
│  ┌──────────────────────────────────────┐  │
│  │  Type Core Layer                     │  │
│  ├──────────────────────────────────────┤  │
│  │ • BaseType: All types inherit        │  │
│  │ • GenericType<T>: Parameterized     │  │
│  │ • TypeVariable: Unresolved types    │  │
│  │ • TypeConstraint: Type bounds       │  │
│  └──────────────────────────────────────┘  │
│                                             │
│  ┌──────────────────────────────────────┐  │
│  │  Type Inference Layer                │  │
│  ├──────────────────────────────────────┤  │
│  │ • Unification: Constraint solving    │  │
│  │ • HM Algorithm: Hindley-Milner      │  │
│  │ • Type Propagation: Forward/Backward│  │
│  │ • Generic Instantiation: T → i64    │  │
│  └──────────────────────────────────────┘  │
│                                             │
│  ┌──────────────────────────────────────┐  │
│  │  Complex Type Layer                  │  │
│  ├──────────────────────────────────────┤  │
│  │ • StructType: User-defined records  │  │
│  │ • EnumType: Algebraic data types    │  │
│  │ • UnionType: Type union             │  │
│  │ • TupleType: Anonymous product      │  │
│  └──────────────────────────────────────┘  │
│                                             │
│  ┌──────────────────────────────────────┐  │
│  │  Lifetime & Ownership Layer          │  │
│  ├──────────────────────────────────────┤  │
│  │ • Lifetime: 'a, 'b parameterization│  │
│  │ • BorrowChecker: Ensure safety      │  │
│  │ • RefType: Reference with lifetime  │  │
│  │ • MoveSemantics: Owned vs borrowed  │  │
│  └──────────────────────────────────────┘  │
│                                             │
│  ┌──────────────────────────────────────┐  │
│  │  Type Compatibility Layer            │  │
│  ├──────────────────────────────────────┤  │
│  │ • Subtyping: T <: U relationships   │  │
│  │ • Coercion: Implicit conversion     │  │
│  │ • Casting: Explicit conversion      │  │
│  │ • Variance: Contra/Co/Invariant     │  │
│  └──────────────────────────────────────┘  │
│                                             │
└─────────────────────────────────────────────┘
```

---

## 📦 구현 계획 (5주)

### Week 1: 기초 인프라 (제너릭 기반)

#### 1.1 헤더: `src/codegen/GenericType.h` (400줄)

```cpp
namespace zlang {

// ────────────────────────────────────────
// Type Variables & Parameters
// ────────────────────────────────────────

// 아직 결정되지 않은 타입 변수
// 예: fn id<T>(x: T) -> T에서 T
struct TypeVariable {
    std::string name;          // "T", "U", "E"
    int id;                    // 고유 ID (정렬용)
    std::set<TypeConstraint> constraints;  // 제약조건

    bool isBound() const;      // 구체적 타입으로 바인드되었는가?
    Type* getBoundType() const;
    void bind(Type* concrete);
};

// 제너릭 파라미터
struct TypeParameter {
    std::string name;
    TypeVariable* variable;
    std::vector<TypeConstraint> bounds;  // trait bounds

    // where T: Clone + Display
    bool satisfies(const Type* t) const;
};

// ────────────────────────────────────────
// Generic Type Construction
// ────────────────────────────────────────

// Vec<T>, Option<U> 같은 제너릭 타입
struct GenericType {
    std::string baseName;      // "Vec", "Option", "Result"
    std::vector<Type*> typeArgs;  // [T], [U], [T, E]
    std::vector<TypeParameter*> typeParams;  // 정의상의 파라미터

    // Vec<i64>의 경우: baseName="Vec", typeArgs=[i64 Type]
    Type* instantiate(const std::vector<Type*>& args) const;
};

// 제너릭 함수 시그니처
struct GenericFunctionType {
    std::string name;
    std::vector<TypeParameter*> typeParams;  // <T, U>
    std::vector<Type*> paramTypes;
    Type* returnType;

    // fn map<T, U>(vec: Vec<T>, f: fn(T) -> U) -> Vec<U>
    // typeParams = [T, U]
    // paramTypes = [Vec<T>, fn(T)->U]
    // returnType = Vec<U>
};

// Type Constraint (제약조건)
struct TypeConstraint {
    enum class Kind {
        Bound,          // T: Clone (T는 Clone을 구현해야 함)
        Equality,       // T = U (T와 U는 같은 타입)
        Subtype,        // T <: U (T는 U의 부타입)
        Compatible      // T ~~ U (T와 U는 호환 가능)
    };

    Kind kind;
    Type* left;
    Type* right;

    bool check(const Type* actual) const;
};

// ────────────────────────────────────────
// Substitution & Unification
// ────────────────────────────────────────

// 타입 변수 치환 맵: {T -> i64, U -> String}
using Substitution = std::map<std::string, Type*>;

// 제너릭 함수 호출 시 타입 인자 결정
class GenericResolver {
public:
    // fn id<T>(x: T) -> T를 호출할 때
    // callSite: id(42)
    // resultType가 추론됨 → T = i64 → i64
    Substitution resolve(const GenericFunctionType* func,
                        const std::vector<Type*>& argTypes) const;

    // 제약조건 확인
    bool satisfiesConstraints(const Substitution& subst,
                             const GenericFunctionType* func) const;
};

}
```

#### 1.2 구현: `src/codegen/GenericType.cpp` (350줄)

```cpp
#include "GenericType.h"
#include <algorithm>

namespace zlang {

// ────────────────────────────────────────
// TypeVariable Implementation
// ────────────────────────────────────────

static int nextTypeVarId = 0;

TypeVariable::TypeVariable(const std::string& n)
    : name(n), id(nextTypeVarId++) {}

bool TypeVariable::isBound() const {
    return boundType != nullptr;
}

Type* TypeVariable::getBoundType() const {
    if (!isBound()) return nullptr;

    // 경로 압축 (Path Compression)
    if (boundType->is_type_var) {
        return static_cast<TypeVariable*>(boundType)->getBoundType();
    }
    return boundType;
}

void TypeVariable::bind(Type* concrete) {
    if (isBound()) {
        // 이미 바인드되었으면 unification 수행
        unify(getBoundType(), concrete);
        return;
    }
    boundType = concrete;
}

// ────────────────────────────────────────
// GenericType Implementation
// ────────────────────────────────────────

Type* GenericType::instantiate(const std::vector<Type*>& args) const {
    if (args.size() != typeParams.size()) {
        return nullptr;  // 인자 개수 불일치
    }

    // 치환 맵 생성: {T -> args[0], U -> args[1]}
    Substitution subst;
    for (size_t i = 0; i < typeParams.size(); i++) {
        subst[typeParams[i]->name] = args[i];
    }

    // 기본 타입의 모든 TypeVariable을 치환
    return applySubstitution(baseType, subst);
}

// ────────────────────────────────────────
// GenericResolver Implementation
// ────────────────────────────────────────

Substitution GenericResolver::resolve(
    const GenericFunctionType* func,
    const std::vector<Type*>& argTypes) const {

    Substitution result;

    // 함수 파라미터 타입과 실제 인자 타입 비교
    for (size_t i = 0; i < func->paramTypes.size(); i++) {
        unify(func->paramTypes[i], argTypes[i], result);
    }

    return result;
}

}
```

---

### Week 2: 타입 추론 엔진

#### 2.1 헤더: `src/codegen/TypeInference.h` (350줄)

```cpp
namespace zlang {

// ────────────────────────────────────────
// Type Inference Engine
// ────────────────────────────────────────

// Hindley-Milner 타입 시스템 구현
class TypeInferenceEngine {
public:
    // 표현식의 타입 추론
    // let x = 42;  // T = i64 추론
    Type* inferExprType(const ASTNode* expr);

    // 함수 서명 추론
    // fn add(a, b) { return a + b; }  // fn(i64, i64) -> i64 추론
    FunctionType* inferFunctionType(const ASTNode* funcDef);

    // 제약조건 수집
    std::vector<TypeConstraint> collectConstraints(const ASTNode* expr);

    // Unification을 통한 제약조건 해결
    bool solveConstraints(const std::vector<TypeConstraint>& constraints,
                         Substitution& solution);

private:
    // 표현식 타입 추론 (내부)
    Type* infer(const ASTNode* expr, Substitution& env);

    // 이항 연산자 타입 추론
    Type* inferBinaryOp(const ASTNode* left, const ASTNode* right,
                       const std::string& op);

    // 함수 호출 타입 추론
    Type* inferFunctionCall(const ASTNode* callExpr);
};

// ────────────────────────────────────────
// Unification Algorithm
// ────────────────────────────────────────

// 두 타입이 같은지 확인하고 치환 생성
class Unification {
public:
    // unify(T, i64) -> {T -> i64}
    bool unify(Type* t1, Type* t2, Substitution& result);

    // 여러 제약조건 동시 해결
    bool unifyAll(const std::vector<std::pair<Type*, Type*>>& pairs,
                 Substitution& result);

private:
    bool unifyHelper(Type* t1, Type* t2, Substitution& result);

    // 발생 검사 (Occurs Check)
    // T = Vec<T>는 무한 타입이므로 거부
    bool occursCheck(TypeVariable* var, Type* type);
};

}
```

#### 2.2 구현: `src/codegen/TypeInference.cpp` (400줄)

---

### Week 3: 복합 타입 시스템

#### 3.1 헤더: `src/codegen/ComplexTypes.h` (300줄)

```cpp
namespace zlang {

// ────────────────────────────────────────
// Complex Type Definitions
// ────────────────────────────────────────

// 구조체 타입
// struct Point { x: i64, y: i64 }
struct StructType {
    std::string name;
    std::vector<FieldDecl> fields;

    struct FieldDecl {
        std::string name;
        Type* type;
        size_t offset;  // 메모리 레이아웃에서의 오프셋
    };

    Type* getFieldType(const std::string& fieldName) const;
    size_t getSize() const;  // 메모리 크기
};

// 열거형 타입
// enum Result<T> { Ok(T), Err(String) }
struct EnumType {
    std::string name;
    std::vector<VariantDecl> variants;

    struct VariantDecl {
        std::string name;
        std::vector<Type*> payloadTypes;  // Ok(T) -> [T]
    };
};

// 튜플 타입 (익명 곱)
// (i64, String, bool)
struct TupleType {
    std::vector<Type*> elements;

    Type* getElementType(size_t index) const;
    size_t getSize() const;
};

// 함수 타입
// fn(i64, String) -> bool
struct FunctionType {
    std::vector<Type*> paramTypes;
    Type* returnType;
    std::vector<TypeParameter*> typeParams;  // 제너릭인 경우
};

}
```

---

### Week 4: 라이프타임 추적

#### 4.1 헤더: `src/codegen/Lifetime.h` (250줄)

```cpp
namespace zlang {

// ────────────────────────────────────────
// Lifetime System
// ────────────────────────────────────────

// 라이프타임 파라미터 'a, 'b, 'c
struct Lifetime {
    std::string name;  // "a", "b", "static"
    int id;

    // 라이프타임 관계: 'a : 'b (a는 b 이상 오래 산다)
    std::vector<Lifetime*> outlives;
};

// 참조 타입과 라이프타임 연동
// &'a T (T에 대한 'a 라이프타임의 참조)
struct RefTypeWithLifetime {
    Type* refereeType;
    Lifetime* lifetime;
    bool isMutable;
};

// 라이프타임 검사기
class BorrowChecker {
public:
    // 함수 내에서 모든 참조가 유효한가?
    bool checkBorrows(const FunctionDef* func);

    // 댕글링 포인터 감지
    bool hasDanglingRef(const RefTypeWithLifetime* ref);

private:
    // SSA 형식으로 라이프타임 추적
    std::map<std::string, Lifetime*> liveness;
};

}
```

---

### Week 5: 타입 호환성 & 통합

#### 5.1 헤더: `src/codegen/TypeCompatibility.h` (300줄)

---

## 🧪 테스트 계획 (50+ 테스트)

### 테스트 카테고리

1. **제너릭 타입 테스트** (15개)
   ```cpp
   test("generic_vector_i64")
   test("generic_vector_string")
   test("nested_generic_option_result")
   test("generic_function_identity")
   test("generic_function_map")
   // ...
   ```

2. **타입 추론 테스트** (15개)
   ```cpp
   test("infer_literal_integer")
   test("infer_literal_float")
   test("infer_binary_op_add")
   test("infer_function_return")
   test("infer_variable_from_context")
   // ...
   ```

3. **복합 타입 테스트** (10개)
   ```cpp
   test("struct_definition")
   test("struct_field_access")
   test("enum_variant_construction")
   test("tuple_destructuring")
   // ...
   ```

4. **라이프타임 테스트** (10개)
   ```cpp
   test("lifetime_basic_ref")
   test("lifetime_multiple_refs")
   test("lifetime_dangling_ref_error")
   test("lifetime_mutable_borrow_conflict")
   // ...
   ```

---

## 📅 마일스톤

| 주차 | Task | 산출물 | 테스트 |
|------|------|--------|--------|
| **1** | 제너릭 기초 | GenericType.h/cpp | 10개 |
| **2** | 타입 추론 | TypeInference.h/cpp | 15개 |
| **3** | 복합 타입 | ComplexTypes.h | 10개 |
| **4** | 라이프타임 | Lifetime.h | 10개 |
| **5** | 통합 & 버그 수정 | 완료 보고서 | 50개 |

---

## 🎯 성공 기준

- ✅ 50+ 테스트 모두 통과
- ✅ 2,000+ 줄 신규 코드
- ✅ 제너릭 함수 구현 가능
- ✅ 타입 추론으로 명시 없이 코드 작성 가능
- ✅ 복합 타입 정의 및 사용 가능
- ✅ 메모리 안전성 보장
- ✅ 최종 평가 95% 이상 달성

---

**작성자**: Claude Code AI
**최종 업데이트**: 2026-03-02
**상태**: 📋 계획 수립 완료, 구현 준비 완료
