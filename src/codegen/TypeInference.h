#ifndef TYPE_INFERENCE_H
#define TYPE_INFERENCE_H

#include "GenericType.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <optional>

namespace zlang {

// Type Inference uses string-based type representation
using TypeInferenceSubstitution = std::unordered_map<std::string, std::string>;

// Forward declarations
struct ASTNode;
struct FunctionType;

// ────────────────────────────────────────────────────────────────
// Type Inference Context
// ────────────────────────────────────────────────────────────────

/**
 * InferenceContext: 타입 추론 중의 상태 관리
 *
 * 변수 환경, 수집된 제약, 해결된 치환을 보관합니다.
 */
struct InferenceContext {
    // 변수 이름 → 타입
    std::unordered_map<std::string, std::string> variable_env;

    // 수집된 제약 (제약 해결 전)
    std::vector<TypeConstraint> constraints;

    // 해결된 치환 ({T -> i64, U -> String, ...})
    TypeInferenceSubstitution substitution;

    // Fresh 타입 변수 ID
    int next_fresh_var_id = 0;

    // Fresh 타입 변수 생성: T_0, T_1, T_2, ...
    std::string createFreshTypeVar() {
        return "T_" + std::to_string(next_fresh_var_id++);
    }

    // 변수 바인딩 (타입 명시)
    void bindVariable(const std::string& name, const std::string& type) {
        variable_env[name] = type;
    }

