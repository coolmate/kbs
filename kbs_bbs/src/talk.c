/*    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "bbs.h"
#ifdef lint
#include <sys/uio.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define M_INT 8         /* monitor mode update interval */
#define P_INT 20        /* interval to check for page req. in talk/chat */
extern int iscolor;
extern int numf,friendmode;

int talkidletime=0;
int ulistpage;
int friend_query();
int friend_mail();
int friend_dele();
int friend_add();
int friend_edit();
int friend_help();
int badlist(); /* Bigman 2000.12.26 */

/* Bigman 2000.9.15 Talk的记录 */
#ifdef TALK_LOG
void    do_log();
int     talkrec = -1;
char    partner[IDLEN + 1];
#endif
struct one_key  friend_list[] = {
                                    'r',        friend_query,
                                    'm',        friend_mail,
                                    'M',        friend_mail,
                                    'a',        friend_add,
                                    'A',        friend_add,
                                    'd',        friend_dele,
                                    'D',        friend_dele,
                                    'E',        friend_edit,
                                    'h',        friend_help,
                                    'H',        friend_help,
                                    '\0',       NULL
                                } ;


struct talk_win {
    int         curcol, curln;
    int         sline, eline;
};

int     nowmovie;
int     bind(/*int,struct sockaddr *, int*/) ;
char    *sysconf_str();


extern int t_columns;
char    *talk_uent_buf;

/* begin - jjyang */
char save_page_requestor[STRLEN];
/* end - jjyang */
char npage_requestor[STRLEN];

/*---	changed to isidhidden by period	2000-10-20	---*
int
ishidden(user)
char *user;
{
    int tuid;
    struct user_info uin;

    if (!(tuid = getuser(user))) return 0;
    search_ulist( &uin, t_cmpuids, tuid );
    return( uin.invisible );
}
---*/

char
canpage(friend, pager)
int friend,pager;
{
    if ((pager&ALL_PAGER) || HAS_PERM(currentuser,PERM_SYSOP)) return YEA;
    if ((pager&FRIEND_PAGER))
    {
        if(friend)
            return YEA;
    }
    return NA;
}

int
listcuent(struct user_info *uentp,char* arg,int pos)
{
    if(uentp == NULL) {
        CreateNameList() ;
        return 0;
    }
    /*    if(uentp->uid == usernum)		rem by Haohmaru,00.5.26,这样才能给自己发msg
            return 0;
    */    if(!uentp->active || !uentp->pid)
        return 0;
    if(uentp->mode == ULDL)
        return 0;
    if(!HAS_PERM(currentuser,PERM_SEECLOAK) && uentp->invisible)
        return 0;
    AddNameList( uentp->userid );
    return 0 ;
}

void
creat_list()
{
    listcuent(NULL,0,0) ;
    apply_ulist_addr( listcuent ,0);
}

int
t_pager()
{

    if(uinfo.pager&ALL_PAGER)
    {
        uinfo.pager&=~ALL_PAGER;
        if( DEFINE(currentuser,DEF_FRIENDCALL))
            uinfo.pager|=FRIEND_PAGER;
        else
            uinfo.pager&=~FRIEND_PAGER;
    }else
    {
        uinfo.pager|=ALL_PAGER;
        uinfo.pager|=FRIEND_PAGER;
    }

    if ( !uinfo.in_chat && uinfo.mode!=TALK) {
        move( 1, 0 );
        prints( "您的呼叫器 (pager) 已经[1m%s[m了!",
                (uinfo.pager&ALL_PAGER) ? "打开" : "关闭" );
        pressreturn();
    }
    UPDATE_UTMP(pager,uinfo);
    return 0 ;
}

/*Add by SmallPig*/
/*此函数只负责列印说明档，并不管清除或定位的问题。*/
int
show_user_plan(userid)
char userid[IDLEN];
{
    int i;
    char pfile[STRLEN],pbuf[256];
    FILE *pf;

    sethomefile(pfile,userid,"plans");
    if ((pf = fopen(pfile, "r")) == NULL)
    {
        prints("[36m没有个人说明档[m\n");
        /*fclose(pf);*/ /* Leeward 98.04.20 */
        return NA;
    }
    else
    {
        prints("[36m个人说明档如下：[m\n");
        for (i=1; i<=MAXQUERYLINES; i++)
        {
            if (fgets(pbuf, sizeof(pbuf), pf))
                prints("%s", pbuf);
            else break;
        }
        fclose(pf);
        return YEA;
    }
}

int t_printstatus(struct user_info* uentp,int* arg,int pos)
{
    if(uentp->invisible==1)
    {
        if(!HAS_PERM(currentuser,PERM_SEECLOAK))
	        return COUNT;
    }
    (*arg)++;
    if(*arg==1)
        strcpy(genbuf,"目前在站上，状态如下：\n");
    if (uentp->invisible)
        strcat(genbuf,"[32m隐身中   [m");
    else {
    	char buf[80];
    	sprintf(buf,"[1m%s[m ", modestring(uentp->mode,
                                     uentp->destuid, 0,/* 1->0 不显示聊天对象等 modified by dong 1996.10.26 */
                                     (uentp->in_chat ? uentp->chatid : NULL)));
        strcat(genbuf,buf);
    }
    if((*arg)%8==0)
           strcat(genbuf,"\n");
    UNUSED_ARG(pos);
    return COUNT;
}

/* Modified By Excellent*/
int
t_query(q_id)
char q_id[IDLEN];
{
    char        uident[STRLEN], *newline ;
    int         tuid=0;
    int         exp,perf;/*Add by SmallPig*/
    struct user_info uin;
    char qry_mail_dir[STRLEN];
    char planid[IDLEN+2];
    char permstr[10];
    char exittime[40];
    time_t exit_time,temp/*Haohmaru.98.12.04*/;
    int logincount,seecount;
    struct userec* lookupuser;

    if(uinfo.mode!=LUSERS&&uinfo.mode!=LAUSERS&&uinfo.mode!=FRIEND&&uinfo.mode!=READING &&uinfo.mode!=MAIL&&uinfo.mode!=RMAIL&&uinfo.mode!=GMENU){
        modify_user_mode( QUERY );
        refresh();
        /*    count = shortulist(NULL);*/
        move(2,0);
        clrtobot();
        prints("<输入使用者代号, 按空白键可列出符合字串>\n");
        move(1,0);
        clrtoeol();
        prints("查询谁: ");
        usercomplete(NULL,uident);
        if(uident[0] == '\0') {
            clear() ;
            return 0 ;
        }
    }else
    {
	char* p;
	for (p=q_id;*p;p++) if (*p==' ') { *p=0; break;};
        strcpy(uident,q_id);
    }
    if(!(tuid = getuser(uident,&lookupuser))) {
        move(2,0) ;
        clrtoeol();
        prints("[1m不正确的使用者代号[m\n") ;
        pressanykey() ;
        move(2,0) ;
        clrtoeol() ;
        return -1 ;
    }
    uinfo.destuid = tuid ;
/*    UPDATE_UTMP(destuid,uinfo);  I think it is not very importance.KCN*/

/*    search_ulist( &uin, t_cmpuids, tuid );*/

    move(1,0);
    clrtobot();
    setmailfile(qry_mail_dir, lookupuser->userid, DOT_DIR);

    exp=countexp(lookupuser);
    perf=countperf(lookupuser);
    /*---	modified by period	2000-11-02	hide posts/logins	---*/
#ifndef _DETAIL_UINFO_
    if((!HAS_PERM(currentuser,PERM_ADMINMENU)) && strcmp(lookupuser->userid, currentuser->userid))
        prints( "%s (%s)", lookupuser->userid, lookupuser->username);
    else
#endif
        prints( "%s (%s) 共上站 %d 次，发表过 %d 篇文章",
                lookupuser->userid, lookupuser->username,
                lookupuser->numlogins,lookupuser->numposts);
    strcpy(planid,lookupuser->userid);
    if( (newline = strchr(genbuf, '\n')) != NULL )
        *newline = '\0';
    seecount=0;
	logincount=apply_utmp(t_printstatus,10,lookupuser->userid,&seecount);
    /* 获得离线时间 Luzi 1998/10/23 */
    exit_time = get_exit_time(lookupuser->userid,exittime);
    if( (newline = strchr(exittime, '\n')) != NULL )
        *newline = '\0';

    if (exit_time <= lookupuser->lastlogin)
    	if (logincount!=seecount)
	    {
    	    temp=lookupuser->lastlogin+((lookupuser->numlogins+lookupuser->numposts)%100)+60;
        	strcpy(exittime,ctime(&temp));/*Haohmaru.98.12.04.让隐身用户看上去离线时间比上线时间晚60到160秒钟*/
	        if( (newline = strchr(exittime, '\n')) != NULL )
    	        *newline = '\0';
	    } else
    	    strcpy(exittime,"因在线上或非常断线不详");
    prints( "\n上次在  [%s] 从 [%s] 到本站一游。\n离线时间[%s] ", Ctime(lookupuser->lastlogin),
            ((lookupuser->lasthost[0] == '\0') /*|| DEFINE(currentuser,DEF_HIDEIP)*/ ? "(不详)" : lookupuser->lasthost),/*Haohmaru.99.12.18. hide ip*/
            exittime);
    /* SNOW CHANGE AT 10.20 (CHANGE THIS MSG)
        prints("信箱：[[5m%2s[m]，经验值：[%d](%s) 表现值：[%d](%s) 生命力：[%d]%s\n"
                ,(check_query_mail(qry_mail_dir)==1)? "信":"  ",exp,cexp(exp),perf,
                  cperf(perf),compute_user_value(lookupuser),
               (lookupuser->userlevel & PERM_SUICIDE)?" (自杀中)":" ");
    */
    uleveltochar(&permstr,lookupuser);
    prints("信箱：[[5m%2s[m] 生命力：[%d] 身份: [%s]%s\n",
           (check_query_mail(qry_mail_dir)==1)? "信":"  ",
           compute_user_value(lookupuser),
           permstr,(lookupuser->userlevel & PERM_SUICIDE)?" (自杀中)":"。");

#if defined(QUERY_REALNAMES)
    if (HAS_PERM(currentuser,PERM_BASIC))
        prints("Real Name: %s \n",lookupuser->realname);
#endif

	if ((genbuf[0])&&seecount) {
		prints(genbuf);
		prints("\n");
	}
    show_user_plan(planid);

    if (uinfo.mode!=LUSERS&&uinfo.mode!=LAUSERS&&uinfo.mode!=FRIEND&&uinfo.mode!=GMENU)
        pressanykey();

    uinfo.destuid = 0;
    return 0;
}

