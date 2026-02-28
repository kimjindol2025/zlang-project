#ifndef WCET_ANALYZER_H
#define WCET_ANALYZER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "../ast/ASTNode.h"

namespace zlang {

/**
 * 【 Step 4: WCET Analysis 】
 *
 * WCET (Worst-Case Execution Time): 최악의 경우 실행 시간
 * - 결정적 실행을 위해 임베디드 시스템에서 필수
 * - Exception path도 분석하여 안정성 보장
 */

struct WCETInfo {
    std::string node_name;
    unsigned long cycles;           // 추정 사이클 수
    unsigned long exception_cycles; // 예외 처리 사이클
    bool has_loop = false;
    int loop_depth = 0;
    bool is_critical_path = false;  // 임계 경로 여부

    // 【 Real-Time 확장 】
    unsigned long loop_bound = 0;   // 루프 반복 횟수 (분석됨)
    bool has_unbounded_loop = false;// 무한 루프 여부
    unsigned long stack_usage = 0;  // 스택 사용량 (바이트)
    bool is_stack_safe = true;      // 스택 오버플로우 안전성
};

/**
 * Memory Safety 분석 정보
 */
struct MemorySafetyInfo {
    bool has_dynamic_allocation = false;    // malloc/new 사용 여부
    bool has_unbounded_arrays = false;      // 크기 미결정 배열 여부
    int pointer_dereference_count = 0;      // 포인터 참조 횟수
    bool is_memory_safe = true;             // 메모리 안전성
};

/**
 * WCET 분석기
 * - 함수별 최악의 경우 실행 시간 계산
 * - Exception path 추적
 * - Handler stack 최적화
 */
class WCETAnalyzer {
public:
    WCETAnalyzer();

    /**
     * 프로그램 분석
     * @param program 분석할 프로그램
     * @return 분석 성공 여부
     */
    bool analyze(const std::shared_ptr<ProgramNode>& program);

    /**
     * 함수의 WCET 조회
     * @param func_name 함수명
     * @return WCET 정보 (없으면 0)
     */
    unsigned long getWCET(const std::string& func_name) const;

    /**
     * Exception path WCET 조회
     */
    unsigned long getExceptionWCET(const std::string& func_name) const;

    /**
     * 임계 경로 함수들
     */
    std::vector<std::string> getCriticalPaths() const;

    /**
     * 분석 결과 보고
     */
    void printReport() const;

    // 【 Real-Time 확장 기능 】

    /**
     * Loop Bound 분석 (루프 반복 횟수 추정)
     */
    bool analyzeLoopBounds(const std::shared_ptr<ProgramNode>& program);

    /**
     * Memory Safety 분석 (동적 할당 감지)
     */
    bool analyzeMemorySafety(const std::shared_ptr<ProgramNode>& program);

    /**
     * Stack Usage 분석 (스택 사용량 계산)
     */
    bool analyzeStackUsage(const std::shared_ptr<ProgramNode>& program);

    /**
     * No-Alloc 검증 (동적 할당 금지 확인)
     */
    bool verifyNoAlloc(const std::shared_ptr<ProgramNode>& program);

    /**
     * Real-Time 적합성 검증
     */
    bool verifyRealTimeCompliance(const std::shared_ptr<ProgramNode>& program);

    /**
     * 상세 보고서 생성
     */
    void printDetailedReport() const;

private:
    std::unordered_map<std::string, WCETInfo> wcet_map;
    std::unordered_map<std::string, MemorySafetyInfo> memory_map;
    std::vector<std::string> critical_paths;

    // Real-Time 검증 정보
    bool is_real_time_safe = true;
    std::vector<std::string> violations;  // 위반 사항 목록

    // 분석 헬퍼
    unsigned long analyzeNode(const std::shared_ptr<ASTNode>& node, int depth);
    unsigned long analyzeFunction(const std::shared_ptr<FunctionNode>& func);

    // 명령어별 사이클 수 추정
    unsigned long getInstructionCycles(const std::string& instr_type);

    // 루프 경계 추정
    unsigned long estimateLoopBound(const std::shared_ptr<ASTNode>& condition);

    // 메모리 접근 분석
    void analyzeMemoryAccess(const std::shared_ptr<ASTNode>& node,
                            MemorySafetyInfo& info);
};

} // namespace zlang

#endif // WCET_ANALYZER_H
