# 🏛️ LLVM 전설: 2.4 코드 생성(Code Generation) - AST에서 기계의 언어로

> **최종 업데이트**: 2026-02-26
> **상태**: ✅ 완성 (논리에서 물리로, 추상에서 실제로!)
> **난이도**: ⭐⭐⭐⭐⭐⭐⭐ (최고 정점)
> **핵심**: 타입 정보가 기록된 Annotated AST → LLVM IR 명령어

---

## 🎯 2.4 단계: 논리의 세계에서 물리의 세계로

### 위대한 변환의 순간

```
【 컴파일러 여정의 절정 】

추상의 세계:
  2.1 렉싱      → 단어
  2.2 파싱      → 문장
  2.3 의미분석  → 논리

물리의 세계 (여기!):
  2.4 코드생성  → 기계어 ← 실제로 CPU가 이해하는 명령!

흐름:
  Annotated AST (모든 타입 정보 포함)
         ↓
  【 2.4 CodeGenerator 】
         ↓
  LLVM IR Module (llvm::Module)
         ↓
  기계어 (x86-64, ARM, RISC-V)
         ↓
  CPU 실행 → 결과!
```

---

## 📚 1. 전역 문맥 설정(Global Context Initialization)

### 1.1 LLVM 기본 객체들

```cpp
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Constants.h>

// 【 코드 생성 엔진의 중추 】
class CodeGenerator {
private:
    // 1. 전역 문맥: LLVM의 모든 타입, 상수, 메타데이터의 보관소
    llvm::LLVMContext context;

    // 2. 모듈: Z-Lang 소스 파일 하나에 대응
    //    (함수들, 전역 변수들, 메타데이터를 담는 컨테이너)
    std::unique_ptr<llvm::Module> module;

    // 3. IR 빌더: 명령어를 생성하고 삽입하는 '커서'
    //    (현재 위치, 현재 함수, 현재 블록을 추적)
    llvm::IRBuilder<> builder;

    // 4. 심볼 테이블: 변수의 LLVM 값 저장
    std::map<std::string, llvm::Value*> named_values;

public:
    CodeGenerator(const std::string& module_name)
        : builder(context),
          module(std::make_unique<llvm::Module>(module_name, context)) {}

    // 【 메인 진입점 】
    std::unique_ptr<llvm::Module> generateCode(ASTNode* ast) {
        // AST를 순회하며 LLVM IR 생성
        visitNode(ast);
        return std::move(module);
    }
};
```

### 1.2 Z-Lang 특화: 실시간 설정

```cpp
void CodeGenerator::initializeForRealTime() {
    // 【 실시간 시스템을 위한 초기화 】

    // 1. 타겟 트리플 설정 (하드웨어 지정)
    module->setTargetTriple("aarch64-none-eabi");  // ARM Cortex-A
    // 또는
    module->setTargetTriple("riscv64-unknown-elf"); // RISC-V bare-metal

    // 2. 데이터 레이아웃 설정 (메모리 정렬)
    // Z-Lang은 메모리 정렬이 명시적이어야 함 (캐시 친화적)
    module->setDataLayout(
        "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128"
    );

    // 3. 함수 어트리뷰트 설정
    // - nounwind: 예외 없음 (WCET 분석 필요)
    // - "no-frame-pointer-elim": 디버깅 정보 유지
    for (auto& func : module->getFunctionList()) {
        func.addFnAttr(llvm::Attribute::NoUnwind);
        func.addFnAttr("no-frame-pointer-elim");
    }
}
```

---

## 🔧 2. 함수 및 블록 생성(Function & Block Generation)

### 2.1 함수 정의 처리

Z-Lang 코드:
```z-lang
fn fibonacci(n: i64): i64 {
    if (n <= 1) {
        return 1;
    } else {
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}
```

