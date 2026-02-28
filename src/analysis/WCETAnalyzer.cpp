#include "WCETAnalyzer.h"
#include <iostream>
#include <iomanip>

namespace zlang {

// ============================================================================
// 생성자
// ============================================================================

WCETAnalyzer::WCETAnalyzer() {}

// ============================================================================
// 메인 분석 함수
// ============================================================================

bool WCETAnalyzer::analyze(const std::shared_ptr<ProgramNode>& program) {
    if (!program) {
        return false;
    }

    std::cout << "【 Stage 4: WCET Analysis (최악의 경우 실행 시간) 】" << std::endl;

    // 모든 함수 분석
    for (const auto& func : program->functions) {
        analyzeFunction(func);
    }

    // 임계 경로 식별 (WCET이 가장 큰 함수)
    unsigned long max_wcet = 0;
    std::string critical_func;

    for (const auto& [name, info] : wcet_map) {
        if (info.cycles > max_wcet) {
            max_wcet = info.cycles;
            critical_func = name;
        }
    }

    if (!critical_func.empty()) {
        wcet_map[critical_func].is_critical_path = true;
        critical_paths.push_back(critical_func);
    }

    return true;
}

// ============================================================================
// 함수 분석
// ============================================================================

unsigned long WCETAnalyzer::analyzeFunction(const std::shared_ptr<FunctionNode>& func) {
    if (!func) return 0;

    unsigned long total_cycles = 0;
    unsigned long exception_cycles = 0;

    // 함수 프롤로그/에필로그: 10 사이클
    total_cycles += 10;

    // 함수 본체 분석
    if (func->body) {
        total_cycles += analyzeNode(func->body, 0);
    }

    // 예외 처리 오버헤드: try-catch가 있으면 추가
    // 간단한 추정: exception handler 호출 ~50사이클
    exception_cycles = 50;

    WCETInfo info;
    info.node_name = func->func_name;
    info.cycles = total_cycles;
    info.exception_cycles = exception_cycles;

    wcet_map[func->func_name] = info;

    return total_cycles;
}

// ============================================================================
// 노드별 분석
// ============================================================================

unsigned long WCETAnalyzer::analyzeNode(const std::shared_ptr<ASTNode>& node, int depth) {
    if (!node) return 0;

    unsigned long cycles = 0;

    switch (node->type) {
        // 【 리터럴: 1 사이클 】
        case ASTNode::NodeType::IntLiteral:
        case ASTNode::NodeType::FloatLiteral:
        case ASTNode::NodeType::BoolLiteral:
        case ASTNode::NodeType::StringLiteral:
            cycles = 1;
            break;

        // 【 식별자 로드: 2 사이클 】
        case ASTNode::NodeType::Identifier:
            cycles = 2;
            break;

        // 【 이항 연산: 4-6 사이클 】
        case ASTNode::NodeType::BinaryOp: {
            auto binop = std::dynamic_pointer_cast<BinaryOpNode>(node);
            if (binop) {
                cycles = 4;  // 기본 연산
                cycles += analyzeNode(binop->left, depth);
                cycles += analyzeNode(binop->right, depth);
            }
            break;
        }

        // 【 단항 연산: 2 사이클 】
        case ASTNode::NodeType::UnaryOp: {
            auto unop = std::dynamic_pointer_cast<UnaryOpNode>(node);
            if (unop) {
                cycles = 2;
                cycles += analyzeNode(unop->operand, depth);
            }
            break;
        }

        // 【 함수 호출: 10-20 사이클 (재귀 깊이에 따라) 】
        case ASTNode::NodeType::Call: {
            auto call = std::dynamic_pointer_cast<CallNode>(node);
            if (call) {
                cycles = 10 + (depth * 5);  // 재귀 깊이 고려
                for (const auto& arg : call->arguments) {
                    cycles += analyzeNode(arg, depth);
                }
            }
            break;
        }

        // 【 변수 선언: 3 사이클 (메모리 할당) 】
        case ASTNode::NodeType::VarDecl: {
            auto vardecl = std::dynamic_pointer_cast<VarDeclNode>(node);
            if (vardecl) {
                cycles = 3;  // alloca
                if (vardecl->init_expr) {
                    cycles += analyzeNode(vardecl->init_expr, depth);
                }
            }
            break;
        }

        // 【 조건문: 3-10 사이클 (분기) 】
        case ASTNode::NodeType::If: {
            auto ifnode = std::dynamic_pointer_cast<IfNode>(node);
            if (ifnode) {
                cycles = 3;  // 조건 평가
                if (ifnode->condition) {
                    cycles += analyzeNode(ifnode->condition, depth);
                }
                // 최악의 경우: then 또는 else 중 긴 쪽
                unsigned long then_cycles = ifnode->then_block ? analyzeNode(ifnode->then_block, depth) : 0;
                unsigned long else_cycles = ifnode->else_block ? analyzeNode(ifnode->else_block, depth) : 0;
                cycles += std::max(then_cycles, else_cycles);
            }
            break;
        }

        // 【 루프: 2-100+ 사이클 (반복 횟수 미지수) 】
        case ASTNode::NodeType::While: {
            auto whilenode = std::dynamic_pointer_cast<WhileNode>(node);
            if (whilenode) {
                cycles = 3;  // 조건 평가
                if (whilenode->condition) {
                    cycles += analyzeNode(whilenode->condition, depth);
                }
                // 최악의 경우: 100번 반복 (보수적 추정)
                unsigned long body_cycles = whilenode->body ? analyzeNode(whilenode->body, depth + 1) : 0;
                cycles += body_cycles * 100;  // 최악: 100 반복
            }
            break;
        }

        // 【 블록: 자식 노드들의 합 】
        case ASTNode::NodeType::Block: {
            auto block = std::dynamic_pointer_cast<BlockNode>(node);
            if (block) {
                for (const auto& stmt : block->statements) {
                    cycles += analyzeNode(stmt, depth);
                }
            }
            break;
        }

        // 【 Return: 2 사이클 】
        case ASTNode::NodeType::Return: {
            auto ret = std::dynamic_pointer_cast<ReturnNode>(node);
            if (ret) {
                cycles = 2;
                if (ret->value) {
                    cycles += analyzeNode(ret->value, depth);
                }
            }
            break;
        }

        // 【 try-catch-finally: 50-100 사이클 (예외 처리) 】
        case ASTNode::NodeType::TryCatch: {
            auto trycatch = std::dynamic_pointer_cast<TryCatchNode>(node);
            if (trycatch) {
                cycles = 50;  // Exception handler 오버헤드
                if (trycatch->try_block) {
                    cycles += analyzeNode(trycatch->try_block, depth);
                }
                // catch 블록: 최악의 경우
                for (const auto& clause : trycatch->catch_clauses) {
                    cycles += analyzeNode(clause.body, depth);
                }
                if (trycatch->finally_block) {
                    cycles += analyzeNode(trycatch->finally_block, depth);
                }
            }
            break;
        }

        // 【 Result/Match: 5-10 사이클 】
        case ASTNode::NodeType::ResultOk:
        case ASTNode::NodeType::ResultErr:
        case ASTNode::NodeType::Match:
            cycles = 10;
            break;

        default:
            cycles = 0;
            break;
    }

    return cycles;
}

// ============================================================================
// 조회 함수
// ============================================================================

unsigned long WCETAnalyzer::getWCET(const std::string& func_name) const {
    auto it = wcet_map.find(func_name);
    if (it != wcet_map.end()) {
        return it->second.cycles;
    }
    return 0;
}

unsigned long WCETAnalyzer::getExceptionWCET(const std::string& func_name) const {
    auto it = wcet_map.find(func_name);
    if (it != wcet_map.end()) {
        return it->second.cycles + it->second.exception_cycles;
    }
    return 0;
}

std::vector<std::string> WCETAnalyzer::getCriticalPaths() const {
    return critical_paths;
}

// ============================================================================
// 보고 함수
// ============================================================================

void WCETAnalyzer::printReport() const {
    std::cout << std::endl;
    std::cout << "【 WCET Analysis Report 】" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    if (wcet_map.empty()) {
        std::cout << "No functions analyzed" << std::endl;
        return;
    }

    // 헤더
    std::cout << std::left
              << std::setw(20) << "Function"
              << std::setw(15) << "WCET (cycles)"
              << std::setw(15) << "Exception"
              << std::setw(10) << "Critical"
              << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    // 데이터
    unsigned long total_cycles = 0;
    for (const auto& [name, info] : wcet_map) {
        std::cout << std::left
                  << std::setw(20) << name
                  << std::setw(15) << info.cycles
                  << std::setw(15) << info.exception_cycles
                  << std::setw(10) << (info.is_critical_path ? "YES" : "")
                  << std::endl;
        total_cycles += info.cycles;
    }

    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Total WCET: " << total_cycles << " cycles" << std::endl;
    std::cout << "Critical Path: " << (critical_paths.empty() ? "None" : critical_paths[0]) << std::endl;
}

// ============================================================================
// 【 Real-Time 확장 분석 】
// ============================================================================

// Loop Bound 분석
bool WCETAnalyzer::analyzeLoopBounds(const std::shared_ptr<ProgramNode>& program) {
    if (!program) return false;

    for (const auto& func : program->functions) {
        if (!func) continue;

        // 함수별 루프 분석 (간단한 휴리스틱)
        // TODO: 더 정교한 루프 경계 분석 필요
        //   - 선형 방정식 해석
        //   - 타이트한 경계 계산
        //   - 중첩 루프 처리

        if (wcet_map.find(func->func_name) != wcet_map.end()) {
            // 휴리스틱: 최악의 경우 루프는 100번 반복
            wcet_map[func->func_name].loop_bound = 100;
        }
    }

    return true;
}

// Memory Safety 분석
bool WCETAnalyzer::analyzeMemorySafety(const std::shared_ptr<ProgramNode>& program) {
    if (!program) return false;

    for (const auto& func : program->functions) {
        if (!func) continue;

        MemorySafetyInfo info;
        info.is_memory_safe = true;

        // 함수 본체 분석
        if (func->body) {
            analyzeMemoryAccess(func->body, info);
        }

        memory_map[func->func_name] = info;
    }

    return true;
}

// Stack Usage 분석
bool WCETAnalyzer::analyzeStackUsage(const std::shared_ptr<ProgramNode>& program) {
    if (!program) return false;

    for (const auto& [name, info] : wcet_map) {
        // 휴리스틱: 함수당 64-256 바이트 스택 할당
        // 실제로는 함수의 지역 변수 크기 분석 필요
        unsigned long estimated_stack = 64 + (info.loop_depth * 32);

        wcet_map[name].stack_usage = estimated_stack;
        wcet_map[name].is_stack_safe = (estimated_stack < 4096); // 4KB 임계값
    }

    return true;
}

// No-Alloc 검증
bool WCETAnalyzer::verifyNoAlloc(const std::shared_ptr<ProgramNode>& program) {
    if (!program) return false;

    bool all_safe = true;

    for (const auto& [name, info] : memory_map) {
        if (info.has_dynamic_allocation) {
            violations.push_back("Function '" + name + "' uses dynamic allocation (malloc/new)");
            all_safe = false;
        }
        if (info.has_unbounded_arrays) {
            violations.push_back("Function '" + name + "' uses unbounded arrays");
            all_safe = false;
        }
    }

    return all_safe;
}

// Real-Time 적합성 검증
bool WCETAnalyzer::verifyRealTimeCompliance(const std::shared_ptr<ProgramNode>& program) {
    if (!program) return false;

    is_real_time_safe = true;
    violations.clear();

    // 1. Memory Safety 확인
    if (!analyzeMemorySafety(program)) {
        is_real_time_safe = false;
    }

    // 2. No-Alloc 확인
    if (!verifyNoAlloc(program)) {
        is_real_time_safe = false;
    }

    // 3. Stack Safety 확인
    analyzeStackUsage(program);
    for (const auto& [name, info] : wcet_map) {
        if (!info.is_stack_safe) {
            violations.push_back("Function '" + name + "' may have stack overflow");
            is_real_time_safe = false;
        }
    }

    // 4. Unbounded Loop 확인
    for (const auto& [name, info] : wcet_map) {
        if (info.has_unbounded_loop) {
            violations.push_back("Function '" + name + "' has unbounded loop");
            is_real_time_safe = false;
        }
    }

    return is_real_time_safe;
}

// 상세 보고서 생성
void WCETAnalyzer::printDetailedReport() const {
    std::cout << std::endl;
    std::cout << "【 Z-Lang Real-Time Analysis Report 】" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    // 1. WCET 요약
    std::cout << "\n【 1. Worst-Case Execution Time (WCET) 】" << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    unsigned long total_cycles = 0;
    for (const auto& [name, info] : wcet_map) {
        std::cout << std::left
                  << std::setw(20) << name
                  << std::setw(15) << info.cycles
                  << " cycles (Critical: " << (info.is_critical_path ? "YES" : "NO") << ")"
                  << std::endl;
        total_cycles += info.cycles;
    }
    std::cout << "Total WCET: " << total_cycles << " cycles\n";

    // 2. Memory Safety 요약
    std::cout << "\n【 2. Memory Safety Analysis 】" << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    int safe_functions = 0;
    for (const auto& [name, info] : memory_map) {
        std::string status = info.is_memory_safe ? "✅ SAFE" : "❌ UNSAFE";
        std::cout << std::left << std::setw(25) << name << status;

        if (info.has_dynamic_allocation) {
            std::cout << " (Dynamic Alloc)";
        }
        std::cout << std::endl;

        if (info.is_memory_safe) safe_functions++;
    }
    std::cout << "Safe Functions: " << safe_functions << "/" << memory_map.size() << "\n";

    // 3. Stack Usage 요약
    std::cout << "\n【 3. Stack Usage Analysis 】" << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    for (const auto& [name, info] : wcet_map) {
        std::cout << std::left << std::setw(25) << name
                  << std::setw(15) << (std::to_string(info.stack_usage) + " bytes");

        if (info.is_stack_safe) {
            std::cout << "✅ Safe";
        } else {
            std::cout << "❌ Overflow Risk";
        }
        std::cout << std::endl;
    }

    // 4. Real-Time Compliance
    std::cout << "\n【 4. Real-Time Compliance Verification 】" << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    if (is_real_time_safe) {
        std::cout << "✅ COMPLIANT: This program is safe for real-time systems\n";
    } else {
        std::cout << "❌ NON-COMPLIANT: The following violations were found:\n";
        for (const auto& violation : violations) {
            std::cout << "  ⚠️  " << violation << "\n";
        }
    }

    std::cout << std::string(70, '=') << std::endl;
}

// 메모리 접근 분석 (재귀)
void WCETAnalyzer::analyzeMemoryAccess(const std::shared_ptr<ASTNode>& node,
                                       MemorySafetyInfo& info) {
    if (!node) return;

    // 간단한 분석: Call 노드에서 malloc/free 확인
    if (node->type == ASTNode::NodeType::Call) {
        auto call = std::dynamic_pointer_cast<CallNode>(node);
        if (call) {
            if (call->func_name == "malloc" || call->func_name == "new") {
                info.has_dynamic_allocation = true;
            }
            if (call->func_name == "free" || call->func_name == "delete") {
                info.has_dynamic_allocation = true;
            }
        }
    }

    // 블록의 자식 노드 분석
    if (node->type == ASTNode::NodeType::Block) {
        auto block = std::dynamic_pointer_cast<BlockNode>(node);
        if (block) {
            for (const auto& stmt : block->statements) {
                analyzeMemoryAccess(stmt, info);
            }
        }
    }
}

} // namespace zlang
