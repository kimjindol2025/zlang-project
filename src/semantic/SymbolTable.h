#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <llvm-c/Core.h>
#include "../codegen/Type.h"

namespace zlang {

// 심볼 정보 (2.3 의미분석에서 수집됨)
struct SymbolInfo {
    std::string name;
    Type type;
    Type::OwnershipState ownership;

    // 2.4 코드생성에서 사용:
    // LLVM의 alloca 결과 (메모리 주소)
    LLVMValueRef llvm_value = nullptr;
};

// 심볼 테이블: 스코프 스택으로 관리
class SymbolTable {
private:
    // 각 스코프는 변수명 → SymbolInfo 매핑
    std::vector<std::unordered_map<std::string, SymbolInfo>> scope_stack;

public:
    SymbolTable() {
        // 전역 스코프로 시작
        pushScope();
    }

    // 새로운 스코프 진입 (함수, 블록 등)
    void pushScope() {
        scope_stack.push_back({});
    }

    // 스코프 종료
    void popScope() {
        if (scope_stack.size() > 1) {
            scope_stack.pop_back();
        }
    }

    // 현재 스코프에 심볼 정의
    void define(const std::string& name, const SymbolInfo& info) {
        if (!scope_stack.empty()) {
            scope_stack.back()[name] = info;
        }
    }

    // 현재 스코프에서만 찾기
    std::optional<SymbolInfo> lookupLocal(const std::string& name) const {
        if (scope_stack.empty()) return std::nullopt;

        auto& current_scope = scope_stack.back();
        auto it = current_scope.find(name);
        if (it != current_scope.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // 모든 스코프에서 찾기 (가장 가까운 스코프부터)
    std::optional<SymbolInfo> resolve(const std::string& name) const {
        // 가장 최신 스코프부터 역순으로 탐색
        for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
            auto sym_it = it->find(name);
            if (sym_it != it->end()) {
                return sym_it->second;
            }
        }
        return std::nullopt;
    }

    // 심볼 업데이트 (소유권 상태 변경 등)
    void update(const std::string& name, const SymbolInfo& info) {
        // 가장 가까운 스코프에서 찾아서 업데이트
        for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
            auto sym_it = it->find(name);
            if (sym_it != it->end()) {
                sym_it->second = info;
                return;
            }
        }
    }

    // 현재 스코프 깊이
    size_t depth() const {
        return scope_stack.size();
    }

    // 디버깅용: 현재 스코프의 모든 심볼 출력
    void dump() const {
        std::cout << "=== SymbolTable (depth=" << scope_stack.size() << ") ===" << std::endl;
        for (size_t i = 0; i < scope_stack.size(); ++i) {
            std::cout << "Scope " << i << ":" << std::endl;
            for (const auto& [name, info] : scope_stack[i]) {
                std::cout << "  " << name << ": " << info.type.toString() << std::endl;
            }
        }
    }
};

} // namespace zlang

#endif // SYMBOL_TABLE_H
