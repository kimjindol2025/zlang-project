# 📝 ASSIGNMENT 2.4: LLVM IR 코드 생성 통합 (Code Generation Integration)

> **난이도**: ⭐⭐⭐⭐⭐
> **예상 소요 시간**: 40-50시간
> **최종 성과**: 완전한 Z-Lang 컴파일러 Frontend 완성

---

## 🎯 개요

**2.3 의미분석**에서 정보를 담은 **Annotated AST**를 받았습니다.
이제 그 정보를 **LLVM IR**로 변환하는 것이 당신의 미션입니다.

```
【 2.4의 미션 】

Annotated AST (완벽한 의미 정보)
         ↓
【 CodeGenerator 당신의 손으로 】
         ↓
LLVM IR Module (기계가 이해하는 언어)
         ↓
【 1.5 JIT 엔진 】
         ↓
실행 결과!
```

---

## 📋 과제 목록

### **과제 1: CodeGenerator Visitor 패턴 구현** (50%)

당신의 역할: **Annotated AST의 각 노드를 LLVM IR로 변환하는 Visitor 메서드들을 구현**

#### 요구사항:

```cpp
class CodeGenerator {
private:
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    SymbolTable& symbol_table;
    std::unordered_map<std::string, LLVMValueRef> named_values;

public:
    // 진입점
    LLVMModuleRef generate(ASTNode* ast);

private:
    // Visitor 메서드들 - 당신이 구현할 메서드들

    // 1. 함수 정의
    LLVMValueRef visitFunction(ASTNode* func);

    // 2. 변수 선언
    LLVMValueRef visitVarDecl(ASTNode* var);

    // 3. 이항 연산
    LLVMValueRef visitBinaryOp(ASTNode* binop);

    // 4. 단항 연산
    LLVMValueRef visitUnaryOp(ASTNode* unop);

    // 5. 함수 호출
    LLVMValueRef visitCall(ASTNode* call);

    // 6. 식별자 (변수 참조)
    LLVMValueRef visitIdentifier(ASTNode* id);

    // 7. 리터럴
    LLVMValueRef visitLiteral(ASTNode* lit);

    // 8. Return 문
    LLVMValueRef visitReturn(ASTNode* ret);

    // 9. If 문
    LLVMValueRef visitIf(ASTNode* if_stmt);

    // 10. While 루프
    LLVMValueRef visitWhile(ASTNode* loop);

    // 11. 블록
    LLVMValueRef visitBlock(ASTNode* block);
};
```

#### 구현 가이드:

**A. 함수 생성 (visitFunction)**
```cpp
LLVMValueRef CodeGenerator::visitFunction(ASTNode* func) {
    // 1️⃣ 파라미터 타입들을 수집
    std::vector<LLVMTypeRef> param_types;
    for (auto& param : func->params) {
        param_types.push_back(convertType(param.type));
    }

    // 2️⃣ 함수 타입 구성 (반환타입, 파라미터타입들, variadic 여부)
    LLVMTypeRef func_type = LLVMFunctionType(
        convertType(func->return_type),
        param_types.data(),
        param_types.size(),
        0  // not variadic
    );

    // 3️⃣ 모듈에 함수 추가
    LLVMValueRef llvm_func = LLVMAddFunction(
        module,
        func->name.c_str(),
        func_type
    );

    // 4️⃣ 진입 기본블록 생성
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(llvm_func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    // 5️⃣ 파라미터를 심볼 테이블에 등록
    symbol_table.pushScope();
    for (unsigned i = 0; i < func->params.size(); ++i) {
        LLVMValueRef param = LLVMGetParam(llvm_func, i);
        symbol_table.define(func->params[i].name, {
            .type = func->params[i].type,
            .llvm_value = param
        });
    }

    // 6️⃣ 함수 본체 생성 (visitBlock 호출)
    visitBlock(func->body);

    // 7️⃣ 스코프 종료
    symbol_table.popScope();

    return llvm_func;
}
```

