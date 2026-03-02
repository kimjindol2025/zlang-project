#include "TypeInference.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace zlang;

// ════════════════════════════════════════════════════════════════════════════
// Test Utilities
// ════════════════════════════════════════════════════════════════════════════

int test_count = 0;
int pass_count = 0;

void assert_equal(const std::string& actual, const std::string& expected,
                 const std::string& test_name) {
    test_count++;
    if (actual == expected) {
        std::cout << "  ✓ " << test_name << ": " << actual << "\n";
        pass_count++;
    } else {
        std::cout << "  ✗ " << test_name << ": expected '" << expected
                  << "', got '" << actual << "'\n";
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Category 1: Literal Type Inference (3 tests)
// ════════════════════════════════════════════════════════════════════════════

void category_1_literal_inference() {
    std::cout << "\n▌ Category 1: Literal Type Inference (리터럴 추론)\n";
    std::cout << "════════════════════════════════════════════════════════════════\n";

    {
        TypeInferenceEngine engine;
        std::string type = engine.inferExprType("42");
        assert_equal(type, "i64", "infer_integer_literal");
    }

    {
        TypeInferenceEngine engine;
        std::string type = engine.inferExprType("3.14");
        assert_equal(type, "f64", "infer_float_literal");
    }

    {
        TypeInferenceEngine engine;
        std::string type = engine.inferExprType("\"hello\"");
        assert_equal(type, "String", "infer_string_literal");
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Category 2: Binary Operation Type Inference (3 tests)
// ════════════════════════════════════════════════════════════════════════════

void category_2_binary_operations() {
    std::cout << "\n▌ Category 2: Binary Operation Type Inference (이항 연산)\n";
    std::cout << "════════════════════════════════════════════════════════════════\n";

    {
        TypeInferenceEngine engine;
        engine.bindVariable("x", "i64");
        std::string type = engine.inferExprType("x + 10");
        assert_equal(type, "i64", "infer_binary_op_add");
    }

    {
        TypeInferenceEngine engine;
        engine.bindVariable("a", "i64");
        std::string type = engine.inferExprType("a * 5");
        assert_equal(type, "i64", "infer_binary_op_multiply");
    }

    {
        TypeInferenceEngine engine;
        std::string type = engine.inferExprType("10 - 3");
        assert_equal(type, "i64", "infer_binary_op_subtract");
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Category 3: Variable Context Type Inference (3 tests)
// ════════════════════════════════════════════════════════════════════════════

void category_3_variable_context() {
    std::cout << "\n▌ Category 3: Variable Context Type Inference (변수 환경)\n";
    std::cout << "════════════════════════════════════════════════════════════════\n";

    {
        TypeInferenceEngine engine;
        engine.bindVariable("x", "i64");
        std::string type = engine.inferExprType("x");
        assert_equal(type, "i64", "infer_variable_from_context");
    }

    {
        TypeInferenceEngine engine;
        engine.bindVariable("a", "i64");
        engine.bindVariable("b", "i64");
        std::string type = engine.inferExprType("a + b");
        assert_equal(type, "i64", "infer_multiple_variables");
    }

    {
        TypeInferenceEngine engine;
        engine.bindVariable("x", "i64");
        std::string type1 = engine.inferExprType("x * 2");
        std::string type2 = engine.inferExprType("x + 1");
        assert_equal(type1, "i64", "infer_variable_reuse_1");
        assert_equal(type2, "i64", "infer_variable_reuse_2");
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Category 4: Literal Operations Type Inference (3 tests)
// ════════════════════════════════════════════════════════════════════════════

void category_4_literal_operations() {
    std::cout << "\n▌ Category 4: Literal Operations Type Inference (리터럴 연산)\n";
    std::cout << "════════════════════════════════════════════════════════════════\n";

    {
        TypeInferenceEngine engine;
        std::string type = engine.inferExprType("10 + 20");
        assert_equal(type, "i64", "infer_literal_addition");
    }

    {
        TypeInferenceEngine engine;
        std::string type = engine.inferExprType("5 * 6");
        assert_equal(type, "i64", "infer_literal_multiplication");
    }

    {
        TypeInferenceEngine engine;
        std::string type = engine.inferExprType("100 / 10");
        assert_equal(type, "i64", "infer_literal_division");
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Category 5: Context and Constraint Resolution (3 tests)
// ════════════════════════════════════════════════════════════════════════════

void category_5_context_constraints() {
    std::cout << "\n▌ Category 5: Context and Constraint Resolution (제약 해결)\n";
    std::cout << "════════════════════════════════════════════════════════════════\n";

    {
        TypeInferenceEngine engine;
        engine.bindVariable("x", "i64");
        engine.bindVariable("y", "i64");
        std::string type = engine.inferExprType("x + y");
        assert_equal(type, "i64", "infer_multiple_variables_sum");
    }

    {
        TypeInferenceEngine engine;
        engine.bindVariable("n", "i64");
        std::string type = engine.inferExprType("n * n");
        assert_equal(type, "i64", "infer_variable_self_operation");
    }

    {
        TypeInferenceEngine engine;
        engine.bindVariable("a", "i64");
        engine.bindVariable("b", "i64");
        engine.bindVariable("c", "i64");
        std::string type = engine.inferExprType("a + b");
        assert_equal(type, "i64", "infer_complex_context");
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Additional Tests: Engine Reset and Context Management
// ════════════════════════════════════════════════════════════════════════════

void additional_tests() {
    std::cout << "\n▌ Additional Tests: Engine Reset and Context Management\n";
    std::cout << "════════════════════════════════════════════════════════════════\n";

    {
        TypeInferenceEngine engine;
        engine.bindVariable("x", "i64");
        auto type1 = engine.getVariableType("x");
        assert(type1.has_value() && type1.value() == "i64");
        std::cout << "  ✓ getVariableType_bound: x bound to i64\n";
        test_count++;
        pass_count++;
    }

    {
        TypeInferenceEngine engine;
        auto type = engine.getVariableType("nonexistent");
        assert(!type.has_value());
        std::cout << "  ✓ getVariableType_unbound: nonexistent returns nullopt\n";
        test_count++;
        pass_count++;
    }

    {
        TypeInferenceEngine engine;
        engine.bindVariable("a", "i64");
        engine.bindVariable("b", "f64");
        engine.reset();
        auto type_a = engine.getVariableType("a");
        assert(!type_a.has_value());
        std::cout << "  ✓ reset_clears_bindings: context cleared after reset\n";
        test_count++;
        pass_count++;
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Main Test Runner
// ════════════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n";
    std::cout << "======================================================================\n";
    std::cout << "   Z-Lang LLVM 1.4: Type Inference Engine - Test Suite\n";
    std::cout << "======================================================================\n";
    std::cout << "Week 2: Hindley-Milner Type Inference Algorithm\n";
    std::cout << "Date: 2026-03-02\n";

    // Run all test categories
    category_1_literal_inference();
    category_2_binary_operations();
    category_3_variable_context();
    category_4_literal_operations();
    category_5_context_constraints();
    additional_tests();

    // Print summary
    std::cout << "\n";
    std::cout << "======================================================================\n";
    std::cout << "Test Summary:\n";
    std::cout << "  Total Tests:        " << test_count << "\n";
    std::cout << "  Passed:             " << pass_count << " ✓\n";
    std::cout << "  Failed:             " << (test_count - pass_count) << "\n";
    if (test_count > 0) {
        std::cout << "  Pass Rate:          " << (pass_count * 100 / test_count) << "%\n";
    }
    std::cout << "======================================================================\n\n";

    return (pass_count == test_count) ? 0 : 1;
}
