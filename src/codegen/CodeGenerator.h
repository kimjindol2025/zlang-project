#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../ast/ASTNode.h"
#include "../semantic/SymbolTable.h"
#include "Type.h"

namespace zlang {

/**
 * CodeGenerator: Annotated AST를 LLVM IR로 변환
 *
 * 입력: 2.3 의미분석이 완료된 Annotated AST
 * 출력: LLVM Module (IR 형태)
 *
 * 각 AST 노드를 순회하며 대응하는 LLVM IR을 생성합니다.
 */
class CodeGenerator {
public:
    CodeGenerator();
    ~CodeGenerator();

    // 【 Public Interface 】

    /**
     * 진입점: AST로부터 LLVM Module 생성
     * @param program 2.3에서 타입 정보가 채워진 ProgramNode
     * @return 생성된 LLVM Module (또는 nullptr if error)
     */
    LLVMModuleRef generate(const std::shared_ptr<ProgramNode>& program);

    /**
     * 생성된 IR을 텍스트 형식으로 저장
     * @param filename 저장할 파일명 (예: "output.ll")
     */
    void dumpIR(const std::string& filename);

    /**
     * 에러 메시지 반환
     */
    const std::vector<std::string>& getErrors() const { return errors; }

private:
    // 【 LLVM Context & Module 】
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    bool owns_module = true;  // module 소유권 플래그

    // 【 Symbol Table & Environment 】
    SymbolTable symbol_table;

    // 【 Error Handling 】
    std::vector<std::string> errors;

    // ========================================================================
    // 【 Task 1: 11개 Visitor 메서드 - 당신이 구현할 부분 】
    // ========================================================================

    /**
     * 1️⃣ visitFunction: 함수 정의 처리
     *
     * Z-Lang 코드:
     *   fn add(x: i64, y: i64) -> i64 {
     *       let sum = x + y;
     *       return sum;
     *   }
     *
     * 생성될 LLVM IR:
     *   define i64 @add(i64 %x, i64 %y) {
     *   entry:
     *       ...
     *   }
     */
    LLVMValueRef visitFunction(const std::shared_ptr<FunctionNode>& func);

    /**
     * 2️⃣ visitVarDecl: 변수 선언 처리
     *
     * Z-Lang 코드:
     *   let x: i64 = 10;
     *
     * 생성될 LLVM IR:
     *   %x = alloca i64
     *   store i64 10, i64* %x
     */
    LLVMValueRef visitVarDecl(const std::shared_ptr<VarDeclNode>& var);

    /**
     * 3️⃣ visitBinaryOp: 이항 연산 처리
     *
     * Z-Lang 코드:
     *   x + y
     *   x * 2
     *   a > b
     *
     * 생성될 LLVM IR (타입에 따라):
     *   %0 = add i64 %x, %y
     *   %1 = mul i64 %x, 2
     *   %2 = icmp sgt i64 %a, %b
     */
    LLVMValueRef visitBinaryOp(const std::shared_ptr<BinaryOpNode>& binop);

    /**
     * 4️⃣ visitUnaryOp: 단항 연산 처리
     *
     * Z-Lang 코드:
     *   -x
     *   !flag
     *
     * 생성될 LLVM IR:
     *   %0 = sub i64 0, %x
     *   %1 = xor i1 %flag, 1
     */
    LLVMValueRef visitUnaryOp(const std::shared_ptr<UnaryOpNode>& unop);

    /**
     * 5️⃣ visitCall: 함수 호출 처리
     *
     * Z-Lang 코드:
     *   println(x);
     *   result = add(10, 20);
     *
     * 생성될 LLVM IR:
     *   call void @println(i64 %x)
     *   %0 = call i64 @add(i64 10, i64 20)
     */
    LLVMValueRef visitCall(const std::shared_ptr<CallNode>& call);

    /**
     * 6️⃣ visitIdentifier: 변수 참조 처리
     *
     * Z-Lang 코드:
     *   x
     *   sum
     *
     * 생성될 LLVM IR:
     *   %0 = load i64, i64* %x
     *   %1 = load i64, i64* %sum
     */
    LLVMValueRef visitIdentifier(const std::shared_ptr<IdentifierNode>& id);

    /**
     * 7️⃣ visitLiteral: 상수값 처리
     *
     * Z-Lang 코드:
     *   10, 3.14, true, "hello"
     *
     * 생성될 LLVM IR:
     *   i64 10
     *   double 3.140000e+00
     *   i1 1
     *   i8* (string pointer)
     */
    LLVMValueRef visitLiteral(const std::shared_ptr<ASTNode>& lit);

    /**
     * 8️⃣ visitReturn: Return 문 처리
     *
     * Z-Lang 코드:
     *   return x;
     *   return;
     *
     * 생성될 LLVM IR:
     *   ret i64 %x
     *   ret void
     */
    LLVMValueRef visitReturn(const std::shared_ptr<ReturnNode>& ret);

