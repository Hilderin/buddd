# Buddd Engine — CI Docker Image
#
# Provides GCC 16+, CMake, Ninja, and system dependencies for headless CI builds.
# Published to ghcr.io for fast CI runs via Docker layer caching.
#
# Build:
#   docker build -f docker/ci.Dockerfile -t ghcr.io/<org>/buddd-ci:latest .
#
# Run:
#   docker run --rm -v $(pwd):/workspace -w /workspace ghcr.io/<org>/buddd-ci:latest \
#     bash -c "cmake --preset debug -DBUDDD_HAS_DISPLAY=OFF -DCMAKE_CXX_COMPILER=g++-16 && cmake --build --preset debug && ctest --preset debug"

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies and toolchain
RUN apt-get update && apt-get install -y --no-install-recommends \
    # Build essentials
    build-essential \
    cmake \
    ninja-build \
    # GCC 16 from the toolchain PPA
    software-properties-common \
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
    g++-16 \
    gcc-16 \
    # OpenGL/display dependencies (needed for compilation, not runtime in headless mode)
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    # Cleanup to reduce image size
    && rm -rf /var/lib/apt/lists/*

# Set GCC 16 as the default compiler
ENV CC=gcc-16
ENV CXX=g++-16

# Default CMake preset: debug, headless
ENV CMAKE_PRESET=debug
ENV BUDDD_HAS_DISPLAY=OFF

# Make /workspace world-writable so the container can run with any UID
RUN mkdir -p /workspace && chmod 777 /workspace

# Default command: configure, build, test
CMD ["bash", "-c", "cmake --preset ${CMAKE_PRESET} -DBUDDD_HAS_DISPLAY=${BUDDD_HAS_DISPLAY} -DCMAKE_CXX_COMPILER=${CXX} && cmake --build --preset ${CMAKE_PRESET} && ctest --preset ${CMAKE_PRESET} --output-on-failure"]
