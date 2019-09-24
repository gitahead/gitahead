FROM ubuntu:19.04

RUN apt update; apt -y install locales
RUN sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && locale-gen
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

#RUN apt upgrade; apt -y install vim nano
#RUN apt install -y build-essential libfontconfig1 mesa-common-dev libglu1-mesa-dev qt5-default ninja-build
#RUN apt install -y git cmake libgit2* libssh2* libsecret* gnome-keyring* libgnome-keyring-dev libcmark-dev http-parser*

COPY . /gitahead

WORKDIR /gitahead
ENTRYPOINT ["/bin/bash"]
