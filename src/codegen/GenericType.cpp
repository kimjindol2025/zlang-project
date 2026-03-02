#include "GenericType.h"
#include "CodeGenerator.h"
#include <sstream>
#include <algorithm>
#include <cassert>

namespace zlang {

// ────────────────────────────────────────────────────────────────
// TypeVariable Implementation
// ────────────────────────────────────────────────────────────────

int TypeVariable::next_id_ = 0;

TypeVariable::TypeVariable(const std::string& name)
    : name_(name), id_(next_id_++), bound_type_(nullptr) {}

Type* TypeVariable::getBoundTypeWithCompression() const {
    if (!isBound()) return nullptr;

    // 경로 압축 (Path Compression)
    // T -> U -> V 형태의 체인을 T -> V로 단축
    if (bound_type_->is_type_var) {
        auto* var = static_cast<TypeVariable*>(bound_type_);
        Type* final_type = var->getBoundTypeWithCompression();
        const_cast<TypeVariable*>(this)->bound_type_ = final_type;
        return final_type;
    }

    return bound_type_;
}

void TypeVariable::bind(Type* concrete_type) {
    if (isBound()) {
        // 이미 바인드되었으면 호환성 확인
        Type* current = getBoundTypeWithCompression();
        if (current != concrete_type) {
            // 타입 불일치 에러 발생 (실제로는 별도 에러 처리 필요)
            // throw std::runtime_error("Type mismatch in unification");
        }
        return;
    }

    bound_type_ = concrete_type;
}

void TypeVariable::addConstraint(const std::string& constraint_name) {
    constraints_.insert(constraint_name);
}

bool TypeVariable::hasConstraint(const std::string& constraint_name) const {
    return constraints_.find(constraint_name) != constraints_.end();
}

bool TypeVariable::unifyWith(TypeVariable* other) {
    if (!other) return false;

    // 더 작은 ID가 더 큰 ID를 가리킴 (방향성 보장)
    // NOTE: bind는 Type*를 요구하지만, 실제로는 같은 TypeVariable 체인
    // Week 2에서 타입 추론 엔진 정리 시 이 부분을 개선

    return true;
}

std::string TypeVariable::toString() const {
    std::ostringstream oss;
    oss << "'" << name_ << (isBound() ? ("_" + std::to_string(id_)) : "");
    return oss.str();
}

// ────────────────────────────────────────────────────────────────
// TypeParameter Implementation
// ────────────────────────────────────────────────────────────────

std::string TypeParameter::toString() const {
    std::ostringstream oss;
    oss << name;

    if (!trait_bounds.empty()) {
        oss << ": ";
        for (size_t i = 0; i < trait_bounds.size(); ++i) {
            if (i > 0) oss << " + ";
            oss << trait_bounds[i];
        }
    }

    if (lifetime) {
        oss << " + " << lifetime->name;
    }

    return oss.str();
}

// ────────────────────────────────────────────────────────────────
// TypeConstraint Implementation
// ────────────────────────────────────────────────────────────────

bool TypeConstraint::check(const Type* actual) const {
    if (!actual) return false;

    switch (kind) {
        case Kind::Bound:
            // T: Clone 검사 (실제로는 trait 테이블 조회)
            return true;  // 간단히 통과 (실제 구현에서는 trait lookup)

        case Kind::Equality:
            // T = U 검사
            return left->toString() == actual->toString();

        case Kind::Subtype:
            // T <: U 검사
            // (subtyping 관계 처리)
            return true;

        case Kind::Compatible:
            // T ~~ U 검사 (coercion 가능한가?)
            return true;

        default:
            return false;
    }
}

std::string TypeConstraint::toString() const {
    std::ostringstream oss;

    switch (kind) {
        case Kind::Bound:
            oss << left->toString() << ": " << right_name;
            break;
        case Kind::Equality:
            oss << left->toString() << " = " << right_type->toString();
            break;
        case Kind::Subtype:
            oss << left->toString() << " <: " << right_type->toString();
            break;
        case Kind::Compatible:
            oss << left->toString() << " ~~ " << right_type->toString();
            break;
    }

    return oss.str();
}

// ────────────────────────────────────────────────────────────────
// GenericType Implementation
// ────────────────────────────────────────────────────────────────

Type* GenericType::instantiate(const std::vector<Type*>& args) const {
    if (args.size() != type_params.size()) {
        return nullptr;  // 인자 개수 불일치
    }

    // Substitution 맵 생성: {T -> args[0], U -> args[1]}
    Substitution subst;
    for (size_t i = 0; i < type_params.size(); ++i) {
        subst[type_params[i]->name] = args[i];
    }

    // 기본 타입에 substitution 적용
    return applySubstitution(element_type, subst);
}

std::vector<TypeConstraint> GenericType::extractConstraints() const {
    std::vector<TypeConstraint> constraints;

    for (const auto* param : type_params) {
        for (const auto& bound : param->trait_bounds) {
            // TypeVariable를 Type*로 변환 (임시 해결)
            // TODO: Week 4에서 Lifetime과 함께 정리
        }
    }

    return constraints;
}

std::string GenericType::toString() const {
    std::ostringstream oss;
    oss << base_name << "<";

    for (size_t i = 0; i < type_args.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << type_args[i]->toString();
    }

    oss << ">";
    return oss.str();
}

// ────────────────────────────────────────────────────────────────
// GenericFunctionType Implementation
// ────────────────────────────────────────────────────────────────

std::string GenericFunctionType::toString() const {
    std::ostringstream oss;
    oss << "fn " << name;

    // Type parameters
    if (!type_params.empty()) {
        oss << "<";
        for (size_t i = 0; i < type_params.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << type_params[i]->toString();
        }
        oss << ">";
    }

    // Parameters
    oss << "(";
    for (size_t i = 0; i < param_types.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << param_types[i].first << ": " << param_types[i].second->toString();
    }
    oss << ")";

    // Return type
    if (return_type) {
        oss << " -> " << return_type->toString();
    }

    return oss.str();
}

// ────────────────────────────────────────────────────────────────
// Substitution Application
// ────────────────────────────────────────────────────────────────

Type* applySubstitution(Type* type, const Substitution& subst) {
    if (!type) return nullptr;

    // TypeVariable인 경우, 치환 맵 확인
    if (type->is_type_var) {
        auto* var = static_cast<TypeVariable*>(type);
        auto it = subst.find(var->name());
        if (it != subst.end()) {
            return it->second;
        }
        return type;
    }

    // 포인터 타입
    if (type->is_pointer) {
        Type* new_pointee = applySubstitution(type->pointee_type, subst);
        if (new_pointee != type->pointee_type) {
            Type* result = new Type(*type);
            result->pointee_type = new_pointee;
            return result;
        }
    }

    // 배열 타입
    if (type->is_array) {
        Type* new_element = applySubstitution(type->element_type, subst);
        if (new_element != type->element_type) {
            Type* result = new Type(*type);
            result->element_type = new_element;
            return result;
        }
    }

    return type;
}

// ────────────────────────────────────────────────────────────────
// GenericResolver Implementation
// ────────────────────────────────────────────────────────────────

Substitution GenericResolver::resolve(
    const GenericFunctionType* func,
    const std::vector<Type*>& arg_types) const {

    Substitution result;

    if (arg_types.size() != func->param_types.size()) {
        return result;  // 빈 substitution (오류)
    }

    // 함수 파라미터 타입과 실제 인자 타입을 통일
    for (size_t i = 0; i < func->param_types.size(); ++i) {
        Type* param_type = func->param_types[i].second;
        Type* arg_type = arg_types[i];

        if (!unify(param_type, arg_type, result)) {
            return Substitution();  // 오류: 통일 실패
        }
    }

    return result;
}

Substitution GenericResolver::resolveWithExplicitArgs(
    const GenericFunctionType* func,
    const std::vector<Type*>& type_args) const {

    Substitution result;

    if (type_args.size() != func->type_params.size()) {
        return result;  // 오류
    }

    for (size_t i = 0; i < func->type_params.size(); ++i) {
        result[func->type_params[i]->name] = type_args[i];
    }

    return result;
}

bool GenericResolver::satisfiesConstraints(
    const Substitution& subst,
    const GenericFunctionType* func) const {

    for (const auto* param : func->type_params) {
        if (subst.find(param->name) == subst.end()) {
            return false;  // 타입 파라미터가 해결되지 않음
        }

        Type* resolved_type = subst.at(param->name);

        // 각 trait bound 확인
        for (const auto& bound : param->trait_bounds) {
            // 실제 trait 테이블 조회 필요 (간단히 통과)
            if (!resolved_type) return false;
        }
    }

    return true;
}

bool GenericResolver::validateInstantiation(
    const GenericType* generic,
    const std::vector<Type*>& args) const {

    if (args.size() != generic->type_params.size()) {
        return false;
    }

    // 각 인자가 대응하는 파라미터의 제약조건을 만족하는지 확인
    for (size_t i = 0; i < args.size(); ++i) {
        const auto* param = generic->type_params[i];
        const auto* arg = args[i];

        // (실제 trait lookup 필요)
        if (!arg) return false;
    }

    return true;
}

bool GenericResolver::unify(Type* t1, Type* t2, Substitution& result) const {
    if (!t1 || !t2) return false;

    // 둘 다 같은 구체적 타입
    if (t1->toString() == t2->toString()) {
        return true;
    }

    // t1이 TypeVariable이면 t2로 바인드
    if (t1->is_type_var) {
        auto* var1 = static_cast<TypeVariable*>(t1);

        // Occurs check
        if (occursCheck(var1, t2)) {
            return false;  // 무한 타입
        }

        result[var1->name()] = t2;
        return true;
    }

    // t2가 TypeVariable이면 t1로 바인드
    if (t2->is_type_var) {
        auto* var2 = static_cast<TypeVariable*>(t2);

        if (occursCheck(var2, t1)) {
            return false;
        }

        result[var2->name()] = t1;
        return true;
    }

    // 둘 다 구체적 타입이지만 다름
    return false;
}

bool GenericResolver::occursCheck(TypeVariable* var, Type* type) const {
    if (!type) return false;

    if (type->is_type_var) {
        auto* other_var = static_cast<TypeVariable*>(type);
        return var->id() == other_var->id();
    }

    // 포인터 또는 배열 타입의 경우
    if (type->is_pointer || type->is_array) {
        Type* element = type->is_pointer ? type->pointee_type : type->element_type;
        return occursCheck(var, element);
    }

    return false;
}

// ────────────────────────────────────────────────────────────────
// GenericTypeRegistry Implementation
// ────────────────────────────────────────────────────────────────

void GenericTypeRegistry::registerGenericType(
    const std::string& name,
    GenericType* generic) {

    generic_types_[name] = generic;
}

GenericType* GenericTypeRegistry::getGenericType(
    const std::string& name) const {

    auto it = generic_types_.find(name);
    if (it != generic_types_.end()) {
        return it->second;
    }
    return nullptr;
}

void GenericTypeRegistry::registerGenericFunction(
    const std::string& name,
    GenericFunctionType* func) {

    generic_functions_[name] = func;
}

GenericFunctionType* GenericTypeRegistry::getGenericFunction(
    const std::string& name) const {

    auto it = generic_functions_.find(name);
    if (it != generic_functions_.end()) {
        return it->second;
    }
    return nullptr;
}

Type* GenericTypeRegistry::instantiate(
    const std::string& generic_name,
    const std::vector<Type*>& args) {

    GenericType* generic = getGenericType(generic_name);
    if (!generic) return nullptr;

    if (!resolver_.validateInstantiation(generic, args)) {
        return nullptr;  // 검증 실패
    }

    return generic->instantiate(args);
}

std::vector<std::string> GenericTypeRegistry::listGenericTypes() const {
    std::vector<std::string> result;
    for (const auto& pair : generic_types_) {
        result.push_back(pair.first);
    }
    return result;
}

}  // namespace zlang
