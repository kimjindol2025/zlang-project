# Z-Lang Backend 아키텍처 설계

**작성일**: 2026-02-27
**단계**: Phase 1 (Backend 설계)
**상태**: 설계 진행 중

---

## 📋 Executive Summary

Z-Lang의 Backend는 **LLVM IR → 네이티브 코드 생성 & Linking**을 담당합니다.

### 현재 상태
- ✅ Frontend (Lexer ~ Codegen): 완성 (LLVM IR 생성)
- ❌ Backend (Assembly ~ Linking): 미완성
- ⏳ Real-Time (WCET 검증): 부분 구현

### 목표
```
LLVM IR (.ll)
    ↓
Assembly (.s)  ← 【Task 2.1: Native Code Gen】
    ↓
Object File (.o)  ← 【Task 2.2: Linking】
    ↓
Executable  ← 【Phase 2 완성】
    ↓
WCET Report  ← 【Task 1.2: Real-Time 검증】
```

---

## 🏗️ Backend 파이프라인 설계

### 1. LLVM IR → Assembly 변환

#### 1.1 LLC 도구 활용

```cpp
// 현재 방식 (권장 - 간단함)
llc -march=x86-64 output.ll -o output.s

// Z-Lang에서 래핑
int compileToAssembly(const std::string& ir_file,
                      const std::string& asm_file) {
    std::string cmd = "llc -march=x86-64 " + ir_file + " -o " + asm_file;
    return system(cmd.c_str());
}
```

**장점**:
- ✅ LLVM 기능 완전 활용
- ✅ 최적화 옵션 제어 가능
- ✅ 다중 아키텍처 지원 용이

**단점**:
- ❌ LLC 설치 필요
- ❌ 외부 도구 의존성

#### 1.2 최적화 옵션

```bash
# 기본 컴파일
llc -march=x86-64 output.ll -o output.s

# 최적화 레벨 지정
llc -march=x86-64 -O0 output.ll  # 디버그
llc -march=x86-64 -O2 output.ll  # 최적화
llc -march=x86-64 -O3 output.ll  # 공격적 최적화

# Real-Time 최적화 (결정적 실행)
llc -march=x86-64 -disable-loop-unrolling output.ll
llc -march=x86-64 -disable-inlining output.ll
```

#### 1.3 대상 아키텍처

```cpp
enum class TargetArchitecture {
    X86_64,      // 64-bit Intel/AMD
    ARM64,       // 64-bit ARM (RPi 4, M1 Mac)
    RISC_V,      // RISC-V (점점 인기)
    WASM,        // WebAssembly
};

std::string getMarch(TargetArchitecture arch) {
    switch(arch) {
        case TargetArchitecture::X86_64:   return "x86-64";
        case TargetArchitecture::ARM64:    return "aarch64";
        case TargetArchitecture::RISC_V:   return "riscv64";
        case TargetArchitecture::WASM:     return "wasm32";
        default:                           return "x86-64";
    }
}
```

---

### 2. Assembly → Object File

#### 2.1 AS (GNU Assembler) 도구 활용

```cpp
// Assembly를 Object 파일로 변환
int assembleToObject(const std::string& asm_file,
                     const std::string& obj_file) {
    std::string cmd = "as " + asm_file + " -o " + obj_file;
    return system(cmd.c_str());
}

// LLVM AS도 가능
// int ret = system(("llvm-as " + asm_file + " -o " + obj_file).c_str());
```

**선택지**:
1. **GNU as**: 널리 사용됨, 최적화 부족
2. **llvm-as**: LLVM 통합, 성능 좋음
3. **자체 어셈블러**: 복잡, 이점 없음

#### 2.2 구현 예제