int
count_visible_active(struct user_info *uentp,int* count,int pos)
{
    if(!uentp->active || !uentp->pid)
        return 0 ;
    if(!(!HAS_PERM(currentuser,PERM_SEECLOAK) && uentp->invisible))
    	(*count)++ ;
    return 1 ;
}


int
alcounter(struct user_info *uentp ,char* arg,int pos)
{
    int canseecloak;

    if(!uentp->active || !uentp->pid)
        return 0 ;

    canseecloak=(!HAS_PERM(currentuser,PERM_SEECLOAK) && uentp->invisible)?0:1;
    if(myfriend(uentp->uid,NULL))
    {
        count_friends++ ;
        if(!canseecloak)
            count_friends--;
    }
    count_users++ ;
    if(!canseecloak)
        count_users--;
    return 1 ;
}

int
num_alcounter()
{
	count_friends=0;
	count_users=0;
    apply_ulist_addr( alcounter,0 ) ;
    return;
}

int
num_user_logins(uid)
char *uid;
{
    strcpy(save_page_requestor,uid);
    return apply_utmp(NULL,0,uid,0) ;
}
int
num_visible_users()
{
	int count;

	count = 0;
    apply_ulist_addr( (APPLY_UTMP_FUNC)count_visible_active,(char*)&count) ;
    return count;
}

int
t_cmpuids(int uid ,struct user_info *up )
{
    return (up->active && uid == up->uid) ;
}

int
/*  modified by netty  */
t_talk()
{
    int  netty_talk ;

    refresh();
    netty_talk = ttt_talk();
    clear() ;
    return (netty_talk);
}

struct _tag_talk_showstatus {
	int count;
	int pos[20];
};

int talk_showstatus(struct user_info * uentp,struct _tag_talk_showstatus* arg,int pos)
{
    char buf[80];
    if (uentp->invisible && !HAS_PERM(currentuser,PERM_SEECLOAK))
    	return 0;
    arg->pos[arg->count++]=pos;

    sprintf(buf,"(%d) 目前状态: %s, 来自: %s \n", arg->count,
        modestring(uentp->mode, uentp->destuid, 0, /* 1->0 不显示聊天对象等 modified by dong 1996.10.26 */
        uentp->in_chat ? uentp->chatid : NULL), uentp->from );
    strcat(genbuf,buf);
	return COUNT;
}

int
ttt_talk(userinfo)
struct user_info *userinfo ;
{
    char uident[STRLEN] ;
    char test[STRLEN];
    int tuid, ucount, unum, tmp ;
    struct user_info uin ;
    struct _tag_talk_showstatus ts;

