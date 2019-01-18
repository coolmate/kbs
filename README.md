# Dockerize SMTH BBS
## A quicker way to run up BBS without pain

## Build the image:
```
$ docker build . -t kbsbbs
```

## Run the image: (Go into the container)
```
$ ./kbsbbssh.sh
```

## After jump into the container, start up bbsd in container
```
$ cd bin
$ ./miscd daemon
$ ./bbslogd
$ sudo ./bbsd -p 23
```