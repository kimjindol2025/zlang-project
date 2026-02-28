# Z-Lang 예외 처리 구현 제안 (Exception Handling Design)

**제안자**: v2-freelang-ai 개발팀  
**날짜**: 2026-02-26  
**상태**: 💡 제안 (최대한 활용 권장)

---

## 🎯 배경

zlang은 현재 Phase 2.5 단계에서 **최적화 & 검증**을 준비 중입니다.  
본 문서는 zlang의 **실시간 시스템 특성**을 살리면서도 **견고한 예외 처리**를 추가하기 위한 설계안을 제안합니다.

> **핵심 아이디어**: "Zero-Latency + Exception Safety"
> - Rust의 `Result<T, E>` 패턴 (GC 없음)
> - 또는 v8 스타일의 완전한 예외 처리 (선택 사항)

---

## 📊 실증: v2-freelang-ai의 v8 예외 처리 시리즈

### v8.1 ~ v8.4: 저수준 구현 (Handler Stack 기초)
```
v8.1: Handler Stack Foundation
  ├─ handlerStack: 예외 핸들러 스택 관리
  ├─ returnAddress: CATCH 블록 PC 저장
  └─ 3개 테스트 PASS

v8.2: Context Snapshot (레지스터 보존)
  ├─ savedSP, savedFP, savedPC 스냅샷
  ├─ TRY 진입 시점의 상태 저장
  └─ 3개 테스트 PASS

v8.3: Non-local Jump (PC 리다이렉션)
  ├─ THROW 발생 시 PC 강제 변경
  ├─ THROW 아래 명령어 모두 건너뜀
  └─ 3개 테스트 PASS

v8.4: Stack Unwinding (프레임 정리)
  ├─ 중간 프레임들 역순 파괴
  ├─ savedScope 복원
  └─ 3개 테스트 PASS
```

### v8.5 ~ v8.7: 고수준 구현 (사용자 인터페이스)
```
v8.5: Exception Objectification (예외 객체화)
  ├─ Exception 클래스 부트스트랩
  ├─ Message, Code, Timestamp, Location 필드
  └─ 3개 테스트 PASS

v8.6: Polymorphic Catch (다형 예외 처리)
  ├─ catch (e: ExceptionType) 타입 필터링
  ├─ isInstanceOf() 상속 체인 탐색
  └─ 4개 테스트 PASS

v8.7: FINALLY Block (정리 코드 보장)
  ├─ try-finally 예외 안전성
  ├─ return 전에 finally 실행
  └─ 6개 테스트 PASS
```

### v8.8 ~ v8.9: 고도화 (추적 & 자동 감지)
```
v8.8: Exception Chaining (원인 추적)
  ├─ Cause 필드로 예외 체이닝
  ├─ get_cause() 함수로 추적
  └─ 4개 테스트 PASS

v8.9: System Exception Mapping (자동 감지)
  ├─ ArithmeticException (0 나눗셈)
  ├─ NullReferenceException (null 접근)
  ├─ StackOverflowException (재귀 초과)
  └─ 5개 테스트 PASS

합계: v8.1~v8.9 = 34/34 테스트 PASS ✅
```

---

## 🚀 Z-Lang에 맞는 예외 처리 전략

### 전략 A: Rust 스타일 (권장 - Zero-Latency 유지)
```z-lang
// Result<T, E> 패턴
fn divide(a: i64, b: i64) -> Result<i64, DivisionError> {
    if b == 0 {
        return Err(DivisionError::DivideByZero);
    }
    return Ok(a / b);
}

// 호출자가 명시적으로 처리
fn main() -> i64 {
    match divide(10, 0) {
        Ok(result) => return result,
        Err(e) => return -1,
    }
}
```

**장점**:
- ✅ GC 없음 (Zero-Latency 보장)
- ✅ 컴파일 타임 검증 (누락 불가)
- ✅ 스택 오버헤드 없음
- ✅ WCET 분석 가능

**단점**:
- ❌ 보일러플레이트 코드 증가
- ❌ 예외 체이닝 복잡함

---

### 전략 B: try-catch-finally (선택 사항)
```z-lang
// v8 스타일: 완전한 예외 처리
try {
    let x: i64 = dangerous_operation();
} catch (e: DivisionError) {
    return -1;
} catch (e: NullPointerError) {
    return -2;
} finally {
    cleanup_resources();
}
```

**장점**:
- ✅ 예외 체이닝 (원인 추적)
- ✅ 다형 예외 (타입별 필터)
- ✅ FINALLY 보장
- ✅ 개발 생산성 높음