    move(1,0);
    clrtobot();
    if(uinfo.mode!=LUSERS&&uinfo.mode!=FRIEND)
    {
        move(2,0) ;
        prints("<输入使用者代号>\n") ;
        move(1,0) ;
        clrtoeol() ;
        prints("跟谁聊天: ") ;
        creat_list() ;
        namecomplete(NULL,uident) ;
        if(uident[0] == '\0') {
            clear() ;
            return 0 ;
        }
        if(!(tuid = searchuser(uident)) || tuid == usernum) {
            move(2,0) ;
            prints("错误代号\n") ;
            pressreturn() ;
            move(2,0) ;
            clrtoeol() ;
            return -1 ;
        }
        genbuf[0]=0;
        ts.count=0;
        ucount = apply_utmp( talk_showstatus, 20,uident, &ts);
        move(3,0);
        prints("目前 %s 的 %d logins 如下: \n", uident, ucount);
        clrtobot() ;
        if(ucount>1) {
        	char buf[6];
list:		move(5,0) ;
            prints("(0) 算了算了，不聊了。\n");
            prints(genbuf);
            clrtobot() ;
            tmp=ucount+8;
            getdata( tmp, 0, "请选一个你看的比较顺眼的 [0]: ",
                     buf, 4, DOECHO, NULL,YEA);
            unum=atoi(buf);
            if(unum == 0) { clear(); return 0; }
            if(unum > ucount || unum < 0) {
                move(tmp,0);
                prints("笨笨！你选错了啦！\n") ;
                clrtobot();
                pressreturn();
                goto list;
            }
            uin=utmpshm->uinfo[ts.pos[unum-1]];
        }else
            search_ulist( &uin, t_cmpuids, tuid );
    }else
    {
        /*     memcpy(&uin,userinfo,sizeof(uin));*/
        uin=*userinfo;
        tuid=uin.uid;
        strcpy(uident,uin.userid);
        move(1,0) ;
        clrtoeol() ;
        prints("跟谁聊天: %s",uin.userid) ;
    }
    /*  check if pager on/off       --gtv */
    if(!canpage(can_override(uin.userid, currentuser->userid),uin.pager))
    {
        move(2,0) ;
        prints("对方呼叫器已关闭.\n") ;
        pressreturn() ;
        move(2,0) ;
        clrtoeol() ;
        return -1 ;
    }
    /*modified by Excellent*/
    if(uin.mode == ULDL || uin.mode == IRCCHAT ||
            uin.mode == BBSNET || uin.mode == FOURM || uin.mode==EXCE_BIG2 ||
            uin.mode == EXCE_MJ || uin.mode==EXCE_CHESS) {
        move(2,0) ;
        prints("目前无法呼叫.\n") ;
        pressreturn() ;
        move(2,0) ;
        clrtoeol() ;
        return -1 ;
    }
    if(LOCKSCREEN == uin.mode) /* Leeward 98.02.28 */
    {
        move(2,0) ;
        prints("对方已经锁定屏幕，请稍候再呼叫他(她)聊天...\n");
        clrtobot();
        pressreturn() ;
        move(2,0) ;
        clrtoeol() ;
        return -1 ;
    }
    if(!uin.active || (kill(uin.pid,0) == -1)) {
        move(2,0) ;
        prints("对方已离开\n") ;
        pressreturn() ;
        move(2,0) ;
        clrtoeol() ;
        return -1 ;
    }
    if (NA==canIsend2(uin.userid))/*Haohmaru.99.6.6.检查是否被ignore*/
    {
        move(2,0) ;
        prints("对方拒绝和你聊天\n");
        pressreturn() ;
        move(2,0) ;
        clrtoeol() ;
        return -1 ;
    }
    else {
        int sock, msgsock;
        int length ;
        struct sockaddr_in server ;
        char c ;
        char buf[512] ;

        move( 3, 0 );
        clrtobot();
        show_user_plan(uident);
        getdata( 2, 0, "确定要和他/她聊天吗? (Y/N) [N]: ", genbuf, 4, DOECHO, NULL,YEA);
        if ( *genbuf != 'y' && *genbuf != 'Y' ) {
            clear();
            return 0;
        }

        sprintf(buf,"Talk to '%s'",uident) ;
        report(buf) ;
        sock = socket(AF_INET, SOCK_STREAM, 0) ;
        if(sock < 0) {
            perror("socket err\n") ;
            return -1 ;
        }

        server.sin_family = AF_INET ;
/*        server.sin_addr.s_addr = INADDR_ANY ;
我想应该用INADDR_LOOPBACK比较好 KCN
*/
    	server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        server.sin_port = 0 ;
        if(bind(sock, (struct sockaddr *) & server, sizeof server ) < 0) {
            perror("bind err") ;
            return -1 ;
        }
        length = sizeof server ;
        if(getsockname(sock, (struct sockaddr *) &server, (socklen_t*)&length) < 0) {
            perror("socket name err") ;
            return -1 ;
        }
        uinfo.sockactive = YEA ;
        uinfo.sockaddr = server.sin_port ;
        uinfo.destuid = tuid ;
        UPDATE_UTMP(sockactive,uinfo);
        UPDATE_UTMP(sockaddr,uinfo);
        UPDATE_UTMP(destuid,uinfo);
        modify_user_mode( PAGE );
        kill(uin.pid,SIGUSR1) ;
        clear() ;
        prints("呼叫 %s 中...\n\n输入 Ctrl-D 结束\n", uident) ; /* modified by dong , 1999.1.27 */

        listen(sock,1) ;
        add_io(sock,20) ;
        while(YEA) {
            int ch ;
            ch = igetch() ;
            if(ch == I_TIMEOUT) {
                move(0,0) ;
                prints("再次呼叫.\n") ;
                bell() ;
                if(kill(uin.pid,SIGUSR1) == -1) {
                    move(0,0) ;
                    prints("对方已离线\n") ;
                    pressreturn() ;
                    /*Add by SmallPig 2 lines*/
                    uinfo.sockactive = NA ;
                    uinfo.destuid = 0 ;
                    return -1 ;
                }
                continue ;
            }
            if(ch == I_OTHERDATA)
                break ;
            if(ch == '\004') {
                add_io(0,0) ;
                close(sock) ;
                uinfo.sockactive = NA ;
                uinfo.destuid = 0 ;
                UPDATE_UTMP(sockactive,uinfo);
                UPDATE_UTMP(destuid,uinfo);
                clear() ;
                return 0 ;
            }
        }

        msgsock = accept(sock, (struct sockaddr *)0, (socklen_t*) 0) ;
        if(msgsock == -1) {
            perror("accept") ;
            return -1 ;
        }
        add_io(0,0) ;
        close(sock) ;
        uinfo.sockactive = NA ;
        uinfo.destuid = 0 ;
        UPDATE_UTMP(sockactive,uinfo);
        UPDATE_UTMP(destuid,uinfo);
        /*      uinfo.destuid = 0 ;*/
        read(msgsock,&c,sizeof c) ;

        clear() ;

        if(c == 'y'|| c=='Y' ) {
            sprintf( save_page_requestor, "%s (%s)", uin.userid, uin.username);

            /* Bigman 2000.9.15 增加Talk记录 */
#ifdef TALK_LOG
            strcpy(partner, uin.userid);
#endif
            do_talk(msgsock) ;
            /*Add by SmallPig*/
        } else if(c=='n'||c=='N'){
            prints("%s (%s)说：抱歉，我现在很忙，不能跟你聊。\n", uin.userid, uin.username) ;
            pressreturn() ;
        } else if(c=='b' || c=='B')
        {   prints("%s (%s)说：我现在很烦，不想跟别人聊天。\n", uin.userid, uin.username) ;
            pressreturn() ;
        } else if(c=='c' || c=='C')
        {   prints("%s (%s)说：我有急事，我等一下再 Call 你。\n", uin.userid, uin.username) ;
            pressreturn() ;
        } else if(c=='d' || c=='D')
        {   prints("%s (%s)说：请你不要再 Page，我不想跟你聊。\n", uin.userid, uin.username) ;
            pressreturn() ;
        } else if(c=='e' || c=='E')
        {   prints("%s (%s)说：我要离开了，下次在聊吧。\n", uin.userid, uin.username) ;
            pressreturn() ;
        } else if(c=='F' || c=='f')
        {   prints("%s (%s)说：请你寄一封信给我，我现在没空。\n", uin.userid, uin.username) ;
            pressreturn() ;
        } else if(c=='M' || c=='m')
        {
            read(msgsock,test,sizeof test) ;
            prints("%s (%s)说：%s\n", uin.userid, uin.username,test);
            pressreturn() ;
        } else{
            sprintf( save_page_requestor, "%s (%s)", uin.userid, uin.username);

            /* Bigman 2000.9.15 增加Talk记录 */
#ifdef TALK_LOG
            strcpy(partner, uin.userid);
#endif
            do_talk(msgsock) ;
        }
        close(msgsock) ;
        clear() ;
        refresh();
    }
    return 0 ;
}

extern int talkrequest ;
extern int ntalkrequest ;
struct user_info  ui ;
char page_requestor[STRLEN];
char page_requestorid[STRLEN];

int
cmpunums(unum,up)
int unum ;
struct user_info *up ;
{
    if(!up->active)
        return 0 ;
    return (unum == up->destuid) ;
}

int
cmpmsgnum(unum,up)
int unum ;
struct user_info *up ;
{
    if(!up->active)
        return 0 ;
    return (unum == up->destuid&&up->sockactive==2) ;
}

int
setpagerequest(mode)
int mode;
{
    int tuid;
    if(mode==0)
        tuid = search_ulist( &ui, cmpunums, usernum );
    else
        tuid = search_ulist( &ui, cmpmsgnum, usernum );
    if(tuid == 0)
        return 1;
    if(!ui.sockactive)
        return 1;
    uinfo.destuid = ui.uid;
    sprintf(page_requestor, "%s (%s)", ui.userid, ui.username);
    strcpy( page_requestorid, ui.userid );
    return 0;
}

int
servicepage( line, mesg )
int     line;
char    *mesg;
{
    static time_t last_check;
    time_t now;
    char buf[STRLEN];
    int tuid = search_ulist( &ui, cmpunums, usernum );

    if(tuid == 0 || !ui.sockactive) talkrequest = NA;
    if (!talkrequest) {
        if (page_requestor[0]) {
            switch (uinfo.mode) {
            case TALK:
                move(line, 0);
                printdash( mesg );
                break;
            default: /* a chat mode */
                sprintf(mesg, "** %s 已停止呼叫.", page_requestor);
            }
            memset(page_requestor, 0, STRLEN);
            last_check = 0;
        }
        return NA;
    } else {
        now = time(0);
        if (now - last_check > P_INT) {
            last_check = now;
            if (!page_requestor[0] && setpagerequest(0/*For Talk*/))
                return NA;
            else switch (uinfo.mode) {
                case TALK:
                    move(line, 0);
                    sprintf(buf, "** %s 正在呼叫你", page_requestor);
                    printdash( buf );
                    break;
                default: /* chat */
                    sprintf(mesg, "** %s 正在呼叫你", page_requestor);
                }
        }
    }
    return YEA;
}

