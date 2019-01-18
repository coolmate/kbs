#! /bin/bash

docker run -d -p 23:23 -it kbsbbs /bin/bash -c "/home/bbs/bin/miscd daemon ; /home/bbs/bin/bbslogd ; sudo /home/bbs/bin/bbsd -p 23 ; /bin/bash "
