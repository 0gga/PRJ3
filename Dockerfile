FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    ninja-build \
    gdb \
    build-essential

WORKDIR /project

COPY . /project

# Configure + build ONLY asioServer target
RUN cmake -S . -B build -G Ninja \
    -DASIOSERVER_ONLY=ON

RUN cmake --build build --target asioServer

CMD ["./source/asioServer/bin/asioServer"]