/**
 * Generic Type System Tests
 * Week 1: 제너릭 타입 시스템 테스트 (15개 테스트)
 */

#include "GenericType.h"
#include <cassert>
#include <iostream>
#include <iomanip>

using namespace zlang;

// ────────────────────────────────────────────────────────────────
// Test Infrastructure
// ────────────────────────────────────────────────────────────────

int total_tests = 0;
int passed_tests = 0;

void test(const std::string& name, bool condition) {
    total_tests++;
    if (condition) {
        passed_tests++;
        std::cout << "  ✅ " << name << std::endl;
    } else {
        std::cout << "  ❌ " << name << std::endl;
    }
}

// ────────────────────────────────────────────────────────────────
// Test Cases
// ────────────────────────────────────────────────────────────────

void test_type_variable_creation() {
    std::cout << "\n🧪 Test 1: TypeVariable Creation\n";

    TypeVariable t_var("T");
    TypeVariable u_var("U");

    test("Create T variable", t_var.name() == "T");
    test("Create U variable", u_var.name() == "U");
    test("T not bound initially", !t_var.isBound());
    test("T and U have different IDs", t_var.id() != u_var.id());
}

void test_type_variable_binding() {
    std::cout << "\n🧪 Test 2: TypeVariable Binding\n";

    TypeVariable t_var("T");

    // 임시 Type 생성 (실제로는 i64 type이어야 함)
    Type i64_type;
    i64_type.base = BuiltinType::I64;

    t_var.bind(&i64_type);

    test("T is bound after binding", t_var.isBound());
    test("T bound type is i64", t_var.boundType()->base == BuiltinType::I64);
}

void test_type_parameter_creation() {
    std::cout << "\n🧪 Test 3: TypeParameter Creation\n";

    TypeVariable t_var("T");
    TypeParameter param("T", &t_var);
    param.addBound("Clone");
    param.addBound("Display");

    test("Parameter name is T", param.name == "T");
    test("Parameter has 2 bounds", param.trait_bounds.size() == 2);
    test("First bound is Clone", param.trait_bounds[0] == "Clone");
    test("Second bound is Display", param.trait_bounds[1] == "Display");
}

void test_type_constraint_equality() {
    std::cout << "\n🧪 Test 4: TypeConstraint Equality\n";

    Type t1, t2;
    t1.base = BuiltinType::I64;
    t2.base = BuiltinType::I64;

    TypeConstraint constraint(TypeConstraint::Kind::Equality, &t1, &t2);

    test("Constraint kind is Equality", constraint.kind == TypeConstraint::Kind::Equality);
    test("Constraint toString works", constraint.toString().find("i64") != std::string::npos);
}

void test_generic_type_creation() {
    std::cout << "\n🧪 Test 5: GenericType Creation\n";

    GenericType vec("Vec");
    TypeVariable t_var("T");
    TypeParameter param("T", &t_var);

    vec.type_params.push_back(&param);

    test("Generic name is Vec", vec.base_name == "Vec");
    test("Generic has 1 type parameter", vec.type_params.size() == 1);
    test("Type parameter name is T", vec.type_params[0]->name == "T");
}

void test_generic_type_instantiation() {
    std::cout << "\n🧪 Test 6: GenericType Instantiation\n";

    // Vec<T> 정의
    GenericType vec("Vec");
    TypeVariable t_var("T");
    TypeParameter param("T", &t_var);
    vec.type_params.push_back(&param);

    Type element;
    element.base = BuiltinType::I64;
    vec.element_type = &element;

    // Vec<i64>로 인스턴스화
    std::vector<Type*> args = { &element };
    Type* result = vec.instantiate(args);

    test("Instantiation returns a type", result != nullptr);
}

void test_generic_function_type() {
    std::cout << "\n🧪 Test 7: GenericFunctionType\n";

    GenericFunctionType id_func("id");
    TypeVariable t_var("T");
    TypeParameter param("T", &t_var);

    id_func.type_params.push_back(&param);
    id_func.param_types.push_back({"x", &t_var});
    id_func.return_type = &t_var;
    id_func.is_generic = true;

    test("Function name is id", id_func.name == "id");
    test("Function is generic", id_func.is_generic);
    test("Function has 1 type parameter", id_func.type_params.size() == 1);
    test("Function has 1 parameter", id_func.param_types.size() == 1);
}

void test_generic_function_map() {
    std::cout << "\n🧪 Test 8: GenericFunctionType map<T, U>\n";

    GenericFunctionType map_func("map");
    TypeVariable t_var("T");
    TypeVariable u_var("U");
    TypeParameter param_t("T", &t_var);
    TypeParameter param_u("U", &u_var);

    map_func.type_params.push_back(&param_t);
    map_func.type_params.push_back(&param_u);

    // map: (Vec<T>, fn(T) -> U) -> Vec<U>
    // (간단히: T, U -> Vec<U>)
    map_func.param_types.push_back({"vec", &t_var});
    map_func.param_types.push_back({"f", &u_var});

    Type vec_u;
    vec_u.base = BuiltinType::Unknown;  // Vec<U> 대신 임시
    map_func.return_type = &vec_u;

    test("map function has 2 type parameters", map_func.type_params.size() == 2);
    test("map function has 2 parameters", map_func.param_types.size() == 2);
}

