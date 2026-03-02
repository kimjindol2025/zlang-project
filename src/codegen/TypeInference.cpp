#include "TypeInference.h"
#include <sstream>
#include <regex>
#include <algorithm>
#include <cctype>

namespace zlang {

// ────────────────────────────────────────────────────────────────
// InferenceContext Implementation
// ────────────────────────────────────────────────────────────────

std::string InferenceContext::toString() const {
    std::ostringstream oss;
    oss << "InferenceContext:\n";
    oss << "  Variables:\n";
    for (const auto& pair : variable_env) {
        oss << "    " << pair.first << ": " << pair.second << "\n";
    }
    oss << "  Constraints: " << constraints.size() << "\n";
    oss << "  Substitution: " << substitution.size() << "\n";
    return oss.str();
}

// ────────────────────────────────────────────────────────────────
// Helper Functions
// ────────────────────────────────────────────────────────────────

bool isLiteral(const std::string& text) {
    if (text.empty()) return false;

    // 문자열 리터럴: "hello"
    if (text.front() == '"' && text.back() == '"') {
        return true;
    }

    // true, false
    if (text == "true" || text == "false") {
        return true;
    }

    // 숫자 리터럴 (정수 또는 실수): 42, -10, 3.14, -2.5
    if (std::isdigit(text[0]) || (text[0] == '-' && text.size() > 1)) {
        size_t start = (text[0] == '-') ? 1 : 0;
        size_t dot_count = 0;
        for (size_t i = start; i < text.size(); ++i) {
            if (text[i] == '.') {
                dot_count++;
                if (dot_count > 1) return false;  // 점이 2개 이상
            } else if (!std::isdigit(text[i])) {
                return false;  // 숫자나 점이 아닌 문자
            }
        }
        return true;  // 정수 또는 실수
    }

    return false;
}

bool isInteger(const std::string& literal) {
    if (literal.empty()) return false;
    size_t start = (literal[0] == '-') ? 1 : 0;
    return std::all_of(literal.begin() + start, literal.end(),
                      [](unsigned char c) { return std::isdigit(c); });
}

bool isFloat(const std::string& literal) {
    return literal.find('.') != std::string::npos;
}

// ────────────────────────────────────────────────────────────────
// ConstraintCollector Implementation
// ────────────────────────────────────────────────────────────────

std::vector<TypeConstraint> ConstraintCollector::collect(
    const std::string& expr_text) {

    std::vector<TypeConstraint> constraints;

    // 간단한 파싱: 숫자, 변수, 이항 연산 지원
    // 더 복잡한 파싱은 AST 필요

    // 예: "42" → i64
    if (isLiteral(expr_text)) {
        // 리터럴은 제약 필요 없음 (타입 직접 결정)
        return constraints;
    }

    // 예: "x + 10"
    // 정규식으로 이항 연산 감지
    std::regex binary_op_regex(R"((\w+)\s*([\+\-\*\/])\s*(\w+))");
    std::smatch match;

    if (std::regex_match(expr_text, match, binary_op_regex)) {
        std::string left = match[1].str();
        std::string op = match[2].str();
        std::string right = match[3].str();

        collectBinaryOpConstraints(left, right, op, constraints);
    }

    return constraints;
}

void ConstraintCollector::collectBinaryOpConstraints(
    const std::string& left_text,
    const std::string& right_text,
    const std::string& op,
    std::vector<TypeConstraint>& constraints) {

    // 좌측 타입
    std::string left_type;
    if (isLiteral(left_text)) {
        left_type = getTypeForLiteral(left_text);
    } else {
        left_type = getTypeForVariable(left_text);
    }

    // 우측 타입
    std::string right_type;
    if (isLiteral(right_text)) {
        right_type = getTypeForLiteral(right_text);
    } else {
        right_type = getTypeForVariable(right_text);
    }

    // 연산자에 따른 제약
    // +, -, *, / : 양쪽 모두 i64 또는 f64 호환
    // (간단히 같은 타입이어야 함)
}

std::string ConstraintCollector::getTypeForLiteral(
    const std::string& literal) {

    if (isInteger(literal)) {
        return "i64";  // 기본 정수 타입
    }

    if (isFloat(literal)) {
        return "f64";  // 기본 실수 타입
    }

    if (literal.front() == '"' && literal.back() == '"') {
        return "String";
    }

    if (literal == "true" || literal == "false") {
        return "bool";
    }

    return "Unknown";
}

std::string ConstraintCollector::getTypeForVariable(
    const std::string& var_name) {

    auto opt_type = context_->lookupVariable(var_name);
    if (opt_type.has_value()) {
        return opt_type.value();
    }

    // 미결정 변수 → Fresh 타입 변수 생성
    return context_->createFreshTypeVar();
}

// ────────────────────────────────────────────────────────────────
// ConstraintSolver Implementation
// ────────────────────────────────────────────────────────────────

bool ConstraintSolver::solve(const std::vector<TypeConstraint>& constraints,
                            TypeInferenceSubstitution& result) {
    // 모든 제약을 하나씩 unify
    for (const auto& constraint : constraints) {
        // constraint를 문자열로 변환하여 처리
        // (실제로는 TypeConstraint 객체를 직접 처리)
    }

    return true;  // 단순화: 항상 성공
}

bool ConstraintSolver::unify(const std::string& t1,
                            const std::string& t2,
                            TypeInferenceSubstitution& result) {
    // 동일한 타입
    if (t1 == t2) {
        return true;
    }

    // t1이 타입 변수면 t2로 바인드
    if (isTypeVariable(t1)) {
        if (occursCheck(t1, t2)) {
            return false;  // 무한 타입
        }
        result[t1] = t2;
        return true;
    }

    // t2가 타입 변수면 t1로 바인드
    if (isTypeVariable(t2)) {
        if (occursCheck(t2, t1)) {
            return false;
        }
        result[t2] = t1;
        return true;
    }

    // 둘 다 구체적 타입이지만 다름
    return false;
}

bool ConstraintSolver::occursCheck(const std::string& var,
                                   const std::string& type) const {
    // 간단한 구현: 문자열 포함 검사
    // Vec<T_0>에서 T_0 검색
    return type.find(var) != std::string::npos;
}

std::string ConstraintSolver::applySubstitution(
    const std::string& type,
    const TypeInferenceSubstitution& subst) const {

    // 타입 변수면 치환
    if (isTypeVariable(type)) {
        auto it = subst.find(type);
        if (it != subst.end()) {
            return it->second;
        }
    }

    return type;
}

// ────────────────────────────────────────────────────────────────
// TypeInferenceEngine Implementation
// ────────────────────────────────────────────────────────────────

TypeInferenceEngine::TypeInferenceEngine()
    : context_(std::make_unique<InferenceContext>()),
      collector_(std::make_unique<ConstraintCollector>(context_.get())),
      solver_(std::make_unique<ConstraintSolver>()) {}

std::string TypeInferenceEngine::inferExprType(const std::string& expr) {
    // Step 1: 표현식 파싱
    if (expr.empty()) {
        return "Unknown";
    }

    // Step 2: 리터럴 처리
    if (isLiteral(expr)) {
        return parseLiteral(expr);
    }

    // Step 3: 변수 처리
    auto var_type = context_->lookupVariable(expr);
    if (var_type.has_value()) {
        return var_type.value();
    }

    // Step 4: 이항 연산 처리
    std::regex binary_op_regex(R"((\w+)\s*([\+\-\*\/])\s*(.+))");
    std::smatch match;

    if (std::regex_match(expr, match, binary_op_regex)) {
        std::string left = match[1].str();
        std::string op = match[2].str();
        std::string right = match[3].str();

        TypeInferenceSubstitution empty_subst;
        return inferBinaryOp(left, right, op, empty_subst);
    }

    // 미지의 표현식
    return context_->createFreshTypeVar();
}

std::string TypeInferenceEngine::inferFunctionSignature(
    const std::string& func_text) {

    // 간단한 파싱: fn name(p1: t1, p2: t2) -> ret { body }
    // 현재는 스켈레톤만 구현

    // 예: fn add(a, b) { a + b }
    // → "fn(i64, i64) -> i64"

    // Step 1: 함수 본문에서 제약 수집
    // Step 2: 파라미터 타입과 반환 타입 추론
    // Step 3: 함수 시그니처 구성

    return "fn(...) -> Unknown";  // 스켈레톤
}

std::vector<TypeConstraint> TypeInferenceEngine::collectConstraints(
    const std::string& expr) {

    return collector_->collect(expr);
}

bool TypeInferenceEngine::solveConstraints(
    const std::vector<TypeConstraint>& constraints,
    TypeInferenceSubstitution& solution) {

    return solver_->solve(constraints, solution);
}

std::string TypeInferenceEngine::parseLiteral(const std::string& literal) {
    return collector_->getTypeForLiteral(literal);
}

std::string TypeInferenceEngine::inferBinaryOp(
    const std::string& left,
    const std::string& right,
    const std::string& op,
    TypeInferenceSubstitution& subst) {

    // 좌측 타입
    std::string left_type = inferExprType(left);

    // 우측 타입
    std::string right_type = inferExprType(right);

    // 연산자 시그니처
    std::string op_type = getOperatorType(op);

    // 타입 호환성 검사
    if (left_type == right_type) {
        // 같은 타입 → 결과는 같은 타입
        return left_type;
    }

    // 타입 불일치
    return "Unknown";
}

std::string TypeInferenceEngine::getOperatorType(const std::string& op) {
    // 이항 연산자의 타입 시그니처
    // +, -, *, / : fn(i64, i64) -> i64
    // && , || : fn(bool, bool) -> bool

    if (op == "+" || op == "-" || op == "*" || op == "/") {
        return "fn(i64, i64) -> i64";
    }

    if (op == "&&" || op == "||") {
        return "fn(bool, bool) -> bool";
    }

    if (op == "==" || op == "!=" || op == "<" || op == ">") {
        return "fn(i64, i64) -> bool";
    }

    return "Unknown";
}

bool TypeInferenceEngine::parseFunctionCall(const std::string& expr,
                                           std::string& func_name,
                                           std::vector<std::string>& args) {
    // 함수 호출 파싱: f(a, b, c)
    // → func_name = "f", args = ["a", "b", "c"]

    size_t paren_pos = expr.find('(');
    if (paren_pos == std::string::npos) {
        return false;
    }

    func_name = expr.substr(0, paren_pos);

    size_t close_paren = expr.rfind(')');
    if (close_paren == std::string::npos) {
        return false;
    }

    std::string args_str = expr.substr(paren_pos + 1,
                                       close_paren - paren_pos - 1);

    // 쉼표로 분리
    std::istringstream iss(args_str);
    std::string arg;
    while (std::getline(iss, arg, ',')) {
        // 앞뒤 공백 제거
        arg.erase(0, arg.find_first_not_of(" \t"));
        arg.erase(arg.find_last_not_of(" \t") + 1);
        if (!arg.empty()) {
            args.push_back(arg);
        }
    }

    return true;
}

std::string TypeInferenceEngine::infer(const std::string& expr,
                                       TypeInferenceSubstitution& substitution) {
    // 내부 추론 함수 (재귀적)
    // 현재는 간단한 구현만 포함

    return inferExprType(expr);
}

}  // namespace zlang
