/**
 * Z-Lang LLVM 1.4: Generic Type System - PoC (Proof of Concept)
 *
 * Type.h의 완전한 정의를 기다리지 않고
 * 제너릭 타입 시스템의 핵심 로직을 테스트
 */

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <cassert>
#include <iomanip>

using namespace std;

// ────────────────────────────────────────────────────────────────
// Standalone Type Variable Implementation (Type.h 의존성 없음)
// ────────────────────────────────────────────────────────────────

class TypeVariable {
private:
    static int next_id_;
    string name_;
    int id_;
    TypeVariable* bound_type_;

public:
    TypeVariable(const string& name)
        : name_(name), id_(next_id_++), bound_type_(nullptr) {}

    const string& name() const { return name_; }
    int id() const { return id_; }

    bool isBound() const { return bound_type_ != nullptr; }
    TypeVariable* getBound() const { return bound_type_; }

    void bind(TypeVariable* concrete) {
        if (!isBound()) {
            bound_type_ = concrete;
        }
    }

    string toString() const {
        return name_ + "_" + to_string(id_);
    }
};

int TypeVariable::next_id_ = 0;

// ────────────────────────────────────────────────────────────────
// Substitution (타입 변수 치환)
// ────────────────────────────────────────────────────────────────

using Substitution = map<string, string>;

string applySubstitution(const string& type_expr, const Substitution& subst) {
    auto it = subst.find(type_expr);
    if (it != subst.end()) {
        return it->second;
    }
    return type_expr;
}

// ────────────────────────────────────────────────────────────────
// Generic Function Type (간단한 버전)
// ────────────────────────────────────────────────────────────────

struct GenericFunctionType {
    string name;
    vector<string> type_params;      // ["T", "U"]
    vector<pair<string, string>> param_types;  // [("x", "T"), ("f", "U")]
    string return_type;

    GenericFunctionType(const string& n)
        : name(n), return_type("void") {}

    void addTypeParam(const string& param) {
        type_params.push_back(param);
    }

    void addParameter(const string& name, const string& type) {
        param_types.push_back({name, type});
    }

    void setReturnType(const string& ret) {
        return_type = ret;
    }

    string toString() const {
        string result = "fn " + name + "<";
        for (size_t i = 0; i < type_params.size(); ++i) {
            if (i > 0) result += ", ";
            result += type_params[i];
        }
        result += ">(";

        for (size_t i = 0; i < param_types.size(); ++i) {
            if (i > 0) result += ", ";
            result += param_types[i].first + ": " + param_types[i].second;
        }

        result += ") -> " + return_type;
        return result;
    }
};

// ────────────────────────────────────────────────────────────────
// Generic Resolver (간단한 Unification)
// ────────────────────────────────────────────────────────────────

class GenericResolver {
public:
    // 함수 호출로부터 타입 인자 추론
    // id(42) → {T -> i64}
    Substitution resolve(const GenericFunctionType* func,
                        const vector<string>& arg_types) const {
        Substitution result;

        if (arg_types.size() != func->param_types.size()) {
            return result;  // 오류
        }

        // 파라미터 타입과 인자 타입 비교
        for (size_t i = 0; i < func->param_types.size(); ++i) {
            const string& param_type = func->param_types[i].second;
            const string& arg_type = arg_types[i];

            // 간단한 unification: param_type이 타입 변수면 인자 타입으로 바인드
            bool is_type_var = (param_type.length() == 1 &&
                               param_type[0] >= 'A' &&
                               param_type[0] <= 'Z');

            if (is_type_var) {
                result[param_type] = arg_type;
            }
        }

        return result;
    }

    // 함수 시그니처에 치환 적용
    GenericFunctionType* instantiate(const GenericFunctionType* func,
                                     const Substitution& subst) const {
        auto* result = new GenericFunctionType(func->name);

        // 타입 파라미터 (남기지 않음 - 인스턴스화됨)
        // result->type_params = func->type_params;

        // 파라미터 타입 치환
        for (const auto& param : func->param_types) {
            string subst_type = applySubstitution(param.second, subst);
            result->addParameter(param.first, subst_type);
        }

        // 반환 타입 치환
        result->setReturnType(applySubstitution(func->return_type, subst));

        return result;
    }
};

// ────────────────────────────────────────────────────────────────
// Test Infrastructure
// ────────────────────────────────────────────────────────────────

int total_tests = 0;
int passed_tests = 0;

void test(const string& name, bool condition) {
    total_tests++;
    if (condition) {
        passed_tests++;
        cout << "  ✅ " << name << endl;
    } else {
        cout << "  ❌ " << name << endl;
    }
}

// ────────────────────────────────────────────────────────────────
// Test Cases
// ────────────────────────────────────────────────────────────────

void test_type_variable() {
    cout << "\n🧪 Test 1: TypeVariable Creation\n";

    TypeVariable t("T");
    TypeVariable u("U");

    test("Create T variable", t.name() == "T");
    test("Create U variable", u.name() == "U");
    test("T not bound initially", !t.isBound());
    test("T and U have different IDs", t.id() != u.id());
}

