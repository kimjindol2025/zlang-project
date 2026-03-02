#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <vector>

namespace zlang {

// Forward declarations
struct Lifetime {
    std::string name;
};

class TypeVariable;  // Forward declaration

// Z-Lang의 모든 타입을 표현하는 열거형
enum class BuiltinType {
    I32,        // 32비트 정수
    I64,        // 64비트 정수
    F32,        // 32비트 실수
    F64,        // 64비트 실수
    Bool,       // 불린
    Void,       // 반환값 없음
    String,     // 문자열
    // 【 Step 2: Result & Option Types 】
    Result,     // Result<T, E> - 성공/실패 타입
    Option,     // Option<T> - 어떤 값이거나 None
    Unknown     // 알 수 없는 타입
};

// Z-Lang 타입 정보 (포인터, 배열 등 수정자 포함)
struct Type {
    BuiltinType base = BuiltinType::Unknown;

    // 타입 변수 (TypeVariable)
    bool is_type_var = false;

    // 포인터 타입: &T
    bool is_pointer = false;
    Type* pointee_type = nullptr;

    // 배열 타입: [T; N]
    bool is_array = false;
    Type* element_type = nullptr;
    int array_size = 0;

    // 【 Step 2: Result<T, E> 타입 】
    // Result<i64, String> 같은 제네릭 타입 표현
    bool is_result = false;
    Type* ok_type = nullptr;      // Result의 Ok(T) 타입
    Type* err_type = nullptr;     // Result의 Err(E) 타입

    // 【 Step 2: Option<T> 타입 】
    bool is_option = false;
    Type* value_type = nullptr;   // Option의 Some(T) 타입

    // 소유권 상태
    enum class OwnershipState {
        Available,  // 사용 가능
        Moved,      // 이동됨 (더 이상 사용 불가)
        Borrowed    // 대여됨 (참조만 가능)
    } ownership = OwnershipState::Available;

    // 편의 함수들
    bool isInteger() const {
        return base == BuiltinType::I32 || base == BuiltinType::I64;
    }

    bool isFloat() const {
        return base == BuiltinType::F32 || base == BuiltinType::F64;
    }

    bool isNumeric() const {
        return isInteger() || isFloat();
    }

    std::string toString() const;
};

} // namespace zlang

#endif // TYPE_H
