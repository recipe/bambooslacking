FROM ubuntu:18.04

ARG HOSTNAME=localhost
ENV TZ=Europe/Kiev

RUN apt-get update -y && apt-get upgrade -y

# install cmake
RUN apt-get install -y vim wget git build-essential libssl-dev
RUN wget https://github.com/Kitware/CMake/releases/download/v3.20.2/cmake-3.20.2.tar.gz \
    && tar -xzf cmake-3.20.2.tar.gz \
    && cd cmake-3.20.2 \
    && ./configure \
    && make \
    && make install \
    && cd .. \
    && rm -fr cmake-3.20.2 cmake-3.20.2.tar.gz

# change time zone
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y tzdata
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone \
    && dpkg-reconfigure --frontend noninteractive tzdata

# install libraries
RUN apt install -y gdb libboost-all-dev libleveldb-dev

# build cpprestsdk static library /usr/local/lib/libcpprest.a
RUN apt install -y libssl-dev ninja-build \
    && cd /usr/src \
    && git clone https://github.com/microsoft/cpprestsdk.git \
    && cd cpprestsdk \
    && git checkout tags/v2.10.16 \
    && git submodule update --init \
    && mkdir build.release \
    && cd build.release \
    && cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=0 \
    && ninja \
    && ninja install

# install googletest
RUN cd /usr/src \
    && apt install -y libgtest-dev valgrind \
    && wget https://github.com/google/googletest/archive/release-1.7.0.tar.gz \
    && tar xf release-1.7.0.tar.gz \
    && cd googletest-release-1.7.0 \
    && cmake -DBUILD_SHARED_LIBS=ON . \
    && make \
    && cp -a include/gtest /usr/include \
    && cp -a libgtest_main.so libgtest.so /usr/lib/

# generate a self signed SSL certificate
RUN openssl req -newkey rsa:2048 -new -nodes -x509 -sha256 -days 3650 \
    -keyout /etc/ssl/private/key.pem -out /etc/ssl/private/cert.pem \
    -subj "/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=${HOSTNAME}"

# install systemctl replacement for docker
RUN wget https://raw.githubusercontent.com/gdraheim/docker-systemctl-replacement/master/files/docker/systemctl.py -O /usr/local/bin/systemctl
RUN chmod a+x /usr/local/bin/systemctl

EXPOSE 80 443