void test_type_variable_binding() {
    cout << "\n🧪 Test 2: TypeVariable Binding\n";

    TypeVariable t("T");
    TypeVariable i64("i64");

    t.bind(&i64);

    test("T is bound after binding", t.isBound());
    test("T bound type is i64", t.getBound()->name() == "i64");
}

void test_type_variable_unification() {
    cout << "\n🧪 Test 3: TypeVariable Unification\n";

    TypeVariable t("T");
    TypeVariable u("U");

    // 간단한 unification 테스트
    test("TypeVariable T created", t.name() == "T");
    test("TypeVariable U created", u.name() == "U");
    test("U has higher id than T", u.id() > t.id());
}

void test_substitution() {
    cout << "\n🧪 Test 4: Substitution\n";

    Substitution subst;
    subst["T"] = "i64";
    subst["U"] = "String";

    test("Substitution T -> i64", applySubstitution("T", subst) == "i64");
    test("Substitution U -> String", applySubstitution("U", subst) == "String");
    test("Unknown type returns itself", applySubstitution("V", subst) == "V");
}

void test_generic_function_identity() {
    cout << "\n🧪 Test 5: Generic Function id<T>\n";

    GenericFunctionType id_func("id");
    id_func.addTypeParam("T");
    id_func.addParameter("x", "T");
    id_func.setReturnType("T");

    test("Function name is id", id_func.name == "id");
    test("Function has 1 type parameter", id_func.type_params.size() == 1);
    test("Function has 1 parameter", id_func.param_types.size() == 1);

    string sig = id_func.toString();
    test("Function signature contains fn", sig.find("fn") != string::npos);
    test("Function signature contains id", sig.find("id") != string::npos);
}

void test_generic_function_map() {
    cout << "\n🧪 Test 6: Generic Function map<T, U>\n";

    GenericFunctionType map_func("map");
    map_func.addTypeParam("T");
    map_func.addTypeParam("U");
    map_func.addParameter("vec", "T");
    map_func.addParameter("f", "U");
    map_func.setReturnType("U");

    test("map has 2 type parameters", map_func.type_params.size() == 2);
    test("map has 2 parameters", map_func.param_types.size() == 2);
    test("map return type is U", map_func.return_type == "U");
}

void test_generic_resolver_resolve() {
    cout << "\n🧪 Test 7: GenericResolver.resolve()\n";

    GenericFunctionType id_func("id");
    id_func.addTypeParam("T");
    id_func.addParameter("x", "T");
    id_func.setReturnType("T");

    GenericResolver resolver;

    // 호출: id(42)
    vector<string> arg_types = {"i64"};
    Substitution subst = resolver.resolve(&id_func, arg_types);

    test("Resolve returns non-empty substitution", !subst.empty());
    test("Substitution contains T -> i64",
         subst.find("T") != subst.end() && subst["T"] == "i64");
}

void test_generic_resolver_instantiate() {
    cout << "\n🧪 Test 8: GenericResolver.instantiate()\n";

    GenericFunctionType id_func("id");
    id_func.addTypeParam("T");
    id_func.addParameter("x", "T");
    id_func.setReturnType("T");

    GenericResolver resolver;

    Substitution subst;
    subst["T"] = "i64";

    GenericFunctionType* instantiated = resolver.instantiate(&id_func, subst);

    test("Instantiation succeeds", instantiated != nullptr);
    test("Instantiated function has 1 parameter",
         instantiated->param_types.size() == 1);
    test("Parameter type is i64",
         instantiated->param_types[0].second == "i64");
    test("Return type is i64", instantiated->return_type == "i64");

    delete instantiated;
}

void test_generic_resolver_map() {
    cout << "\n🧪 Test 9: Generic Resolution with map<T, U>\n";

    GenericFunctionType map_func("map");
    map_func.addTypeParam("T");
    map_func.addTypeParam("U");
    map_func.addParameter("vec", "T");
    map_func.addParameter("f", "U");
    map_func.setReturnType("U");

    GenericResolver resolver;

    // 호출: map(vec, |x| x * 2)
    // 인자: Vec<i64>, fn(i64) -> i64
    vector<string> arg_types = {"Vec<i64>", "fn(i64)->i64"};
    Substitution subst = resolver.resolve(&map_func, arg_types);

    test("map resolution succeeds", !subst.empty());
    test("T -> Vec<i64>",
         subst.find("T") != subst.end() && subst["T"] == "Vec<i64>");
    test("U -> fn(i64)->i64",
         subst.find("U") != subst.end() && subst["U"] == "fn(i64)->i64");
}