int
setnpagerequest()
{
    char tmp_buf[STRLEN + 128];
    FILE *getpager;

    if (sysconf_str("GETPAGER") == NULL) return 1;

    sprintf(tmp_buf,"%s %s", sysconf_str("GETPAGER"),currentuser->userid);
    getpager = popen(tmp_buf,"r");
    if (getpager == NULL)
        return 1;
    fgets(npage_requestor, STRLEN, getpager);
    if ( *npage_requestor == '\0')
        return 1;
    return 0;
}

int
talkreply()
{
    int a ;
    struct hostent *h ;
    char buf[512] ;
    char reason[51];
    char hostname[STRLEN] ;
    struct sockaddr_in sin ;
    char inbuf[STRLEN*2];

    talkrequest = NA ;
#ifdef BBSNTALKD
    ntalkrequest = NA ;
#endif
    if (setpagerequest(0/*For Talk*/)) return 0;
    /*  added by netty  */
    set_alarm(0,NULL,NULL);
    clear() ;

    /* to show plan -cuteyu */

    move( 5, 0 );
    clrtobot();
    show_user_plan(page_requestorid);
    /*Add by SmallPig*/
    move(1,0);
    prints("(N)【抱歉，我现在很忙，不能跟你聊。】(B)【我现在很烦，不想跟别人聊天。 】\n");
    prints("(C)【我有急事，我等一下再 Call 你。】(D)【请不要再 Page，我不想跟你聊。】\n");
    prints("(E)【我要离开了，下次在聊吧。      】(F)【请寄一封信给我，我现在没空。 】\n");
    prints("(M)【留言给 %-12s           】\n",page_requestorid);
    sprintf( inbuf, "你想跟 %s 聊聊天吗? (Y N B C D E F)[Y]: ", page_requestor );
    strcpy(save_page_requestor, page_requestor);

    /* 2000.9.15 Bigman 添加Talk记录 */
#ifdef TALK_LOG
    strcpy(partner, page_requestorid);
#endif
    memset(page_requestor, 0, sizeof(page_requestor));
    memset(page_requestorid, 0, sizeof(page_requestorid));
    getdata(0,0, inbuf ,buf,STRLEN,DOECHO,NULL,YEA) ;
    /*        
    gethostname(hostname,STRLEN) ;
    if(!(h = gethostbyname(hostname))) {
        perror("gethostbyname") ;
        return -1 ;
    }
    memcpy( &sin.sin_addr, h->h_addr, h->h_length) ;
我想应该用INADDR_LOOPBACK比较好 KCN
*/
    memset(&sin, 0, sizeof sin) ;

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sin.sin_port = ui.sockaddr ;
    a = socket(sin.sin_family,SOCK_STREAM,0) ;
    if((connect(a, (struct sockaddr *)&sin, sizeof sin))) {
        prints("connect err") ;
        return -1 ;
    }
    if(buf[0] != 'n' && buf[0] != 'N'&& buf[0] != 'B'&& buf[0] != 'b'
            && buf[0] != 'C'&& buf[0] != 'c'&& buf[0] != 'D'&& buf[0] != 'd'
            && buf[0] != 'e'&& buf[0] != 'E'&& buf[0] != 'f'&& buf[0] != 'F'
            && buf[0] != 'm'&& buf[0] != 'M')
        buf[0]='y';
    if(buf[0]=='M'||buf[0]=='m')
    {
        move(1,0);
        clrtobot();
        getdata(1,0, "留话：" ,reason,50,DOECHO,NULL,YEA) ;
    }
    write(a,buf,1) ;
    if(buf[0]=='M'||buf[0]=='m')
        write(a,reason,sizeof reason) ;
    if(buf[0] != 'y') {
        close(a) ;
        report("page refused");
        clear() ;
        refresh();
        return 0 ;
    }
    report("page accepted");
    clear();
    do_talk(a) ;
    close(a) ;
    clear() ;
    refresh();
    return 0 ;
}

int
dotalkent(uentp, buf)
struct user_info *uentp;
char *buf;
{
    char mch;
    if (!uentp->active || !uentp->pid) return -1;
    if(!HAS_PERM(currentuser,PERM_SEECLOAK) && uentp->invisible)
        return -1;
    switch(uentp->mode) {
    case ULDL: mch = 'U'; break;
    case TALK: mch = 'T'; break;
    case CHAT1:
    case CHAT2:
    case CHAT3:
    case CHAT4: mch = 'C'; break;
    case IRCCHAT: mch = 'I'; break;
    case FOURM: mch = '4'; break;
    case BBSNET: mch = 'B'; break;
    case READNEW:
    case READING: mch = 'R'; break;
    case POSTING: mch = 'P'; break;
    case SMAIL:
    case RMAIL:
    case MAIL: mch = 'M'; break;
    default: mch = '-';
    }
    sprintf(buf, "%s%s(%c), ", uentp->invisible?"*":"", uentp->userid, mch);
    return 0;
}

int
dotalkuent(struct user_info *uentp,char* arg,int pos)
{
    char        buf[ STRLEN ];

    if( dotalkent( uentp, buf ) != -1 ) {
        strcpy( talk_uent_buf, buf );
        talk_uent_buf += strlen( buf );
    }
    return 0;
}

void
do_talk_nextline( twin )
struct talk_win *twin;
{
    twin->curln = twin->curln + 1;
    if( twin->curln > twin->eline )
        twin->curln = twin->sline;
    /*    if( curln != twin->eline ) {
            move( curln + 1, 0 );
            clrtoeol();
        }*/
    move( twin->curln, 0 );
    clrtoeol();
    twin->curcol = 0;
}

void
do_talk_char( twin, ch )
struct talk_win *twin;
int             ch ;
{

    if(isprint2(ch)) {
        if( twin->curcol < 79) {
            move( twin->curln, (twin->curcol)++ );
            prints( "%c",ch );
            return;
        }
        do_talk_nextline( twin );
        twin->curcol++;
        prints( "%c", ch );
        return;
    }
    switch(ch) {
    case Ctrl('H'):
                case '\177':
if( twin->curcol == 0 ) {
            return;
        }
        (twin->curcol)-- ;
        move( twin->curln, twin->curcol );
        prints(" ") ;
        move( twin->curln, twin->curcol );
        return ;
    case Ctrl('M'):
                case Ctrl('J'):
        do_talk_nextline( twin );
        return ;
    case Ctrl('G'):
                    bell() ;
        return ;
    default:
        break ;
    }
    return ;
}

void
do_talk_string( twin, str )
struct talk_win *twin;
char            *str;
{
    while( *str ) {
        do_talk_char( twin, *str++ );
    }
}

void
dotalkuserlist( twin )
struct talk_win *twin;
{
    char        bigbuf[ USHM_SIZE * 20 ]; /* change MAXACTIVE->USHM_SIZE, dong, 1999.9.15 */
    int         savecolumns;

    do_talk_string( twin, "\n*** 上线网友 ***\n" );
    savecolumns = (t_columns > STRLEN ? t_columns : 0);
    talk_uent_buf = bigbuf;
    if( apply_ulist_addr( dotalkuent,0 ) == -1 ) {
        strcpy( bigbuf, "没有任何使用者上线\n" );
    }
    strcpy( talk_uent_buf, "\n" );
    do_talk_string( twin, bigbuf );
    if (savecolumns) t_columns = savecolumns;
}

char talkobuf[80] ;
int talkobuflen ;
int talkflushfd ;

void
talkflush()
{
    if(talkobuflen)
        write(talkflushfd,talkobuf,talkobuflen) ;
    talkobuflen = 0 ;
}

int
moveto(mode,twin)
int mode;
struct talk_win *twin;
{
    if(mode==1)
        twin->curln--;
    if(mode==2)
        twin->curln++;
    if(mode==3)
        twin->curcol++;
    if(mode==4)
        twin->curcol--;
    if(twin->curcol<0)
    {
        twin->curln--;
        twin->curcol=0;
    }else if(twin->curcol>79)
    {
        twin->curln++;
        twin->curcol=0;
    }
    if(twin->curln<twin->sline)
    {
        twin->curln=twin->eline;
    }
    if(twin->curln>twin->eline)
    {
        twin->curln=twin->sline;
    }
    move(twin->curln,twin->curcol);
}

