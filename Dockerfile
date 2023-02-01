FROM ubuntu:jammy
SHELL ["/bin/bash", "-c"]

ARG UID=1000
ARG GID=1000

RUN echo $UID $GID

RUN apt-get update

# tools for Rustle
RUN apt-get install -y wget gnupg2
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/keyrings/llvm.asc
RUN echo "deb [signed-by=/etc/apt/keyrings/llvm.asc] http://apt.llvm.org/jammy/ llvm-toolchain-jammy-15 main" > /etc/apt/sources.list.d/llvm.list
RUN apt-get update
RUN apt-get install -y llvm-15 clang-15 python3 python3-pip libudev-dev figlet

# tools for users
RUN apt-get install -y sudo vim git build-essential curl

# create user
RUN groupadd -g $GID -o rustle
RUN useradd -m -d /home/rustle -u $UID -g $GID -s /bin/bash rustle
RUN usermod -aG sudo rustle
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

USER rustle
WORKDIR /home/rustle

# other components
RUN curl --proto '=https' --tlsv1.2 https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain 1.67.0
# RUN echo 'source /home/rustle/.cargo/env' >> /home/rustle/.bashrc

ENV PATH="/home/rustle/.cargo/bin:/home/rustle/.local/bin:$PATH"

RUN rustup target add wasm32-unknown-unknown
RUN cargo install rustfilt
RUN pip3 install pytablewriter tqdm toml
