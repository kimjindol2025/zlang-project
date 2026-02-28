# Z-Lang 설치 가이드

**작성일**: 2026-02-27
**버전**: 0.1-Dev
**대상**: Ubuntu/Debian, CentOS/RHEL, macOS

---

## 🚀 빠른 설치 (권장)

```bash
# 1. 저장소 클론
git clone https://gogs.dclub.kr/kim/zlang.git
cd zlang

# 2. 설치 스크립트 실행
chmod +x install.sh
./install.sh

# 3. 검증
zlang --help
```

---

## 📦 수동 설치

### Step 1: 의존성 설치

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    llvm-14-dev \
    llvm-14
```

#### CentOS/RHEL
```bash
sudo dnf groupinstall -y "Development Tools"
sudo dnf install -y \
    cmake \
    git \
    llvm-devel \
    llvm-libs
```

#### macOS
```bash
# Homebrew 설치 (https://brew.sh)
brew install cmake llvm git
```

### Step 2: 소스 코드 다운로드

```bash
git clone https://gogs.dclub.kr/kim/zlang.git
cd zlang
```

### Step 3: 빌드

#### 방식 1: CMake (권장)
```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -- -j$(nproc)
cd ..
```

#### 방식 2: 직접 빌드 스크립트
```bash
chmod +x build.sh
./build.sh
```

#### 방식 3: Docker
```bash
docker build -t zlang:latest .
docker run -it zlang:latest --help
```

### Step 4: 설치

```bash
# 바이너리를 PATH에 추가
sudo cp zlang /usr/local/bin/
sudo chmod +x /usr/local/bin/zlang

# 또는 brew로 설치 (macOS)
ln -s $(pwd)/zlang /usr/local/bin/zlang
```

### Step 5: 검증

```bash
zlang --help
```

출력 예시:
```
【 Z-Lang 컴파일러 】

사용법: zlang <source.z> [옵션]

옵션:
  -o <file>           출력 파일명 (기본값: output.ll)
  -v, --verbose       상세 출력
  --emit-ir           LLVM IR 파일 생성 (.ll)
  --emit-object       Object 파일 생성 (.o)
  --execute           컴파일 후 실행
  -h, --help          도움말
```

---

## 🔧 빌드 옵션

### build.sh 옵션

```bash
# Debug 빌드
./build.sh --debug

# 병렬도 지정
./build.sh -j 8

# 빌드 디렉토리 초기화
./build.sh --clean

# 상세 로그
./build.sh --verbose

# GCC 직접 컴파일
./build.sh --gcc

# CMake 사용
./build.sh --cmake
```

### CMake 옵션

```bash
# 최적화 빌드
cmake .. -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=/usr/lib/llvm-14/

# Debug 빌드
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 컴파일 데이터베이스 생성 (IDE 지원)
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

---

## 🐳 Docker를 사용한 설치

### Dockerfile 사용

```bash
# 이미지 빌드
docker build -t zlang:0.1 .

# 컨테이너 실행
docker run -it zlang:0.1 --help

# 바이너리 추출
docker create --name zlang_extract zlang:0.1
docker cp zlang_extract:/zlang/build/zlang .
docker rm zlang_extract
```

### Docker Compose

```yaml
version: '3'
services:
  zlang:
    build: .
    volumes:
      - ./examples:/examples
    working_dir: /examples
    entrypoint: /zlang/build/zlang
```

```bash
docker-compose run zlang test_wcet.z -o output.ll
```

---

## 🎯 설치 후 첫 번째 프로그램

### 1. 간단한 프로그램 작성

```z-lang
// hello.z
fn main() -> i64 {
    return 42;
}
```

### 2. 컴파일

```bash
zlang hello.z -o hello.ll --verbose
```

### 3. LLVM IR 확인

```bash
cat hello.ll
```

출력:
```llvm
define i64 @main() {
entry:
  ret i64 42
}
```

### 4. 더 복잡한 예제

```z-lang
// fibonacci.z
fn fibonacci(n: i64) -> i64 {
    if n <= 1 {
        return n;
    }

    let a: i64 = 0;
    let b: i64 = 1;
    let i: i64 = 2;

    while i <= n {
        let temp: i64 = a + b;
        a = b;
        b = temp;
        i = i + 1;
    }

    return b;
}
```

```bash
zlang fibonacci.z -o fib.ll --verbose
```

---

## 📋 문제 해결

### "cmake: command not found"

```bash
# Ubuntu/Debian
sudo apt-get install cmake

# CentOS/RHEL
sudo dnf install cmake

# macOS
brew install cmake
```

### "llvm-config: command not found"

```bash
# Ubuntu/Debian
sudo apt-get install llvm-14-dev

# CentOS/RHEL
sudo dnf install llvm-devel

# macOS
brew install llvm

# macOS에서 PATH 설정
export LLVM_DIR=/usr/local/opt/llvm
export PATH="/usr/local/opt/llvm/bin:$PATH"
```

### "Exec format error" (바이너리 호환성)

현재 알려진 이슈: x86-64 바이너리를 ARM64에서 실행 불가

**해결책**:
1. 현재 아키텍처에서 재빌드: `./build.sh --clean`
2. Docker 사용 (권장)
3. 다른 머신에서 빌드

---

## ✅ 설치 검증 체크리스트

- [ ] CMake 설치됨 (`cmake --version`)
- [ ] LLVM 설치됨 (`llvm-config --version`)
- [ ] G++ 설치됨 (`g++ --version`)
- [ ] 빌드 성공 (`./zlang --help`)
- [ ] 경로 설정됨 (`which zlang`)
- [ ] 테스트 통과 (`zlang test_wcet.z -o test.ll`)

---

## 🚀 다음 단계

1. [GETTING_STARTED.md](GETTING_STARTED.md) - 사용 시작하기
2. [README.md](README.md) - 프로젝트 개요
3. [BACKEND_DESIGN.md](BACKEND_DESIGN.md) - 아키텍처 설계

---

**기록이 증명이다.** 📋

*최신 업데이트: 2026-02-27*
