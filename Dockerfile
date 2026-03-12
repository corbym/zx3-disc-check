# Multi-stage Dockerfile for zx3-disk-test
# Stage 1: Build environment with z88dk and ZEsarUX

FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV ZESARUX_VERSION=10.3

# Install build dependencies and runtime requirements
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    wget \
    curl \
    python3 \
    python3-pip \
    perl \
    bison \
    flex \
    libboost-all-dev \
    libsdl2-dev \
    libpng-dev \
    zlib1g-dev \
    libgtk-3-dev \
    pkg-config \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install z88dk from GitHub releases
RUN cd /tmp && \
    wget -q https://github.com/z88dk/z88dk/releases/download/v2.3/z88dk-2.3_3.1_amd64.deb && \
    dpkg -i z88dk-2.3_3.1_amd64.deb && \
    rm z88dk-2.3_3.1_amd64.deb && \
    z88dk-setup-prefix 2>&1 || true && \
    # Verify z88dk was installed and zcc is available after setup
    /root/.zcc/bin/zcc --version || echo "Warning: /root/.zcc/bin/zcc not found, trying fallback" && \
    which zcc || find /opt /usr -name "zcc" -type f 2>/dev/null | head -1 || true

ENV PATH=/root/.zcc/bin:$PATH
ENV ZCCCFG=/root/.zcc/lib/config.toml

# Install ZEsarUX from source (headless build)
RUN cd /tmp && \
    wget -q https://github.com/chernandezba/zesarux/releases/download/v${ZESARUX_VERSION}/zesarux_${ZESARUX_VERSION}_linux_x86_64.zip && \
    unzip -q zesarux_${ZESARUX_VERSION}_linux_x86_64.zip && \
    mv zesarux /opt/zesarux && \
    chmod +x /opt/zesarux && \
    rm zesarux_${ZESARUX_VERSION}_linux_x86_64.zip && \
    # Test that zesarux works with --help
    /opt/zesarux --helpall > /dev/null && echo "ZEsarUX installed successfully"

# Final stage: Runtime image
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies only
# Note: --vo null --ao null means no SDL2/GTK needed; just min libs for zesarux
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy z88dk and ZEsarUX from builder
COPY --from=builder /root/.zcc /root/.zcc
COPY --from=builder /opt/zesarux /opt/zesarux

# Set up environment - z88dk should be available in PATH from builder
ENV PATH=/root/.zcc/bin:/opt:$PATH
ENV ZCCCFG=/root/.zcc/lib/config.toml

# Create working directory
WORKDIR /workspace

# Verify both tools work (zcc and zesarux)
RUN zcc --version && /opt/zesarux --helpall > /dev/null && echo "All tools verified"

# Default command: bash for interactive use or script execution
CMD ["/bin/bash"]
