#include "bbs.h"

char IP[20];

int check(struct userec *uentp, char *arg)
{
    if(strstr(uentp->lasthost, IP)!=NULL) {
        printf("%s\t%s\n", uentp->userid, uentp->lasthost);
    }
    return 0;
}

int main(int argc, char **argv)
{
    if(argc<=1) return 0;
    strcpy(IP, argc[1]);
    chdir(BBSHOME);
    resolve_boards();
    resolve_ucache();
    apply_users(check,NULL);
    return 0;
}

