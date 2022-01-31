FROM ubuntu:latest
RUN apt update
RUN DEBIAN_FRONTEND=noninteractive apt install -y gcc make git binutils libc6-dev gdb sudo nasm
RUN adduser --disabled-password --gecos '' user
RUN echo 'user ALL=(root) NOPASSWD:ALL' > /etc/sudoers.d/user
RUN echo 'set disassembly-flavor intel\nlayout asm\nlayout reg\n' > /home/user/.gdbinit
USER user
WORKDIR /home/user
