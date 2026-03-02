#include "IntegratedTypeChecker.h"
#include <algorithm>
#include <sstream>

namespace zlang {

// ════════════════════════════════════════════════════════════════════════════
// Constructor & Destructor
// ════════════════════════════════════════════════════════════════════════════

IntegratedTypeChecker::IntegratedTypeChecker()
    : type_inference_(std::make_unique<TypeInferenceEngine>()) {
}

// ════════════════════════════════════════════════════════════════════════════
// Function Registration
// ════════════════════════════════════════════════════════════════════════════

void IntegratedTypeChecker::registerGenericFunction(
    const std::string& func_name,
    const GenericFunctionType& generic_fn) {
    generic_functions_[func_name] = generic_fn;
}

void IntegratedTypeChecker::registerConcreteFunction(
    const std::string& func_name,
    const std::string& return_type,
    const std::vector<std::string>& param_types) {
    // 함수 타입 저장: "fn(i64, i64) -> i64"
    std::ostringstream oss;
    oss << "fn(";
    for (size_t i = 0; i < param_types.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << param_types[i];
    }
    oss << ") -> " << return_type;
    function_types_[func_name] = oss.str();
}

// ════════════════════════════════════════════════════════════════════════════
// Function Call Type Checking
// ════════════════════════════════════════════════════════════════════════════

std::string IntegratedTypeChecker::checkFunctionCall(
    const std::string& func_name,
    const std::vector<std::string>& arg_exprs) {

    // 1. 함수 정의 확인
    if (function_types_.find(func_name) == function_types_.end() &&
        generic_functions_.find(func_name) == generic_functions_.end()) {
        return "";  // 함수 정의 없음
    }

    // 2. 인수 타입 추론
    std::vector<std::string> arg_types;
    for (const auto& arg : arg_exprs) {
        std::string arg_type = type_inference_->inferExprType(arg);
        arg_types.push_back(arg_type);
    }

    // 3. 구체적 함수인 경우
    if (function_types_.find(func_name) != function_types_.end()) {
        const auto& func_type = function_types_[func_name];

        // 함수 타입 파싱: "fn(i64, i64) -> i64"
        size_t arrow_pos = func_type.find("->");
        if (arrow_pos == std::string::npos) return "";

        std::string return_type = func_type.substr(arrow_pos + 3);

        // TODO: 매개변수 타입 추출 및 검증
        return return_type;
    }

    // 4. 제너릭 함수인 경우
    if (generic_functions_.find(func_name) != generic_functions_.end()) {
        // 타입 인수 자동 추론
        return instantiateGenericFunction(func_name, arg_types);
    }

    return "";
}

// ════════════════════════════════════════════════════════════════════════════
// Type Inference (Forward to TypeInferenceEngine)
// ════════════════════════════════════════════════════════════════════════════

std::string IntegratedTypeChecker::inferExprType(const std::string& expr) {
    return type_inference_->inferExprType(expr);
}

void IntegratedTypeChecker::bindVariable(
    const std::string& name,
    const std::string& type) {
    type_inference_->bindVariable(name, type);
}

std::optional<std::string> IntegratedTypeChecker::getVariableType(
    const std::string& name) const {
    return type_inference_->getVariableType(name);
}

void IntegratedTypeChecker::reset() {
    type_inference_->reset();
}

// ════════════════════════════════════════════════════════════════════════════
// Type Compatibility
// ════════════════════════════════════════════════════════════════════════════

bool IntegratedTypeChecker::isTypeCompatible(
    const std::string& actual,
    const std::string& expected) const {

    // 정확히 같으면 호환
    if (actual == expected) return true;

    // 타입 변수는 모든 타입과 호환 (나중에 해결)
    if (isTypeVariable(actual) || isTypeVariable(expected)) {
        return true;
    }

    // 다른 경우 비호환
    return false;
}

bool IntegratedTypeChecker::isTypeVariable(const std::string& type) {
    return type.find("T_") == 0;
}

bool IntegratedTypeChecker::isConcrete(const std::string& type) {
    return !isTypeVariable(type);
}

// ════════════════════════════════════════════════════════════════════════════
// Type Resolution
// ════════════════════════════════════════════════════════════════════════════

std::string IntegratedTypeChecker::resolveTypeVariable(
    const std::string& type_var) {
    // TODO: 제약 기반 해결
    return type_var;
}

std::string IntegratedTypeChecker::instantiateGenericFunction(
    const std::string& func_name,
    const std::vector<std::string>& type_args) {

    if (generic_functions_.find(func_name) == generic_functions_.end()) {
        return "";
    }

    const auto& generic_fn = generic_functions_[func_name];

    // 간단한 경우: 첫 번째 인수 타입을 반환
    // 실제로는 더 복잡한 템플릿 해결이 필요
    if (!type_args.empty()) {
        return type_args[0];
    }

    return "";
}

}  // namespace zlang