void
endmsg(void* data)
{
    int x,y;
    int tmpansi;
    tmpansi=showansi;
    showansi=1;
    talkidletime+=60;
    if(talkidletime>=IDLE_TIMEOUT)
        kill(getpid(),SIGHUP);
    if(uinfo.in_chat == YEA)
        return;
    getyx(&x,&y);
    update_endline();
    move(x,y);
    refresh();
    set_alarm(60,endmsg,NULL);
    showansi=tmpansi;
    UNUSED_ARG(data);
    return;
}

int
do_talk(fd)
int fd ;
{
    struct talk_win     mywin, itswin;
    char        mid_line[ 256 ];
    int         page_pending = NA;
    int         i,i2;
    int         previous_mode;
#ifdef TALK_LOG
    char    mywords[80], itswords[80], buf[80];
    int     mlen = 0, ilen = 0;
    time_t  now;
    mywords[0] = itswords[0] = '\0';
#endif
    endmsg(NULL);
    refresh() ;
    previous_mode=uinfo.mode;
    modify_user_mode( TALK );
    sprintf( mid_line, " %s (%s) 和 %s 正在畅谈中",
             currentuser->userid, currentuser->username, save_page_requestor );

    memset( &mywin,  0, sizeof( mywin ) );
    memset( &itswin, 0, sizeof( itswin ) );
    i = (t_lines-1) / 2;
    mywin.eline = i - 1;
    itswin.curln = itswin.sline = i + 1;
    itswin.eline = t_lines - 2;
    move( i, 0 );
    printdash( mid_line );
    move( 0, 0 );

    talkobuflen = 0 ;
    talkflushfd = fd ;
    add_io(fd,0) ;
    add_flush(talkflush) ;

    while(YEA) {
        int ch ;
        if (talkrequest) 
            page_pending = servicepage( (t_lines-1) / 2, mid_line );
        ch = igetkey();
        talkidletime=0;
        if ( ch == '' )
        {
            igetch();
            igetch();
            continue;
        }
        if(ch == I_OTHERDATA) {
            char data[80];
            int  datac;
            register int i ;

            datac = read(fd,data,80) ;
            if(datac<=0)
                break ;
            for(i=0;i<datac;i++)
            {
                if(data[i]>=1&&data[i]<=4)
                {
                    moveto(data[i]-'\0',&itswin);
                    continue;
                }

#ifdef TALK_LOG
                /* Bigman 2000.9.15 添加TALK记录
                 */
                /* existing do_log() overflow problem       */
                else if (isprint2(data[i])) {
                    if (ilen >= 80) {
                        itswords[80] = '\0';
                        (void) do_log(itswords, 2);
                        ilen = 0;
                    } else {
                        itswords[ilen] = data[i];
                        ilen++;
                    }
                } else if ((data[i] == Ctrl('H') || data[i] == '\177') && !ilen) {
                    itswords[ilen--] = '\0';
                } else if (data[i] == Ctrl('M') || data[i] == '\r' ||
                           data[i] == '\n') {
                    itswords[ilen] = '\0';
                    (void) do_log(itswords, 2);
                    ilen = 0;
                }
#endif
                do_talk_char( &itswin, data[i] );
            }
        } else {
            if(ch == Ctrl('D') || ch == Ctrl('C'))
                break ;
            if(isprint2(ch) || ch == Ctrl('H') || ch == '\177'
                    || ch == Ctrl('G')/* || ch == Ctrl('M')*/ ) {
                talkobuf[talkobuflen++] = ch ;
                if(talkobuflen == 80) talkflush() ;

#ifdef TALK_LOG
                if (mlen < 80) {
                    if ((ch == Ctrl('H') || ch == '\177') && mlen != 0) {
                        mywords[mlen--] = '\0';
                    } else {
                        mywords[mlen] = ch;
                        mlen++;
                    }
                } else if (mlen >= 80) {
                    mywords[80] = '\0';
                    (void) do_log(mywords, 1);
                    mlen = 0;
                }
#endif
                do_talk_char( &mywin, ch );

                /*            } else if (ch == '\n') {   Bigman 2000.9.15 */
            } else if (ch == '\n' || ch == Ctrl('M') || ch == '\r') {
#ifdef TALK_LOG
                if (mywords[0] != '\0') {
                    mywords[mlen++] = '\0';
                    (void) do_log(mywords, 1);
                    mlen = 0;
                }
#endif
                talkobuf[talkobuflen++] = '\r';
                talkflush();
                do_talk_char( &mywin, '\r' );
                /*            } else if (ch == Ctrl('T')) {
                                now=time(0);
                                strcpy(ct,ctime(&now));
                                do_talk_string( &mywin, ct);
                            } else if (ch == Ctrl('U') || ch == Ctrl('W')) {
                                dotalkuserlist( &mywin );*/
            }else if(ch>=KEY_UP&&ch<=KEY_LEFT)
            {
                moveto(ch-KEY_UP+1,&mywin);
                talkobuf[talkobuflen++] = ch -KEY_UP+1;
                if(talkobuflen == 80) talkflush() ;
            }
            else if (ch == Ctrl('E')) {
                for(i2=0;i2<=10;i2++)
                {
                    talkobuf[talkobuflen++] = '\r';
                    talkflush();
                    do_talk_char( &mywin, '\r' );
                }
            } else if (ch == Ctrl('P') && HAS_PERM(currentuser,PERM_BASIC)) {
                t_pager();
                update_endline();
            }
            else if (Ctrl('Z') == ch)
                r_lastmsg(); /* Leeward 98.07.30 support msgX */
        }
    }
    add_io(0,0) ;
    talkflush() ;
    set_alarm(0,NULL,NULL);
    add_flush(NULL) ;
    modify_user_mode(previous_mode);

#ifdef TALK_LOG
    /* 2000.9.15 Bigman 添加Talk记录 */
    mywords[mlen] = '\0';
    itswords[ilen] = '\0';

    if (mywords[0] != '\0')
        do_log(mywords, 1);
    if (itswords[0] != '\0')
        do_log(itswords, 2);

    now = time(0);
    sprintf(buf, "\n\033[1;34m通话结束, 时间: %s \033[m\n", Cdate(now));
    write(talkrec, buf, strlen(buf));

    close(talkrec);

    /*---	这句有用吗?	commented by period	---*/
    /*    sethomefile(genbuf, currentuser->userid, "talklog");	*/

    /*---	changed by period	2000-09-18	---*/
    *genbuf = 0;
    move(t_lines-1,0);
    if(askyn("是否寄回聊天纪录 ", NA)==YEA) {
        /*---						---*
            getdata(23, 0, "是否寄回聊天纪录 [Y/n]: ", genbuf, 2, DOECHO, NULL, YEA); 

            if (genbuf[0] != 'N' || genbuf[0] != 'n')  {
         *---	also '||' used above is wrong...	---*/
        sethomefile(buf, currentuser->userid, "talklog");
        sprintf(mywords, "跟 %s 的聊天记录 [%12.12s]", partner, Ctime(now) + 6);
        mail_file(currentuser->userid,buf, currentuser->userid, mywords,0);
    }
    sethomefile(buf, currentuser->userid, "talklog");
    unlink(buf);
#endif

    return 0;
}

int
shortulist(uentp)
struct user_info *uentp;
{
    int i;
    int pageusers=60;
    extern struct user_info *user_record[];
    extern int range;

    fill_userlist();
    if(ulistpage>((range-1)/pageusers))
        ulistpage=0;
    if(ulistpage<0)
        ulistpage=(range-1)/pageusers;
    move(1,0);
    clrtoeol();
    prints("每隔 %d 秒更新一次，Ctrl-C 或 Ctrl-D 离开，[F]更换模式[↑↓]上、下一页  第%1d页",M_INT,ulistpage+1);
    clrtoeol();
    move(3 , 0);
    clrtobot();
    for(i=ulistpage*pageusers;i<(ulistpage+1)*pageusers&&i<range;i++)
    {
        char ubuf[STRLEN];
        int ovv;

        if(i<numf||friendmode)
            ovv=YEA;
        else
            ovv=NA;
        sprintf(ubuf,"%s%-12.12s %s%-10.10s[m",(ovv)?"[32m．":"  ",user_record[i]->userid,(user_record[i]->invisible==YEA)?"[34m":"",
                modestring(user_record[i]->mode, user_record[i]->destuid, 0, NULL));
        prints("%s",ubuf);
        if((i+1)%3==0)
            prints("\n");
        else
            prints(" |");
    }
    return range;
}

