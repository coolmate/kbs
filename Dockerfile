FROM ubuntu:12.04

MAINTAINER Koumei <koumeibb@gmail.com>

# disable interactive functions
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update && \
apt-get install -y zlib1g-dev  && \
apt-get install -y autoconf  && \
apt-get install -y automake  && \
apt-get install -y libtool  && \
apt-get install -y libgmp3c2  && \
apt-get install -y libgmp3-dev  && \
apt-get install -y openssl  && \
apt-get install -y apache2  && \
apt-get install -y libapache2-mod-php5  && \
apt-get install -y php5-dev  && \
apt-get install -y php5-gd  && \
apt-get install -y bison  && \
apt-get install -y byacc  && \
apt-get install -y libmysqlclient-dev  && \
apt-get install -y php5-mysql	&& \
apt-get install -y sendmail && \
apt-get install -y sudo

COPY ./kbs_bbs /home/kbs_bbs/

RUN mv /home/kbs_bbs /home/kbsbbs_src

RUN echo "root:root" | chpasswd
RUN groupadd bbs
RUN useradd -m -d /home/bbs -g bbs bbs
RUN adduser bbs sudo
RUN echo 'bbs:bbs' | chpasswd
RUN sudo chown -R bbs:bbs /home/kbsbbs_src

# RUN echo 'root ALL=(ALL) ALL \
#         bbs ALL=(ALL) NOPASSWD: ALL \
#         Defaults    env_reset \
#         Defaults    secure_path="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"' > /home/suders.txt
# ADD /home/suders.txt /etc/sudoers

RUN echo "bbs ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER bbs

## build bbs
WORKDIR /home/kbsbbs_src
RUN ./autogen.sh
RUN ./configure --prefix=/home/bbs --enable-site=fb2k-v2 \
              --with-php --with-mysql --enable-customizing
RUN make
RUN sudo make install
RUN sudo echo -e 'Y' | make install-home

WORKDIR /home/bbs

RUN sudo chown -R bbs:bbs /home/bbs 
RUN touch .PASSWDS .BOARDS
RUN sudo chmod 666 .PASSWDS .BOARDS

RUN sudo echo -e 'Go ahead!\npassword\npassword' | ./bin/bootstrap

EXPOSE 23
EXPOSE 22
EXPOSE 80

VOLUME /home/bbs

CMD ["/bin/sh"]
# COPY ./docker-entrypoint.sh /

# ENTRYPOINT ["/docker-entrypoint.sh"]

#add apache2 service to supervisor
#ADD supervisor/conf.d/apache2.conf /etc/supervisor/conf.d/
