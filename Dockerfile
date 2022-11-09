FROM ubuntu:latest
SHELL ["/bin/bash", "-c"]

ARG UID=1000
ARG GID=1000

RUN echo $UID $GID

RUN apt-get update

# tools for Rustle
RUN apt-get install -y \
    build-essential curl vim \
    llvm-14 clang-14 python3 python3-pip libudev-dev figlet

# tools for users
RUN apt-get install -y sudo vim git

# create user
RUN groupadd -g $GID -o rustle
RUN useradd -m -d /home/rustle -u $UID -g $GID -s /bin/bash rustle
RUN usermod -aG sudo rustle
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

USER rustle
WORKDIR /home/rustle

RUN curl --proto '=https' --tlsv1.2 https://sh.rustup.rs -sSf | sh -s -- -y
# RUN echo 'source /home/rustle/.cargo/env' >> /home/rustle/.bashrc

ENV PATH="/home/rustle/.cargo/bin:/home/rustle/.local/bin:$PATH"

RUN rustup default 1.64.0

RUN rustup target add wasm32-unknown-unknown
RUN cargo install rustfilt
RUN pip3 install pytablewriter tqdm toml