코드 생성:
```cpp
llvm::Value* CodeGenerator::visitFunction(ASTNode* func_node) {
    // 1. 함수 시그니처 구성
    std::vector<llvm::Type*> param_types;
    for (const auto& param : func_node->params) {
        param_types.push_back(getLLVMType(param.type));
    }

    // 함수 타입 생성
    llvm::Type* return_type = getLLVMType(func_node->return_type);
    llvm::FunctionType* func_type = llvm::FunctionType::get(
        return_type,          // 반환 타입
        param_types,          // 파라미터 타입들
        false                 // variadic 아님
    );

    // 2. 함수 생성 및 module에 추가
    llvm::Function* llvm_func = llvm::Function::Create(
        func_type,
        llvm::Function::ExternalLinkage,  // 외부 호출 가능
        func_node->name,
        module.get()
    );

    // 3. 실시간 제약 설정 (함수별)
    if (hasAnnotation(func_node, "no_alloc")) {
        llvm_func->addFnAttr("no-alloc");
    }
    if (hasAnnotation(func_node, "wcet_bound")) {
        std::string wcet_str = getAnnotationValue(func_node, "wcet_bound");
        llvm_func->addFnAttr("wcet_bound", wcet_str);
    }

    // 4. 함수 바디를 위한 entry 블록 생성
    llvm::BasicBlock* entry_block = llvm::BasicBlock::Create(
        context,
        "entry",
        llvm_func
    );

    // 빌더를 entry 블록의 끝에 배치
    builder.SetInsertPoint(entry_block);

    // 5. 파라미터를 심볼 테이블에 등록
    named_values.clear();  // 함수마다 새로운 네임스페이스
    unsigned arg_idx = 0;
    for (auto& arg : llvm_func->args()) {
        const auto& param = func_node->params[arg_idx];
        arg.setName(param.name);

        // 파라미터는 이미 레지스터에 있으므로 직접 저장
        named_values[param.name] = &arg;
        arg_idx++;
    }

    // 6. 함수 바디 생성
    llvm::Value* body_result = visitNode(func_node->body);

    // 7. 마지막 블록에 return이 없으면 추가 (에러 방지)
    if (!llvm::isa<llvm::ReturnInst>(
        builder.GetInsertBlock()->back())) {
        builder.CreateRet(llvm::Constant::getNullValue(return_type));
    }

    return llvm_func;
}

llvm::Type* CodeGenerator::getLLVMType(const std::string& type_str) {
    if (type_str == "i32") return llvm::Type::getInt32Ty(context);
    if (type_str == "i64") return llvm::Type::getInt64Ty(context);
    if (type_str == "f32") return llvm::Type::getFloatTy(context);
    if (type_str == "f64") return llvm::Type::getDoubleTy(context);
    if (type_str == "bool") return llvm::Type::getInt1Ty(context);
    if (type_str == "string") {
        // string은 i8* (char 포인터)로 표현
        return llvm::Type::getInt8PtrTy(context);
    }
    return nullptr;  // 알 수 없는 타입
}
```

### 2.2 베이직 블록(Basic Block) 관리

```cpp
llvm::BasicBlock* CodeGenerator::createBasicBlock(
    llvm::Function* func,
    const std::string& name) {
    return llvm::BasicBlock::Create(context, name, func);
}

void CodeGenerator::setInsertPoint(llvm::BasicBlock* block) {
    builder.SetInsertPoint(block);
}

llvm::BasicBlock* CodeGenerator::getCurrentBlock() {
    return builder.GetInsertBlock();
}
```

---

## 💾 3. 변수 할당과 스택 관리(Variable Allocation & Stack Management)

### 3.1 변수 선언 처리

Z-Lang 코드:
```z-lang
let x: i64 = 10;
var y = 20;
```

