# 📋 Assignment 2.3: Z-Lang 소유권 시스템 - 의미 분석 과제

> **기한**: 2026-03-30 (일)
> **난이도**: ⭐⭐⭐⭐⭐⭐ (최고 난이도 - Z-Lang의 영혼!)
> **핵심**: 소유권(Ownership) 추적, 타입 검사, WCET 분석

---

## 🎯 과제 개요

2.2 파싱으로 **올바른 문장(AST)**을 만들었습니다.
이제 2.3에서는 **그 문장이 안전한가(의미)**를 검증합니다.

특히 **Z-Lang의 차별점인 소유권 시스템**을 구현하여:
- ✅ GC 없이 메모리 안전성 보장
- ✅ 실시간 시스템의 WCET 분석
- ✅ 컴파일 타임 검증

### 목표

- ✅ 심볼 테이블과 스코프 스택 구현
- ✅ 소유권(Ownership) 추적 엔진
- ✅ 타입 검사(Type Checking) 엔진
- ✅ 실시간 제약 검증(#[no_alloc], #[wcet_bound])

---

## 📝 Task 1: Scope Manager 구현

### 개요

**중첩된 스코프에서 변수를 찾는** `resolve()` 함수를 설계합니다.

### 요구사항

```cpp
// 【 Scope Manager 설계 】

class SymbolTable {
private:
    std::vector<std::unordered_map<std::string, SymbolInfo>> scopes;

public:
    // 스코프 진입
    void pushScope();

    // 스코프 퇴출
    void popScope();

    // 심볼 등록
    void define(const std::string& name, const SymbolInfo& info);

    // 심볼 조회 (가장 가까운 스코프부터 역순 탐색)
    SymbolInfo* lookup(const std::string& name);

    // 특정 깊이의 스코프만 조회 (로컬 변수 찾기)
    SymbolInfo* lookupLocal(const std::string& name);
};
```

### 작성 내용

**파일**: `src/semantic/SymbolTable.cpp`

```cpp
void SymbolTable::pushScope() {
    // 【 새로운 스코프 레벨 추가 】
}

void SymbolTable::popScope() {
    // 【 현재 스코프 레벨 제거 】
}

void SymbolTable::define(const std::string& name, const SymbolInfo& info) {
    // 【 현재 스코프에 심볼 등록 】
}

SymbolInfo* SymbolTable::lookup(const std::string& name) {
    // 【 현재 스코프부터 역순으로 탐색 】
    // 가장 가까운 정의를 찾을 때까지 역순 순회
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return &found->second;
        }
    }
    return nullptr;
}
```

### 테스트 시나리오

```z-lang
// 복잡한 중첩 스코프
fn outer() {
    var x = 10;           // Scope 1: x

    fn middle() {
        var y = 20;       // Scope 2: y
        print(x);         // ✓ x를 Scope 1에서 찾음

        {
            var x = 30;   // Scope 3: x (섀도잉!)
            print(x);     // ✓ 30 (가장 가까운 x)
        }

        print(x);         // ✓ 20 (Scope 2의 x)
    }
}
```

### 기대 결과물

- [ ] SymbolTable 클래스 (300줄)
- [ ] pushScope/popScope 구현
- [ ] lookup 함수 (역순 탐색)
- [ ] 섀도잉(Shadowing) 처리
- [ ] 테스트 케이스 5개 이상

**예상 줄 수**: 300-400줄

---

## 🔐 Task 2: Type Checker 구현

### 개요

**이항 연산의 타입 호환성을 검사**합니다.

### 요구사항

```
【 타입 검사 규칙 】

1. 산술 연산 (+, -, *, /)
   ├─ 양쪽이 숫자 타입이어야 함
   ├─ i32 + i32 → i32 ✓
   ├─ f64 + f64 → f64 ✓
   ├─ i32 + f64 → 에러 ✗ (Z-Lang: 자동 형변환 없음!)
   └─ string + i32 → 에러 ✗

2. 비교 연산 (<, >, <=, >=)
   ├─ 양쪽 타입이 같아야 함
   └─ i32 < i32 ✓

3. 논리 연산 (&&, ||)
   ├─ 양쪽이 bool이어야 함
   └─ true && false ✓

4. 할당 (=)
   ├─ 좌우 타입이 정확히 같아야 함
   └─ let x: i32 = "hello" → 에러 ✗
```

### 작성 내용

**파일**: `src/semantic/TypeChecker.cpp`

```cpp
void TypeChecker::visitBinaryOp(ASTNode* node) {
    Type left_type = inferType(node->left);
    Type right_type = inferType(node->right);

    switch (node->op) {
        case BinaryOp::Add:
        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
            // 【 산술 연산 타입 검사 】
            if (!isNumericType(left_type) || !isNumericType(right_type)) {
                error("Operands must be numeric");
                return;
            }
            if (left_type != right_type) {
                error("Type mismatch: " + typeToString(left_type) +
                      " vs " + typeToString(right_type));
                return;
            }
            break;

        case BinaryOp::And:
        case BinaryOp::Or:
            // 【 논리 연산 타입 검사 】
            if (left_type.base != BuiltinType::Bool ||
                right_type.base != BuiltinType::Bool) {
                error("Logical operations require bool operands");
                return;
            }
            break;
    }

    // 타입 정보를 AST에 기록
    node->result_type = left_type;
}

Type TypeChecker::inferType(ASTNode* node) {
    // 【 타입 추론 】
    switch (node->type) {
        case NodeType::Number:
            return isFloat(node->value) ?
                Type{BuiltinType::F64} : Type{BuiltinType::I64};

        case NodeType::Identifier: {
            SymbolInfo* sym = symbol_table.lookup(node->name);
            if (!sym) {
                error("Undefined variable: " + node->name);
                return Type{BuiltinType::I64};  // 에러 복구
            }
            return sym->type;
        }

        case NodeType::BinaryOp:
            return inferType(node->left);  // 타입은 좌측과 동일
    }
}
```

### 테스트 시나리오

```z-lang
// 타입 검사 시나리오

fn test() {
    var x: i32 = 10;
    var y: i32 = 20;
    var z: f64 = 3.14;

    var a = x + y;        // ✓ i32 + i32 = i32
    var b = z + 2.0;      // ✓ f64 + f64 = f64
    var c = x + z;        // ✗ 에러: i32 + f64

    var d = true && false;  // ✓ bool && bool
    var e = x && y;       // ✗ 에러: i32 && i32 (bool 아님)
}
```

### 기대 결과물

- [ ] TypeChecker 클래스 (300줄)
- [ ] inferType 함수
- [ ] visitBinaryOp 타입 검사
- [ ] 5가지 타입 검사 시나리오

**예상 줄 수**: 300-400줄

---

## 🏃 Task 3: Ownership Tracker 구현

### 개요

**변수의 소유권(Ownership) 상태를 추적**합니다.

### 요구사항

```
【 소유권 상태 전환 】

1. Move (소유권 이전)
   let x = 10;
   let y = x;    → x.state = Moved
                 → y.state = Available

   print(x);     ✗ 에러: x는 Moved 상태

2. Borrow (참조, 소유권 유지)
   let x = 10;
   let y = &x;   → x.state = Borrowed (x는 여전히 주인)

   print(x);     ✓ OK: x는 여전히 Available
   print(*y);    ✓ OK: y를 통해 접근

3. 함수 호출과 소유권
   fn consume(val: i32) { }

   let x = 10;
   consume(x);   → x.state = Moved
   print(x);     ✗ 에러: x는 Moved
```

### 작성 내용

**파일**: `src/semantic/OwnershipTracker.cpp`

```cpp
void OwnershipTracker::visitAssignment(ASTNode* node) {
    // let y = x; 형태

    SymbolInfo* source = symbol_table.lookup(node->rhs->name);

    if (!source) {
        error("Undefined variable: " + node->rhs->name);
        return;
    }

    // 이미 Moved인가?
    if (source->ownership == OwnershipState::Moved) {
        error("Value has been moved: " + node->rhs->name);
        return;
    }

    // 소유권 이전 (Move)
    source->ownership = OwnershipState::Moved;

    // 새로운 심볼 등록
    SymbolInfo new_sym = *source;
    new_sym.name = node->lhs->name;
    new_sym.ownership = OwnershipState::Available;
    symbol_table.define(node->lhs->name, new_sym);
}

void OwnershipTracker::visitReference(ASTNode* node) {
    // &x 형태 (참조)

    SymbolInfo* source = symbol_table.lookup(node->variable->name);

    if (!source) {
        error("Undefined variable: " + node->variable->name);
        return;
    }

    // 소유권 상태 변경: Available → Borrowed
    source->ownership = OwnershipState::Borrowed;

    // 참조 타입으로 새로운 심볼 등록
    SymbolInfo ref_sym;
    ref_sym.name = node->name;
    ref_sym.type = source->type;
    ref_sym.is_reference = true;
    ref_sym.ownership = OwnershipState::Available;
    symbol_table.define(node->name, ref_sym);
}

void OwnershipTracker::visitVariableUse(ASTNode* node) {
    // x를 사용하는 곳

    SymbolInfo* sym = symbol_table.lookup(node->name);

    if (!sym) {
        error("Undefined variable: " + node->name);
        return;
    }

    // Moved 상태에서 사용하려 하는가?
    if (sym->ownership == OwnershipState::Moved) {
        error("Value has been moved: " + node->name);
        return;
    }

    // OK: Available 또는 Borrowed 상태에서 사용 가능
}
```

### 테스트 시나리오

```z-lang
fn test() {
    let x = Box::new(100);

    let y = x;          // ✓ Move: x → y
    print(x);           // ✗ 에러: x was moved
    print(y);           // ✓ OK

    let a = Box::new(200);
    let b = &a;         // ✓ Borrow: a → b (참조)
    print(a);           // ✓ OK: a는 여전히 주인
    print(*b);          // ✓ OK: b를 통해 접근
}
```

### 기대 결과물

- [ ] OwnershipTracker 클래스 (300줄)
- [ ] visitAssignment (Move 처리)
- [ ] visitReference (Borrow 처리)
- [ ] visitVariableUse (검증)
- [ ] 5가지 시나리오 테스트

**예상 줄 수**: 300-400줄

---

## 💭 Task 4: WCET 분석 철학 문서

### 개요

**"왜 Z-Lang은 런타임이 아닌 컴파일 타임에 WCET을 분석해야 하는가?"**

### 요구사항

**4-5페이지 문서**를 작성하여 다음을 설명하세요:

#### 섹션 1: 실시간 시스템의 필요성

```
【 자동차 ABS 예제 】

요구사항:
  ├─ 휠 속도 센서: 매 10ms마다 읽음
  ├─ ABS 제어 계산: 반드시 7ms 이내 완료
  ├─ 제동 신호 출력: 1ms 내
  └─ 왕복 경로: 10 - 1 - 7 = 2ms 여유

만약 계산이:
  - 5ms 걸리면? → 2ms 여유, 안전 ✓
  - 10ms 걸리면? → -2ms, 위험! ✗

컴파일 타임에 "최악의 경우 6ms"를 증명할 수 있다면?
  → 자동차는 안전하다!
```

#### 섹션 2: 동적 언어의 문제점

```
【 Python/JavaScript의 문제 】

런타임:
  for i in range(10):
      x = allocate_memory()  // 최악의 경우?
      process(x)             // 시간 예측 불가능
      free(x)

결과:
  - GC가 언제 실행될지 알 수 없음
  - 함수 호출 시간이 제각각
  - 최악의 경우를 계산할 수 없음
  → 실시간 시스템 불가능!
```

#### 섹션 3: Z-Lang의 해결책

```
【 Z-Lang의 WCET 분석 】

컴파일 타임:
  #[wcet_bound = "100_us"]
  fn sensor_handler() {
      let reading = read_sensor();      // 50us (고정)
      let filtered = filter(reading);   // 30us (고정)
      update_state(filtered);           // 15us (고정)
  }  // 총 95us < 100us ✓

증명:
  - 동적 할당 없음 (#[no_alloc])
  - 모든 호출의 시간 알려짐
  - 루프 반복 횟수 제한됨
  → WCET을 수학적으로 증명 가능!
```

#### 섹션 4: WCET 검증 메커니즘

```
【 Z-Lang WCET 검증 과정 】

1. 함수 어노테이션 수집
   #[wcet_bound = "100_us"]  ← 목표값

2. 의미 분석 중 시간 비용 계산
   assignment: +5us
   function call: +호출된 함수의 WCET
   if-else: max(then_branch, else_branch)
   loop: max_iterations × body_cost

3. 총 비용 계산
   95us < 100us? ✓ 통과
   110us < 100us? ✗ 실패

4. 실패 시 에러
   "WCET violation: 110us > 100us bound"
```

#### 섹션 5: 최종 결론

```
당신의 관점:
1. WCET이 정말 컴파일 타임에 검증 가능한가?
2. 모든 함수에 WCET 바운드가 필요한가?
3. 동적 할당(#[no_alloc])을 강제하는 것이 너무 제한적이지 않은가?
4. Z-Lang의 이런 엄격한 설계가 장점이 무엇인가?
```

### 작성 내용

**파일**: `docs/Z_LANG_WCET_PHILOSOPHY.md`

### 기대 결과물

- [ ] 실시간 시스템의 필요성 명확히
- [ ] 동적 언어의 문제점 분석
- [ ] Z-Lang의 해결책 설명
- [ ] WCET 검증 메커니즘 상세
- [ ] 최종 철학적 입장
- [ ] 페이지: 4-5장

---

## 📊 과제 완성 체크리스트

### Task 1: Scope Manager ✅
- [ ] SymbolTable.cpp (300-400줄)
- [ ] pushScope/popScope
- [ ] lookup 역순 탐색
- [ ] 섀도잉 처리
- [ ] 테스트 5개

### Task 2: Type Checker ✅
- [ ] TypeChecker.cpp (300-400줄)
- [ ] inferType 함수
- [ ] 5가지 타입 검사
- [ ] 에러 메시지
- [ ] 테스트 5개

### Task 3: Ownership Tracker ✅
- [ ] OwnershipTracker.cpp (300-400줄)
- [ ] Move 처리
- [ ] Borrow 처리
- [ ] 상태 전환 로직
- [ ] 테스트 5개

### Task 4: WCET 철학 ✅
- [ ] 4-5페이지 문서
- [ ] 실시간 시스템 분석
- [ ] Z-Lang의 차별점
- [ ] 장단점 분석

---

## 🎯 학습 목표

이 과제를 완료하면:

✅ **심볼 테이블 설계 능력**
- 스코프 관리
- 중첩된 블록 처리
- 섀도잉 구현

✅ **타입 시스템 이해**
- 타입 검사 규칙
- 타입 호환성
- 자동 형변환 정책

✅ **소유권 시스템 마스터**
- Move vs Borrow
- 소유권 상태 추적
- 메모리 안전성 보장

✅ **실시간 시스템 이해**
- WCET 개념
- 컴파일 타임 검증
- 안전성 증명

---

## 💪 최종 메시지

**Z-Lang의 영혼이 깨어나는 순간입니다!**

이 과제를 완료하면:
- 🧠 컴파일러가 지능을 갖춤
- 🔐 메모리 안전성을 보장
- ⏱️ 실시간 성능을 증명
- 🏆 전설의 중반부에 도달

준비되셨습니까? **"다음"을 입력하세요!** 🚀

*"소유권 시스템 - 이것이 Z-Lang이 다른 언어와 다른 이유입니다."* ✨