int
do_list( modestr )
char *modestr;
{
    char        buf[ STRLEN ];
    int         count;
    extern int RMSG;
    int chkmailflag=0;

    if(RMSG!=YEA)/*如果收到 Msg 第一行不显示。*/
    {
        move(0,0);
        clrtoeol();
        chkmailflag=chkmail();

        if(chkmailflag==2)/*Haohmaru.99.4.4.对收信也加限制*/
            showtitle(modestr,"[您的信箱超过容量,不能再收信!]");
        else if(chkmailflag)
            showtitle(modestr,"[您有信件]");
        else
            showtitle(modestr,BBS_FULL_NAME);
    }
    move(2,0);
    clrtoeol();
    sprintf( buf, "  %-12s %-10s", "使用者代号", "目前动态" );
    prints( "[33m[44m%s |%s |%s[m", buf, buf, buf );
    /*    if(apply_ulist( shortulist ) == -1) {
            prints("No Users Exist\n") ;
            return 0;
        }
        count = shortulist(NULL);*/
    count = shortulist(NULL);
    if (uinfo.mode == MONITOR) {
        time_t thetime = time(0);
        move(t_lines-1, 0);
        prints("[44m[33m目前有 %3d %6s上线, 时间: %s , 目前状态：%10s   [m"
               ,count, friendmode ? "好朋友":"使用者",Ctime(thetime),friendmode ? "你的好朋友":"所有使用者");
    }
    refresh();
    return 0;
}

int
t_list()
{
    modify_user_mode( LUSERS );
    report("t_list");
    do_list("使用者状态");
    pressreturn();
    refresh();
    clear();
    return 0;
}


void
sig_catcher(void* data)
{
    ulistpage++;
    if (uinfo.mode != MONITOR) {
		set_alarm(0,NULL,NULL);
        return;
    }
    do_list("探视民情");
    idle_count++;
    set_alarm(M_INT*idle_count,sig_catcher,NULL);
    UNUSED_ARG(data);
}

int
t_monitor()
{
    int i;


    set_alarm(0,NULL,NULL);
    /*    idle_monitor_time = 0;*/
    report("monitor");
    modify_user_mode( MONITOR );
    ulistpage=0;
    do_list("探视民情");
    set_alarm(M_INT*idle_count,sig_catcher,NULL);
    while (YEA) {
        i=egetch();
        if (Ctrl('Z') == i) r_lastmsg(); /* Leeward 98.07.30 support msgX */
        if(i=='f' ||i=='F'){
            if(friendmode==YEA)
                friendmode=NA;
            else
                friendmode=YEA;
            do_list("探视民情");
        }
        if(i == KEY_DOWN)
        {
            ulistpage++;
            do_list("探视民情");
        }
        if(i == KEY_UP)
        {
            ulistpage--;
            do_list("探视民情");
        }
        if (i == Ctrl('D') || i == Ctrl('C')|| i== KEY_LEFT) break;
        /*        else if (i == -1) {
                    if (errno != EINTR) { perror("read"); exit(1); }
                } else idle_monitor_time = 0;*/
    }
    move(2,0);
    clrtoeol();
    clear();
    return 0;
}

#ifdef IRC
int count_useshell(struct user_info *uentp ,int* count,int pos)
{

    if(!uentp->active || !uentp->pid)
        return 0 ;
    if(uentp->mode==WWW||uentp->mode==CSIE_TIN||uentp->mode==CSIE_GOPHER
            ||uentp->mode==EXCE_CHESS||uentp->mode==EXCE_BIG2||uentp->mode==EXCE_MJ
            ||uentp->mode==IRCCHAT)
        (*count)++ ;
    return 1 ;
}

int num_useshell()
{
	int count ;
	count=0;
    apply_ulist( (APPLY_UTMP_FUNC)count_useshell,(char*)&count) ;
    return count;
}

void
exec_cmd( umode, pager, cmdfile )
int     umode, pager;
char    *cmdfile;
{
    char buf[STRLEN*2] ;
    char userhome[STRLEN];
    int save_pager;
    extern int RUNSH;

    if(num_useshell()>=15)
    {
        clear();
        prints("太多人使用外部程式了，你等一下在用...");
        pressanykey();
        return ;
    }

    if( ! dashf( cmdfile ) ) {
        move(2,0);
        prints( "no %s\n", cmdfile );
        pressreturn();
        return;
    }
    save_pager = uinfo.pager;
    if( pager == NA ) {
        uinfo.pager = 0;
    }
    modify_user_mode( umode );
    /***** modified by netty, March 15,1995
        sprintf( buf, "/bin/sh %s", cmdfile );
    ******/    
    sethomepath(userhome, currentuser->userid);
    sprintf( buf, "/bin/sh %s %s %s %s", cmdfile, userhome,currentuser->userid, currentuser->username);
    RUNSH=YEA;
    do_exec(buf,NULL) ;
    RUNSH=NA;
    uinfo.pager = save_pager;
    clear();
}

void
t_irc() {
    exec_cmd( IRCCHAT, NA, "bin/irc.sh" );
}
#endif /* IRC */

/*
void
t_announce() {
    exec_cmd( CSIE_ANNOUNCE, YEA, "bin/faq.sh" );
}

void
t_tin() {
    exec_cmd( CSIE_TIN, YEA, "bin/tin.sh" );
}

void
t_gopher() {
    exec_cmd( CSIE_GOPHER, YEA, "bin/gopher.sh" );
}
void
t_www() {
    exec_cmd( WWW, YEA, "bin/www.sh" );
}*/

/*Add By Excellent*/
/*void
x_excemj() {
    clear();
    exec_cmd( EXCE_MJ, YEA, "bin/excemj.sh" );
        }
*/
/*Add By Excellent */
/*void
x_excebig2() {
    clear();
    exec_cmd( EXCE_BIG2, YEA, "bin/excebig2.sh" );
        }
*/
/* Add By Excellent */
/*void
x_excechess() {
    clear();
    exec_cmd( EXCE_CHESS, YEA, "bin/excechess.sh" );
        }        
*/

int
listfilecontent(fname)
char *fname;
{
    FILE *fp;
    int x = 0, y = 3, cnt = 0, max = 0, len;
    char u_buf[20], line[STRLEN], *nick;

    move(y,x);
    CreateNameList();
    strcpy(genbuf,fname);
    if ((fp = fopen(genbuf, "r")) == NULL) {
        prints("(none)\n");
        return 0;
    }
    while(fgets(genbuf, STRLEN, fp) != NULL) {
        strtok( genbuf, " \n\r\t" );
        strcpy( u_buf, genbuf );
        AddNameList( u_buf );
        nick = (char *) strtok( NULL, "\n\r\t" );
        if( nick != NULL ) {
            while( *nick == ' ' )  nick++;
            if( *nick == '\0' )  nick = NULL;
        }
        if( nick == NULL ) {
            strcpy( line, u_buf );
        } else {
            sprintf( line, "%-12s%s", u_buf, nick );
        }
        if( (len = strlen( line )) > max )  max = len;
        if( x + len > 78 )  line[ 78 - x ] = '\0';
        prints( "%s", line );
        cnt++;
        if ((++y) >= t_lines-1) {
            y = 3;
            x += max + 2;
            max = 0;
            if( x > 70 )  break;
        }
        move(y,x);
    }
    fclose(fp);
    if (cnt == 0) prints("(none)\n");
    return cnt;
}

int
addtofile(filename,str)
char filename[STRLEN],str[STRLEN];
{
    FILE *fp;
    int rc;

    if ((fp = fopen(filename, "a")) == NULL)
        return -1;
    flock(fileno(fp), LOCK_EX);
    rc = fprintf( fp, "%s\n",str);
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    return(rc == EOF ? -1 : 1);
}