코드 생성:
```cpp
llvm::Value* CodeGenerator::visitVarDecl(ASTNode* var_node) {
    // 1. LLVM 타입 결정
    llvm::Type* var_type = getLLVMType(var_node->declared_type);

    // 2. 스택에 메모리 할당 (alloca)
    llvm::AllocaInst* alloca = builder.CreateAlloca(
        var_type,
        nullptr,  // 배열 크기 (nullptr = 1개)
        var_node->name
    );

    // 3. 초기값이 있으면 저장 (store)
    if (var_node->init_expr) {
        llvm::Value* init_val = visitNode(var_node->init_expr);
        builder.CreateStore(init_val, alloca);
    }

    // 4. 심볼 테이블에 등록
    //    (변수 사용 시 이 주소를 load)
    named_values[var_node->name] = alloca;

    return alloca;
}
```

**생성되는 LLVM IR:**
```llvm
entry:
  %x = alloca i64                    ; 8바이트 메모리 할당
  store i64 10, i64* %x              ; 초기값 저장
  %y = alloca i64                    ; 또 다른 8바이트
  store i64 20, i64* %y              ; 초기값 저장
```

### 3.2 Z-Lang 특화: 소유권 기반 메모리 해제

```cpp
void CodeGenerator::handleOwnershipMove(ASTNode* var_use) {
    // 【 소유권 이동(Move)을 IR에 반영 】

    SymbolInfo* var_info = symbol_table.lookup(var_use->name);

    if (var_info->ownership == OwnershipState::Moved) {
        // 이미 Moved된 변수를 사용하려 하면?
        // → 2.3 의미분석에서 이미 에러 발생!
        // → 여기 도달하지 않음 (안전!)
        return;
    }

    // Owned 자원이면, 사용 후 drop 로직 필요
    if (var_info->ownership == OwnershipState::Available &&
        isOwnedType(var_info->type)) {
        // 나중에 scope 벗어날 때:
        //   1. 변수 메모리 로드
        //   2. destructor 호출 (있으면)
        //   3. 메모리 해제 또는 정리
        // 이를 위해 drop instruction 마킹
        markForDrop(var_use->name);
    }
}

void CodeGenerator::handleScopeExit() {
    // 【 블록을 벗어날 때 자동 정리 】

    // 1. 이 스코프에서 정의된 변수들을 역순으로 drop
    for (auto it = marked_for_drop.rbegin();
         it != marked_for_drop.rend(); ++it) {
        std::string var_name = *it;
        llvm::Value* var_ptr = named_values[var_name];

        // 2. destructor 호출 (있으면)
        // 예: Box::new()로 할당된 메모리는 여기서 해제

        // 3. 심볼 테이블에서 제거
        named_values.erase(var_name);
    }

    marked_for_drop.clear();
}
```

---

## ⚡ 4. 이항 연산 생성(Binary Operation Generation)

### 4.1 산술 연산

Z-Lang 코드:
```z-lang
var result = x + y * z;
```

코드 생성:
```cpp
llvm::Value* CodeGenerator::visitBinaryOp(ASTNode* binop) {
    // 1. 좌우 피연산자 코드 생성
    llvm::Value* lhs = visitNode(binop->left);
    llvm::Value* rhs = visitNode(binop->right);

    if (!lhs || !rhs) return nullptr;

    // 2. 연산자별로 적절한 LLVM 명령어 선택
    llvm::Type* result_type = lhs->getType();

    // 정수 연산 vs 부동소수 연산을 구분해야 함!
    bool is_integer = result_type->isIntegerTy();
    bool is_float = result_type->isFloatingPointTy();

    switch (binop->op) {
        case BinaryOp::Add:
            if (is_integer) {
                return builder.CreateAdd(lhs, rhs, "addtmp");
            } else if (is_float) {
                return builder.CreateFAdd(lhs, rhs, "faddtmp");
            }
            break;

        case BinaryOp::Sub:
            if (is_integer) {
                return builder.CreateSub(lhs, rhs, "subtmp");
            } else if (is_float) {
                return builder.CreateFSub(lhs, rhs, "fsubtmp");
            }
            break;

        case BinaryOp::Mul:
            if (is_integer) {
                return builder.CreateMul(lhs, rhs, "multmp");
            } else if (is_float) {
                return builder.CreateFMul(lhs, rhs, "fmultmp");
            }
            break;

        case BinaryOp::Div:
            if (is_integer) {
                // 부호 있는 정수 나눗셈
                return builder.CreateSDiv(lhs, rhs, "divtmp");
            } else if (is_float) {
                return builder.CreateFDiv(lhs, rhs, "fdivtmp");
            }
            break;

        default:
            return nullptr;
    }
}
```

