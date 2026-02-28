#include "BackendCompiler.h"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace zlang {

// ============================================================================
// 생성자 & 소멸자
// ============================================================================

BackendCompiler::BackendCompiler() {}

BackendCompiler::~BackendCompiler() {
    cleanup();
}

// ============================================================================
// 메인 컴파일 함수
// ============================================================================

bool BackendCompiler::compile(const std::string& ir_file,
                             const std::string& exe_file,
                             const BackendOptions& opts) {
    errors.clear();

    std::cout << "\n【 Backend Compilation 】" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // Step 1-2: IR → Object 직접 생성 (어셈블러 없이)
    std::cout << "\n🔄 Stage 7-8: Object 파일 생성 중..." << std::endl;
    std::string obj_file = exe_file + ".o";
    if (!compileToObject(ir_file, obj_file, opts.target_arch,
                        opts.optimize, opts.wcet_friendly)) {
        errors.push_back("Object 파일 생성 실패");
        return false;
    }
    temp_files.push_back(obj_file);
    std::cout << "✅ Object 파일 생성 완료: " << obj_file << std::endl;

    // Step 3: 링킹
    std::cout << "\n🔄 Stage 9: Linking 중..." << std::endl;
    if (!linkToExecutable({obj_file}, exe_file, opts.static_linking)) {
        errors.push_back("Linking 실패");
        return false;
    }
    std::cout << "✅ Linking 완료: " << exe_file << std::endl;

    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "✅ Backend 컴파일 완료!" << std::endl;

    return true;
}

// ============================================================================
// Stage 1: LLVM IR → Assembly
// ============================================================================

bool BackendCompiler::compileToAssembly(const std::string& ir_file,
                                       const std::string& asm_file,
                                       TargetArchitecture arch,
                                       bool optimize,
                                       bool wcet_friendly) {
    // llc (LLVM Compiler) 명령어 구성
    std::string cmd = "llc";

    // 1. 아키텍처 지정
    cmd += " -march=" + getMarch(arch);

    // 2. 최적화 레벨
    // Note: LLVM 21+ doesn't support -disable-* flags, use -O level instead
    if (wcet_friendly) {
        // WCET-friendly 컴파일: 보수적 최적화
        cmd += " -O1";
    } else if (optimize) {
        cmd += " -O2";  // 표준 최적화
    } else {
        cmd += " -O0";  // 디버그 빌드
    }

    // 3. 입출력 파일
    cmd += " " + ir_file;
    cmd += " -o " + asm_file;

    return runCommand(cmd, "Assembly 생성");
}

// ============================================================================
// Stage 1+2: IR → Object (어셈블러 없이, -filetype=obj 사용)
// ============================================================================

bool BackendCompiler::compileToObject(const std::string& ir_file,
                                      const std::string& obj_file,
                                      TargetArchitecture arch,
                                      bool optimize,
                                      bool wcet_friendly) {
    // llc (LLVM Compiler) 명령어 구성, Object 파일 직접 생성
    std::string cmd = "llc";

    // 1. 아키텍처 지정
    cmd += " -march=" + getMarch(arch);

    // 2. Object 파일 형식 직접 생성
    cmd += " -filetype=obj";

    // 3. 최적화 레벨 (Stage 7-8과 동일)
    if (wcet_friendly) {
        cmd += " -O1";
    } else if (optimize) {
        cmd += " -O2";
    } else {
        cmd += " -O0";
    }

    // 4. 입출력 파일
    cmd += " " + ir_file;
    cmd += " -o " + obj_file;

    return runCommand(cmd, "Object 파일 생성");
}

// ============================================================================
// Stage 2: Assembly → Object
// ============================================================================

bool BackendCompiler::assembleToObject(const std::string& asm_file,
                                      const std::string& obj_file) {
    // GNU as (assembler) 또는 llvm-as 사용
    // GNU as가 더 널리 사용되므로 기본값
    std::string cmd = "as " + asm_file + " -o " + obj_file;

    return runCommand(cmd, "Object 파일 생성");
}

