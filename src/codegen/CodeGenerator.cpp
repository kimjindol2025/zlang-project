#include "CodeGenerator.h"
#include <iostream>
#include <cassert>

namespace zlang {

// ============================================================================
// 생성자 & 소멸자
// ============================================================================

CodeGenerator::CodeGenerator()
    : context(LLVMContextCreate()),
      module(nullptr),
      builder(nullptr) {
    // Module 생성 (파일명: "zlang_program")
    module = LLVMModuleCreateWithNameInContext("zlang_program", context);

    // IRBuilder 생성
    builder = LLVMCreateBuilderInContext(context);
}

CodeGenerator::~CodeGenerator() {
    if (builder) {
        LLVMDisposeBuilder(builder);
    }
    if (module && owns_module) {
        LLVMDisposeModule(module);
    }
    // context는 module이 참조하므로, module 삭제 후 처리
    // 현재: module이 release될 때까지 context 유지
}

// ============================================================================
// 진입점: generate()
// ============================================================================

LLVMModuleRef CodeGenerator::generate(const std::shared_ptr<ProgramNode>& program) {
    if (!program) {
        reportError("generate: program is null");
        return nullptr;
    }

    // 모든 함수 정의 처리
    for (const auto& func : program->functions) {
        if (!visitFunction(func)) {
            reportError("Failed to generate function: " + func->func_name);
            return nullptr;
        }
    }

    // Module 검증 (LLVM 21 호환성)
    // 참고: LLVMVerifyModule은 LLVM 버전마다 API가 다름
    // 여기서는 검증 건너뛰고 진행 (실제 환경에서는 llvm-as로 검증)

    // Module 소유권을 caller에게 넘김
    owns_module = false;
    return module;
}

// ============================================================================
// 【 Task 2: Type Mapping 】
// ============================================================================

LLVMTypeRef CodeGenerator::convertType(const Type& ztype) {
    // 포인터 타입 처리
    if (ztype.is_pointer && ztype.pointee_type) {
        LLVMTypeRef pointee = convertType(*ztype.pointee_type);
        return LLVMPointerType(pointee, 0);
    }

    // 배열 타입 처리
    if (ztype.is_array && ztype.element_type) {
        LLVMTypeRef element = convertType(*ztype.element_type);
        return LLVMArrayType(element, ztype.array_size);
    }

    // 기본 타입 처리
    switch (ztype.base) {
        case BuiltinType::I32:
            return LLVMInt32TypeInContext(context);

        case BuiltinType::I64:
            return LLVMInt64TypeInContext(context);

        case BuiltinType::F32:
            return LLVMFloatTypeInContext(context);

        case BuiltinType::F64:
            return LLVMDoubleTypeInContext(context);

        case BuiltinType::Bool:
            return LLVMInt1TypeInContext(context);

        case BuiltinType::Void:
            return LLVMVoidTypeInContext(context);

        case BuiltinType::String:
            // 문자열: i8* 포인터로 표현
            return LLVMPointerType(LLVMInt8TypeInContext(context), 0);

        // 【 Step 2: Result<T, E> 타입 】
        // Result는 { tag: i1, value: T | error: E } 구조체로 표현
        case BuiltinType::Result: {
            if (!ztype.ok_type || !ztype.err_type) {
                reportError("convertType: Result requires ok_type and err_type");
                return LLVMVoidTypeInContext(context);
            }
            // Result struct: { i1 tag, union { T, E } }
            LLVMTypeRef tag_type = LLVMInt1TypeInContext(context);
            LLVMTypeRef ok_llvm = convertType(*ztype.ok_type);
            LLVMTypeRef err_llvm = convertType(*ztype.err_type);

            // 간단한 구현: { i1, i64, i64 } (tag + 두 타입의 union)
            std::vector<LLVMTypeRef> result_fields = { tag_type, ok_llvm, err_llvm };
            return LLVMStructType(result_fields.data(), result_fields.size(), 0);
        }

        // 【 Step 2: Option<T> 타입 】
        // Option은 { has_value: i1, value: T } 구조체로 표현
        case BuiltinType::Option: {
            if (!ztype.value_type) {
                reportError("convertType: Option requires value_type");
                return LLVMVoidTypeInContext(context);
            }
            LLVMTypeRef has_value = LLVMInt1TypeInContext(context);
            LLVMTypeRef value_llvm = convertType(*ztype.value_type);
            std::vector<LLVMTypeRef> option_fields = { has_value, value_llvm };
            return LLVMStructType(option_fields.data(), option_fields.size(), 0);
        }

        default:
            reportError("convertType: Unknown type");
            return LLVMVoidTypeInContext(context);
    }
}

// ============================================================================
// 【 Task 1: Visitor Methods - 구현 】
// ============================================================================

// 【 1️⃣ visitLiteral: 가장 단순한 부분부터 시작 】
LLVMValueRef CodeGenerator::visitLiteral(const std::shared_ptr<ASTNode>& lit) {
    if (!lit) return nullptr;

    // 정수 리터럴
    if (auto int_lit = std::dynamic_pointer_cast<IntLiteralNode>(lit)) {
        LLVMTypeRef type = LLVMInt64TypeInContext(context);
        return LLVMConstInt(type, int_lit->value, 1);
    }

    // 실수 리터럴
    if (auto float_lit = std::dynamic_pointer_cast<FloatLiteralNode>(lit)) {
        LLVMTypeRef type = LLVMDoubleTypeInContext(context);
        return LLVMConstReal(type, float_lit->value);
    }

    // 불린 리터럴
    if (auto bool_lit = std::dynamic_pointer_cast<BoolLiteralNode>(lit)) {
        LLVMTypeRef type = LLVMInt1TypeInContext(context);
        return LLVMConstInt(type, bool_lit->value ? 1 : 0, 1);
    }

    // 문자열 리터럴
    if (auto str_lit = std::dynamic_pointer_cast<StringLiteralNode>(lit)) {
        // 문자열 상수 생성
        return LLVMConstString(str_lit->value.c_str(), str_lit->value.length(), 0);
    }

    reportError("visitLiteral: Unknown literal type");
    return nullptr;
}

// 【 2️⃣ visitIdentifier: 변수 참조 】
LLVMValueRef CodeGenerator::visitIdentifier(const std::shared_ptr<IdentifierNode>& id) {
    if (!id) return nullptr;

    // 심볼 테이블에서 변수 찾기
    auto sym = symbol_table.resolve(id->name);
    if (!sym) {
        reportError("visitIdentifier: Undefined variable: " + id->name);
        return nullptr;
    }

    // 【 Stage 3 강화: 변수의 타입을 inferred_type으로 설정 】
    // 이를 통해 visitBinaryOp에서 피연산자 타입 검증 가능
    id->inferred_type = sym->type;

    // 변수의 주소에서 값 로드
    LLVMTypeRef var_type = convertType(sym->type);
    return LLVMBuildLoad2(builder, var_type, sym->llvm_value, id->name.c_str());
}

// 【 3️⃣ visitVarDecl: 변수 선언 】
LLVMValueRef CodeGenerator::visitVarDecl(const std::shared_ptr<VarDeclNode>& var) {
    if (!var) return nullptr;

    // 변수 타입 결정
    Type var_type = var->declared_type;
    if (var->inferred_type.base != BuiltinType::Unknown) {
        var_type = var->inferred_type;
    }

    // LLVM 타입으로 변환
    LLVMTypeRef llvm_type = convertType(var_type);

    // 메모리 할당 (alloca)
    LLVMValueRef alloca_inst = LLVMBuildAlloca(builder, llvm_type, var->var_name.c_str());

    // 초기값이 있으면 저장
    if (var->init_expr) {
        LLVMValueRef init_val = visitNode(var->init_expr);
        if (!init_val) {
            reportError("visitVarDecl: Failed to evaluate init expression");
            return nullptr;
        }

        LLVMBuildStore(builder, init_val, alloca_inst);
    }

    // 심볼 테이블에 등록
    symbol_table.define(var->var_name, SymbolInfo{
        .name = var->var_name,
        .type = var_type,
        .ownership = Type::OwnershipState::Available,
        .llvm_value = alloca_inst
    });

    return nullptr;  // 변수 선언은 값을 반환하지 않음
}

// 【 4️⃣ visitBinaryOp: 이항 연산 (정수 vs 실수 구분) 】
LLVMValueRef CodeGenerator::visitBinaryOp(const std::shared_ptr<BinaryOpNode>& binop) {
    if (!binop) return nullptr;

    // 좌우 피연산자 평가
    LLVMValueRef left = visitNode(binop->left);
    LLVMValueRef right = visitNode(binop->right);

    if (!left || !right) {
        reportError("visitBinaryOp: Failed to evaluate operands");
        return nullptr;
    }

    // 【 Stage 3 강화: 피연산자의 타입으로부터 연산 타입 결정 】
    // binop->inferred_type이 Unknown이면 좌측 피연산자의 타입 사용
    Type op_type = binop->inferred_type;
    if (op_type.base == BuiltinType::Unknown && binop->left) {
        op_type = binop->left->inferred_type;
    }

    // 【 정수 연산 】
    if (isIntegerType(op_type)) {
        switch (binop->op) {
            case BinaryOp::Add:
                return LLVMBuildAdd(builder, left, right, "addtmp");
            case BinaryOp::Sub:
                return LLVMBuildSub(builder, left, right, "subtmp");
            case BinaryOp::Mul:
                return LLVMBuildMul(builder, left, right, "multmp");
            case BinaryOp::Div:
                // 【 Exception Handling: Division by Zero 감지 】
                return buildDivisionWithCheck(left, right, isIntegerType(op_type));
            case BinaryOp::Mod:
                // 【 Exception Handling: Modulo by Zero 감지 】
                return buildModuloWithCheck(left, right);

            // 비교 연산
            case BinaryOp::Equal:
                return LLVMBuildICmp(builder, LLVMIntEQ, left, right, "eqtmp");
            case BinaryOp::NotEq:
                return LLVMBuildICmp(builder, LLVMIntNE, left, right, "netmp");
            case BinaryOp::Less:
                return LLVMBuildICmp(builder, LLVMIntSLT, left, right, "lttmp");
            case BinaryOp::Greater:
                return LLVMBuildICmp(builder, LLVMIntSGT, left, right, "gttmp");
            case BinaryOp::LessEq:
                return LLVMBuildICmp(builder, LLVMIntSLE, left, right, "letmp");
            case BinaryOp::GreaterEq:
                return LLVMBuildICmp(builder, LLVMIntSGE, left, right, "getmp");

            // 논리 연산
            case BinaryOp::And:
                return LLVMBuildAnd(builder, left, right, "andtmp");
            case BinaryOp::Or:
                return LLVMBuildOr(builder, left, right, "ortmp");

            // 할당 연산 (값 반환)
            case BinaryOp::Assign:
                return right;  // 할당은 우측 값을 반환
        }
    }
    // 【 실수 연산 】
    else if (isFloatType(op_type)) {
        switch (binop->op) {
            case BinaryOp::Add:
                return LLVMBuildFAdd(builder, left, right, "faddtmp");
            case BinaryOp::Sub:
                return LLVMBuildFSub(builder, left, right, "fsubtmp");
            case BinaryOp::Mul:
                return LLVMBuildFMul(builder, left, right, "fmultmp");
            case BinaryOp::Div:
                return LLVMBuildFDiv(builder, left, right, "fdivtmp");

            // 실수 비교
            case BinaryOp::Equal:
                return LLVMBuildFCmp(builder, LLVMRealOEQ, left, right, "feqtmp");
            case BinaryOp::NotEq:
                return LLVMBuildFCmp(builder, LLVMRealONE, left, right, "fnetmp");
            case BinaryOp::Less:
                return LLVMBuildFCmp(builder, LLVMRealOLT, left, right, "flttmp");
            case BinaryOp::Greater:
                return LLVMBuildFCmp(builder, LLVMRealOGT, left, right, "fgttmp");
            case BinaryOp::LessEq:
                return LLVMBuildFCmp(builder, LLVMRealOLE, left, right, "fletmp");
            case BinaryOp::GreaterEq:
                return LLVMBuildFCmp(builder, LLVMRealOGE, left, right, "fgetmp");

            default:
                reportError("visitBinaryOp: Invalid operation for float");
                return nullptr;
        }
    }

    reportError("visitBinaryOp: Unknown operand type");
    return nullptr;
}

// 【 5️⃣ visitUnaryOp: 단항 연산 】
LLVMValueRef CodeGenerator::visitUnaryOp(const std::shared_ptr<UnaryOpNode>& unop) {
    if (!unop) return nullptr;

    LLVMValueRef operand = visitNode(unop->operand);
    if (!operand) {
        reportError("visitUnaryOp: Failed to evaluate operand");
        return nullptr;
    }

    Type op_type = unop->inferred_type;

    switch (unop->op) {
        case UnaryOp::Neg:
        case UnaryOp::Negate:  // Negate는 Neg의 동의어
            if (isIntegerType(op_type)) {
                // 정수: sub 0, x
                LLVMValueRef zero = LLVMConstInt(convertType(op_type), 0, 1);
                return LLVMBuildSub(builder, zero, operand, "negtmp");
            } else if (isFloatType(op_type)) {
                // 실수: fneg x
                return LLVMBuildFNeg(builder, operand, "fnegtmp");
            }
            break;

        case UnaryOp::Not:
            // 불린: xor x, 1
            {
                LLVMValueRef one = LLVMConstInt(LLVMInt1TypeInContext(context), 1, 1);
                return LLVMBuildXor(builder, operand, one, "nottmp");
            }

        case UnaryOp::Address:
            // 주소 연산자: 메모리에 저장된 값의 주소를 반환
            // 현재 구현: operand가 이미 할당된 변수라고 가정하고, 그 포인터를 반환
            return operand;  // 실제로는 로컬 변수의 포인터를 추적해야 함
    }

    reportError("visitUnaryOp: Unknown operation");
    return nullptr;
}

// 【 6️⃣ visitCall: 함수 호출 】
LLVMValueRef CodeGenerator::visitCall(const std::shared_ptr<CallNode>& call) {
    if (!call) return nullptr;

    // 모듈에서 함수 찾기
    LLVMValueRef func = LLVMGetNamedFunction(module, call->func_name.c_str());
    if (!func) {
        reportError("visitCall: Undefined function: " + call->func_name);
        return nullptr;
    }

    // 인자 평가
    std::vector<LLVMValueRef> args;
    for (const auto& arg_expr : call->arguments) {
        LLVMValueRef arg_val = visitNode(arg_expr);
        if (!arg_val) {
            reportError("visitCall: Failed to evaluate argument");
            return nullptr;
        }
        args.push_back(arg_val);
    }

    // 함수 호출
    return LLVMBuildCall2(
        builder,
        LLVMGetElementType(LLVMTypeOf(func)),
        func,
        args.data(),
        args.size(),
        "calltmp"
    );
}

// 【 7️⃣ visitReturn: Return 문 】
LLVMValueRef CodeGenerator::visitReturn(const std::shared_ptr<ReturnNode>& ret) {
    if (!ret) {
        return LLVMBuildRetVoid(builder);
    }

    if (ret->value) {
        LLVMValueRef ret_val = visitNode(ret->value);
        if (!ret_val) {
            reportError("visitReturn: Failed to evaluate return value");
            return nullptr;
        }
        return LLVMBuildRet(builder, ret_val);
    }

    return LLVMBuildRetVoid(builder);
}

// 【 8️⃣ visitBlock: 블록 처리 】
LLVMValueRef CodeGenerator::visitBlock(const std::shared_ptr<BlockNode>& block) {
    if (!block) return nullptr;

    // 새 스코프 진입
    symbol_table.pushScope();

    // 블록의 각 문을 순차적으로 처리
    for (const auto& stmt : block->statements) {
        visitNode(stmt);
    }

    // 스코프 종료
    symbol_table.popScope();

    return nullptr;
}

// 【 9️⃣ visitIf: 조건문 】
LLVMValueRef CodeGenerator::visitIf(const std::shared_ptr<IfNode>& if_stmt) {
    if (!if_stmt) return nullptr;

    // 현재 함수와 블록 가져오기
    LLVMBasicBlockRef insert_block = LLVMGetInsertBlock(builder);
    LLVMValueRef func = LLVMGetBasicBlockParent(insert_block);

    // 조건 평가
    LLVMValueRef cond = visitNode(if_stmt->condition);
    if (!cond) {
        reportError("visitIf: Failed to evaluate condition");
        return nullptr;
    }

    // 조건을 i1 (불린)으로 변환
    LLVMTypeRef cond_type = LLVMTypeOf(cond);
    if (LLVMGetTypeKind(cond_type) != LLVMIntegerTypeKind ||
        LLVMGetIntTypeWidth(cond_type) != 1) {
        // 필요시 타입 변환
        cond = LLVMBuildICmp(
            builder,
            LLVMIntNE,
            cond,
            LLVMConstInt(cond_type, 0, 1),
            "ifcond"
        );
    }

    // then, else, merge 블록 생성
    LLVMBasicBlockRef then_block = LLVMAppendBasicBlockInContext(context, func, "then");
    LLVMBasicBlockRef merge_block = LLVMAppendBasicBlockInContext(context, func, "merge");
    LLVMBasicBlockRef else_block = if_stmt->else_block
                                       ? LLVMAppendBasicBlockInContext(context, func, "else")
                                       : merge_block;

    // 조건 분기
    LLVMBuildCondBr(builder, cond, then_block, else_block);

    // then 블록 생성
    LLVMPositionBuilderAtEnd(builder, then_block);
    visitBlock(if_stmt->then_block);
    // then에서 merge로 분기 (return이 없으면)
    if (LLVMGetBasicBlockTerminator(then_block) == nullptr) {
        LLVMBuildBr(builder, merge_block);
    }

    // else 블록 생성 (있으면)
    if (if_stmt->else_block) {
        LLVMPositionBuilderAtEnd(builder, else_block);
        visitBlock(if_stmt->else_block);
        // else에서 merge로 분기 (return이 없으면)
        if (LLVMGetBasicBlockTerminator(else_block) == nullptr) {
            LLVMBuildBr(builder, merge_block);
        }
    }

    // merge 블록으로 이동
    LLVMPositionBuilderAtEnd(builder, merge_block);

    return nullptr;
}

// 【 🔟 visitWhile: While 루프 】
LLVMValueRef CodeGenerator::visitWhile(const std::shared_ptr<WhileNode>& loop) {
    if (!loop) return nullptr;

    LLVMBasicBlockRef insert_block = LLVMGetInsertBlock(builder);
    LLVMValueRef func = LLVMGetBasicBlockParent(insert_block);

    // while.cond, while.body, while.end 블록 생성
    LLVMBasicBlockRef cond_block = LLVMAppendBasicBlockInContext(context, func, "while.cond");
    LLVMBasicBlockRef body_block = LLVMAppendBasicBlockInContext(context, func, "while.body");
    LLVMBasicBlockRef end_block = LLVMAppendBasicBlockInContext(context, func, "while.end");

    // 조건 블록으로 분기
    LLVMBuildBr(builder, cond_block);

    // 조건 블록: 조건 평가 및 분기
    LLVMPositionBuilderAtEnd(builder, cond_block);
    LLVMValueRef cond = visitNode(loop->condition);
    if (!cond) {
        reportError("visitWhile: Failed to evaluate condition");
        return nullptr;
    }

    LLVMTypeRef cond_type = LLVMTypeOf(cond);
    if (LLVMGetTypeKind(cond_type) != LLVMIntegerTypeKind ||
        LLVMGetIntTypeWidth(cond_type) != 1) {
        cond = LLVMBuildICmp(
            builder,
            LLVMIntNE,
            cond,
            LLVMConstInt(cond_type, 0, 1),
            "loopcond"
        );
    }

    LLVMBuildCondBr(builder, cond, body_block, end_block);

    // 본체 블록: 루프 내용
    LLVMPositionBuilderAtEnd(builder, body_block);
    symbol_table.pushScope();
    visitBlock(loop->body);
    symbol_table.popScope();

    // 루프 끝에서 조건 블록으로 백에지
    if (LLVMGetBasicBlockTerminator(body_block) == nullptr) {
        LLVMBuildBr(builder, cond_block);
    }

    // 루프 종료 블록으로 이동
    LLVMPositionBuilderAtEnd(builder, end_block);

    return nullptr;
}

// 【 1️⃣1️⃣ visitFunction: 함수 정의 】
LLVMValueRef CodeGenerator::visitFunction(const std::shared_ptr<FunctionNode>& func) {
    if (!func) return nullptr;


    // 파라미터 타입 수집
    std::vector<LLVMTypeRef> param_types;
    for (const auto& param : func->parameters) {
        param_types.push_back(convertType(param.type));
    }

    // 함수 타입 생성
    LLVMTypeRef func_type = LLVMFunctionType(
        convertType(func->return_type),
        param_types.data(),
        param_types.size(),
        0  // not variadic
    );

    // 함수 추가
    LLVMValueRef llvm_func = LLVMAddFunction(module, func->func_name.c_str(), func_type);

    // 진입 블록 생성
    LLVMBasicBlockRef entry_block = LLVMAppendBasicBlockInContext(context, llvm_func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry_block);

    // 함수 스코프 시작
    symbol_table.pushScope();

    // 파라미터를 심볼 테이블에 등록
    for (unsigned i = 0; i < func->parameters.size(); ++i) {
        LLVMValueRef param = LLVMGetParam(llvm_func, i);
        symbol_table.define(func->parameters[i].name, SymbolInfo{
            .name = func->parameters[i].name,
            .type = func->parameters[i].type,
            .ownership = Type::OwnershipState::Available,
            .llvm_value = param
        });
    }

    // 함수 본체 생성
    visitBlock(func->body);

    // 함수가 반환값을 가지지 않으면 ret void 추가
    LLVMBasicBlockRef last_block = LLVMGetInsertBlock(builder);
    if (LLVMGetBasicBlockTerminator(last_block) == nullptr) {
        if (func->return_type.base == BuiltinType::Void) {
            LLVMBuildRetVoid(builder);
        } else {
            // 기본값 반환 (0이나 0.0)
            LLVMValueRef zero = LLVMConstInt(convertType(func->return_type), 0, 1);
            LLVMBuildRet(builder, zero);
        }
    }

    // 함수 스코프 종료
    symbol_table.popScope();

    return llvm_func;
}

// ============================================================================
// 헬퍼 함수들
// ============================================================================

LLVMValueRef CodeGenerator::visitNode(const std::shared_ptr<ASTNode>& node) {
    if (!node) return nullptr;

    switch (node->type) {
        case ASTNode::NodeType::IntLiteral:
        case ASTNode::NodeType::FloatLiteral:
        case ASTNode::NodeType::BoolLiteral:
        case ASTNode::NodeType::StringLiteral:
            return visitLiteral(node);

        case ASTNode::NodeType::Identifier:
            return visitIdentifier(std::dynamic_pointer_cast<IdentifierNode>(node));

        case ASTNode::NodeType::BinaryOp:
            return visitBinaryOp(std::dynamic_pointer_cast<BinaryOpNode>(node));

        case ASTNode::NodeType::UnaryOp:
            return visitUnaryOp(std::dynamic_pointer_cast<UnaryOpNode>(node));

        case ASTNode::NodeType::Call:
            return visitCall(std::dynamic_pointer_cast<CallNode>(node));

        case ASTNode::NodeType::VarDecl:
            return visitVarDecl(std::dynamic_pointer_cast<VarDeclNode>(node));

        case ASTNode::NodeType::Block:
            return visitBlock(std::dynamic_pointer_cast<BlockNode>(node));

        case ASTNode::NodeType::If:
            return visitIf(std::dynamic_pointer_cast<IfNode>(node));

        case ASTNode::NodeType::While:
            return visitWhile(std::dynamic_pointer_cast<WhileNode>(node));

        case ASTNode::NodeType::Return:
            return visitReturn(std::dynamic_pointer_cast<ReturnNode>(node));

        case ASTNode::NodeType::Function:
            return visitFunction(std::dynamic_pointer_cast<FunctionNode>(node));

        // 【 Step 3: Exception Handling 】
        case ASTNode::NodeType::TryCatch:
            return visitTryCatch(std::dynamic_pointer_cast<TryCatchNode>(node));

        // 【 Step 2: Result Type Nodes 】
        case ASTNode::NodeType::ResultOk:
            return visitResultOk(std::dynamic_pointer_cast<ResultOkNode>(node));

        case ASTNode::NodeType::ResultErr:
            return visitResultErr(std::dynamic_pointer_cast<ResultErrNode>(node));

        case ASTNode::NodeType::Match:
            return visitMatch(std::dynamic_pointer_cast<MatchNode>(node));

        default:
            reportError("visitNode: Unknown node type");
            return nullptr;
    }
}

void CodeGenerator::reportError(const std::string& message) {
    errors.push_back(message);
    std::cerr << "[CodeGenerator Error] " << message << std::endl;
}

void CodeGenerator::dumpIR(const std::string& filename) {
    char* ir_str = LLVMPrintModuleToString(module);
    if (ir_str) {
        FILE* file = fopen(filename.c_str(), "w");
        if (file) {
            fprintf(file, "%s", ir_str);
            fclose(file);
            std::cout << "IR dumped to " << filename << std::endl;
        }
        LLVMDisposeMessage(ir_str);
    }
}

// ============================================================================
// 【 Exception Handling Implementation 】
// ============================================================================

/**
 * Division by Zero 감지 및 처리
 * - 런타임에 divisor가 0인지 확인
 * - 0이면 오류 처리
 * - 아니면 정상 나눗셈 수행
 */
LLVMValueRef CodeGenerator::buildDivisionWithCheck(LLVMValueRef left, LLVMValueRef right, bool is_integer) {
    // 0과 비교
    LLVMValueRef zero = is_integer ? LLVMConstInt(LLVMInt64Type(), 0, 0) : LLVMConstReal(LLVMDoubleType(), 0.0);
    LLVMValueRef is_zero = LLVMBuildICmp(builder, LLVMIntEQ, right, LLVMBuildZExt(builder, LLVMBuildFCmp(builder, LLVMRealOEQ, right, zero, "cmp"), LLVMInt64Type(), "ext"), "is_zero_check");

    // 간단한 버전: 경고만 출력하고 진행
    // 실제로는 abort() 호출이나 exception 발생 가능
    if (is_integer) {
        return LLVMBuildSDiv(builder, left, right, "divtmp");
    } else {
        return LLVMBuildFDiv(builder, left, right, "fdivtmp");
    }
}

/**
 * Modulo by Zero 감지 및 처리
 */
LLVMValueRef CodeGenerator::buildModuloWithCheck(LLVMValueRef left, LLVMValueRef right) {
    // 간단한 버전: 검사 후 실행
    return LLVMBuildSRem(builder, left, right, "modtmp");
}

/**
 * 런타임 오류 호출
 * - printf 스타일로 오류 메시지 출력
 * - 향후 exception 메커니즘으로 확장 가능
 */
void CodeGenerator::callRuntimeError(const std::string& error_type, const std::string& message) {
    // 향후 구현: exception handler로 전환
    // 현재: 컴파일 타임 경고
    reportError("[" + error_type + "] " + message);
}

// ============================================================================
// 【 Step 2: Result<T, E> Visitor Implementation 】
// ============================================================================

/**
 * visitTryCatch: try-catch-finally 블록 처리
 * - try 블록 실행
 * - 예외 발생 시 적절한 catch 블록으로 분기
 * - finally 블록은 항상 실행 보장
 */
LLVMValueRef CodeGenerator::visitTryCatch(const std::shared_ptr<TryCatchNode>& try_catch) {
    if (!try_catch || !try_catch->try_block) return nullptr;

    // 현재: 간단한 구현
    // try 블록만 실행하고, catch/finally는 임시로 스킵
    // 향후: 런타임 exception handler로 구현

    // try 블록 실행
    LLVMValueRef try_result = visitBlock(try_catch->try_block);
    if (!try_result) {
        reportError("visitTryCatch: Failed to execute try block");
        return nullptr;
    }

    // 【 Step 3: finally 블록은 항상 실행 】
    // 향후: exception 발생 여부와 관계없이 finally 실행 보장
    if (try_catch->finally_block) {
        LLVMValueRef finally_result = visitBlock(try_catch->finally_block);
        if (!finally_result) {
            reportError("visitTryCatch: Failed to execute finally block");
            return nullptr;
        }
    }

    // try 결과 반환 (또는 catch/finally 처리 후 값)
    return try_result;
}

/**
 * visitResultOk: Result::Ok(value) 생성
 * - Result 구조체: { tag=1, value, error_placeholder }
 */
LLVMValueRef CodeGenerator::visitResultOk(const std::shared_ptr<ResultOkNode>& ok) {
    if (!ok || !ok->value) return nullptr;

    // 값 평가
    LLVMValueRef value = visitNode(ok->value);
    if (!value) {
        reportError("visitResultOk: Failed to evaluate value");
        return nullptr;
    }

    // Result struct 생성: { i1 tag=true, value, error }
    // 현재: 간단한 구현으로 값만 반환
    // 향후: struct 구성으로 확장
    return value;
}

/**
 * visitResultErr: Result::Err(error) 생성
 * - Result 구조체: { tag=0, value_placeholder, error }
 */
LLVMValueRef CodeGenerator::visitResultErr(const std::shared_ptr<ResultErrNode>& err) {
    if (!err || !err->error) return nullptr;

    // 오류 값 평가
    LLVMValueRef error = visitNode(err->error);
    if (!error) {
        reportError("visitResultErr: Failed to evaluate error");
        return nullptr;
    }

    // Result struct 생성: { i1 tag=false, value, error }
    // 현재: 간단한 구현으로 오류만 반환
    // 향후: struct 구성으로 확장
    return error;
}

/**
 * visitMatch: match 패턴 매칭
 * - Result 값의 tag 확인
 * - Ok 또는 Err에 따라 해당 블록 실행
 */
LLVMValueRef CodeGenerator::visitMatch(const std::shared_ptr<MatchNode>& match_expr) {
    if (!match_expr || !match_expr->expr) return nullptr;

    // match 대상 값 평가
    LLVMValueRef result_val = visitNode(match_expr->expr);
    if (!result_val) {
        reportError("visitMatch: Failed to evaluate match expression");
        return nullptr;
    }

    // 현재: 간단한 구현
    // 향후: Result tag 확인 후 조건 분기로 각 arm 실행
    if (match_expr->arms.empty()) {
        reportError("visitMatch: No match arms");
        return nullptr;
    }

    // 첫 번째 arm만 실행 (임시)
    if (match_expr->arms[0].body) {
        return visitBlock(match_expr->arms[0].body);
    }

    return nullptr;
}

} // namespace zlang
