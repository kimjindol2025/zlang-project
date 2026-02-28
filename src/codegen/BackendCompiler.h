#ifndef BACKEND_COMPILER_H
#define BACKEND_COMPILER_H

#include <string>
#include <vector>
#include <memory>

namespace zlang {

/**
 * 【 Phase 2: Backend Compiler 】
 *
 * 역할: LLVM IR을 네이티브 코드(Assembly)로 변환
 *
 * 파이프라인:
 *   LLVM IR (.ll)
 *     ↓
 *   Assembly (.s)    ← compileToAssembly()
 *     ↓
 *   Object (.o)      ← assembleToObject()
 *     ↓
 *   Executable       ← linkToExecutable()
 *     ↓
 *   WCET Report      ← WCETAnalyzer (이미 구현)
 */

enum class TargetArchitecture {
    X86_64,      // 64-bit Intel/AMD (가장 널리 사용)
    ARM64,       // 64-bit ARM (M1 Mac, RPi 4)
    RISC_V,      // RISC-V 아키텍처
    AUTO,        // 자동 감지 (현재 머신)
};

/**
 * 컴파일 옵션
 */
struct BackendOptions {
    TargetArchitecture target_arch = TargetArchitecture::AUTO;
    bool optimize = true;               // 최적화 활성화
    bool wcet_friendly = true;          // WCET-friendly 컴파일
    bool static_linking = true;         // 정적 링킹
    bool debug_info = false;            // 디버그 정보 포함
    bool verbose = false;               // 상세 로그
};

/**
 * Backend Compiler 클래스
 *
 * LLVM IR을 네이티브 코드로 변환하는 최종 단계
 */
class BackendCompiler {
public:
    BackendCompiler();
    ~BackendCompiler();

    /**
     * 전체 Backend 파이프라인 수행
     *
     * @param ir_file LLVM IR 파일 경로 (.ll)
     * @param exe_file 출력 실행 파일 경로
     * @param opts 컴파일 옵션
     * @return 성공 여부
     */
    bool compile(const std::string& ir_file,
                const std::string& exe_file,
                const BackendOptions& opts = BackendOptions());

    /**
     * Stage 1: LLVM IR → Assembly 변환
     *
     * @param ir_file 입력 LLVM IR 파일
     * @param asm_file 출력 Assembly 파일
     * @param arch 대상 아키텍처
     * @param optimize 최적화 여부
     * @param wcet_friendly WCET-friendly 옵션
     * @return 성공 여부
     */
    bool compileToAssembly(const std::string& ir_file,
                          const std::string& asm_file,
                          TargetArchitecture arch = TargetArchitecture::X86_64,
                          bool optimize = true,
                          bool wcet_friendly = true);

    /**
     * Stage 1+2: LLVM IR → Object 파일 변환 (어셈블러 없이)
     *
     * @param ir_file 입력 LLVM IR 파일
     * @param obj_file 출력 Object 파일
     * @param arch 대상 아키텍처
     * @param optimize 최적화 여부
     * @param wcet_friendly WCET-friendly 옵션
     * @return 성공 여부
     */
    bool compileToObject(const std::string& ir_file,
                        const std::string& obj_file,
                        TargetArchitecture arch = TargetArchitecture::X86_64,
                        bool optimize = true,
                        bool wcet_friendly = true);

    /**
     * Stage 2: Assembly → Object 파일 변환
     *
     * @param asm_file 입력 Assembly 파일
     * @param obj_file 출력 Object 파일
     * @return 성공 여부
     */
    bool assembleToObject(const std::string& asm_file,
                         const std::string& obj_file);

    /**
     * Stage 3: Object 파일 링킹 → 실행 파일
     *
     * @param obj_files Object 파일 리스트
     * @param exe_file 출력 실행 파일
     * @param static_link 정적 링킹 여부
     * @return 성공 여부
     */
    bool linkToExecutable(const std::vector<std::string>& obj_files,
                         const std::string& exe_file,
                         bool static_link = true);

    /**
     * 표준 라이브러리와 함께 링킹
     *
     * @param obj_files Object 파일 리스트
     * @param exe_file 출력 실행 파일
     * @param stdlib_path Z-Lang 표준 라이브러리 경로
     * @return 성공 여부
     */
    bool linkWithStdlib(const std::vector<std::string>& obj_files,
                       const std::string& exe_file,
                       const std::string& stdlib_path = "");

    /**
     * 임시 파일 정리
     */
    void cleanup();

    /**
     * 에러 메시지 조회
     */
    const std::vector<std::string>& getErrors() const;

private:
    std::vector<std::string> errors;
    std::vector<std::string> temp_files;  // 생성된 임시 파일 추적

    // Helper 메서드들
    std::string getMarch(TargetArchitecture arch) const;
    std::string getWCETOptimizations() const;
    bool runCommand(const std::string& cmd, const std::string& description);
};

} // namespace zlang

#endif // BACKEND_COMPILER_H
