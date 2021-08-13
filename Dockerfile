FROM centos:centos8.3.2011
# set up basic build environment
RUN yum install -y automake autoconf libtool make gcc gcc-c++ gdb libarchive cmake gdb-gdbserver&& \
    yum install -y libpmem libpmem-devel libpmemobj libpmemobj-devel libvmem libpmemobj++-devel &&\
    yum install -y git vim &&\
    yum install -y fuse fuse-common fuse-devel fuse-libs fuse-overlayfs && \
    mkdir /home/tablefs

# set up remote access
RUN yum install -y openssl openssh-server rsync && \
    sed -ri 's/^PermitRootLogin\s+.*/PermitRootLogin yes/' /etc/ssh/sshd_config &&\
    cmake --version && \
    g++ -v && \
    echo 'root:123456' | chpasswd && \
    mkdir /var/run/sshd
#    mkdir /tmp/clion
RUN ssh-keygen -t rsa -f /etc/ssh/ssh_host_rsa_key -N "" && \
    ssh-keygen -t ecdsa -f /etc/ssh/ssh_host_ecdsa_key -N "" && \
    ssh-keygen -t ed25519 -f /etc/ssh/ssh_host_ed25519_key -N ""
EXPOSE 22
#CMD ["/usr/bin/rsync --daemon --config=/etc/rsync.conf"]
CMD ["/usr/sbin/sshd", "-D"]
#CMD ["/bin/bash"]