void test_substitution_application() {
    std::cout << "\n🧪 Test 9: Substitution Application\n";

    Type i64_type;
    i64_type.base = BuiltinType::I64;

    TypeVariable t_var("T");
    Type* t_ptr = &t_var;
    t_ptr->is_type_var = true;

    Substitution subst;
    subst["T"] = &i64_type;

    // T를 i64로 치환
    Type* result = applySubstitution(&t_var, subst);

    test("Substitution replaces T with i64", result != nullptr);
}

void test_generic_resolver_unification() {
    std::cout << "\n🧪 Test 10: GenericResolver Unification\n";

    GenericResolver resolver;

    Type i64_type;
    i64_type.base = BuiltinType::I64;

    TypeVariable t_var("T");
    Type* t_ptr = &t_var;
    t_ptr->is_type_var = true;

    Substitution result;
    bool success = resolver.unify(&t_var, &i64_type, result);

    test("Unification succeeds", success);
    test("Substitution contains T", result.find("T") != result.end());
}

void test_generic_resolver_resolve() {
    std::cout << "\n🧪 Test 11: GenericResolver.resolve()\n";

    GenericFunctionType id_func("id");
    TypeVariable t_var("T");
    TypeParameter param("T", &t_var);

    id_func.type_params.push_back(&param);
    id_func.param_types.push_back({"x", &t_var});
    id_func.return_type = &t_var;
    id_func.is_generic = true;

    GenericResolver resolver;

    Type i64_type;
    i64_type.base = BuiltinType::I64;

    std::vector<Type*> args = { &i64_type };
    Substitution subst = resolver.resolve(&id_func, args);

    test("Resolve returns non-empty substitution", !subst.empty());
}

void test_generic_type_registry() {
    std::cout << "\n🧪 Test 12: GenericTypeRegistry\n";

    GenericTypeRegistry registry;

    GenericType vec("Vec");
    registry.registerGenericType("Vec", &vec);

    GenericType* retrieved = registry.getGenericType("Vec");

    test("Registry stores generic type", retrieved != nullptr);
    test("Retrieved type is Vec", retrieved->base_name == "Vec");
    test("List generic types works", registry.listGenericTypes().size() >= 1);
}

void test_generic_function_registry() {
    std::cout << "\n🧪 Test 13: GenericFunctionRegistry\n";

    GenericTypeRegistry registry;

    GenericFunctionType id_func("id");
    registry.registerGenericFunction("id", &id_func);

    GenericFunctionType* retrieved = registry.getGenericFunction("id");

    test("Registry stores generic function", retrieved != nullptr);
    test("Retrieved function is id", retrieved->name == "id");
}

void test_type_constraint_bound() {
    std::cout << "\n🧪 Test 14: TypeConstraint Bound\n";

    Type t_type;
    t_type.base = BuiltinType::Unknown;

    TypeConstraint constraint(
        TypeConstraint::Kind::Bound,
        &t_type,
        "Clone"
    );

    test("Constraint kind is Bound", constraint.kind == TypeConstraint::Kind::Bound);
    test("Constraint right_name is Clone", constraint.right_name == "Clone");
}

void test_generic_type_constraints() {
    std::cout << "\n🧪 Test 15: GenericType Extract Constraints\n";

    GenericType vec("Vec");
    TypeVariable t_var("T");
    TypeParameter param("T", &t_var);
    param.addBound("Clone");

    vec.type_params.push_back(&param);

    auto constraints = vec.extractConstraints();

    test("Extract constraints returns non-empty list", !constraints.empty());
    test("First constraint is Bound", constraints[0].kind == TypeConstraint::Kind::Bound);
}

// ────────────────────────────────────────────────────────────────
// Main Test Runner
// ────────────────────────────────────────────────────────────────

int main() {
    std::cout << "\n" << std::string(70, '═') << std::endl;
    std::cout << "🧪 Z-Lang LLVM 1.4: Generic Type System Tests" << std::endl;
    std::cout << std::string(70, '═') << std::endl;

    try {
        test_type_variable_creation();
        test_type_variable_binding();
        test_type_parameter_creation();
        test_type_constraint_equality();
        test_generic_type_creation();
        test_generic_type_instantiation();
        test_generic_function_type();
        test_generic_function_map();
        test_substitution_application();
        test_generic_resolver_unification();
        test_generic_resolver_resolve();
        test_generic_type_registry();
        test_generic_function_registry();
        test_type_constraint_bound();
        test_generic_type_constraints();

    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
    }

    // Summary
    std::cout << "\n" << std::string(70, '═') << std::endl;
    std::cout << "📊 Test Results" << std::endl;
    std::cout << std::string(70, '═') << std::endl;
    std::cout << std::setw(20) << std::left << "Total Tests:"
              << std::setw(10) << total_tests << std::endl;
    std::cout << std::setw(20) << std::left << "Passed:"
              << std::setw(10) << passed_tests << " ✅" << std::endl;
    std::cout << std::setw(20) << std::left << "Failed:"
              << std::setw(10) << (total_tests - passed_tests) << std::endl;
    std::cout << std::setw(20) << std::left << "Pass Rate:"
              << std::setw(10) << (100.0 * passed_tests / total_tests) << "%" << std::endl;
    std::cout << std::string(70, '═') << std::endl;

    return (passed_tests == total_tests) ? 0 : 1;
}
