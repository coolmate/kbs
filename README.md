# Dockerise SMTH BBS
## A quicker way to run up BBS without pain
### 水木清华BBS Dockerisation
### 源自 https://github.com/zhouqt/kbs
### 2019 01 19 Koumei Deng (https://koumei.net)

## Build the image:
```
$ docker build . -t kbsbbs
```

## Run the image with one script:
```
$ ./startup.sh
```

### If you don't want to use startup.sh and want to run it step by step, you can:
```
$ docker run -p 23:23 -it kbsbbs /bin/bash
```
After the command above, you are in the bash of the container, now you can start up bbsd in container
```
$ cd bin
$ ./miscd daemon
$ ./bbslogd
$ sudo ./bbsd -p 23
```

## Use another termianl to access the BBS (Set terminal to character encoding to GBK)
```
$ telnet localhost
```

## Initial SYSOP credential:
Username: `sysop`
Password: `password`