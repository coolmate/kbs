# Dockerise SMTH BBS
## A quicker way to run up BBS without pain
### 水木清华BBS Dockerisation
### 源自 https://github.com/zhouqt/kbs
### 2019 01 19 Koumei Deng (https://koumei.net)

## Build the image:
```
$ docker build . -t kbsbbs
```

## Run the image: (Go into the container)
```
$ ./kbsbbssh.sh
```
or just run from the command line:
```
$ docker run -p 23:23 -it kbsbbs /bin/bash
```

## After jump into the container, start up bbsd in container
```
$ cd bin
$ ./miscd daemon
$ ./bbslogd
$ sudo ./bbsd -p 23
```

## Use termianl to access the BBS (Set terminal to character encoding to GBK)
```
$ telnet localhost
```