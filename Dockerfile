FROM ubuntu:26.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y sudo just

WORKDIR /root/exchange-sim

COPY . .
RUN DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC just bootstrap

CMD ["/bin/bash"]
