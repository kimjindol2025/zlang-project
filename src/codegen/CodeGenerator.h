#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include "IntegratedTypeChecker.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace zlang {

class CodeGenerator {
private:
    std::unique_ptr<IntegratedTypeChecker> type_checker_;
    int reg_counter_ = 0;
    int block_counter_ = 0;
    std::unordered_map<std::string, int> var_reg_map_;
    std::string current_ir_;
    std::vector<std::string> functions_;

public:
    CodeGenerator();
    ~CodeGenerator() = default;

    // Expression code generation
    std::string generateExprCode(
        const std::string& expr,
        const std::string& expr_type
    );

    std::string generateBinaryOp(
        const std::string& left,
        const std::string& right,
        const std::string& op,
        const std::string& result_type
    );

    std::string generateLiteral(const std::string& literal);
    std::string generateVariable(const std::string& var_name);

    // Function code generation
    std::string generateFunctionCode(
        const std::string& func_name,
        const std::vector<std::string>& param_names,
        const std::vector<std::string>& param_types,
        const std::string& return_type,
        const std::string& body
    );

    std::string generateFunctionSignature(
        const std::string& func_name,
        const std::vector<std::string>& param_names,
        const std::vector<std::string>& param_types,
        const std::string& return_type
    );

    // Module generation
    std::string generateModule(
        const std::vector<std::string>& function_defs
    );

    const std::vector<std::string>& getGeneratedFunctions() const {
        return functions_;
    }

    // Utilities
    std::string newRegister();
    std::string newBlock();
    void reset();
    static std::string getLLVMType(const std::string& zlang_type);
};

}  // namespace zlang

#endif  // CODE_GENERATOR_H
