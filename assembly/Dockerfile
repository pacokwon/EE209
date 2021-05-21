FROM ubuntu:20.04

ARG USER=ubuntu
ARG PASSWORD=1234
ENV HOME=/home/$USER

RUN apt update && \
    apt upgrade -y && \
    apt install -y software-properties-common && \
    add-apt-repository ppa:deadsnakes/ppa && \
    apt install -y sudo python3.6 gdb vim wget make gcc gcc-multilib g++-multilib

RUN useradd --create-home --shell /bin/bash $USER && \
    usermod -aG sudo $USER && \
    echo "$USER:$PASSWORD" | chpasswd

USER $USER
WORKDIR $HOME/workspace

COPY . .
CMD ["/bin/bash"]
