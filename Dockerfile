FROM ubuntu:18.04 AS builder

ENV LD_LIBRARY_PATH=/usr/local/clang/lib:$LD_LIBRARY_PATH;

RUN apt-get update
RUN apt-get install -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test

RUN apt-add-repository ppa:cginternals/ppa -y;
RUN apt-add-repository ppa:cginternals/backports-ppa -y;
RUN apt-get update -qq;
RUN apt-get install -qq qt5-default libglm-dev libglbinding-dev libglobjects-dev libcpplocate-dev;

RUN apt-get install -y \
    cmake \
    cppcheck \
    gcc-5 \
    g++-5 \
    libfreetype6-dev \
    libpng-dev \
    libfontconfig1-dev

WORKDIR /usr/src/llassetgen

ENV PATH="/usr/local/opt/qt/bin:/usr/local/opt/glbinding:/usr/local/opt/globjects:$PATH"

COPY . .
RUN chmod +x ./configure

RUN unlink /usr/bin/gcc && \
    ln -s /usr/bin/gcc-5 /usr/bin/gcc && \
    unlink /usr/bin/g++ && \
    ln -s /usr/bin/g++-5 /usr/bin/g++
RUN ./configure
RUN ./configure release
RUN cmake --build build --target llassetgen-cmd

# second build stage with minimal dependencies for running the tool
FROM ubuntu:18.04

# circumvent EULA prompt for ttf-mscorefonts-installer
RUN echo ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true | debconf-set-selections
RUN apt-get update && apt-get install -y \
    curl \
    libgomp1 \
    libfreetype6 \
    libfontconfig1 \
    fontconfig \
    fonts-roboto \
    fonts-open-sans \
    ttf-mscorefonts-installer \
    sudo \
&& rm -rf /var/lib/apt/lists/*

# install all Google Fonts
RUN curl https://raw.githubusercontent.com/qrpike/Web-Font-Load/master/install.sh | bash

COPY --from=builder /usr/src/llassetgen/build/llassetgen-cmd .
COPY --from=builder /usr/src/llassetgen/build/libllassetgen.so.1 .

CMD ["./llassetgen-cmd"]
