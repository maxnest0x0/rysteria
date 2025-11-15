FROM mcr.microsoft.com/devcontainers/base:debian

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive && \
    apt-get install --assume-yes --no-install-recommends \
        npm nodejs cmake clang libwebsockets-dev libclang-rt-dev

RUN npm install --global http-server

RUN cd /opt && git clone https://github.com/emscripten-core/emsdk && \
    cd emsdk && ./emsdk install latest && ./emsdk activate latest

ENV PATH=/opt/emsdk:/opt/emsdk/upstream/emscripten:$PATH