**생성되는 LLVM IR:**
```llvm
%0 = load i64, i64* %x
%1 = load i64, i64* %z
%2 = mul i64 %1, %z          ; y * z
%3 = add i64 %0, %2          ; x + (y * z)
```

---

## 🎯 5. 함수 호출 생성(Function Call Generation)

```cpp
llvm::Value* CodeGenerator::visitCall(ASTNode* call_node) {
    // 1. 호출할 함수 찾기
    llvm::Function* func = module->getFunction(call_node->callee);

    if (!func) {
        // 내부 함수 또는 외부 라이브러리 함수 검색
        func = getBuiltinFunction(call_node->callee);
    }

    if (!func) {
        return nullptr;  // 함수 없음 (2.3에서 이미 검증됨)
    }

    // 2. 인자들의 LLVM 값 생성
    std::vector<llvm::Value*> arg_values;
    for (ASTNode* arg : call_node->args) {
        llvm::Value* arg_val = visitNode(arg);
        arg_values.push_back(arg_val);
    }

    // 3. 함수 호출 명령어 생성
    return builder.CreateCall(func, arg_values, "calltmp");
}
```

**생성되는 LLVM IR:**
```llvm
%0 = load i64, i64* %x
%1 = load i64, i64* %y
%2 = call i64 @fibonacci(i64 %0)
%3 = call i64 @add(i64 %2, i64 %1)
```

---

## 🚀 6. Z-Lang 특화: 실시간 최적화(Real-Time Optimizations)

### 6.1 Loop Unrolling (루프 펼치기)

Z-Lang 코드:
```z-lang
#[unroll(4)]
fn process() {
    for i in 0..100 {
        buffer[i] = data[i] * 2;
    }
}
```

코드 생성:
```cpp
// 【 고정 크기 루프의 자동 펼침 】

void CodeGenerator::applyLoopOptimizations(ASTNode* loop) {
    if (!hasAnnotation(loop, "unroll")) return;

    int unroll_factor = getAnnotationValue(loop, "unroll");

    // 루프를 unroll_factor배 펼침
    // 결과: 분기 예측 실패(Branch Misprediction) 감소
    //      → 더 예측 가능한 성능!

    // LLVM에는 LoopUnrollPass가 있음
    llvm::Loop* loop_info = getLoopInfo(loop);
    loop_info->setUnrollFactor(unroll_factor);
}
```

**생성되는 IR 의사코드:**
```llvm
; 펼치기 전:
loop:
  %i = load ...
  %i_next = add %i, 1
  br %i_next < 100, loop, exit

; 펼친 후:
loop:
  ; 반복 1
  %i = load ...
  buffer[i] = ...
  ; 반복 2
  %i_next = add %i, 1
  buffer[i_next] = ...
  ; 반복 3
  %i_next2 = add %i, 2
  buffer[i_next2] = ...
  ; 반복 4
  %i_next3 = add %i, 3
  buffer[i_next3] = ...
  ; 조건
  %i_next4 = add %i, 4
  br %i_next4 < 100, loop, exit
```

### 6.2 Inline Assembly (인라인 어셈블리)

Z-Lang 코드:
```z-lang
fn read_timer(): i64 {
    #[asm = "rdtsc"]  // Read Time Stamp Counter
    return 0;
}
```