**B. 변수 선언 (visitVarDecl)**
```cpp
LLVMValueRef CodeGenerator::visitVarDecl(ASTNode* var) {
    // 1️⃣ 변수의 LLVM 타입 결정
    LLVMTypeRef llvm_type = convertType(var->inferred_type);

    // 2️⃣ 메모리 할당 (alloca)
    LLVMValueRef addr = LLVMBuildAlloca(builder, llvm_type, var->name.c_str());

    // 3️⃣ 초기값이 있으면 저장
    if (var->init_expr) {
        LLVMValueRef init = visitNode(var->init_expr);
        LLVMBuildStore(builder, init, addr);
    }

    // 4️⃣ 심볼 테이블에 등록
    symbol_table.define(var->name, {
        .type = var->inferred_type,
        .llvm_value = addr
    });

    return nullptr;  // var decl는 값을 반환하지 않음
}
```

**C. 이항 연산 (visitBinaryOp)**
```cpp
LLVMValueRef CodeGenerator::visitBinaryOp(ASTNode* binop) {
    // 1️⃣ 좌우 피연산자 생성
    LLVMValueRef left = visitNode(binop->left);
    LLVMValueRef right = visitNode(binop->right);

    Type type = binop->inferred_type;

    // 2️⃣ 타입별로 올바른 명령어 선택
    if (isIntegerType(type)) {
        switch (binop->op) {
            case BinOp::Add: return LLVMBuildAdd(builder, left, right, "addtmp");
            case BinOp::Sub: return LLVMBuildSub(builder, left, right, "subtmp");
            case BinOp::Mul: return LLVMBuildMul(builder, left, right, "multmp");
            case BinOp::Div: return LLVMBuildSDiv(builder, left, right, "divtmp");
            case BinOp::Rem: return LLVMBuildSRem(builder, left, right, "remtmp");
        }
    } else if (isFloatType(type)) {
        switch (binop->op) {
            case BinOp::Add: return LLVMBuildFAdd(builder, left, right, "faddtmp");
            case BinOp::Sub: return LLVMBuildFSub(builder, left, right, "fsubtmp");
            case BinOp::Mul: return LLVMBuildFMul(builder, left, right, "fmultmp");
            case BinOp::Div: return LLVMBuildFDiv(builder, left, right, "fdivtmp");
        }
    }

    return nullptr;
}
```

#### 체크리스트:
- [ ] visitFunction: 함수 선언 및 파라미터 처리
- [ ] visitVarDecl: 변수 할당 및 초기화
- [ ] visitBinaryOp: 정수와 실수 연산 구분
- [ ] visitUnaryOp: 단항 연산자 (-x, !x 등)
- [ ] visitCall: 함수 호출 (인자 전달)
- [ ] visitIdentifier: 변수 참조 (load)
- [ ] visitLiteral: 상수값 (i64, f64 등)
- [ ] visitReturn: Return 문 처리
- [ ] visitIf: 조건분기 (br i1, PHI 노드)
- [ ] visitWhile: 루프 구조 (br, backedge)
- [ ] visitBlock: 여러 문장의 순차 처리

---

### **과제 2: 타입 매핑 시스템** (20%)

당신의 역할: **Z-Lang의 타입 시스템을 LLVM의 타입으로 정확하게 변환**

#### 요구사항:

Z-Lang 타입 → LLVM 타입 변환 함수 구현:

```cpp
LLVMTypeRef CodeGenerator::convertType(const Type& ztype) {
    // 당신이 구현해야 할 부분

    // 기본 타입 (Primitive Types)
    switch (ztype.base) {
        case BuiltinType::I32:
            return LLVMInt32Type();  // i32

        case BuiltinType::I64:
            return LLVMInt64Type();  // i64

        case BuiltinType::F32:
            return LLVMFloatType();  // float

        case BuiltinType::F64:
            return LLVMDoubleType(); // double

        case BuiltinType::Bool:
            return LLVMInt1Type();   // i1

        case BuiltinType::String:
            // string: 동적 길이이므로 포인터로 표현
            return LLVMPointerType(LLVMInt8Type(), 0);

        case BuiltinType::Void:
            return LLVMVoidType();   // void
    }

    // 배열 타입
    if (ztype.is_array) {
        LLVMTypeRef element_type = convertType(ztype.element_type);
        return LLVMArrayType(element_type, ztype.array_size);
    }

    // 포인터 타입
    if (ztype.is_pointer) {
        LLVMTypeRef pointee_type = convertType(ztype.pointee_type);
        return LLVMPointerType(pointee_type, 0);
    }

    // 구조체 타입 (나중)
    if (ztype.is_struct) {
        // LLVM 구조체 타입 생성
        // 필드들의 타입들을 모아서
        // LLVMStructTypeInContext 호출
    }

    return nullptr;  // error
}
```

#### 테스트 케이스:

당신이 검증해야 할 변환들:

```z-lang
// Test 1: 기본 타입들
fn test_primitives() {
    let a: i32;      // i32 → LLVMInt32Type()
    let b: i64;      // i64 → LLVMInt64Type()
    let c: f32;      // f32 → LLVMFloatType()
    let d: f64;      // f64 → LLVMDoubleType()
    let e: bool;     // bool → i1
    let f: void;     // void → void
}

// Test 2: 배열 타입
fn test_arrays() {
    let arr1: [i32; 10];   // [10 x i32]
    let arr2: [f64; 100];  // [100 x f64]
}

// Test 3: 포인터 타입
fn test_pointers() {
    let ptr1: &i32;        // i32*
    let ptr2: &f64;        // f64*
}

// Test 4: 복합 타입
fn test_complex() {
    let arr: [i32; 5];     // [5 x i32]
    let ptr_arr: &[i32; 5]; // [5 x i32]*
}
```

#### 체크리스트:
- [ ] I32, I64, F32, F64, Bool, Void 매핑
- [ ] String 타입 매핑 (i8* 포인터)
- [ ] 배열 타입 변환 (array size 포함)
- [ ] 포인터 타입 변환
- [ ] 구조체 타입 변환 (선택사항)
- [ ] 모든 테스트 케이스 통과

---

### **과제 3: 표현식 코드 생성 시뮬레이션** (20%)

당신의 역할: **복잡한 표현식을 LLVM IR로 변환하는 과정을 직접 따라하기**

#### 요구사항:

다음 Z-Lang 코드를 LLVM IR로 변환하세요. **손으로 직접 작성**하면서 각 단계를 설명하세요.

**예제 1: 산술 표현식**
```z-lang
fn calculate() -> i64 {
    let x: i64 = 10;
    let y: i64 = 20;
    let result: i64 = (x + y) * 2;
    return result;
}
```

당신의 손으로 변환해야 할 LLVM IR:
```llvm
define i64 @calculate() {
entry:
  ; 【 당신의 IR 작성 】
  ; Step 1: x 메모리 할당
  %x.addr = alloca i64

  ; Step 2: x에 10 저장
  store i64 10, i64* %x.addr

  ; Step 3: y 메모리 할당
  %y.addr = alloca i64

  ; Step 4: y에 20 저장
  store i64 20, i64* %y.addr

  ; Step 5: result 메모리 할당
  %result.addr = alloca i64

  ; Step 6: x 값 로드
  %x.val = load i64, i64* %x.addr

  ; Step 7: y 값 로드
  %y.val = load i64, i64* %y.addr

  ; Step 8: x + y 계산
  %sum = add i64 %x.val, %y.val

  ; Step 9: sum * 2 계산
  %final = mul i64 %sum, 2

  ; Step 10: result에 최종값 저장
  store i64 %final, i64* %result.addr

  ; Step 11: result 로드하여 반환
  %ret.val = load i64, i64* %result.addr
  ret i64 %ret.val
}
```

