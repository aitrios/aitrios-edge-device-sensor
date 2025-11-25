FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

# Explicitly set shell for consistent && behavior
SHELL ["/bin/bash", "-c"]

# Define build arguments and environment variables for maintainability
ARG CMAKE_VERSION=3.31.6
ARG NODE_VERSION=20.x
ARG ALLURE_VERSION=2.34.1

# Set non-interactive mode, timezone, and locale
ENV DEBIAN_FRONTEND=noninteractive \
    TZ=UTC \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    CMAKE_VERSION=${CMAKE_VERSION} \
    NODE_VERSION=${NODE_VERSION} \
    ALLURE_VERSION=${ALLURE_VERSION}

# Configure timezone and locale
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# build tools and unit test environment
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake meson ninja-build \
    pkg-config \
    gcc-13 \
    g++-13 \
    # Network and download tools
    curl \
    wget \
    ca-certificates \
    gnupg \
    # Version control
    git \
    # Utilities
    unzip \
    sudo \
    # Locale support
    locales \
    # Testing frameworks
    libgtest-dev \
    libgmock-dev \
    # Set GCC alternatives to use GCC-13 (compatible with GCOV 13.3.0)
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100 \
    && update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov-13 100 \
    # Configure locale
    && locale-gen en_US.UTF-8 \
    && update-locale LANG=en_US.UTF-8 \
    # Clean up apt cache
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install specific version of lcov (1.15-1) from source if not available in package manager
RUN LCOV_VERSION="1.15" \
    && wget https://github.com/linux-test-project/lcov/releases/download/v${LCOV_VERSION}/lcov-${LCOV_VERSION}.tar.gz \
    && tar -xzf lcov-${LCOV_VERSION}.tar.gz \
    && cd lcov-${LCOV_VERSION} \
    && make install \
    && cd .. \
    && rm -rf lcov-${LCOV_VERSION} lcov-${LCOV_VERSION}.tar.gz

# for documentation
RUN apt-get update && apt-get install -y --no-install-recommends \
    openjdk-17-jre \
    openjdk-11-jre \
    ruby-full \
    && gem install asciidoctor asciidoctor-diagram asciidoctor-diagram-plantuml --no-document \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install CMake from source using environment variable
WORKDIR /usr/local/src
RUN wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz \
    && tar -xzf cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz -C /usr/local --strip-components=1 \
    && rm cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz

# Install Node.js and Allure in one layer using environment variables
RUN curl -fsSL https://deb.nodesource.com/setup_${NODE_VERSION} | bash - \
    && apt-get install -y --no-install-recommends nodejs \
    && npm install -g allure-commandline@${ALLURE_VERSION} \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# test - google test installation
RUN git clone https://github.com/google/googletest.git -b v1.17.0 && \
    cd googletest && \
    mkdir build && \
    cd build && \
    cmake .. -DBUILD_SHARED_LIBS=ON && \
    make && \
    make install && \
    cd ../.. && \
    rm -rf googletest

# senscord dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    libxml2-dev \
    debhelper \
    devscripts \
    libcamera-dev \
    libjpeg-dev \
    libdrm-dev \
    libexif-dev \
    libtiff5-dev \
    libpng-dev \
    libboost-all-dev \
    python3-kconfiglib \
    nlohmann-json3-dev \
    python3-build \
    python3-stdeb \
    dh-python \
    python3-msgpack \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Use existing vscode user (UID 1000) for compatibility
# Set working directory
WORKDIR /workspace

# Change ownership of workspace to vscode user
RUN chown -R vscode:vscode /workspace

# Configure bash environment for vscode user
USER vscode
RUN echo "# Auto-navigate to test directory" >> ~/.bashrc \
    && echo "if [ -d '/workspace/test/scripts' ]; then cd /workspace/test/scripts; fi" >> ~/.bashrc \
    && echo "" >> ~/.bashrc \
    && echo "# Unit Test Environment Welcome Message" >> ~/.bashrc \
    && echo "echo '📋 SensCord Development Environment Ready!'" >> ~/.bashrc \
    && echo "echo '📂 Current directory: \$(pwd)'" >> ~/.bashrc \
    && echo "echo '🚀 Run ./run_unit_test.sh to execute all tests (from test/scripts)'" >> ~/.bashrc \
    && echo "echo '🧹 Run ./clean_test_artifacts.sh to clean up all test artifacts'" >> ~/.bashrc \
    && echo "echo ''" >> ~/.bashrc \
    && echo "" >> ~/.bashrc \
    && echo "# Convenient aliases for unit testing" >> ~/.bashrc \
    && echo "alias test-run='./run_unit_test.sh'" >> ~/.bashrc \
    && echo "alias test-clean='./clean_test_artifacts.sh'" >> ~/.bashrc \
    && echo "alias test-build='./exec_unit_test.sh build sample'" >> ~/.bashrc \
    && echo "alias devuser='echo \"Using vscode user (UID 1000) for compatibility\"'" >> ~/.bashrc \
    && echo "alias ll='ls -alF'" >> ~/.bashrc \
    && echo "alias la='ls -A'" >> ~/.bashrc \
    && echo "alias l='ls -CF'" >> ~/.bashrc

# Ensure vscode user has sudo privileges
USER root
RUN echo "vscode ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Verify installations and compatibility
RUN echo "=== Verifying installations ===" \
    && echo "Build tools:" \
    && cmake --version | head -1 \
    && gcc --version | head -1 \
    && gcov --version | head -1 \
    && echo "Coverage tools:" \
    && echo "LCOV: $(lcov --version | head -n 1)" \
    && echo "GCC/GCOV compatibility check:" \
    && GCC_VERSION=$(gcc -dumpversion) \
    && GCOV_VERSION=$(gcov --version | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1) \
    && echo "  GCC version: ${GCC_VERSION}" \
    && echo "  GCOV version: ${GCOV_VERSION}" \
    && GCC_MAJOR=$(echo ${GCC_VERSION} | cut -d. -f1) \
    && GCOV_MAJOR=$(echo ${GCOV_VERSION} | cut -d. -f1) \
    && if [ "${GCC_MAJOR}" = "${GCOV_MAJOR}" ]; then \
        echo "  ✓ GCC and GCOV major versions match (${GCC_MAJOR}) - coverage will work correctly"; \
    else \
        echo "  ⚠ GCC and GCOV major versions differ (GCC: ${GCC_MAJOR}, GCOV: ${GCOV_MAJOR}) - may cause coverage format issues"; \
    fi \
    && echo "Test frameworks:" \
    && echo "Google Test: Source available in /usr/src/gtest ($(ls /usr/src/gtest/ 2>/dev/null | wc -l || echo 0) files)" \
    && echo "Allure dependencies:" \
    && echo "Node.js: $(node --version)" \
    && echo "Java: $(java -version 2>&1 | head -1)" \
    && echo "Allure: $(allure --version 2>/dev/null | head -1)"

# Switch back to vscode user for dev container compatibility
USER vscode