int
addtooverride(uident)
char *uident;
{
    struct friends tmp;
    int  n;
    char buf[STRLEN];

    memset(&tmp,0,sizeof(tmp));
    sethomefile( buf, currentuser->userid,"friends" );
    if((!HAS_PERM(currentuser,PERM_ACCOUNTS) && !HAS_PERM(currentuser,PERM_SYSOP)) &&
            (get_num_records(buf,sizeof(struct friends))>=MAXFRIENDS) )
    {
        move(t_lines-2,0);
        clrtoeol();
        prints("抱歉，本站目前仅可以设定 %d 个好友, 请按任何件继续...",MAXFRIENDS);
        igetkey();
        move(t_lines-2,0);
        clrtoeol();
        return -1;
    }
    if( myfriend( searchuser(uident) , NULL) )
        return -1;
    if(uinfo.mode!=LUSERS&&uinfo.mode!=LAUSERS&&uinfo.mode!=FRIEND)
    {
        strcpy(tmp.id,uident);
        move(2,0);
        clrtoeol();
        sprintf(genbuf,"请输入给好友【%s】的说明: ",tmp.id);
        getdata(2,0,genbuf, tmp.exp,15,DOECHO,NULL,YEA);
    }
    else
    {
        move(t_lines-2,0);
        clrtoeol();
        refresh();
        strcpy(tmp.id,uident);
        sprintf(genbuf,"请输入给好友【%s】的说明: ",tmp.id);
        getdata(t_lines-2,0,genbuf, tmp.exp,15,DOECHO,NULL,YEA);
    }
    sethomefile( genbuf, currentuser->userid,"friends" );
    n=append_record(genbuf,&tmp,sizeof(struct friends));
    if(n!=-1)
        getfriendstr();
    else
        report("append friendfile error");
    return n;
}

int
del_from_file(filename,str)
char filename[STRLEN],str[STRLEN];
{
    FILE *fp, *nfp;
    int deleted = NA;
    char fnnew[256/*STRLEN*/];

    if ((fp = fopen(filename, "r")) == NULL) return -1;
    sprintf( fnnew, "%s.%d", filename, getuid());
    if ((nfp = fopen(fnnew, "w")) == NULL) return -1;
    while(fgets(genbuf, 256/*STRLEN*/, fp) != NULL) {
        if( strncasecmp(genbuf, str, strlen(str)) == 0 && genbuf[strlen(str)] <= 32)
            deleted = YEA;
        else if( *genbuf > ' ' )
            fputs(genbuf, nfp);
    }
    fclose(fp);
    fclose(nfp);
    if (!deleted) return -1;
    return(Rename(fnnew, filename));
}


int
deleteoverride(uident)
char *uident;
{
    int deleted;
    struct friends fh;

    sethomefile( genbuf, currentuser->userid,"friends" );
    deleted = search_record( genbuf, &fh, sizeof(fh), cmpfnames, uident );
    if(deleted>0)
    {
        if(delete_record(genbuf,sizeof(fh),deleted,NULL,NULL)==0)
            getfriendstr();
        else
        {
            deleted=-1;
            report("delete friend error");
        }
    }
    return (deleted>0)?1:-1;
}

friend_title()
{
    int chkmailflag=0;
    chkmailflag=chkmail();
    if(chkmailflag==2)/*Haohmaru.99.4.4.对收信也加限制*/
        strcpy(genbuf,"[您的信箱超过容量,不能再收信!]");
    else if ( chkmailflag )
        strcpy( genbuf, "[您有信件]" );
    else
        strcpy( genbuf, BBS_FULL_NAME );
    showtitle("[编辑好友名单]",genbuf);

    prints(" [←,e] 离开 [h] 求助 [→,r] 好友说明档 [↑,↓] 选择 [a] 增加好友 [d] 删除好友\n");
    prints("[44m 编号  好友列表      代号说明                                                   [m\n");
}

char *
friend_doentry(char* buf,int ent,struct friends *fh)
{
    sprintf(buf," %4d  %-12.12s  %s",ent,fh->id,fh->exp);
    return buf;
}

int
friend_edit(ent,fh,direc)
int ent;
struct friends *fh;
char *direc;
{
    struct friends nh;
    char buf[STRLEN/2];
    int pos;

    pos=search_record( direc, &nh, sizeof(nh), cmpfnames, fh->id );
    move(t_lines-2,0);
    clrtoeol();
    if(pos>0)
    {
        sprintf(buf,"请输入 %s 的新好友说明: ",fh->id);
        getdata(t_lines-2,0,buf,nh.exp,15,DOECHO,NULL,NA);
    }
    if(substitute_record(direc, &nh, sizeof(nh), pos)<0)
        report("Friend files subs err");
    move(t_lines-2,0);
    clrtoeol();
    return NEWDIRECT;
}

int
friend_help()
{
    show_help("help/friendshelp");
    return FULLUPDATE;
}

int
friend_add(ent,fh,direct)
int ent;
struct friends *fh;
char *direct;
{
    char uident[13];

    clear();
    move(1,0);
    usercomplete("请输入要增加的代号: ", uident);
    if( uident[0] != '\0' )
    {
        if(searchuser(uident)<=0)
        {
            move(2,0);
            prints( MSG_ERR_USERID );
            pressanykey();
        }else
            addtooverride(uident);
    }
    return FULLUPDATE;
}

int
friend_dele(ent,fh,direct)
int ent;
struct friends *fh;
char *direct;
{
    char buf[STRLEN];
    int deleted=NA;

    saveline(t_lines-2, 0, NULL);
    move(t_lines-2,0);
    clrtoeol();
    sprintf(buf,"是否把【%s】从好友名单中去除",fh->id);
    if(askyn(buf,NA)==YEA)
    {
        move(t_lines-2,0);
        clrtoeol();
        if(deleteoverride(fh->id)==1)
        {
            prints("已从好友名单中移除【%s】,按任何键继续...",fh->id);
            deleted=YEA;
        }
        else
            prints("找不到【%s】,按任何键继续...",fh->id);
    }
    else
    {
        move(t_lines-2,0);
        clrtoeol();
        prints("取消删除好友...");
    }
    igetkey();
    move(t_lines-2,0);
    clrtoeol();
    saveline(t_lines-2, 1, NULL);
    return (deleted)?FULLUPDATE:DONOTHING;
}

int
friend_mail(ent,fh,direct)
int ent;
struct friends *fh;
char *direct;
{
    if(!HAS_PERM(currentuser,PERM_POST))
        return DONOTHING;
    m_send(fh->id);
    return FULLUPDATE;
}

int
friend_query(ent,fh,direct)
int ent;
struct friends *fh;
char *direct;
{
    int ch;

    if(t_query(fh->id)==-1)
        return FULLUPDATE;
    move(t_lines-1, 0);
    clrtoeol();
    prints("[44m[31m[读取好友说明档][33m 寄信给好友 m │ 结束 Q,← │上一位 ↑│下一位 <Space>,↓      [m");
    ch = egetch();
    switch( ch ) {
    case Ctrl('Z'): r_lastmsg(); /* Leeward 98.07.30 support msgX */
        break;
case 'N': case 'Q':
case 'n': case 'q': case KEY_LEFT:
        break;
case 'm': case 'M':
        m_send(fh->id);
        break;
    case ' ':
case 'j': case KEY_RIGHT: case KEY_DOWN: case KEY_PGDN:
        return READ_NEXT;
case KEY_UP: case KEY_PGUP:
        return READ_PREV;
    default : break;
    }
    return FULLUPDATE ;
}

void
t_override()
{

    sethomefile( genbuf, currentuser->userid,"friends" );
    i_read( GMENU, genbuf , friend_title , friend_doentry, friend_list ,sizeof(struct friends));
    clear();
    return;
}

int
cmpfuid( a,b )
struct friends   *a,*b;
{
    return strcasecmp(a->id,b->id);
}

