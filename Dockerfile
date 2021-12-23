FROM ubuntu:20.04 AS builder

<<<<<<< HEAD
RUN apt-get update; \
  apt-get install -y \
    build-essential \
    cmake \
    git-core \
    libboost-atomic1.65-dev \
    libboost-chrono1.65-dev \
    libboost-date-time1.65-dev \
    libboost-filesystem1.65-dev \
    libboost-system1.65-dev \
    libboost-thread1.65-dev \
    libglew-dev \
    liblua5.1-0-dev \
    libncurses5-dev \
    libopenal-dev \
    libssl-dev \
    libvorbis-dev \
    zlib1g-dev; \
  apt-get clean && apt-get autoclean

WORKDIR /
RUN git clone --branch release-3.0.2 --depth 1 https://github.com/icculus/physfs.git
WORKDIR /physfs/build/
RUN cmake ..
RUN make -j$(nproc)
RUN make install
=======
RUN export DEBIAN_FRONTEND=noninteractive \
	&& ln -fs /usr/share/zoneinfo/America/Sao_Paulo /etc/localtime

RUN apt-get update && apt-get install -y \
	build-essential \
	cmake \
	curl \
	git \
	libglew-dev \
	liblua5.1-0-dev \
	libluajit-5.1-dev \
	libncurses5-dev \
	libopenal-dev \
	libssl-dev \
	libvorbis-dev \
	mercurial \
	tar \
	unzip \
	zip \
	zlib1g-dev \
	&& dpkg-reconfigure --frontend noninteractive tzdata \
	&& apt-get clean && apt-get autoclean

WORKDIR /opt
RUN git clone https://github.com/microsoft/vcpkg
RUN ./vcpkg/bootstrap-vcpkg.sh

WORKDIR /opt/vcpkg
COPY vcpkg.json /opt/vcpkg/
RUN /opt/vcpkg/vcpkg --feature-flags=binarycaching,manifests,versions install

COPY ./ /otclient/

RUN apt-get install -y libluajit-5.1-dev
>>>>>>> 4a2b0f4b... Sync with current client

WORKDIR /otclient/build/
RUN cmake -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake ..
RUN make -j$(nproc)

FROM ubuntu:20.04

RUN apt-get update; \
	apt-get install -y \
	libglew2.1 \
	libopenal1 \
	libopengl0 \
	&& apt-get clean && apt-get autoclean

COPY --from=builder /otclient/build/otclient /otclient/bin/otclient
COPY ./data/ /otclient/data/.
COPY ./mods/ /otclient/mods/.
COPY ./modules/ /otclient/modules/.
COPY ./init.lua /otclient/.
WORKDIR /otclient
CMD ["./bin/otclient"]