```cpp
class BackendCompiler {
public:
    bool compileToObject(const std::string& ir_file,
                        const std::string& obj_file,
                        TargetArchitecture arch = TargetArchitecture::X86_64) {
        // Step 1: IR → Assembly
        std::string asm_file = obj_file + ".s";
        std::string ir_cmd = "llc -march=" + getMarch(arch) +
                            " " + ir_file + " -o " + asm_file;
        if (system(ir_cmd.c_str()) != 0) {
            std::cerr << "❌ Assembly 생성 실패" << std::endl;
            return false;
        }

        // Step 2: Assembly → Object
        std::string as_cmd = "as " + asm_file + " -o " + obj_file;
        if (system(as_cmd.c_str()) != 0) {
            std::cerr << "❌ Object 파일 생성 실패" << std::endl;
            return false;
        }

        // Step 3: 임시 Assembly 파일 삭제
        remove(asm_file.c_str());

        return true;
    }
};
```

---

### 3. Object File → Executable (Linking)

#### 3.1 LD (GNU Linker) 활용

```cpp
// Object 파일들을 실행 파일로 링킹
int linkToExecutable(const std::vector<std::string>& obj_files,
                     const std::string& exe_file) {
    std::string cmd = "gcc ";
    for (const auto& obj : obj_files) {
        cmd += obj + " ";
    }
    cmd += "-o " + exe_file;
    return system(cmd.c_str());
}
```

#### 3.2 표준 라이브러리 연결

```cpp
// Z-Lang 표준 라이브러리 링킹
bool linkWithStdlib(const std::vector<std::string>& obj_files,
                    const std::string& exe_file,
                    const std::string& stdlib_path) {
    std::string cmd = "gcc ";

    // 사용자 Object 파일
    for (const auto& obj : obj_files) {
        cmd += obj + " ";
    }

    // Z-Lang 표준 라이브러리
    cmd += stdlib_path + "/zlang_stdlib.a ";

    // C 표준 라이브러리 (필요시)
    cmd += "-lc -lm ";

    // 출력 파일
    cmd += "-o " + exe_file;

    return system(cmd.c_str()) == 0;
}
```

#### 3.3 정적 vs 동적 링킹

```cpp
// 정적 링킹 (권장 - Real-Time)
gcc -static app.o -o app_static

// 동적 링킹 (더 작은 바이너리)
gcc app.o -o app_dynamic
```

**Real-Time 시스템에서는 정적 링킹 권장**:
- ✅ 의존성 없음 (배포 용이)
- ✅ 실행 시간 결정적 (WCET 보장)
- ✅ 보안성 증대

---

### 4. 디버그 정보 생성

#### 4.1 DWARF 디버그 포맷

```cpp
// -g 플래그로 디버그 정보 포함
llc -march=x86-64 -g output.ll -o output.s
as output.s -o output.o  // DWARF 정보 유지

// 결과: GDB에서 소스 코드 수준 디버깅 가능
gdb ./app
(gdb) break main
(gdb) continue
```

#### 4.2 구현

```cpp
bool compileWithDebugInfo(const std::string& ir_file,
                          const std::string& obj_file) {
    std::string cmd = "llc -march=x86-64 -g " + ir_file +
                     " -o " + obj_file + ".s";
    system(cmd.c_str());

    cmd = "as " + obj_file + ".s -o " + obj_file;
    return system(cmd.c_str()) == 0;
}
```

---

## ⚡ Real-Time 최적화

### 1. 결정적 실행 시간 (Deterministic Execution)

```cpp
// WCET-Friendly 컴파일 옵션
std::string getWCETOptimization(bool enable_wcet) {
    if (!enable_wcet) return "";

    return " -disable-loop-unrolling"          // 루프 언롤링 비활성화
           " -disable-inlining"                // 인라인 비활성화
           " -disable-tail-calls"              // 꼬리 호출 비활성화
           " -O2";                             // 기본 최적화만
}
```

### 2. 메모리 레이아웃 최적화

```cpp
// Stack allocation만 사용 (No dynamic allocation)
// LLVM IR에서 이미 보장됨 (alloca만 사용)

// 로컬 변수 스택 프레임 분석
struct StackAnalysis {
    size_t local_vars_size;      // 로컬 변수 크기
    size_t max_call_stack_depth; // 호출 스택 깊이
    size_t total_stack_needed;   // 필요한 스택 크기
};
```

