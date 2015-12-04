#qemu builder
FROM tianon/ubuntu-core:14.04
MAINTAINER Michael Vogt <support@neeo.com>
WORKDIR /builder

RUN apt-get update -qq && apt-get install -yqq --no-install-recommends python wget ssh git tcl8.5-dev tk8.5-dev build-essential lib32z1 lib32ncurses5 lib32bz2-1.0 lib32stdc++6 python-pip libxml2-dev libxslt-dev python-dev libncurses5-dev libncurses5
RUN git config --global http.sslVerify false

RUN easy_install awscli
ENV AWS_ACCESS_KEY_ID AKIAJFT6VVFDLAZA2BFA
ENV AWS_SECRET_ACCESS_KEY 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
RUN aws s3 cp s3://neeo-build/contiki-confidential-toolchain.tar.bz2 contiki-confidential-toolchain.tar.bz2

# unpack toolchain (jn5168)
RUN tar -xjvf contiki-confidential-toolchain.tar.bz2

# get arm toolchain
RUN apt-get install -yqq gcc-arm-linux-gnueabihf


# JN SDK Download URL: http://www.nxp.com/search?rows=10&type=keyword&q=jn-sw-4163&page=1&tab=All&filterChanged=pcon%5B%5D
RUN wget https://s3-eu-west-1.amazonaws.com/neeo-build/JN-SW-4163-SDK-Build1281.tar && tar -C / -xf JN-SW-4163-SDK-Build1281.tar
ENV SDK_BASE_DIR /JN-SW-4163
ENV TARGET jn516x

# workaround strange issue...
RUN cp /JN-SW-4163/Platform/Common/Build/Config.mk /JN-SW-4163/Platform/Common/Build/config.mk