**예제 2: 조건문**
```z-lang
fn max(a: i64, b: i64) -> i64 {
    if a > b {
        return a;
    } else {
        return b;
    }
}
```

당신의 손으로 변환해야 할 LLVM IR:
```llvm
define i64 @max(i64 %a, i64 %b) {
entry:
  ; 【 당신의 IR 작성 】
  ; a > b 비교
  %cond = icmp sgt i64 %a, %b

  ; true/false 분기
  br i1 %cond, label %then, label %else

then:
  ; a 반환
  ret i64 %a

else:
  ; b 반환
  ret i64 %b
}
```

**예제 3: 루프**
```z-lang
fn sum_up_to(n: i64) -> i64 {
    let sum: i64 = 0;
    let i: i64 = 0;
    while i <= n {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
```

당신의 손으로 변환해야 할 LLVM IR:
```llvm
define i64 @sum_up_to(i64 %n) {
entry:
  ; 【 당신의 IR 작성 】
  %sum.addr = alloca i64
  store i64 0, i64* %sum.addr

  %i.addr = alloca i64
  store i64 0, i64* %i.addr

  br label %while.cond

while.cond:
  ; i 로드
  %i.val = load i64, i64* %i.addr

  ; i <= n 비교
  %cond = icmp sle i64 %i.val, %n

  ; 루프 계속 또는 종료
  br i1 %cond, label %while.body, label %while.end

while.body:
  ; sum = sum + i
  %sum.val = load i64, i64* %sum.addr
  %new.sum = add i64 %sum.val, %i.val
  store i64 %new.sum, i64* %sum.addr

  ; i = i + 1
  %new.i = add i64 %i.val, 1
  store i64 %new.i, i64* %i.addr

  br label %while.cond

while.end:
  %result = load i64, i64* %sum.addr
  ret i64 %result
}
```

#### 체크리스트:
- [ ] 예제 1: 산술 표현식 변환 (alloca → store → load → 연산)
- [ ] 예제 2: 조건문 변환 (icmp → br)
- [ ] 예제 3: 루프 변환 (while.cond → while.body → backedge)
- [ ] 각 단계마다 상세한 주석 작성
- [ ] LLVM IR 형식 정확성 (타입, 레지스터 이름 등)

---

### **과제 4: 철학 에세이 - 왜 Z-Lang의 IR 생성이 고급 언어보다 우수한가?** (10%)

당신의 역할: **컴파일러 설계자의 관점에서 2.4 코드 생성의 의미를 깊이 있게 이해하기**

#### 요구사항:

**5-7 페이지 분량의 에세이 작성** (한글, 마크다운 형식):

**주제**: "LLVM 중간표현(IR) 생성이 고급 언어의 동적 최적화보다 실시간 시스템에 우수한 이유"

#### 다루어야 할 주제들:

**1. 정적 정보 vs 동적 정보**
```
고급 언어 (Python, JavaScript):
  - 런타임에 타입 결정
  - 런타임에 메모리 할당
  - JIT 컴파일러가 동적 최적화
  - 예측 불가능한 지연 (GC pause, deoptimization)

Z-Lang LLVM IR:
  - 컴파일 타임에 모든 타입 결정 (static)
  - 컴파일 타임에 메모리 배치 결정
  - 최적화는 컴파일 타임에 완료
  - 실행 시간 = 예측 가능한 시간
```

**2. WCET (Worst Case Execution Time) 보장**
```
왜 LLVM IR은 WCET를 계산할 수 있는가?

❌ Python:
   for i in range(n):
       print(i)  # 얼마나 걸릴까?

   - 메모리 할당: 런타임 (예측 불가)
   - 문자열 변환: 런타임 (입력 크기에 따라 다름)
   - 출력: I/O 지연 (운영체제 스케줄에 의존)

   → WCET 계산: 불가능

✅ Z-Lang IR:
   for i in range(10) {  // 루프 횟수 정적
       println(i);  // 루프 언롤 가능
   }

   - 루프 횟수: 컴파일타임 상수 → 언롤 가능
   - 메모리: 컴파일타임 할당 → 일정 시간
   - 명령어 수: 정적 계산 가능

   → WCET 계산: 가능!
```