    /**
     * 9️⃣ visitIf: If 문 처리 (조건분기)
     *
     * Z-Lang 코드:
     *   if x > y {
     *       result = x;
     *   } else {
     *       result = y;
     *   }
     *
     * 생성될 LLVM IR:
     *   %cond = icmp sgt i64 %x, %y
     *   br i1 %cond, label %then, label %else
     *   then:
     *     store ...
     *     br label %merge
     *   else:
     *     store ...
     *     br label %merge
     *   merge:
     */
    LLVMValueRef visitIf(const std::shared_ptr<IfNode>& if_stmt);

    /**
     * 🔟 visitWhile: While 루프 처리
     *
     * Z-Lang 코드:
     *   while i < n {
     *       sum = sum + i;
     *       i = i + 1;
     *   }
     *
     * 생성될 LLVM IR:
     *   br label %while.cond
     *   while.cond:
     *     %cond = icmp slt i64 %i, %n
     *     br i1 %cond, label %while.body, label %while.end
     *   while.body:
     *     ... (loop body)
     *     br label %while.cond
     *   while.end:
     */
    LLVMValueRef visitWhile(const std::shared_ptr<WhileNode>& loop);

    /**
     * 1️⃣1️⃣ visitBlock: 문 블록 처리
     *
     * Z-Lang 코드:
     *   {
     *       let x = 10;
     *       let y = 20;
     *       println(x + y);
     *   }
     *
     * 처리: 순차적으로 각 문을 방문하여 IR 생성
     */
    LLVMValueRef visitBlock(const std::shared_ptr<BlockNode>& block);

    /**
     * 1️⃣2️⃣ visitTryCatch: try-catch-finally 처리
     *
     * Z-Lang 코드:
     *   try {
     *       let x = dangerous_op();
     *   } catch (e: DivisionError) {
     *       return -1;
     *   } finally {
     *       cleanup();
     *   }
     *
     * 처리: try 블록 실행 → 예외 감지 → catch 블록 분기 → finally 실행 보장
     */
    LLVMValueRef visitTryCatch(const std::shared_ptr<TryCatchNode>& try_catch);

    /**
     * 1️⃣3️⃣ visitResultOk: Result::Ok(value) 처리
     *
     * Z-Lang 코드:
     *   return Ok(42);
     *
     * 처리: Ok 변형을 struct으로 표현
     */
    LLVMValueRef visitResultOk(const std::shared_ptr<ResultOkNode>& ok);

    /**
     * 1️⃣3️⃣ visitResultErr: Result::Err(error) 처리
     *
     * Z-Lang 코드:
     *   return Err(DivisionError::DivideByZero);
     *
     * 처리: Err 변형을 struct으로 표현
     */
    LLVMValueRef visitResultErr(const std::shared_ptr<ResultErrNode>& err);

    /**
     * 1️⃣4️⃣ visitMatch: match 패턴 매칭 처리
     *
     * Z-Lang 코드:
     *   match divide(10, 0) {
     *       Ok(result) => return result,
     *       Err(e) => return -1,
     *   }
     *
     * 처리: Result 값의 변형(tag) 확인 후 해당 블록 실행
     */
    LLVMValueRef visitMatch(const std::shared_ptr<MatchNode>& match_expr);

    // ========================================================================
    // 【 Task 2: 타입 매핑 헬퍼 함수 】
    // ========================================================================

    /**
     * Z-Lang의 Type을 LLVM의 LLVMTypeRef로 변환
     * @param ztype Z-Lang 타입 정보
     * @return 대응하는 LLVM 타입
     */
    LLVMTypeRef convertType(const Type& ztype);

    /**
     * 편의 함수들
     */
    bool isIntegerType(const Type& t) const {
        return t.base == BuiltinType::I32 || t.base == BuiltinType::I64;
    }

    bool isFloatType(const Type& t) const {
        return t.base == BuiltinType::F32 || t.base == BuiltinType::F64;
    }

    // ========================================================================
    // 【 Internal Helper Methods 】
    // ========================================================================

    /**
     * 일반적인 노드 방문 (dispatch)
     */
    LLVMValueRef visitNode(const std::shared_ptr<ASTNode>& node);

    /**
     * 에러 기록
     */
    void reportError(const std::string& message);

    // ========================================================================
    // 【 Exception Handling Helpers 】- System Exception Detection
    // ========================================================================

    /**
     * Division by Zero 감지
     * - right == 0 인 경우 런타임 오류 호출
     * - 그 외의 경우 정상 나눗셈 실행
     */
    LLVMValueRef buildDivisionWithCheck(LLVMValueRef left, LLVMValueRef right, bool is_integer);

    /**
     * Modulo by Zero 감지
     * - right == 0 인 경우 런타임 오류 호출
     * - 그 외의 경우 정상 나머지 계산 실행
     */
    LLVMValueRef buildModuloWithCheck(LLVMValueRef left, LLVMValueRef right);

    /**
     * 런타임 오류 핸들러 호출
     * - 오류 메시지를 출력하고 프로그램 종료
     */
    void callRuntimeError(const std::string& error_type, const std::string& message);
};

} // namespace zlang

#endif // CODE_GENERATOR_H
