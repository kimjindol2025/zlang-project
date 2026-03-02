#ifndef COMPILER_PIPELINE_H
#define COMPILER_PIPELINE_H

#include "CodeGenerator.h"
#include <string>
#include <vector>

namespace zlang {

class CompilerPipeline {
private:
    std::unique_ptr<IntegratedTypeChecker> type_checker_;
    std::unique_ptr<CodeGenerator> code_generator_;

public:
    CompilerPipeline();
    ~CompilerPipeline() = default;

    // Main compilation
    bool compile(
        const std::string& source_code,
        const std::string& output_file
    );

    // Step-by-step compilation
    std::string typeCheck(const std::string& ast);
    std::string generateIR(const std::string& typed_ast);
    std::string compileLLVM(const std::string& ir);

    // Error handling
    const std::string& getLastError() const { return last_error_; }

private:
    std::string last_error_;

    // Helper functions
    std::vector<std::string> parseProgram(const std::string& source);
    std::string executeCommand(const std::string& cmd);
};

}  // namespace zlang

#endif  // COMPILER_PIPELINE_H