코드 생성:
```cpp
llvm::Value* CodeGenerator::visitInlineAsm(ASTNode* asm_node) {
    std::string asm_code = asm_node->asm_template;
    std::string constraints = asm_node->constraints;

    // LLVM의 InlineAsm 생성
    llvm::InlineAsm* inline_asm = llvm::InlineAsm::get(
        func_type,      // 반환 타입, 파라미터 타입
        asm_code,       // 어셈블리 코드 문자열
        constraints,    // 제약 ("=r" = output register)
        true            // side effects
    );

    return builder.CreateCall(inline_asm);
}
```

---

## 📊 7. 전체 코드 생성 흐름

### 프로그램 생성의 완전한 흐름

```cpp
std::unique_ptr<llvm::Module> CodeGenerator::generateCode(ASTNode* program) {
    // 1단계: 모듈 초기화
    initializeForRealTime();

    // 2단계: 전역 함수 선언 (forward declaration)
    // → 재귀 함수를 지원하기 위함
    for (ASTNode* func_node : program->functions) {
        declareFunctionSignature(func_node);
    }

    // 3단계: 함수 바디 생성
    for (ASTNode* func_node : program->functions) {
        visitFunction(func_node);
    }

    // 4단계: 검증
    std::string error_str;
    llvm::raw_string_ostream error_stream(error_str);

    if (llvm::verifyModule(*module, &error_stream)) {
        // IR이 유효하지 않음
        std::cerr << "LLVM verification failed:\n" << error_str;
        return nullptr;
    }

    return std::move(module);
}
```

---

## 💪 8. 코드 생성 결과물

### "zlang --emit-llvm" 실행 결과

```bash
$ cat fibonacci.z
fn fibonacci(n: i64): i64 {
    if (n <= 1) {
        return 1;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

$ zlang --emit-llvm fibonacci.z
$ cat fibonacci.ll

; ModuleID = 'fibonacci.z'
target triple = "aarch64-none-eabi"
target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128"

define i64 @fibonacci(i64 %n) #0 {
entry:
  %0 = icmp sle i64 %n, 1
  br i1 %0, label %then, label %else

then:
  ret i64 1

else:
  %1 = sub i64 %n, 1
  %2 = call i64 @fibonacci(i64 %1)
  %3 = sub i64 %n, 2
  %4 = call i64 @fibonacci(i64 %3)
  %5 = add i64 %2, %4
  ret i64 %5
}

attributes #0 = { nounwind "target-cpu"="cortex-a72" }
```

---

## 🎓 요약: 2.4의 가치

```
【 코드 생성이 하는 일 】

Input:  Annotated AST
        ├─ 모든 노드의 타입
        ├─ 모든 변수의 위치
        └─ 모든 함수의 시그니처

Output: LLVM IR Module
        ├─ 함수 정의들
        ├─ 기본 블록들
        ├─ 명령어들
        └─ 메타데이터

변환:
  AST Node                 →    LLVM Instruction
  ─────────────────────────────────────────
  FunctionDef              →    llvm::Function
  VarDecl                  →    llvm::AllocaInst
  BinaryOp(+)             →    llvm::BinaryOperator::CreateAdd
  Call                     →    llvm::CallInst
  IfStmt                   →    llvm::BranchInst + PHI
  Return                   →    llvm::ReturnInst
```

---

## 🏆 2.4의 의미

**Z-Lang이 이제:**
- ✅ 추상 구문 트리를 기계가 이해하는 명령으로 변환
- ✅ LLVM 백엔드와 통합 (자동으로 기계어 생성!)
- ✅ 실제로 CPU 위에서 실행 가능한 바이너리 생성
- ✅ "기록"(zlang --emit-llvm)으로 자신의 완벽함을 증명

---

*"2.4는 컴파일러의 마지막 우아한 스텝입니다. 여기서 추상이 실제가 됩니다."* 🚀

*"Annotated AST → LLVM IR → 기계어 → 실행. 당신의 언어가 살아 숨 쉽니다!"* ✨
