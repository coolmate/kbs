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
apt-get install -y sendmail

WORKDIR /home/kbsbbs

VOLUME /home/kbsbbs

CMD [ "/bin/sh" ]

#add apache2 service to supervisor
#ADD supervisor/conf.d/apache2.conf /etc/supervisor/conf.d/