    // 변수 타입 조회
    std::optional<std::string> lookupVariable(const std::string& name) const {
        auto it = variable_env.find(name);
        if (it != variable_env.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // 컨텍스트 초기화
    void reset() {
        variable_env.clear();
        constraints.clear();
        substitution.clear();
        next_fresh_var_id = 0;
    }

    // 현재 상태 디버그 출력
    std::string toString() const;
};

// ────────────────────────────────────────────────────────────────
// Constraint Collector
// ────────────────────────────────────────────────────────────────

/**
 * ConstraintCollector: AST로부터 타입 제약 추출
 *
 * 표현식을 분석하여 타입 관계를 제약으로 변환합니다.
 *
 * 예:
 *   x + 10
 *   → 제약: x: T_0, 10: i64, (+): fn(i64, i64) -> i64
 *   → T_0 = i64
 */
class ConstraintCollector {
private:
    InferenceContext* context_;

public:
    ConstraintCollector(InferenceContext* ctx) : context_(ctx) {}

    /**
     * collect: 표현식으로부터 제약 수집
     *
     * @param expr AST 노드
     * @return 수집된 제약들
     */
    std::vector<TypeConstraint> collect(const std::string& expr_text);

    /**
     * 이항 연산 제약 수집
     * a + b → {a: T_0, b: i64, T_0 = i64}
     */
    void collectBinaryOpConstraints(
        const std::string& left_text,
        const std::string& right_text,
        const std::string& op,
        std::vector<TypeConstraint>& constraints
    );

    /**
     * 함수 호출 제약 수집
     * f(x, y) → {f: fn(T_0, T_1) -> T_2, x: T_0, y: T_1}
     */
    void collectFunctionCallConstraints(
        const std::string& func_text,
        const std::vector<std::string>& arg_texts,
        std::vector<TypeConstraint>& constraints
    );

    /**
     * 리터럴 타입 제약
     * 42 → i64
     * 3.14 → f64
     */
    std::string getTypeForLiteral(const std::string& literal);

    /**
     * 변수 타입 제약
     * x (where x: i64) → i64
     */
    std::string getTypeForVariable(const std::string& var_name);
};

// ────────────────────────────────────────────────────────────────
// Constraint Solver (Unification)
// ────────────────────────────────────────────────────────────────

/**
 * ConstraintSolver: 제약 해결 (Unification)
 *
 * 수집된 제약들을 unification으로 해결합니다.
 *
 * 예:
 *   제약: [T_0 = i64, T_1 = i64, T_2 = T_1]
 *   → Unification
 *   → {T_0 -> i64, T_1 -> i64, T_2 -> i64}
 */
class ConstraintSolver {
public:
    /**
     * solve: 모든 제약 해결
     *
     * @param constraints 수집된 제약들
     * @param result 해결된 치환 (출력 파라미터)
     * @return 성공 여부
     */
    bool solve(
        const std::vector<TypeConstraint>& constraints,
        TypeInferenceSubstitution& result
    );

    /**
     * unify: 두 타입 통일
     *
     * @param t1 첫 번째 타입 (문자열)
     * @param t2 두 번째 타입 (문자열)
     * @param result 누적 치환
     * @return 성공 여부
     *
     * 예:
     *   unify("T_0", "i64", result) → result["T_0"] = "i64"
     */
    bool unify(const std::string& t1, const std::string& t2, TypeInferenceSubstitution& result);

    /**
     * occursCheck: 무한 타입 감지
     *
     * T_0 = Vec<T_0>는 무한 타입이므로 거부
     *
     * @param var 타입 변수
     * @param type 체크할 타입
     * @return 발생 여부 (true = 무한 타입)
     */
    bool occursCheck(const std::string& var, const std::string& type) const;

    /**
     * applySubstitution: 치환 적용
     *
     * @param type 원본 타입
     * @param subst 치환 ({T_0 -> i64, ...})
     * @return 치환된 타입
     */
    std::string applySubstitution(const std::string& type,
                                  const TypeInferenceSubstitution& subst) const;
};

// ────────────────────────────────────────────────────────────────
// Type Inference Engine
// ────────────────────────────────────────────────────────────────

/**
 * TypeInferenceEngine: 전체 타입 추론 엔진
 *
 * 표현식의 타입을 Hindley-Milner 알고리즘으로 자동 추론합니다.
 *
 * 예:
 *   let x = 42;
 *   → 타입 추론 → x: i64
 *
 *   let y = x + 10;
 *   → 타입 추론 → y: i64
 *
 *   fn add(a, b) { a + b }
 *   → 타입 추론 → fn(i64, i64) -> i64
 */
class TypeInferenceEngine {
private:
    std::unique_ptr<InferenceContext> context_;
    std::unique_ptr<ConstraintCollector> collector_;
    std::unique_ptr<ConstraintSolver> solver_;

public:
    TypeInferenceEngine();
    ~TypeInferenceEngine() = default;

    // ────────────────────────────────────────────────────────────
    // Public API
    // ────────────────────────────────────────────────────────────

    /**
     * inferExprType: 표현식 타입 추론
     *
     * @param expr 표현식 (문자열)
     * @return 추론된 타입
     *
     * 예:
     *   inferExprType("42") → "i64"
     *   inferExprType("x + 10") → "i64" (if x: i64)
     */
    std::string inferExprType(const std::string& expr);

    /**
     * inferFunctionSignature: 함수 서명 추론
     *
     * @param func_text 함수 정의 (문자열)
     * @return 추론된 함수 타입
     *
     * 예:
     *   inferFunctionSignature("fn add(a, b) { a + b }")
     *   → "fn(i64, i64) -> i64"
     */
    std::string inferFunctionSignature(const std::string& func_text);

    /**
     * collectConstraints: 제약 수집 (공개 인터페이스)
     */
    std::vector<TypeConstraint> collectConstraints(const std::string& expr);

    /**
     * solveConstraints: 제약 해결 (공개 인터페이스)
     */
    bool solveConstraints(const std::vector<TypeConstraint>& constraints,
                         TypeInferenceSubstitution& solution);

    /**
     * bindVariable: 변수 명시적 바인딩
     *
     * let x: i64 = 42;에서 x를 i64로 바인드
     */
    void bindVariable(const std::string& name, const std::string& type) {
        context_->bindVariable(name, type);
    }

    /**
     * getVariableType: 변수 타입 조회
     */
    std::optional<std::string> getVariableType(const std::string& name) const {
        return context_->lookupVariable(name);
    }

    /**
     * reset: 추론 엔진 초기화
     */
    void reset() {
        context_->reset();
    }

    /**
     * getContext: 현재 추론 컨텍스트 조회 (테스트용)
     */
    const InferenceContext* getContext() const {
        return context_.get();
    }

private:
    // ────────────────────────────────────────────────────────────
    // Internal Methods
    // ────────────────────────────────────────────────────────────

    /**
     * infer: 내부 추론 함수 (재귀)
     *
     * @param expr 표현식
     * @param substitution 누적 치환
     * @return 추론된 타입
     */
    std::string infer(const std::string& expr, TypeInferenceSubstitution& substitution);

    /**
     * parseLiteral: 리터럴 파싱 및 타입 결정
     *
     * "42" → i64
     * "3.14" → f64
     * "hello" → String
     */
    std::string parseLiteral(const std::string& literal);

    /**
     * inferBinaryOp: 이항 연산 타입 추론
     *
     * a + b → a의 타입과 b의 타입을 분석
     */
    std::string inferBinaryOp(const std::string& left,
                             const std::string& right,
                             const std::string& op,
                             TypeInferenceSubstitution& subst);

    /**
     * getOperatorType: 연산자 시그니처
     *
     * "+" → fn(i64, i64) -> i64
     */
    std::string getOperatorType(const std::string& op);

    /**
     * parseFunctionCall: 함수 호출 파싱
     *
     * "f(1, 2)" → func="f", args=["1", "2"]
     */
    bool parseFunctionCall(const std::string& expr,
                          std::string& func_name,
                          std::vector<std::string>& args);
};

// ────────────────────────────────────────────────────────────────
// Helper Functions
// ────────────────────────────────────────────────────────────────

/**
 * isTypeVariable: 문자열이 타입 변수인지 확인
 * T_0, T_1, ... → true
 */
inline bool isTypeVariable(const std::string& type) {
    return type.find("T_") == 0;
}

/**
 * isConcrete: 타입이 구체적 타입인지 확인
 * i64, String, Vec<i64> → true
 */
inline bool isConcrete(const std::string& type) {
    return !isTypeVariable(type);
}

/**
 * isLiteral: 문자열이 리터럴인지 확인
 * "42" → true
 * "3.14" → true
 * "x" → false
 */
bool isLiteral(const std::string& text);

/**
 * isInteger: 리터럴이 정수인지 확인
 */
bool isInteger(const std::string& literal);

/**
 * isFloat: 리터럴이 실수인지 확인
 */
bool isFloat(const std::string& literal);

}  // namespace zlang

#endif  // TYPE_INFERENCE_H
