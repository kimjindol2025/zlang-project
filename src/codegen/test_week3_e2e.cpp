#include "CompilerPipeline.h"
#include "CodeGenerator.h"
#include "IntegratedTypeChecker.h"
#include <iostream>
#include <cassert>
#include <string>

using namespace zlang;

int test_count = 0;
int pass_count = 0;

void test_assert(bool condition, const std::string& test_name) {
    test_count++;
    if (condition) {
        std::cout << "  ✓ " << test_name << "\n";
        pass_count++;
    } else {
        std::cout << "  ✗ " << test_name << "\n";
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Test 1: IntegratedTypeChecker
// ════════════════════════════════════════════════════════════════════════════

void test_integrated_type_checker() {
    std::cout << "\n▌ Test 1: IntegratedTypeChecker (통합 타입 검사)\n";
    std::cout << "════════════════════════════════════════════════════════════════\n";

    IntegratedTypeChecker checker;

    // Test 1.1: 변수 바인딩 및 타입 추론
    checker.bindVariable("x", "i64");
    std::string expr_type = checker.inferExprType("x + 10");
    test_assert(expr_type == "i64", "binary_op_inference");

    // Test 1.2: 함수 등록 및 호출
    checker.registerConcreteFunction("add", "i64", {"i64", "i64"});
    std::string call_type = checker.checkFunctionCall("add", {"10", "20"});
    test_assert(!call_type.empty(), "function_call_check");

    // Test 1.3: 타입 호환성
    bool compatible = checker.isTypeCompatible("i64", "i64");
    test_assert(compatible, "type_compatibility");

    // Test 1.4: 타입 변수
    bool is_var = IntegratedTypeChecker::isTypeVariable("T_0");
    test_assert(is_var, "type_variable_detection");
}

// ════════════════════════════════════════════════════════════════════════════
// Test 2: CodeGenerator - Expression
// ════════════════════════════════════════════════════════════════════════════

void test_code_generator_expr() {
    std::cout << "\n▌ Test 2: CodeGenerator - Expression (표현식 생성)\n";
    std::cout << "════════════════════════════════════════════════════════════════\n";

    CodeGenerator gen;

    // Test 2.1: 리터럴
    std::string lit = gen.generateLiteral("42");
    test_assert(lit == "42", "literal_generation");

    // Test 2.2: 변수
    std::string var = gen.generateVariable("x");
    test_assert(var == "%x", "variable_generation");

    // Test 2.3: 이항 연산
    std::string binop = gen.generateBinaryOp("x", "10", "+", "i64");
    test_assert(!binop.empty(), "binary_op_generation");

    // Test 2.4: 레지스터 생성
    std::string reg1 = gen.newRegister();
    std::string reg2 = gen.newRegister();
    test_assert(reg1 == "%0" && reg2 == "%1", "register_generation");
}

// ════════════════════════════════════════════════════════════════════════════
// Test 3: CodeGenerator - Function
// ════════════════════════════════════════════════════════════════════════════

void test_code_generator_func() {
    std::cout << "\n▌ Test 3: CodeGenerator - Function (함수 생성)\n";
    std::cout << "════════════════════════════════════════════════════════════════\n";

    CodeGenerator gen;

    // Test 3.1: 함수 시그니처
    std::string sig = gen.generateFunctionSignature(
        "add", {"a", "b"}, {"i64", "i64"}, "i64"
    );
    test_assert(sig.find("@add") != std::string::npos, "function_signature");

    // Test 3.2: 함수 정의
    std::string func_def = gen.generateFunctionCode(
        "double", {"x"}, {"i64"}, "i64", "x + x"
    );
    test_assert(func_def.find("@double") != std::string::npos, "function_definition");

    // Test 3.3: LLVM 타입 변환
    std::string llvm_i64 = CodeGenerator::getLLVMType("i64");
    test_assert(llvm_i64 == "i64", "llvm_type_i64");

    std::string llvm_f64 = CodeGenerator::getLLVMType("f64");
    test_assert(llvm_f64 == "double", "llvm_type_f64");
}

// ════════════════════════════════════════════════════════════════════════════
// Test 4: CodeGenerator - Module
// ════════════════════════════════════════════════════════════════════════════

void test_code_generator_module() {
    std::cout << "\n▌ Test 4: CodeGenerator - Module (모듈 생성)\n";
    std::cout << "════════════════════════════════════════════════════════════════\n";

    CodeGenerator gen;

    // 함수 생성
    std::vector<std::string> funcs;
    std::string func1 = gen.generateFunctionCode(
        "add", {"a", "b"}, {"i64", "i64"}, "i64", "a + b"
    );
    funcs.push_back(func1);

    // 모듈 생성
    std::string module = gen.generateModule(funcs);
    test_assert(module.find("target triple") != std::string::npos, "module_generation");
    test_assert(module.find("@add") != std::string::npos, "module_contains_function");
}

// ════════════════════════════════════════════════════════════════════════════
// Test 5: CompilerPipeline
// ════════════════════════════════════════════════════════════════════════════

void test_compiler_pipeline() {
    std::cout << "\n▌ Test 5: CompilerPipeline (컴파일러 파이프라인)\n";
    std::cout << "════════════════════════════════════════════════════════════════\n";

    CompilerPipeline pipeline;

    // Test 5.1: 타입 체크
    std::string source1 = "fn add(a, b) { a + b }";
    std::string typed = pipeline.typeCheck(source1);
    test_assert(!typed.empty(), "type_checking");

    // Test 5.2: IR 생성
    std::string ir = pipeline.generateIR(source1);
    test_assert(!ir.empty(), "ir_generation");
    test_assert(ir.find("@add") != std::string::npos, "ir_contains_function");
}

// ════════════════════════════════════════════════════════════════════════════
// Test 6: Integration - Type & Code Gen
// ════════════════════════════════════════════════════════════════════════════

void test_integration() {
    std::cout << "\n▌ Test 6: Integration (통합 테스트)\n";
    std::cout << "════════════════════════════════════════════════════════════════\n";

    IntegratedTypeChecker checker;
    CodeGenerator gen;

    // 함수 등록
    checker.registerConcreteFunction("multiply", "i64", {"i64", "i64"});

    // 호출 체크
    std::string result_type = checker.checkFunctionCall("multiply", {"3", "4"});
    test_assert(!result_type.empty(), "integrated_function_call");

    // 함수 생성
    std::string func_ir = gen.generateFunctionCode(
        "multiply", {"x", "y"}, {"i64", "i64"}, "i64", "x * y"
    );
    test_assert(func_ir.find("mul") != std::string::npos, "integrated_codegen");
}

// ════════════════════════════════════════════════════════════════════════════
// Main Test Runner
// ════════════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n";
    std::cout << "======================================================================\n";
    std::cout << "   Z-Lang LLVM 1.4: Week 3 - End-to-End Integration Tests\n";
    std::cout << "======================================================================\n";
    std::cout << "Testing: IntegratedTypeChecker + CodeGenerator + CompilerPipeline\n";

    test_integrated_type_checker();
    test_code_generator_expr();
    test_code_generator_func();
    test_code_generator_module();
    test_compiler_pipeline();
    test_integration();

    // Print summary
    std::cout << "\n";
    std::cout << "======================================================================\n";
    std::cout << "Week 3 E2E Test Summary:\n";
    std::cout << "  Total Tests:        " << test_count << "\n";
    std::cout << "  Passed:             " << pass_count << " ✓\n";
    std::cout << "  Failed:             " << (test_count - pass_count) << "\n";
    if (test_count > 0) {
        std::cout << "  Pass Rate:          " << (pass_count * 100 / test_count) << "%\n";
    }
    std::cout << "======================================================================\n\n";

    return (pass_count == test_count) ? 0 : 1;
}
