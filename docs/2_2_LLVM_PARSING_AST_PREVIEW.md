# 🎬 Preview: LLVM 2.2 - 구문 분석(Parsing)과 추상 구문 트리(AST)

> **최종 업데이트**: 2026-02-26
> **상태**: 🔜 준비 중 (다음 주 공개)
> **기대도**: ⭐⭐⭐⭐⭐ (컴파일러의 핵심!)

---

## 🎯 2.2 단계의 핵심

### 토큰 → AST 변환

```
【 렉싱 vs 파싱 】

렉싱 (2.1 완료):
  "fn add(a, b) { return a + b; }"
         ↓
  [keyword_fn, identifier("add"), lparen, ...]
         (단어의 배열)

파싱 (2.2):
  [keyword_fn, identifier("add"), lparen, ...]
         ↓
  FunctionDef {
    name: "add",
    params: ["a", "b"],
    body: Block { ... }
  }
         (구조화된 나무)
```

### 파서의 역할

```
【 파서 = 문법 규칙을 적용하여 구조 파악 】

입력: 토큰 배열
처리: 문법 규칙 검증
  예: "fn"은 반드시 "identifier"를 뒤따르는가?
      "("는 반드시 인자 목록으로 시작되는가?
출력: 올바른 구조 (AST)
```

---

## 📚 2.2에서 배울 것

### 1. 재귀 강하 파서 (Recursive Descent Parser)

```
가장 구현하기 쉬운 파서

원리: 각 문법 규칙마다 함수를 하나씩 작성

fn parse_function() {
    // fn 토큰 기대
    // identifier 기대
    // ( 기대
    // 인자 목록 파싱
    // ) 기대
    // { 기대
    // 문 목록 파싱
    // } 기대
}

fn parse_statement() {
    // if? → parse_if_stmt()
    // while? → parse_while_stmt()
    // identifier? → parse_assignment()
}
```

### 2. AST (Abstract Syntax Tree) 설계

```
모든 코드 구조를 나무로 표현:

        Program
          ↓
      FunctionDef
        ↙      ↘
      name    Block
     ("add")    ↓
             Statement
               ↓
            Return
              ↓
            Binary
            ↙    ↘
          Var    Var
         ("a")   ("b")
```

### 3. 에러 복구 (Error Recovery)

```
파서 에러: 기대하지 않은 토큰

나쁜 대응: 그냥 중단
좋은 대응: 복구 규칙 적용 → 계속 파싱

예:
  "fn add(a b) { }" ← 쉼표 빠짐
  파서: 쉼표를 자동 삽입해서 계속
         또는 다음 인자로 건너뜀
  결과: 에러는 보고하지만 나머지 코드는 파싱!
```

### 4. 연산자 우선순위 (Operator Precedence)

```
문제: "1 + 2 * 3"을 어떻게 파싱?

잘못: ((1 + 2) * 3) = 9
올바름: (1 + (2 * 3)) = 7

파서의 책임: 수학 규칙 적용
  * > +
  / > -
  ^ > *
```

---

## 💻 2.2 예제 미리보기

### AST 노드 정의

```zig
pub const ASTNode = union(enum) {
    // 리터럴
    number: i32,
    identifier: []const u8,
    string: []const u8,

    // 이항 연산
    binary: struct {
        left: *ASTNode,
        op: []const u8,
        right: *ASTNode,
    },

    // 함수 정의
    function: struct {
        name: []const u8,
        params: [][]const u8,
        body: *ASTNode,
    },

    // 블록
    block: []ASTNode,

    // if 문
    if_stmt: struct {
        condition: *ASTNode,
        then_body: *ASTNode,
        else_body: ?*ASTNode,
    },
};
```

### 파서 사용 예

```zig
const source = "fn add(a, b) { return a + b; }";

// 1단계: 렉싱
let lexer = Lexer.init(allocator, source);
let tokens = try lexer.tokenize();

// 2단계: 파싱
let parser = Parser.init(allocator, tokens);
let ast = try parser.parse();

// 3단계: AST 순회
print_ast(ast);
// 출력:
// Program
//   FunctionDef(name: "add")
//     Block
//       Return(Binary(...))
```

---

## 🧠 파서 알고리즘의 종류

### Recursive Descent (재귀 강하)

**장점:**
- ✅ 구현 간단
- ✅ 이해하기 쉬움
- ✅ 에러 메시지 좋음

**단점:**
- ❌ 좌 재귀 처리 어려움
- ❌ 백트래킹 불가능

**추천:** 학습용, 소규모 언어

### LL(1) 파서 (ANTLR)

**특징:**
- 문법 규칙 자동 생성
- 강력한 에러 처리

**추천:** 중규모 언어

### LR 파서 (Yacc, Bison)

**특징:**
- 가장 강력함
- 복잡한 문법 처리 가능

**추천:** 대규모 언어 (C, Java, Go)

---

## 📊 2.2 완성 후

### AST를 활용한 다음 단계

```
【 컴파일러 파이프라인 】

소스 코드
    ↓
2.1 렉싱 (토큰)
    ↓
2.2 파싱 (AST) ← 여기!
    ↓
2.3-2.5 의미분석 (타입 체크, 심볼 관리)
    ↓
2.6-2.7 코드생성 (LLVM IR 변환) ← 1.1-1.5 배운 것!
    ↓
1.5 실행 (ExecutionEngine)
    ↓
결과!
```

---

## 🎓 2.1 → 2.2 진행도

```
【 프론트엔드 2단계의 진행 】

2.1 렉싱: 토큰화 ✅ 완료
  └─ "var x = 10;" → [var, id, =, num, ;]

2.2 파싱: AST 생성 🔜 다음!
  └─ [var, id, =, num, ;] → VariableDecl { ... }

2.3 의미분석: 타입 체크 🔜 이후
  └─ VariableDecl: 타입은? 값은 할당됐나?

2.4 코드생성: LLVM IR 🔜 최종 통합
  └─ VariableDecl → alloca, store
```

---

## 💡 최종 메시지

**당신의 언어가 드디어 구조(Structure)를 갖춥니다!** 🏗️

2.1: 단어 (Tokens)
2.2: 문장 (Sentences - AST)
2.3: 의미 (Semantics)
2.4-2.7: 기계어 (Machine Code)

---

**예정 공개**: 2026-03-16 (일)
**난이도**: ⭐⭐⭐⭐⭐ (최고급)
**흥미도**: ⭐⭐⭐⭐⭐ (핵심!)
**준비**: "다음"을 입력하세요! 🚀

---

*"당신의 언어의 구조가 완성되려고 합니다..."* 🎬
