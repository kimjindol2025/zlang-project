#ifndef INTEGRATED_TYPE_CHECKER_H
#define INTEGRATED_TYPE_CHECKER_H

#include "TypeInference.h"
#include "GenericType.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <optional>

namespace zlang {

// ────────────────────────────────────────────────────────────────
// Integrated Type Checker
// ────────────────────────────────────────────────────────────────

/**
 * IntegratedTypeChecker: Week 1 GenericType + Week 2 TypeInference 통합
 *
 * 역할:
 *   1. 함수 호출 시 전달된 인수의 타입을 TypeInference로 추론
 *   2. 함수 정의의 매개변수 타입을 GenericType으로 취득
 *   3. 두 타입의 호환성 검증
 *   4. 필요시 템플릿 인스턴스화
 *
 * 예:
 *   template<T> T identity(T x) { return x; }
 *   let y = identity(42);
 *   → T = i64 추론 → 함수 인스턴스화 → 호출
 */
class IntegratedTypeChecker {
private:
    std::unique_ptr<TypeInferenceEngine> type_inference_;
    std::unordered_map<std::string, GenericFunctionType> generic_functions_;
    std::unordered_map<std::string, std::string> function_types_;

public:
    IntegratedTypeChecker();
    ~IntegratedTypeChecker() = default;

    // ────────────────────────────────────────────────────────────
    // Public API
    // ────────────────────────────────────────────────────────────

    /**
     * registerGenericFunction: 템플릿 함수 등록
     *
     * @param func_name 함수 이름
     * @param generic_fn 제너릭 함수 타입
     */
    void registerGenericFunction(
        const std::string& func_name,
        const GenericFunctionType& generic_fn
    );

    /**
     * registerConcreteFunction: 구체적 함수 등록
     *
     * @param func_name 함수 이름
     * @param return_type 반환 타입 (예: "i64")
     * @param param_types 매개변수 타입들
     */
    void registerConcreteFunction(
        const std::string& func_name,
        const std::string& return_type,
        const std::vector<std::string>& param_types
    );

    /**
     * checkFunctionCall: 함수 호출 타입 검사
     *
     * @param func_name 함수 이름
     * @param arg_exprs 인수 표현식들 (예: ["42", "x"])
     * @return 반환 타입 (예: "i64") 또는 빈 문자열 (오류)
     */
    std::string checkFunctionCall(
        const std::string& func_name,
        const std::vector<std::string>& arg_exprs
    );

    /**
     * inferExprType: 표현식 타입 추론
     *
     * @param expr 표현식
     * @return 추론된 타입
     */
    std::string inferExprType(const std::string& expr);

    /**
     * bindVariable: 변수 바인딩
     *
     * @param name 변수 이름
     * @param type 타입
     */
    void bindVariable(const std::string& name, const std::string& type);

    /**
     * getVariableType: 변수 타입 조회
     */
    std::optional<std::string> getVariableType(const std::string& name) const;

    /**
     * reset: 체커 초기화
     */
    void reset();

    // ────────────────────────────────────────────────────────────
    // Type Compatibility
    // ────────────────────────────────────────────────────────────

    /**
     * isTypeCompatible: 두 타입의 호환성 검증
     *
     * @param actual 실제 타입
     * @param expected 기대하는 타입
     * @return 호환 여부
     *
     * 규칙:
     *   - i64 = i64 ✓
     *   - T_0 = i64 ✓ (T_0은 미결정)
     *   - i64 = f64 ✗
     */
    bool isTypeCompatible(
        const std::string& actual,
        const std::string& expected
    ) const;

    /**
     * isTypeVariable: 타입이 변수인지 확인
     */
    static bool isTypeVariable(const std::string& type);

    /**
     * isConcrete: 타입이 구체적인지 확인
     */
    static bool isConcrete(const std::string& type);

    // ────────────────────────────────────────────────────────────
    // Type Resolution
    // ────────────────────────────────────────────────────────────

    /**
     * resolveTypeVariable: 타입 변수 해결
     *
     * 예: T_0 → i64 (제약으로부터)
     */
    std::string resolveTypeVariable(const std::string& type_var);

    /**
     * instantiateGenericFunction: 템플릿 인스턴스화
     *
     * @param func_name 함수 이름
     * @param type_args 타입 인수들
     * @return 인스턴스화된 함수의 반환 타입
     */
    std::string instantiateGenericFunction(
        const std::string& func_name,
        const std::vector<std::string>& type_args
    );
};

}  // namespace zlang

#endif  // INTEGRATED_TYPE_CHECKER_H
