#include "CodeGenerator.h"
#include <sstream>
#include <regex>

namespace zlang {

CodeGenerator::CodeGenerator()
    : type_checker_(std::make_unique<IntegratedTypeChecker>()) {
}

std::string CodeGenerator::generateExprCode(
    const std::string& expr,
    const std::string& expr_type) {

    // 리터럴 검사
    if (std::isdigit(expr[0]) || (expr[0] == '-' && expr.size() > 1)) {
        return generateLiteral(expr);
    }

    // 변수 검사
    if (expr.find('+') != std::string::npos ||
        expr.find('-') != std::string::npos ||
        expr.find('*') != std::string::npos ||
        expr.find('/') != std::string::npos) {

        // 이항 연산
        std::regex op_regex(R"((\w+)\s*([\+\-\*/])\s*(.+))");
        std::smatch match;

        if (std::regex_match(expr, match, op_regex)) {
            std::string left = match[1].str();
            std::string op = match[2].str();
            std::string right = match[3].str();

            return generateBinaryOp(left, right, op, expr_type);
        }
    }

    // 단순 변수
    return generateVariable(expr);
}

std::string CodeGenerator::generateBinaryOp(
    const std::string& left,
    const std::string& right,
    const std::string& op,
    const std::string& result_type) {

    std::string left_val = generateExprCode(left, result_type);
    std::string right_val = generateExprCode(right, result_type);

    std::string result = newRegister();
    std::string llvm_type = getLLVMType(result_type);

    // LLVM 연산자 매핑
    std::string llvm_op;
    if (op == "+") llvm_op = "add";
    else if (op == "-") llvm_op = "sub";
    else if (op == "*") llvm_op = "mul";
    else if (op == "/") llvm_op = "sdiv";  // signed division
    else return "";

    current_ir_ += "  " + result + " = " + llvm_op + " " + llvm_type +
                   " " + left_val + ", " + right_val + "\n";

    return result;
}

std::string CodeGenerator::generateLiteral(const std::string& literal) {
    // 리터럴은 직접 반환 (새 레지스터 필요 없음)
    return literal;
}

std::string CodeGenerator::generateVariable(const std::string& var_name) {
    // 변수는 레지스터처럼 참조 (%var_name)
    return "%" + var_name;
}

std::string CodeGenerator::generateFunctionCode(
    const std::string& func_name,
    const std::vector<std::string>& param_names,
    const std::vector<std::string>& param_types,
    const std::string& return_type,
    const std::string& body) {

    std::ostringstream oss;

    // 함수 시그니처
    oss << generateFunctionSignature(func_name, param_names, param_types, return_type);
    oss << "\n";

    // 매개변수를 변수 맵에 추가
    for (size_t i = 0; i < param_names.size(); ++i) {
        type_checker_->bindVariable(param_names[i], param_types[i]);
    }

    // 본문 코드 생성
    std::string body_type = type_checker_->inferExprType(body);
    std::string body_code = generateExprCode(body, body_type);

    // 반환 명령
    oss << "  ret " << getLLVMType(return_type) << " " << body_code << "\n";
    oss << "}\n\n";

    std::string func_def = oss.str();
    functions_.push_back(func_def);

    return func_def;
}

std::string CodeGenerator::generateFunctionSignature(
    const std::string& func_name,
    const std::vector<std::string>& param_names,
    const std::vector<std::string>& param_types,
    const std::string& return_type) {

    std::ostringstream oss;
    oss << "define " << getLLVMType(return_type) << " @" << func_name << "(";

    for (size_t i = 0; i < param_names.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << getLLVMType(param_types[i]) << " %" << param_names[i];
    }

    oss << ") {";

    return oss.str();
}

std::string CodeGenerator::generateModule(
    const std::vector<std::string>& function_defs) {

    std::ostringstream oss;

    // LLVM 모듈 헤더
    oss << "; Generated LLVM IR\n";
    oss << "target datalayout = \"e-m:e-i64:64-f80:128-n8:16:32:64-S128\"\n";
    oss << "target triple = \"x86_64-unknown-linux-gnu\"\n\n";

    // 함수 정의들
    for (const auto& func_def : function_defs) {
        oss << func_def << "\n";
    }

    return oss.str();
}

std::string CodeGenerator::newRegister() {
    return "%" + std::to_string(reg_counter_++);
}

std::string CodeGenerator::newBlock() {
    return "bb" + std::to_string(block_counter_++);
}

void CodeGenerator::reset() {
    reg_counter_ = 0;
    block_counter_ = 0;
    var_reg_map_.clear();
    current_ir_.clear();
    functions_.clear();
}

std::string CodeGenerator::getLLVMType(const std::string& zlang_type) {
    if (zlang_type == "i64") return "i64";
    if (zlang_type == "f64") return "double";
    if (zlang_type == "String") return "i8*";
    if (zlang_type == "bool") return "i1";
    if (zlang_type == "void") return "void";
    return "i64";  // 기본값
}

}  // namespace zlang