void test_multiple_calls() {
    cout << "\n🧪 Test 10: Multiple Generic Function Calls\n";

    GenericFunctionType id_func("id");
    id_func.addTypeParam("T");
    id_func.addParameter("x", "T");
    id_func.setReturnType("T");

    GenericResolver resolver;

    // 첫 번째 호출: id(42)
    Substitution subst1 = resolver.resolve(&id_func, {"i64"});
    test("First call: id(42) → T = i64", subst1["T"] == "i64");

    // 두 번째 호출: id("hello")
    Substitution subst2 = resolver.resolve(&id_func, {"String"});
    test("Second call: id(\"hello\") → T = String", subst2["T"] == "String");

    // 인스턴스화
    GenericFunctionType* id_i64 = resolver.instantiate(&id_func, subst1);
    GenericFunctionType* id_string = resolver.instantiate(&id_func, subst2);

    test("id<i64> correct", id_i64->param_types[0].second == "i64");
    test("id<String> correct", id_string->param_types[0].second == "String");

    delete id_i64;
    delete id_string;
}

void test_no_type_variable_functions() {
    cout << "\n🧪 Test 11: Non-generic Functions\n";

    GenericFunctionType add_func("add");
    add_func.addParameter("a", "i64");
    add_func.addParameter("b", "i64");
    add_func.setReturnType("i64");

    test("Non-generic function has no type params",
         add_func.type_params.empty());
    test("add function is concrete", add_func.name == "add");

    string sig = add_func.toString();
    test("Signature correct", sig.find("add") != string::npos);
}

void test_type_variable_equality() {
    cout << "\n🧪 Test 12: TypeVariable Equality\n";

    TypeVariable t1("T");
    TypeVariable t2("T");

    test("Same name, different IDs",
         t1.name() == t2.name() && t1.id() != t2.id());
    test("IDs are sequential", t2.id() > t1.id());
}

void test_nested_types() {
    cout << "\n🧪 Test 13: Nested Generic Types\n";

    GenericFunctionType option_func("wrap");
    option_func.addTypeParam("T");
    option_func.addParameter("x", "T");
    option_func.setReturnType("Option<T>");

    GenericResolver resolver;
    Substitution subst = resolver.resolve(&option_func, {"i64"});

    test("Nested type resolution", subst["T"] == "i64");

    GenericFunctionType* instantiated = resolver.instantiate(&option_func, subst);
    test("Return type Option<T> → Option<i64>",
         instantiated->return_type == "Option<i64>");

    delete instantiated;
}

void test_many_type_parameters() {
    cout << "\n🧪 Test 14: Many Type Parameters\n";

    GenericFunctionType result_func("make_result");
    result_func.addTypeParam("T");
    result_func.addTypeParam("E");
    result_func.addParameter("value", "T");
    result_func.addParameter("error", "E");
    result_func.setReturnType("Result<T, E>");

    test("Result<T, E> has 2 type params",
         result_func.type_params.size() == 2);

    GenericResolver resolver;
    Substitution subst = resolver.resolve(&result_func, {"i64", "String"});

    test("T -> i64", subst["T"] == "i64");
    test("E -> String", subst["E"] == "String");
}

void test_type_variable_toString() {
    cout << "\n🧪 Test 15: TypeVariable.toString()\n";

    TypeVariable t("T");
    TypeVariable u("U");

    string t_str = t.toString();
    string u_str = u.toString();

    test("T toString contains T", t_str.find("T") != string::npos);
    test("U toString contains U", u_str.find("U") != string::npos);
    test("Different IDs in string", t_str != u_str);
}

// ────────────────────────────────────────────────────────────────
// Main Test Runner
// ────────────────────────────────────────────────────────────────

int main() {
    cout << "\n" << string(70, '=') << endl;
    cout << "🧪 Z-Lang LLVM 1.4: Generic Type System PoC Tests" << endl;
    cout << string(70, '=') << endl;

    try {
        test_type_variable();
        test_type_variable_binding();
        test_type_variable_unification();
        test_substitution();
        test_generic_function_identity();
        test_generic_function_map();
        test_generic_resolver_resolve();
        test_generic_resolver_instantiate();
        test_generic_resolver_map();
        test_multiple_calls();
        test_no_type_variable_functions();
        test_type_variable_equality();
        test_nested_types();
        test_many_type_parameters();
        test_type_variable_toString();

    } catch (const exception& e) {
        cout << "❌ Exception: " << e.what() << endl;
    }

    // Summary
    cout << "\n" << string(70, '=') << endl;
    cout << "📊 Test Results" << endl;
    cout << string(70, '=') << endl;
    cout << setw(20) << left << "Total Tests:"
         << setw(10) << total_tests << endl;
    cout << setw(20) << left << "Passed:"
         << setw(10) << passed_tests << " ✅" << endl;
    cout << setw(20) << left << "Failed:"
         << setw(10) << (total_tests - passed_tests) << endl;
    cout << setw(20) << left << "Pass Rate:"
         << setw(10) << fixed << setprecision(1)
         << (100.0 * passed_tests / total_tests) << "%" << endl;
    cout << string(70, '=') << endl;

    return (passed_tests == total_tests) ? 0 : 1;
}
