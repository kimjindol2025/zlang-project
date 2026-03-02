#include "CompilerPipeline.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <regex>

namespace zlang {

CompilerPipeline::CompilerPipeline()
    : type_checker_(std::make_unique<IntegratedTypeChecker>()),
      code_generator_(std::make_unique<CodeGenerator>()) {
}

bool CompilerPipeline::compile(
    const std::string& source_code,
    const std::string& output_file) {

    // Step 1: Parse program
    auto functions = parseProgram(source_code);
    if (functions.empty()) {
        last_error_ = "No functions found in program";
        return false;
    }

    // Step 2: Type check
    std::string typed_ast;
    for (const auto& func : functions) {
        typed_ast += typeCheck(func) + "\n";
    }

    // Step 3: Generate IR
    std::string ir = generateIR(typed_ast);
    if (ir.empty()) {
        last_error_ = "IR generation failed";
        return false;
    }

    // Step 4: Write IR to file
    std::ofstream ir_file(output_file + ".ll");
    ir_file << ir;
    ir_file.close();

    // Step 5: Compile with llc
    std::string obj_file = output_file + ".o";
    std::string compile_cmd = "llc -filetype=obj -o " + obj_file + " " + output_file + ".ll";
    int ret = system(compile_cmd.c_str());

    if (ret != 0) {
        last_error_ = "LLVM compilation failed";
        return false;
    }

    // Step 6: Link with clang
    std::string link_cmd = "clang -o " + output_file + " " + obj_file;
    ret = system(link_cmd.c_str());

    if (ret != 0) {
        last_error_ = "Linking failed";
        return false;
    }

    return true;
}

std::string CompilerPipeline::typeCheck(const std::string& ast) {
    // 간단한 구현: 함수 정의 파싱
    // 예: "fn add(a, b) { a + b }"

    std::regex func_regex(R"(fn\s+(\w+)\s*\(([^)]*)\)\s*\{([^}]*)\})");
    std::smatch match;

    if (std::regex_search(ast, match, func_regex)) {
        std::string func_name = match[1].str();
        std::string params = match[2].str();
        std::string body = match[3].str();

        // 매개변수 파싱 (간단화)
        std::vector<std::string> param_names, param_types;

        if (!params.empty()) {
            // "a, b" → params
            std::istringstream iss(params);
            std::string param;
            while (std::getline(iss, param, ',')) {
                // 공백 제거
                param.erase(0, param.find_first_not_of(" \t"));
                param.erase(param.find_last_not_of(" \t") + 1);

                param_names.push_back(param);
                param_types.push_back("i64");  // 기본값
            }
        }

        // 바인딩
        for (const auto& param : param_names) {
            type_checker_->bindVariable(param, "i64");
        }

        // 반환 타입 추론
        std::string return_type = type_checker_->inferExprType(body);

        // 검증된 함수 등록
        type_checker_->registerConcreteFunction(func_name, return_type, param_types);

        return ast;  // 타입 체크 완료
    }

    return ast;
}

std::string CompilerPipeline::generateIR(const std::string& typed_ast) {
    // 함수들 파싱
    std::vector<std::string> function_defs;

    std::regex func_regex(R"(fn\s+(\w+)\s*\(([^)]*)\)\s*\{([^}]*)\})");
    std::sregex_iterator iter(typed_ast.begin(), typed_ast.end(), func_regex);
    std::sregex_iterator end;

    while (iter != end) {
        std::smatch match = *iter;
        std::string func_name = match[1].str();
        std::string params = match[2].str();
        std::string body = match[3].str();

        // 매개변수 파싱
        std::vector<std::string> param_names, param_types;
        if (!params.empty()) {
            std::istringstream iss(params);
            std::string param;
            while (std::getline(iss, param, ',')) {
                param.erase(0, param.find_first_not_of(" \t"));
                param.erase(param.find_last_not_of(" \t") + 1);
                param_names.push_back(param);
                param_types.push_back("i64");
            }
        }

        // 반환 타입 추론
        std::string return_type = type_checker_->inferExprType(body);

        // 코드 생성
        std::string func_ir = code_generator_->generateFunctionCode(
            func_name, param_names, param_types, return_type, body
        );
        function_defs.push_back(func_ir);

        ++iter;
    }

    // 모듈 생성
    return code_generator_->generateModule(function_defs);
}

std::string CompilerPipeline::compileLLVM(const std::string& ir) {
    // IR을 파일로 저장하고 llc 호출
    std::ofstream temp_file("_temp.ll");
    temp_file << ir;
    temp_file.close();

    int ret = system("llc -filetype=obj -o _temp.o _temp.ll");
    if (ret != 0) {
        return "";
    }

    return "_temp.o";
}

std::vector<std::string> CompilerPipeline::parseProgram(
    const std::string& source) {

    std::vector<std::string> functions;

    // 함수 정의 파싱
    std::regex func_regex(R"(fn\s+\w+\s*\([^)]*\)\s*\{[^}]*\})");
    std::sregex_iterator iter(source.begin(), source.end(), func_regex);
    std::sregex_iterator end;

    while (iter != end) {
        functions.push_back(iter->str());
        ++iter;
    }

    return functions;
}

std::string CompilerPipeline::executeCommand(const std::string& cmd) {
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    pclose(pipe);
    return result;
}

}  // namespace zlang
