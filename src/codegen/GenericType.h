#ifndef GENERIC_TYPE_H
#define GENERIC_TYPE_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <optional>

namespace zlang {

// Forward declarations
struct Type;
struct TypeParameter;
struct TypeConstraint;

// ────────────────────────────────────────────────────────────────
// Type Variables
// ────────────────────────────────────────────────────────────────

/**
 * TypeVariable: 아직 결정되지 않은 타입 변수
 *
 * 예: fn id<T>(x: T) -> T에서 T는 TypeVariable
 * 호출 시: id(42) → T가 i64로 결정됨
 */
class TypeVariable {
private:
    std::string name_;           // "T", "U", "E"
    int id_;                     // 고유 ID (타입 변수 정렬용)
    Type* bound_type_;           // 바인드된 구체적 타입 (nullptr = 미결정)
    std::set<std::string> constraints_;  // 제약조건 이름들

public:
    static int next_id_;         // 다음 ID 발급용

    TypeVariable(const std::string& name);
    ~TypeVariable() = default;

    // Getter
    const std::string& name() const { return name_; }
    int id() const { return id_; }
    Type* boundType() const { return bound_type_; }
    bool isBound() const { return bound_type_ != nullptr; }

    // 경로 압축과 함께 바인드된 타입 반환
    Type* getBoundTypeWithCompression() const;

    // 바인드 수행
    void bind(Type* concrete_type);

    // 제약조건 추가
    void addConstraint(const std::string& constraint_name);
    bool hasConstraint(const std::string& constraint_name) const;

    // Unification (두 타입 변수를 하나로 병합)
    bool unifyWith(TypeVariable* other);

    // 문자열 표현
    std::string toString() const;
};

// ────────────────────────────────────────────────────────────────
// Type Parameters
// ────────────────────────────────────────────────────────────────

/**
 * TypeParameter: 제너릭 정의 시 사용된 타입 파라미터
 *
 * 예: struct Vec<T> { ... }에서 T는 TypeParameter
 * where절의 제약조건 포함: <T: Clone + Display>
 */
struct TypeParameter {
    std::string name;           // "T", "U", "E"
    TypeVariable* variable;     // 대응하는 TypeVariable
    std::vector<std::string> trait_bounds;  // ["Clone", "Display"]
    Lifetime* lifetime;         // 라이프타임 (nullable)

    TypeParameter(const std::string& n, TypeVariable* var)
        : name(n), variable(var), lifetime(nullptr) {}

    // 제약조건 추가
    void addBound(const std::string& bound) {
        trait_bounds.push_back(bound);
    }

    std::string toString() const;
};

// ────────────────────────────────────────────────────────────────
// Type Constraints
// ────────────────────────────────────────────────────────────────

/**
 * TypeConstraint: 타입 간의 관계식
 *
 * 종류:
 * - Bound: T: Clone (T는 Clone trait을 구현해야 함)
 * - Equality: T = U (T와 U는 같은 타입)
 * - Subtype: T <: U (T는 U의 부타입)
 * - Compatible: T ~~ U (T와 U는 호환 가능)
 */
struct TypeConstraint {
    enum class Kind {
        Bound,          // T: Trait
        Equality,       // T = U
        Subtype,        // T <: U (subtypes)
        Compatible      // T ~~ U (coercion possible)
    };

    Kind kind;
    Type* left;             // 제약 대상
    std::string right_name; // 제약 이름 ("Clone", "Display" 등)
    Type* right_type;       // 서브타입 제약의 우변 타입

    TypeConstraint(Kind k, Type* l, const std::string& r)
        : kind(k), left(l), right_name(r), right_type(nullptr) {}

    TypeConstraint(Kind k, Type* l, Type* r)
        : kind(k), left(l), right_name(""), right_type(r) {}

    bool check(const Type* actual) const;
    std::string toString() const;
};

// ────────────────────────────────────────────────────────────────
// Generic Type Construction
// ────────────────────────────────────────────────────────────────

/**
 * GenericType: Vec<T>, Option<U>, Result<T, E> 같은 제너릭 타입
 *
 * 정의: Vec<T> where T: Clone
 * 사용: Vec<i64>, Vec<String>
 */
struct GenericType {
    std::string base_name;                      // "Vec", "Option", "Result"
    std::vector<Type*> type_args;              // 호출 시 인자들 [T]
    std::vector<TypeParameter*> type_params;   // 정의 시 파라미터들
    Type* element_type;                        // 기본 타입 (nullable)

    GenericType(const std::string& name)
        : base_name(name), element_type(nullptr) {}