**단점**:
- ❌ 런타임 오버헤드 (GC 필요)
- ❌ WCET 분석 어려움
- ❌ 예측 불가능한 멈춤

---

## 💡 하이브리드 제안: "Best of Both Worlds"

```z-lang
// 1. Critical Path: Result<T, E> 사용 (Zero-Latency)
#[wcet_bound = "100_us"]
fn sensor_read() -> Result<i32, SensorError> {
    if sensor == NULL {
        return Err(SensorError::NotInitialized);
    }
    return Ok(read_adc());
}

// 2. Non-Critical Path: try-catch 허용 (선택사항)
fn user_input() -> i64 {
    try {
        return parse_integer(input);
    } catch (e: ParseError) {
        return 0;
    }
}

// 3. System Exceptions: 자동 감지 (강제)
fn divide(a: i64, b: i64) -> i64 {
    // 자동으로 b == 0 감지 → ArithmeticException
    return a / b;
}
```

---

## 📋 구현 로드맵 (Phase 2.5+ 제안)

### Step 1: System Exception 자동 감지 (1주)
```
✅ ArithmeticException: 0 나눗셈/나머지
✅ NullReferenceException: null 멤버 접근
✅ StackOverflowException: 재귀 깊이 초과
✅ IndexOutOfBoundsException: 배열 범위 초과
✅ TypeMismatchException: 타입 불일치

테스트: 10+ 케이스
```

### Step 2: Result<T, E> 타입 추가 (1주)
```
enum Result<T, E> {
    Ok(T),
    Err(E),
}

match result {
    Ok(value) => { ... },
    Err(error) => { ... },
}
```

### Step 3: 선택적 try-catch 추가 (2주)
```
try { ... }
catch (e: ExceptionType) { ... }
finally { ... }
```

### Step 4: 최적화 & WCET 분석 (2주)
```
- Exception path WCET 계산
- Handler stack 최적화
- 경합 감소 (contention-free)
```

---

## 🎯 예상 효과

### 코드 품질 향상
```
Before: 예외 처리 전혀 없음
After:  3단계 예외 처리 (자동 감지 + Result + try-catch)
```

### WCET 분석 강화
```
Before: "예외 상황? 무시"
After:  "예외 경로도 WCET 계산 가능"
```

### 안전성 증대
```
Before: Silent Failure (오류 무시)
After:  Explicit Failure (명시적 처리 강제)
```

---

## 📚 참고 자료

### v2-freelang-ai 구현 (34/34 테스트 PASS)
- 파일: `/home/kimjin/Desktop/kim/v2-freelang-ai/src/cli/pc-interpreter.ts`
- 핸들러 스택: 줄 1-100
- 예외 처리: 줄 2400-2500, 3600-3700
- 시스템 트랩: 줄 1612-1726

### 참고 언어
- **Rust**: Result<T, E>, panic! macro
- **Go**: error interface, defer cleanup
- **Python**: try-except-finally
- **Java**: Exception hierarchy

---

## 🤝 협력 제안

이 설계안을 zlang 프로젝트에 통합하려면:

1. **Phase 2.5 스케줄 조정** (4주)
2. **Step 1~4 순차 구현**
3. **v2-freelang-ai 코드 참고** (LLVM IR 생성 방식 다르지만, 논리 구조는 동일)
4. **WCET 테스트 작성**

---

## ✅ 체크리스트

- [ ] System Exception 자동 감지 구현
- [ ] Result<T, E> 타입 시스템 추가
- [ ] try-catch 문법 파서 확장
- [ ] 예외 객체화 (Message, Code, Cause)
- [ ] 다형 CATCH (타입별 필터)
- [ ] FINALLY 블록 실행 보장
- [ ] 예외 체이닝 (원인 추적)
- [ ] WCET 분석 도구 추가
- [ ] 테스트 (50+ 케이스)
- [ ] 문서화

---

## 💬 피드백 환영

이 설계안에 대한 피드백은 언제든지 환영합니다:
- 🎯 Real-Time 특성과의 충돌?
- 🛡️ 안전성 개선 아이디어?
- ⚡ 성능 최적화 방안?

---

**"기록이 증명이다"** - 이 설계는 34개의 테스트 케이스로 입증되었습니다.

**제안**: v2-freelang-ai의 v8 예외 처리 이론을 zlang의 LLVM 백엔드에 맞게 변환하면, 
"Zero-Latency + Exception Safety"의 새로운 표준이 될 수 있습니다.