// ============================================================================
// Stage 3: Linking
// ============================================================================

bool BackendCompiler::linkToExecutable(const std::vector<std::string>& obj_files,
                                      const std::string& exe_file,
                                      bool static_link) {
    // Clang을 링커로 사용 (GCC 호환)
    std::string cmd = "clang";

    // Object 파일 추가
    for (const auto& obj : obj_files) {
        cmd += " " + obj;
    }

    // 정적/동적 링킹 옵션 (Termux 호환)
    // -static은 대부분의 Android 시스템에서 지원되지 않으므로 생략
    if (static_link) {
        // cmd += " -static";  // Termux에서 지원 안 함
    }

    // C 표준 라이브러리
    cmd += " -lc -lm";

    // 출력 파일
    cmd += " -o " + exe_file;

    return runCommand(cmd, "Linking");
}

// ============================================================================
// 표준 라이브러리와 함께 링킹
// ============================================================================

bool BackendCompiler::linkWithStdlib(const std::vector<std::string>& obj_files,
                                    const std::string& exe_file,
                                    const std::string& stdlib_path) {
    std::string cmd = "clang";

    // Object 파일
    for (const auto& obj : obj_files) {
        cmd += " " + obj;
    }

    // Z-Lang 표준 라이브러리
    if (!stdlib_path.empty()) {
        cmd += " " + stdlib_path + "/zlang_stdlib.a";
    }

    // C 표준 라이브러리
    cmd += " -static -lc -lm";

    // 출력
    cmd += " -o " + exe_file;

    return runCommand(cmd, "Stdlib Linking");
}

// ============================================================================
// Helper 메서드
// ============================================================================

std::string BackendCompiler::getMarch(TargetArchitecture arch) const {
    switch (arch) {
        case TargetArchitecture::X86_64:   return "x86-64";
        case TargetArchitecture::ARM64:    return "aarch64";
        case TargetArchitecture::RISC_V:   return "riscv64";
        case TargetArchitecture::AUTO:     {
            // 자동 감지: 현재 시스템 아키텍처 감지
            // llvm-config를 사용하여 현재 타겟 아키텍처 확인
            FILE* pipe = popen("llvm-config --host-target", "r");
            if (pipe) {
                char buffer[128];
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    std::string target(buffer);
                    // 개행 문자 제거
                    if (target.back() == '\n') target.pop_back();
                    pclose(pipe);

                    // 아키텍처 감지
                    if (target.find("aarch64") != std::string::npos) {
                        return "aarch64";
                    } else if (target.find("x86_64") != std::string::npos) {
                        return "x86-64";
                    } else if (target.find("riscv64") != std::string::npos) {
                        return "riscv64";
                    }
                }
                pclose(pipe);
            }
            // 기본값: aarch64 (현재 사용 환경)
            return "aarch64";
        }
        default:                           return "aarch64";  // 기본값 변경
    }
}

std::string BackendCompiler::getWCETOptimizations() const {
    // LLVM 21+ compatible WCET-friendly optimization flags
    return " -O1";  // Conservative optimization for deterministic execution
}

bool BackendCompiler::runCommand(const std::string& cmd,
                                const std::string& description) {
    std::cout << "  🔧 " << description << "..." << std::endl;

    int ret = system(cmd.c_str());

    if (ret != 0) {
        std::string error = description + " 실패 (Return code: " +
                           std::to_string(ret) + ")";
        errors.push_back(error);
        std::cerr << "  ❌ " << error << std::endl;
        return false;
    }

    return true;
}

// ============================================================================
// 임시 파일 정리
// ============================================================================

void BackendCompiler::cleanup() {
    for (const auto& temp : temp_files) {
        if (std::remove(temp.c_str()) == 0) {
            std::cout << "  🗑️  임시 파일 삭제: " << temp << std::endl;
        }
    }
    temp_files.clear();
}

// ============================================================================
// 에러 조회
// ============================================================================

const std::vector<std::string>& BackendCompiler::getErrors() const {
    return errors;
}

} // namespace zlang
