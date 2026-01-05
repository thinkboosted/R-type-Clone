# Base image used by Epitech
FROM epitechcontent/epitest-docker:latest

# Avoid interactive prompts during apt install
ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies required for Vcpkg libraries (SFML, OpenGL, Audio, etc.)
RUN apt-get update && apt-get install -y --no-install-recommends \
    curl zip unzip tar git pkg-config \
    libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev \
    libgl1-mesa-dev libglu1-mesa-dev \
    libflac-dev libogg-dev libvorbis-dev libopenal-dev libfreetype6-dev \
    libzmq3-dev \
    ca-certificates \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Setup Vcpkg globally to avoid bootstrapping in every CI run
# We install it in a system-wide accessible location
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH=$VCPKG_ROOT:$PATH

RUN git clone https://github.com/Microsoft/vcpkg.git $VCPKG_ROOT \
    && $VCPKG_ROOT/bootstrap-vcpkg.sh -disableMetrics \
    && chmod -R 777 $VCPKG_ROOT

# Reset frontend
ENV DEBIAN_FRONTEND=dialog
