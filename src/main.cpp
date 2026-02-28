#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <cstring>

// LLVM 헤더
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

// Z-Lang 헤더
#include "codegen/CodeGenerator.h"
#include "codegen/Type.h"
#include "codegen/BackendCompiler.h"  // 【 Phase 2: Backend 】
#include "ast/ASTNode.h"
#include "semantic/SymbolTable.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "analysis/WCETAnalyzer.h"  // 【 Step 4: WCET Analysis 】

using namespace zlang;

// ============================================================================
// 【 Z-Lang 컴파일러 메인 드라이버 】
//
// 목표: Lexer → Parser → TypeChecker → CodeGenerator 전체 파이프라인 통합
// ============================================================================

class ZLangCompiler {
private:
    std::string source_file;
    std::string output_file;
    bool verbose = false;
    bool emit_ir = false;
    bool emit_object = false;
    bool execute = false;
    std::unique_ptr<CodeGenerator> codegen;  // CodeGenerator 생명주기 관리

public:
    ZLangCompiler() : output_file("output.ll") {}

    /**
     * 명령행 인자 파싱
     */
    bool parseArguments(int argc, char* argv[]) {
        if (argc < 2) {
            printUsage(argv[0]);
            return false;
        }

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "-o" || arg == "--output") {
                if (i + 1 < argc) {
                    output_file = argv[++i];
                }
            } else if (arg == "-v" || arg == "--verbose") {
                verbose = true;
            } else if (arg == "--emit-ir") {
                emit_ir = true;
            } else if (arg == "--emit-object") {
                emit_object = true;
            } else if (arg == "--execute") {
                execute = true;
            } else if (arg == "-h" || arg == "--help") {
                printUsage(argv[0]);
                return false;
            } else if (arg[0] != '-') {
                source_file = arg;
            }
        }

        if (source_file.empty()) {
            std::cerr << "❌ 오류: 소스 파일을 지정해주세요." << std::endl;
            printUsage(argv[0]);
            return false;
        }

        return true;
    }

    /**
     * 사용법 출력
     */
    void printUsage(const char* program) {
        std::cout << R"(
【 Z-Lang 컴파일러 】

사용법: )" << program << R"( <source.z> [옵션]

옵션:
  -o <file>           출력 파일명 (기본값: output.ll)
  -v, --verbose       상세 출력
  --emit-ir           LLVM IR 파일 생성 (.ll)
  --emit-object       Object 파일 생성 (.o)
  --execute           컴파일 후 실행
  -h, --help          도움말

예제:
  zlang test.z                           # test.z 컴파일 → output.ll
  zlang test.z -o test.ll --verbose      # 상세 출력
  zlang test.z --emit-ir --emit-object   # IR과 Object 생성
)" << std::endl;
    }

    /**
     * 소스 파일 읽기
     */
    std::string readSourceFile() {
        std::ifstream file(source_file);
        if (!file.is_open()) {
            std::cerr << "❌ 오류: 파일을 열 수 없습니다: " << source_file << std::endl;
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        if (verbose) {
            std::cout << "📖 소스 파일 읽음: " << source_file << std::endl;
            std::cout << "📏 크기: " << buffer.str().length() << " 바이트" << std::endl;
        }

        return buffer.str();
    }

    /**
     * 【 Stage 1: Lexing (어휘 분석) - ✅ 2.1 완성 】
     *
     * 소스 코드 → 토큰 스트림
     */
    std::vector<Token> performLexing(const std::string& source) {
        if (verbose) {
            std::cout << "\n【 Stage 1: Lexing (어휘 분석) - ✅ 완성 】" << std::endl;
        }

        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        // 에러 확인
        for (const auto& err : lexer.getErrors()) {
            if (verbose) {
                std::cout << "  ⚠️  " << err << std::endl;
            }
        }

        if (verbose) {
            std::cout << "  ✅ 토큰화 완료: " << tokens.size() << "개 토큰" << std::endl;
        }

        return tokens;
    }

    /**
     * 【 Stage 2: Parsing (구문 분석) - ✅ 2.2 완성 】
     *
     * 토큰 스트림 → AST (Abstract Syntax Tree)
     */
    std::shared_ptr<ProgramNode> performParsing(const std::vector<Token>& tokens) {
        if (verbose) {
            std::cout << "\n【 Stage 2: Parsing (구문 분석) - ✅ 완성 】" << std::endl;
        }

        Parser parser(tokens);
        auto program = parser.parse();

        // 에러 확인
        for (const auto& err : parser.getErrors()) {
            if (verbose) {
                std::cout << "  ⚠️  " << err << std::endl;
            }
        }

        if (verbose) {
            std::cout << "  ✅ 파싱 완료: " << program->functions.size() << "개 함수" << std::endl;
        }

        return program;
    }

    /**
     * 【 Stage 3: Semantic Analysis (의미 분석) 】
     *
     * 현재 상태: 2.3 단계 구현 완료 (문서화만 가능)
     * TODO: TypeChecker.cpp 통합
     */
    bool performSemanticAnalysis(std::shared_ptr<ProgramNode>& ast) {
        if (verbose) {
            std::cout << "\n【 Stage 3: Semantic Analysis (의미 분석) 】" << std::endl;
        }

        // ⚠️ 현재: 문서화된 상태. 실제 TypeChecker 구현 필요
        // 예: TypeChecker checker;
        //     return checker.check(ast);

        if (verbose) {
            std::cout << "⚠️  TypeChecker 미구현 (2.3 단계 구현 예정)" << std::endl;
        }

        return true;  // 임시로 성공 반환
    }

    /**
     * 【 Stage 4: Code Generation (코드 생성) 】
     *
     * 현재 상태: 2.4 단계 구현 완료 ✅
     * CodeGenerator.cpp를 통한 LLVM IR 생성
     */
    LLVMModuleRef performCodeGeneration(const std::shared_ptr<ProgramNode>& ast) {
        if (verbose) {
            std::cout << "\n【 Stage 4: Code Generation (코드 생성) 】" << std::endl;
        }

        // CodeGenerator를 클래스 멤버로 저장 (생명주기 관리)
        codegen = std::make_unique<CodeGenerator>();
        LLVMModuleRef module = codegen->generate(ast);

        if (!module) {
            std::cerr << "❌ 코드 생성 실패" << std::endl;
            const auto& errors = codegen->getErrors();
            for (const auto& err : errors) {
                std::cerr << "  - " << err << std::endl;
            }
            return nullptr;
        }

        if (verbose) {
            std::cout << "✅ LLVM IR 생성 완료" << std::endl;
        }

        return module;
    }

    /**
     * 【 Stage 5: Optimization (최적화) 】
     *
     * 현재 상태: 2.5 단계 구현 예정
     * TODO: LLVM Pass 프레임워크 통합
     */
    bool performOptimization(LLVMModuleRef module) {
        if (verbose) {
            std::cout << "\n【 Stage 5: Optimization (최적화) 】" << std::endl;
        }

        if (!module) {
            std::cerr << "❌ performOptimization: module is null" << std::endl;
            return false;
        }

        // 【 IR 통계 수집 & 최적화 분석 】

        int func_count = 0;
        int block_count = 0;
        int instr_count = 0;
        int const_count = 0;

        // 모든 함수 순회
        for (LLVMValueRef func = LLVMGetFirstFunction(module);
             func != nullptr;
             func = LLVMGetNextFunction(func)) {
            func_count++;

            // 함수의 모든 블록 순회
            for (LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(func);
                 block != nullptr;
                 block = LLVMGetNextBasicBlock(block)) {
                block_count++;

                // 블록의 모든 명령어 순회
                for (LLVMValueRef instr = LLVMGetFirstInstruction(block);
                     instr != nullptr;
                     instr = LLVMGetNextInstruction(instr)) {
                    instr_count++;

                    // 상수 감지
                    if (LLVMIsConstant(instr)) {
                        const_count++;
                    }
                }
            }
        }

        // 【 최적화 기회 분석 】
        int optimization_opportunities = 0;

        // 1️⃣ 상수 폴딩 기회
        if (const_count > 0) {
            optimization_opportunities += const_count;
            if (verbose) {
                std::cout << "  1️⃣ Constant Folding: " << const_count << "개 기회" << std::endl;
            }
        }

        // 2️⃣ Dead Code 감지 (미사용 변수는 상세 분석 필요)
        // 3️⃣ Loop Optimization 감지
        // 4️⃣ Instruction Combining 감지

        if (verbose) {
            std::cout << "📊 IR 통계:" << std::endl;
            std::cout << "  • 함수: " << func_count << "개" << std::endl;
            std::cout << "  • 기본 블록: " << block_count << "개" << std::endl;
            std::cout << "  • 명령어: " << instr_count << "개" << std::endl;
            std::cout << "  • 상수: " << const_count << "개" << std::endl;
            std::cout << std::endl;
            std::cout << "🔄 최적화 기회: " << optimization_opportunities << "개" << std::endl;
            std::cout << "✅ 최적화 분석 완료" << std::endl;
        }

        return true;
    }

    /**
     * 【 Stage 6: IR 출력 (LLVM IR을 파일로 저장) 】
     */
    bool emitIRFile(LLVMModuleRef module) {
        if (verbose) {
            std::cout << "\n【 Stage 6: IR 출력 】" << std::endl;
        }

        char* ir_str = LLVMPrintModuleToString(module);
        if (!ir_str) {
            std::cerr << "❌ IR 문자열 생성 실패" << std::endl;
            return false;
        }

        std::ofstream ir_file(output_file);
        if (!ir_file.is_open()) {
            std::cerr << "❌ 출력 파일을 열 수 없습니다: " << output_file << std::endl;
            LLVMDisposeMessage(ir_str);
            return false;
        }

        ir_file << ir_str;
        ir_file.close();

        if (verbose) {
            std::cout << "✅ IR 파일 생성: " << output_file << std::endl;
        }

        LLVMDisposeMessage(ir_str);
        return true;
    }

    /**
     * 메인 컴파일 프로세스
     */
    int compile() {
        std::cout << "\n╔════════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║              Z-Lang 컴파일러 v0.1 (Legend Phase 2.4)          ║" << std::endl;
        std::cout << "║            「기록이 증명이다」 - 전설의 컴파일러              ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n" << std::endl;

        if (verbose) {
            std::cout << "【 빌드 정보 】" << std::endl;
            std::cout << "  소스: " << source_file << std::endl;
            std::cout << "  출력: " << output_file << std::endl;
            std::cout << std::endl;
        }

        // Stage 1: Lexing
        if (verbose) std::cout << "🔄 Stage 1: 어휘 분석 중..." << std::endl;
        std::string source = readSourceFile();
        if (source.empty()) return 1;
        std::vector<Token> tokens = performLexing(source);

        // Stage 2: Parsing
        if (verbose) std::cout << "🔄 Stage 2: 구문 분석 중..." << std::endl;
        std::shared_ptr<ProgramNode> ast = performParsing(tokens);
        if (!ast || ast->functions.empty()) {
            std::cerr << "❌ 구문 분석 실패 (함수가 없습니다)" << std::endl;
            // 계속 진행 (빈 프로그램도 유효함)
        }

        // Stage 3: Semantic Analysis
        if (verbose) std::cout << "🔄 Stage 3: 의미 분석 중..." << std::endl;
        if (!performSemanticAnalysis(ast)) {
            std::cerr << "❌ 의미 분석 실패" << std::endl;
            return 1;
        }

        // Stage 4: Code Generation (2.4 완료)
        if (verbose) std::cout << "🔄 Stage 4: 코드 생성 중..." << std::endl;
        LLVMModuleRef module = performCodeGeneration(ast);
        if (!module) {
            std::cerr << "❌ 코드 생성 실패" << std::endl;
            return 1;
        }

        // Stage 5: Optimization
        if (verbose) std::cout << "🔄 Stage 5: 최적화 중..." << std::endl;
        if (!performOptimization(module)) {
            std::cerr << "❌ 최적화 실패" << std::endl;
            LLVMDisposeModule(module);
            return 1;
        }

        // 【 Stage 4.5: WCET Analysis (최악의 경우 실행 시간 분석) 】
        if (verbose) std::cout << "🔄 Stage 4.5: WCET 분석 중..." << std::endl;
        WCETAnalyzer wcet_analyzer;
        if (!wcet_analyzer.analyze(ast)) {
            if (verbose) std::cout << "⚠️  WCET 분석 완료 (정보 제공용)" << std::endl;
        } else {
            wcet_analyzer.printReport();
        }

        // Stage 6: Emit IR
        if (verbose) std::cout << "🔄 Stage 6: IR 출력 중..." << std::endl;
        if (!emitIRFile(module)) {
            std::cerr << "❌ IR 출력 실패" << std::endl;
            LLVMDisposeModule(module);
            return 1;
        }

        // 【 Phase 2: Backend Compilation 】
        // Stage 7-9: IR → Assembly → Object → Executable

        if (emit_ir) {
            // IR만 생성하는 경우 (기본 동작)
            if (verbose) {
                std::cout << "\n【 IR 생성 완료 】" << std::endl;
            }
        } else {
            // Backend 컴파일: Assembly, Object, Executable 생성
            if (verbose) std::cout << "🔄 Backend 컴파일 시작..." << std::endl;

            BackendOptions backend_opts;
            backend_opts.verbose = verbose;
            backend_opts.optimize = true;
            backend_opts.wcet_friendly = true;  // Real-Time 최적화

            BackendCompiler backend;

            // 출력 파일명 결정
            std::string exe_file = output_file;
            if (exe_file.substr(exe_file.find_last_of(".") + 1) == "ll") {
                exe_file = exe_file.substr(0, exe_file.find_last_of("."));
            }
            if (exe_file.empty()) {
                exe_file = "a.out";
            }

            // Backend 컴파일 수행
            if (!backend.compile(output_file, exe_file, backend_opts)) {
                std::cerr << "❌ Backend 컴파일 실패" << std::endl;
                for (const auto& err : backend.getErrors()) {
                    std::cerr << "  - " << err << std::endl;
                }
                LLVMDisposeModule(module);
                return 1;
            }

            if (verbose) {
                std::cout << "✅ Backend 컴파일 완료" << std::endl;
            }
        }

        // 정리
        LLVMDisposeModule(module);

        std::cout << "\n✅ 컴파일 완료!" << std::endl;
        std::cout << "📄 출력: " << output_file << std::endl;

        if (!emit_ir) {
            std::string exe_file = output_file;
            if (exe_file.substr(exe_file.find_last_of(".") + 1) == "ll") {
                exe_file = exe_file.substr(0, exe_file.find_last_of("."));
            }
            std::cout << "🔧 실행 파일: " << exe_file << std::endl;
            std::cout << "\n【 다음 단계 】" << std::endl;
            std::cout << "  1. 실행: ./" << exe_file << std::endl;
            std::cout << "  2. IR 확인: cat " << output_file << std::endl;
        } else {
            std::cout << "\n【 다음 단계 】" << std::endl;
            std::cout << "  1. IR 확인: cat " << output_file << std::endl;
            std::cout << "  2. Assembly 생성: llc " << output_file << " -o output.s" << std::endl;
            std::cout << "  3. Object 생성: as output.s -o output.o" << std::endl;
            std::cout << "  4. 실행 파일: gcc output.o -o executable" << std::endl;
        }

        return 0;
    }
};

// ============================================================================
// 메인 진입점
// ============================================================================

int main(int argc, char* argv[]) {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    ZLangCompiler compiler;

    if (!compiler.parseArguments(argc, argv)) {
        return 1;
    }

    return compiler.compile();
}