    // Vec<i64>로 구체화
    Type* instantiate(const std::vector<Type*>& args) const;

    // Vec<T>에서 제약조건 추출
    std::vector<TypeConstraint> extractConstraints() const;

    std::string toString() const;
};

// ────────────────────────────────────────────────────────────────
// Generic Function Type
// ────────────────────────────────────────────────────────────────

/**
 * GenericFunctionType: 제너릭 함수의 타입 정보
 *
 * 예: fn map<T, U>(vec: Vec<T>, f: fn(T) -> U) -> Vec<U>
 * 타입 파라미터: [T, U]
 * 파라미터 타입: [Vec<T>, fn(T)->U]
 * 반환 타입: Vec<U>
 */
struct GenericFunctionType {
    std::string name;
    std::vector<TypeParameter*> type_params;    // <T, U>
    std::vector<std::pair<std::string, Type*>> param_types;  // [(x, T), (f, fn(T)->U)]
    Type* return_type;
    bool is_generic;

    GenericFunctionType(const std::string& n)
        : name(n), return_type(nullptr), is_generic(false) {}

    std::string toString() const;
};

// ────────────────────────────────────────────────────────────────
// Substitution (Type Variable Replacement)
// ────────────────────────────────────────────────────────────────

/**
 * Substitution: 타입 변수 치환 맵
 * {T -> i64, U -> String}
 *
 * 제너릭 함수 호출 시:
 * fn id<T>(x: T) -> T를 id(42) 호출 → T는 i64로 치환
 */
using Substitution = std::map<std::string, Type*>;

// Type에 Substitution을 적용
Type* applySubstitution(Type* type, const Substitution& subst);

// ────────────────────────────────────────────────────────────────
// Unification & Resolution
// ────────────────────────────────────────────────────────────────

/**
 * GenericResolver: 제너릭 함수 호출 시 타입 인자 결정
 *
 * 1. 함수 시그니처: fn id<T>(x: T) -> T
 * 2. 호출: id(42)
 * 3. 추론: T = i64
 * 4. 결과: fn(i64) -> i64
 */
class GenericResolver {
public:
    /**
     * resolve: 함수 호출로부터 타입 인자 추론
     *
     * @param func 제너릭 함수 타입
     * @param arg_types 실제 인자 타입들
     * @return 추론된 치환 {T -> i64, ...}
     */
    Substitution resolve(const GenericFunctionType* func,
                        const std::vector<Type*>& arg_types) const;

    /**
     * resolveWithExplicitArgs: 명시적 타입 인자 제공
     * id<i64>(42) 형태
     */
    Substitution resolveWithExplicitArgs(
        const GenericFunctionType* func,
        const std::vector<Type*>& type_args) const;

    /**
     * satisfiesConstraints: 추론된 치환이 제약조건을 만족하는가?
     * Vec<i64>가 Vec<T: Clone>인가? (i64는 Copy라서 Clone 구현)
     */
    bool satisfiesConstraints(const Substitution& subst,
                             const GenericFunctionType* func) const;

    /**
     * validateInstantiation: 타입 인자가 유효한가?
     */
    bool validateInstantiation(const GenericType* generic,
                              const std::vector<Type*>& args) const;

private:
    // 내부 unification 알고리즘
    bool unify(Type* t1, Type* t2, Substitution& result) const;

    // 발생 검사: T = Vec<T>는 무한 타입이므로 거부
    bool occursCheck(TypeVariable* var, Type* type) const;
};

// ────────────────────────────────────────────────────────────────
// Generic Type Database
// ────────────────────────────────────────────────────────────────

/**
 * GenericTypeRegistry: 프로그램 전체의 제너릭 타입 저장소
 */
class GenericTypeRegistry {
private:
    std::map<std::string, GenericType*> generic_types_;
    std::map<std::string, GenericFunctionType*> generic_functions_;
    GenericResolver resolver_;

public:
    // 제너릭 타입 등록
    void registerGenericType(const std::string& name, GenericType* generic);
    GenericType* getGenericType(const std::string& name) const;

    // 제너릭 함수 등록
    void registerGenericFunction(const std::string& name, GenericFunctionType* func);
    GenericFunctionType* getGenericFunction(const std::string& name) const;

    // 인스턴스화
    Type* instantiate(const std::string& generic_name,
                     const std::vector<Type*>& args);

    GenericResolver& getResolver() { return resolver_; }

    // 모든 등록된 제너릭 타입 목록
    std::vector<std::string> listGenericTypes() const;
};

}  // namespace zlang

#endif  // GENERIC_TYPE_H
