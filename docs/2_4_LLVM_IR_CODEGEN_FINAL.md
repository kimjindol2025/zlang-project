# 🚀 Preview: LLVM 2.4 - IR 코드 생성 통합 (The Final Bridge)

> **최종 업데이트**: 2026-02-26
> **상태**: 🔜 다음 단계 (소유권 정보를 IR에 담다)
> **기대도**: ⭐⭐⭐⭐⭐ (컴파일러의 완성!)

---

## 🎯 2.4 단계: 의미에서 실행으로

### 여정의 정점

```
【 전설의 완성 직전 】

2.1 렉싱          ✅ 단어를 읽는다 (눈)
2.2 파싱          ✅ 문장을 이해한다 (귀)
2.3 의미분석      ✅ 의미를 검증한다 (뇌)
2.4 IR 생성       🔜 실행을 지시한다 (입) ← 여기!

정보 흐름:
Annotated AST (모든 타입, 소유권 정보 포함)
         ↓
【 2.4: IR 코드 생성 】
         ↓
LLVM IR (기계가 이해하는 언어)
         ↓
1.5 JIT 실행 (CPU가 춤을 춘다)
         ↓
결과!
```

---

## 📚 2.4에서 배울 것

### 1. Symbol Table → LLVM IR 변환

```cpp
// 【 심볼 테이블의 정보를 LLVM IR로 변환 】

SymbolInfo {
    name: "x",
    type: i64,
    ownership: Available,
    llvm_value: %0 (alloca)
}

        ↓

LLVM IR:
  %0 = alloca i64              // 메모리 할당
  store i64 10, i64* %0        // 초기값 저장
  %1 = load i64, i64* %0       // 사용할 때 로드
```

### 2. 소유권 정보를 IR에 반영

```
【 Move vs Borrow의 IR 표현 】

Move (소유권 이전):
  let x = 10;
  let y = x;  → x 메모리 해제 (필요시)

Borrow (참조):
  let x = 10;
  let y = &x; → x의 포인터만 전달
              → x는 해제되지 않음
```

### 3. 실시간 제약을 IR에 인코딩

```
【 #[no_alloc] 검증을 IR에 반영 】

#[no_alloc]
fn safe_function() {
    let arr: [i32; 100];  // 정적 할당만
}

        ↓

LLVM IR:
  @safe_function() {
  entry:
    %0 = alloca [100 x i32]     // 스택 할당만!
    ...
  }

검증:
  - call malloc? NO ✓
  - call free? NO ✓
  - call new? NO ✓
  → 안전성 증명!
```

---

## 💻 2.4의 핵심 구현

### Codegen 구조

```cpp
class CodeGenerator {
private:
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    SymbolTable& symbol_table;
    std::vector<std::string> errors;

public:
    // 【 입력: Annotated AST 】
    // 【 출력: LLVM Module (IR) 】
    LLVMModuleRef generate(ASTNode* ast);

private:
    // 각 노드 타입별 코드 생성
    LLVMValueRef visitVarDecl(ASTNode* node);
    LLVMValueRef visitFunction(ASTNode* node);
    LLVMValueRef visitBinaryOp(ASTNode* node);
    LLVMValueRef visitCall(ASTNode* node);
    LLVMValueRef visitIdentifier(ASTNode* node);
};
```

### 구체적 예제: 함수 + 변수

```z-lang
【 Z-Lang 소스 】

fn add(x: i64, y: i64): i64 {
    var sum = x + y;
    return sum;
}
```

**2.3 의미분석 후 (Annotated AST):**
```
Function {
    name: "add",
    params: [
        {name: "x", type: i64, ownership: Available},
        {name: "y", type: i64, ownership: Available}
    ],
    body: Block {
        VarDecl {
            name: "sum",
            type: i64,
            init: BinaryOp {
                op: Add,
                left: Identifier("x", type: i64, ownership: Available),
                right: Identifier("y", type: i64, ownership: Available),
                result_type: i64
            }
        },
        Return {
            value: Identifier("sum", type: i64, ownership: Available),
            result_type: i64
        }
    }
}
```

**2.4 코드생성 후 (LLVM IR):**
```llvm
define i64 @add(i64 %x, i64 %y) {
entry:
  %sum = alloca i64
  %0 = add i64 %x, %y
  store i64 %0, i64* %sum
  %1 = load i64, i64* %sum
  ret i64 %1
}
```

### 코드 생성 흐름

