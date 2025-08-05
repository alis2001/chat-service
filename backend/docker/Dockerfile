FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libpqxx-dev \
    libssl-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build application with debugging
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc) && \
    ls -la && \
    echo "Build complete, files in build directory:" && \
    find . -name "*caffis*" -type f

# Expose port
EXPOSE 5004

# Run application - use absolute path to be sure
CMD ["/app/build/caffis_chat"]