int
getfriendstr()
{
    extern int nf;
    int i;
    struct friends* friendsdata;

    if(topfriend!=NULL)
        free(topfriend);
    sethomefile( genbuf, currentuser->userid,"friends" );
    nf=get_num_records(genbuf,sizeof(struct friends));
    if(nf<=0)
        return 0;
    if(!HAS_PERM(currentuser,PERM_ACCOUNTS) && !HAS_PERM(currentuser,PERM_SYSOP))/*Haohmaru.98.11.16*/
        nf=(nf>=MAXFRIENDS)?MAXFRIENDS:nf;
    friendsdata=(struct friends *)calloc(sizeof(struct friends),nf);
    get_records(genbuf,friendsdata,sizeof(struct friends),1,nf);
    
    qsort( friendsdata, nf, sizeof( friendsdata[0] ), (int (*)(const void *, const void *))cmpfuid );/*For Bi_Search*/
    topfriend=(struct friends_info *)calloc(sizeof(struct friends_info),nf);
    for (i=0;i<nf;i++) {
    	topfriend[i].uid=searchuser(friendsdata[i].id);
    	strcpy(topfriend[i].exp,friendsdata[i].exp);
    }
    free(friendsdata);
    return 0;
}

int
wait_friend()
{
    FILE *fp;
    int tuid;
    char buf[STRLEN];
    char uid[13];

    modify_user_mode( WFRIEND );
    clear();
    move(1,0);
    usercomplete("请输入使用者代号以加入系统的寻人名册: ", uid);
    if(uid[0] == '\0')
    {
        clear() ;
        return 0 ;
    }
    if(!(tuid = getuser(uid,NULL)))
    {
        move(2,0) ;
        prints("[1m不正确的使用者代号[m\n") ;
        pressanykey() ;
        clear();
        return -1 ;
    }
    sprintf(buf,"你确定要把 %s 加入系统寻人名单中",uid);
    move(2,0);
    if(askyn(buf,YEA)==NA)
    {
        clear();
        return -1;
    }
    if((fp=fopen("friendbook","a"))==NULL)
    {
        prints("系统的寻人名册无法开启，请通知站长...\n");
        pressanykey();
        return -1;
    }
    sprintf(buf,"%d@%s",tuid,currentuser->userid);
    if(!seek_in_file("friendbook",buf))
        fprintf(fp,"%s\n",buf);
    fclose(fp);
    move(3,0);
    prints("已经帮你加入寻人名册中，%s 上站系统一定会通知你...\n",uid);
    pressanykey();
    clear();
    return 0;
}
/* 坏人名单:Bigman 2000.12.26 */
int list_ignore(fname)
char *fname;
{
    FILE *fp;
    int x = 0, y = 4, nIdx = 0;
    char buf[IDLEN+1],buf2[80];

    clear();
    move(y,x);
    /*	clrtoeol();*/

    if((fp=fopen(fname,"r"))==NULL)
    {
        prints("\033[1;33m*** 尚未设定黑名单 ***\033[m");
        return(0);
    }
    else
    {
        strcpy(buf2,"\033[1;32m〖黑名单上的用户ID列表〗\033[m");

        while(fread(buf, IDLEN+1, 1,fp)>0 )
        {
            if (nIdx%4==0)
            {
                prints(buf2);
                memset(buf2,0,IDLEN+1);
                y++;
                move(y,x);
            }
            nIdx++;
            sprintf(buf2+strlen(buf2),"  %-13s",buf);
        }
        if (nIdx>0) {
            prints(buf2);
        }
        else
        {
            prints("\033[1;32m*** 尚未设定黑名单 ***\033[m");
        }
        y++;
        move(y,x);
        clrtoeol();
        fclose(fp);

        return(nIdx);

    }
}
void clear_press()	/* 2000.12.28 Bigman 重复输入的回车，清除 */
{
    pressreturn();
    move(1,0);
    clrtoeol();
    move(2,0);
    clrtoeol();
}
int badlist()
{
    char userid[IDLEN+1],tmp[3];
    int cnt,nIdx;
    char ignoreuser[IDLEN+1],path[40];

    int cmpinames();
    int search_record(),append_record();
    int usercomplete(),namecomplete();

    modify_user_mode( EDITUFILE);
    clear();

    sethomefile( path, currentuser->userid , "/ignores");

    while(1)
    {	cnt=list_ignore(path);

        if (cnt>=MAX_IGNORE)
        {
            move(1,0);
            prints("已经到达黑名单最大人数限制");
            getdata(0,0,"(D)删除 (C)清除 (Q)返回? [Q]： " , tmp,2,DOECHO,NULL,YEA);
        }
        else if (cnt<=0)
            getdata(0,0,"(A)增加 (Q)返回? [Q]： " , tmp,2,DOECHO,NULL,YEA);
        else
            getdata(0,0,"(A)增加 (D)删除 (C)清除 (Q)返回? [Q]： " , tmp,2,DOECHO,NULL,YEA);


        if(tmp[0]=='Q'||tmp[0]=='q'||tmp[0]=='\0')
        {
            break;
        }

        if (((tmp[0]=='a'||tmp[0]=='A')&&(cnt<MAX_IGNORE))||((tmp[0]=='d'||tmp[0]=='D')&&(cnt>0)))
        {

            usercomplete("请输入使用者代号(只按 ENTER 结束输入): ",userid) ;

            if(userid[0] == '\0')
            {
                move(1,0);
                clrtoeol();
                continue ;
            }
            if(!searchuser(userid))
            {
                prints("这个使用者代号是错误的.\n");
                clear_press();
            }
            else if (!strcasecmp(userid,currentuser->userid))
            {
                prints("不能是自己的代号\n");
                clear_press();
            }
            else
            {
                nIdx=search_record( path,ignoreuser, IDLEN+1, cmpinames, userid );

                if (tmp[0]=='a' || tmp[0]=='A')
                {
                    if (nIdx>0)
                    {
                        prints("该ID已经在黑名单上！");
                        clear_press();
                    }
                    else
                    {
                        if (append_record( path, userid, IDLEN+1)==0)
                        {
                            /*	prints("已经成功添加到黑名单中"); */
                            /*		cnt=list_ignore(path); */
                        }
                        else {
                            prints("*** 系统错误 , 请与SYSOP联系***");
                            clear_press();}
                    }
                }
                else
                {
                    if (nIdx <= 0)
                    {
                        prints("该ID没有在黑名单上！");
                        clear_press();
                    }
                    else
                    {
                        if (delete_record( path, IDLEN+1, nIdx,NULL,NULL)==0)
                        {
                            ;
			    /*		prints("已经成功从黑名单中删除"); */
                            /*		cnt=list_ignore(path); */
                        }
                        else
                        {
                            prints("*** 系统错误 , 请与SYSOP联系***");
                            clear_press();}
                    }
                }
            }

        }
        else if ((tmp[0]=='c'||tmp[0]=='C')&&(cnt>0))
        {
            getdata(1,0,"确定删除黑名单? (Y/N) [N]:" , tmp,2,DOECHO,NULL,YEA);
            if (tmp[0]=='y'||tmp[0]=='Y') {
                unlink(path);
                /*cnt=list_ignore(path);*/
            }
            else {
                move(1,0);
                clrtoeol();}
        }

    }
    pressreturn();
    return(cnt);
}
#ifdef TALK_LOG
/* Bigman 2000.9.15 分别为两位聊天的人作纪录 */
/* -=> 自己说的话 */
/* --> 对方说的话 */

void
do_log(char *msg, int who)
{    time_t  now;
    char    buf[100];
    now = time(0);
    if (msg[strlen(msg)] == '\n')
        msg[strlen(msg)] = '\0';

    if (strlen(msg) < 1 || msg[0] == '\r' || msg[0] == '\n')
        return;

    /* 只帮自己做 */
    sethomefile(buf, currentuser->userid, "talklog");

    if (!dashf(buf) || talkrec == -1) {
        talkrec = open(buf, O_RDWR | O_CREAT | O_TRUNC, 0644);
        sprintf(buf, "\033[1;32m与 %s 的聊天记录, 日期: %s \033[m\n", save_page_requestor, Cdate(now));
        write(talkrec, buf, strlen(buf));
        sprintf(buf, "\t颜色分别代表: \033[1;33m%s\033[m \033[1;36m%s\033[m \n\n", currentuser->userid, partner);
        write(talkrec, buf, strlen(buf));
    }
    if (who == 1) {     /* 自己说的话 */
        sprintf(buf, "\033[1;33m%s \033[m\n", msg);
        write(talkrec, buf, strlen(buf));
    } else if (who == 2) {  /* 别人说的话 */
        sprintf(buf, "\033[1;36m%s \033[m\n", msg);
        write(talkrec, buf, strlen(buf));
    }
}
#endif