```cpp
LLVMValueRef CodeGenerator::visitFunction(ASTNode* func_node) {
    // 1. 함수 타입 구성
    std::vector<LLVMTypeRef> param_types;
    for (auto& param : func_node->params) {
        param_types.push_back(getLLVMType(param.type));
    }

    LLVMTypeRef func_type = LLVMFunctionType(
        getLLVMType(func_node->return_type),  // 반환 타입
        param_types.data(),
        param_types.size(),
        0  // variadic 아님
    );

    // 2. 함수 생성
    LLVMValueRef llvm_func = LLVMAddFunction(
        module,
        func_node->name.c_str(),
        func_type
    );

    // 3. 함수 바디 생성을 위해 기본 블록 생성
    LLVMBasicBlockRef entry_block =
        LLVMAppendBasicBlock(llvm_func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry_block);

    // 4. 파라미터를 심볼 테이블에 등록
    symbol_table.pushScope();
    for (unsigned i = 0; i < func_node->params.size(); ++i) {
        LLVMValueRef param_value = LLVMGetParam(llvm_func, i);
        symbol_table.define(
            func_node->params[i].name,
            {
                .type = func_node->params[i].type,
                .llvm_value = param_value,
                .ownership = OwnershipState::Available
            }
        );
    }

    // 5. 함수 바디 생성
    visitNode(func_node->body);

    // 6. 함수 스코프 종료
    symbol_table.popScope();

    return llvm_func;
}

LLVMValueRef CodeGenerator::visitVarDecl(ASTNode* var_node) {
    // 1. LLVM 타입 결정
    LLVMTypeRef llvm_type = getLLVMType(var_node->inferred_type);

    // 2. 메모리 할당 (alloca)
    LLVMValueRef alloca_inst = LLVMBuildAlloca(
        builder,
        llvm_type,
        var_node->name.c_str()
    );

    // 3. 초기값이 있으면 저장
    if (var_node->init_expr) {
        LLVMValueRef init_val = visitNode(var_node->init_expr);
        LLVMBuildStore(builder, init_val, alloca_inst);
    }

    // 4. 심볼 테이블에 등록 (LLVM 값 포함)
    symbol_table.define(
        var_node->name,
        {
            .type = var_node->inferred_type,
            .llvm_value = alloca_inst,
            .ownership = OwnershipState::Available
        }
    );

    return nullptr;  // 변수 선언은 값을 반환하지 않음
}

LLVMValueRef CodeGenerator::visitBinaryOp(ASTNode* binop_node) {
    // 1. 좌우 피연산자 코드 생성
    LLVMValueRef left = visitNode(binop_node->left);
    LLVMValueRef right = visitNode(binop_node->right);

    // 2. 피연산자 타입 확인 (2.3에서 검증되었음)
    Type op_type = binop_node->inferred_type;

    // 3. 타입에 맞는 명령어 선택
    if (op_type.base == BuiltinType::I64 ||
        op_type.base == BuiltinType::I32) {
        // 정수 연산
        switch (binop_node->op) {
            case BinaryOp::Add:
                return LLVMBuildAdd(builder, left, right, "addtmp");
            case BinaryOp::Sub:
                return LLVMBuildSub(builder, left, right, "subtmp");
            case BinaryOp::Mul:
                return LLVMBuildMul(builder, left, right, "multmp");
            case BinaryOp::Div:
                return LLVMBuildSDiv(builder, left, right, "divtmp");
        }
    } else if (op_type.base == BuiltinType::F64 ||
               op_type.base == BuiltinType::F32) {
        // 부동소수 연산
        switch (binop_node->op) {
            case BinaryOp::Add:
                return LLVMBuildFAdd(builder, left, right, "faddtmp");
            case BinaryOp::Sub:
                return LLVMBuildFSub(builder, left, right, "fsubtmp");
            case BinaryOp::Mul:
                return LLVMBuildFMul(builder, left, right, "fmultmp");
            case BinaryOp::Div:
                return LLVMBuildFDiv(builder, left, right, "fdivtmp");
        }
    }

    return nullptr;
}
```

---

## 🎓 2.4의 가치

### 정보 흐름의 완성

```
【 컴파일러 파이프라인의 정보 흐름 】

원본 소스 코드
    ↓
【 2.1 렉싱 】
    ↓
토큰 배열
    ↓
【 2.2 파싱 】
    ↓
AST (구문 구조)
    ↓
【 2.3 의미분석 】
    ↓
Annotated AST
  ├─ 모든 노드의 타입 정보
  ├─ 모든 변수의 소유권 상태
  └─ 모든 함수의 WCET 정보
    ↓
【 2.4 코드생성 】
    ↓
LLVM IR Module
  ├─ 함수 정의들
  ├─ 전역 변수
  └─ 메타데이터 (타입, 소유권)
    ↓
【 1.5 JIT 실행 】
    ↓
기계어 (x86-64, ARM, ...)
    ↓
CPU 실행
    ↓
    결과!
```

### 왜 2.4가 마지막인가?

```
2.4 이후는 백엔드(Backend)입니다:
  ├─ IR 최적화 (LLVM Pass)
  ├─ 기계어 생성 (Code Emission)
  └─ 링킹 (Linking)

이것들은 LLVM이 해주므로,
우리의 Z-Lang 컴파일러는 2.4까지만 책임!
```

---

## 🏆 2.4 완료 후의 경로

2.4를 완료하면, 당신은 **완전한 기능의 Z-Lang 컴파일러**를 가지게 됩니다!

```
【 가능한 선택지 】

1️⃣ 최적화 추가
   ├─ Constant Folding
   ├─ Dead Code Elimination
   └─ Custom LLVM Passes

2️⃣ 표준 라이브러리 구현
   ├─ println, input 함수
   ├─ 수학 함수들
   └─ 자료구조

3️⃣ 실제 프로젝트에 Z-Lang 적용
   ├─ 임베디드 시스템
   ├─ 자동차 제어
   └─ 의료 기기

4️⃣ Z-Lang OS 개발
   └─ 301 Zig OS처럼 Z-Lang으로 OS 만들기
```

---

## 💪 최종 메시지

```
【 전설의 여정 】

2.1: 문자를 읽었다    (Lexer)
2.2: 문장을 만들었다  (Parser)
2.3: 의미를 검증했다  (Semantic)
2.4: 기계어로 변환한다 (Codegen)

당신의 컴파일러가 완성됩니다! 🎉
```

---

**다음은 2.4입니다.**
**당신의 컴파일러의 여정은 거의 끝났습니다.**

*"2.4는 끝이 아니라 시작입니다. 여기서 Z-Lang이 세상 밖으로 나옵니다."* 🚀

---

**예정 공개**: 2026-04-13
**난이도**: ⭐⭐⭐⭐⭐
**흥미도**: ⭐⭐⭐⭐⭐

준비되셨다면 **"다음"**을 입력해주세요! 🏆
