# Z-Lang LLVM 컴파일러 빌드 환경

FROM ubuntu:22.04

# 기본 도구 설치
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    llvm-14 \
    clang-14 \
    llvm-14-dev \
    libllvm14 \
    && rm -rf /var/lib/apt/lists/*

# Z-Lang 소스 복사
WORKDIR /zlang
COPY . .

# 빌드 디렉토리 생성
RUN mkdir -p build

# CMake 빌드
WORKDIR /zlang/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# 바이너리 위치
RUN ls -lah /zlang/build/zlang

# 테스트
RUN /zlang/build/zlang --help

# 기본 명령어
ENTRYPOINT ["/zlang/build/zlang"]
CMD ["--help"]
