FROM ubuntu:20.04

ARG USER=ubuntu
ARG PASSWORD=1234
ENV HOME=/home/$USER

RUN apt update && \
    apt upgrade -y && \
    apt install -y sudo gdb vim wget make gcc gdb valgrind

RUN useradd --create-home --shell /bin/bash $USER && \
    usermod -aG sudo $USER && \
    echo "$USER:$PASSWORD" | chpasswd

USER $USER
WORKDIR $HOME/workspace

COPY . .
CMD ["/bin/bash"]