**3. 메모리 안전성**
```
Python의 메모리 관리:
  - GC가 주기적으로 실행
  - GC pause: 예측 불가능
  - 자동차가 GC 중에 멈추면? ☠️

Z-Lang의 메모리 관리:
  - 스택: 함수 진입/종료 시 자동
  - 힙: ownership으로 명시적 해제
  - GC 없음 → 예측 가능한 메모리 시간
```

**4. 컴파일 타임 검증**
```
오류 발견 시점:

Python:
  런타임 에러!
  program.py:47: TypeError: unsupported operand type(s)

  → 자동차가 주행 중 크래시

Z-Lang:
  컴파일 에러!
  error: cannot add i32 and f64
         ^~~~

  → 배포 전에 발견!
```

#### 구성:

1. **서론** (1 페이지)
   - 실시간 시스템의 정의 및 요구사항
   - 고급 언어의 한계

2. **본론** (3-4 페이지)
   - 정적 타입과 동적 타입의 차이
   - 메모리 관리 방식의 영향
   - WCET 계산 가능성
   - 최적화 시점의 중요성

3. **사례 분석** (1-2 페이지)
   - 자동차 안전 시스템
   - 의료 기기 제어
   - 항공우주 시스템

4. **결론** (1 페이지)
   - 2.4 코드 생성의 역할
   - 미래의 실시간 언어

#### 체크리스트:
- [ ] 최소 5 페이지 (마크다운으로 2,000+ 단어)
- [ ] 고급 언어 vs Z-Lang 비교 (3개 이상 사례)
- [ ] WCET, 메모리 안전성, 컴파일타임 검증 포함
- [ ] 실시간 시스템의 구체적 사례 (자동차, 의료기기 등)
- [ ] 학술적 관점과 엔지니어링 관점의 균형

---

## 🎓 제출 요구사항

### 파일 목록:
1. `src/codegen/CodeGenerator.cpp` - 과제 1 구현
2. `src/codegen/CodeGenerator.h` - 헤더 파일
3. `ASSIGNMENT_2_4_SUBMISSION.md` - 과제 2, 3, 4 작성
   - 섹션 1: 타입 매핑 (코드 + 설명)
   - 섹션 2: 표현식 변환 (3개 예제의 LLVM IR)
   - 섹션 3: 철학 에세이 (5-7 페이지)

### 테스트:
```bash
# 컴파일 검증
cmake --build . --target codegen

# 코드 생성 테스트
./zlang --emit-llvm examples/hello.z
./zlang --emit-llvm examples/fibonacci.z
./zlang --emit-llvm examples/arithmetic.z
```

### 검수 기준:
```
✅ 11개 Visitor 메서드 구현 완료
✅ 7개 타입 변환 케이스 통과
✅ 3개 표현식 예제 올바른 IR 생성
✅ 철학 에세이 5 페이지 이상
✅ 모든 테스트 PASS
```

---

## 💪 최종 메시지

```
【 2.4 코드 생성의 진정한 의미 】

당신은 지금까지:
  2.1 → 문자를 읽었다 (렉싱)
  2.2 → 문장을 이해했다 (파싱)
  2.3 → 의미를 검증했다 (의미분석)

이제:
  2.4 → 기계어로 변환한다 (코드생성)

이 과제를 완료하면,
당신은 **완전한 Z-Lang 컴파일러**를 손에 쥔 것입니다!
```

---

**난이도**: ⭐⭐⭐⭐⭐
**흥미도**: ⭐⭐⭐⭐⭐
**실용성**: ⭐⭐⭐⭐⭐

당신이 만든 IR이 실제로 기계어가 되어 CPU에서 춤을 춥니다. 🚀

**마감**: 2026년 4월 13일