### 3. 캐시 고려사항

```cpp
// 캐시 친화적 코드 생성
// - 함수 크기 제한 (L1 I-cache 고려)
// - 데이터 지역성 최대화
// - 분기 예측 최적화

enum class CacheOptimization {
    NONE,           // 캐시 고려 없음
    L1_FRIENDLY,    // L1 캐시 맞춤 (32-64KB)
    L2_FRIENDLY,    // L2 캐시 맞춤 (256KB)
    NUMA_AWARE,     // NUMA 아키텍처 최적화
};
```

---

## 🔗 Linking 전략

### 1. 표준 라이브러리 구성

```
zlang_stdlib/
├── zlang_stdlib.a       # 정적 라이브러리
├── zlang_stdlib.so      # 동적 라이브러리 (선택)
├── zlang.h              # 헤더 파일
├── math/                # 수학 함수
├── io/                  # 입출력 함수
├── collections/         # 컬렉션 함수
└── runtime/             # 런타임 지원 함수
```

### 2. 외부 C 함수 연동

```z-lang
// Z-Lang에서 C 함수 호출
extern "C" {
    fn printf(format: string, ...) -> i32;
    fn malloc(size: i64) -> ptr;
    fn free(ptr: ptr) -> void;
}

fn main() -> i64 {
    printf("Hello from Z-Lang!\n");
    return 0;
}
```

### 3. 실행 파일 생성 전체 파이프라인

```cpp
class Z언Compiler {
public:
    int fullCompilation(const std::string& source_file,
                       const std::string& exe_file,
                       const Options& opts) {
        // 1단계: Lexing
        // ... (기존 구현)

        // 2단계: Parsing
        // ... (기존 구현)

        // 3단계: Semantic Analysis
        // ... (기존 구현)

        // 4단계: Codegen (IR 생성)
        // ... (기존 구현)

        // 5단계: Backend (Assembly 생성)
        std::string ir_file = "output.ll";
        std::string asm_file = "output.s";
        std::string obj_file = "output.o";

        // 5.1: IR → Assembly
        if (!compileToAssembly(ir_file, asm_file, opts)) {
            return 1;
        }

        // 5.2: Assembly → Object
        if (!assembleToObject(asm_file, obj_file)) {
            return 1;
        }

        // 6단계: Linking
        if (!linkToExecutable({obj_file}, exe_file, opts)) {
            return 1;
        }

        // 7단계: 임시 파일 정리
        remove(ir_file.c_str());
        remove(asm_file.c_str());
        remove(obj_file.c_str());

        return 0;
    }
};
```

---

## 📊 구현 로드맵

### Phase 2 (Week 2)

```
Day 1-2: Backend 기본 구현
  [ ] compileToAssembly() 구현
  [ ] assembleToObject() 구현
  [ ] linkToExecutable() 구현

Day 3-4: Real-Time 최적화
  [ ] WCET 옵션 통합
  [ ] 메모리 레이아웃 분석
  [ ] 캐시 최적화

Day 5: 테스트 & 검증
  [ ] 실제 프로그램 컴파일
  [ ] 성능 벤치마크
  [ ] 바이너리 호환성 확인
```

---

## ✅ 체크리스트

### 설계 완료 항목
- [x] Architecture 선택 (LLC + GAS + LD)
- [x] Optimization 전략 정의
- [x] Real-Time 고려사항 문서화
- [x] Linking 전략 설계

### 구현 예정 항목
- [ ] BackendCompiler 클래스 구현
- [ ] 다중 아키텍처 지원
- [ ] 디버그 정보 생성
- [ ] 성능 벤치마크

---

## 📚 참고 자료

- LLVM Backend: https://llvm.org/docs/CodeGenerator/
- GNU Assembler: https://sourceware.org/binutils/docs/as/
- WCET 분석: https://en.wikipedia.org/wiki/Worst-case_execution_time
- Real-Time Systems: Buttazzo et al., "Hard Real-Time Computing Systems"

---

**기록이 증명이다.** 📋

*설계 완료: 2026-02-27*
