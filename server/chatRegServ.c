/***************************************************************************
 * chatRegServ.c: Chat server that accepts client registrations
 * Author: Vijay Sivaraman
 * History: v1.0: created 22 Apr 2007
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <assert.h>
#include "skiplist.h"

#define SERV_PORT    31180     /* server UDP port */
#define SERV_PASSWD  "3118miniproject" /* password */
#define MAX_MSG_LEN  1200      /* max size of any msg */
#define SERV_AGE_TIME 120      /* seconds after which user is timed out */

typedef struct {
    char passwd[16];
    char username[14];
    unsigned short tcpPort;
} RegMsg_t;

typedef struct {
    unsigned long nusers; /* number of users (no more than 50) */
    struct UserInfo_s {
        char username[14];
        unsigned short tcpPort;
        unsigned long ipAddr;
    } user[50]; /* info about each user */
} RegRespMsg_t;

typedef struct User_s {
    char username[16];
    unsigned short tcpPort;
    unsigned long ipAddr;
    unsigned long timestamp;
} User_t;

typedef struct {
    SkipList_t *userTable; /* list of users */
} ChatRegServ_t;

ChatRegServ_t serv;

/******************************************************************************/
int userTableCmp(void *k1, void *k2)
{
    User_t *u1 = (User_t *)k1;
    User_t *u2 = (User_t *)k2;
    assert(u1 && u2);
    return strcmp(u1->username, u2->username);
}
void userTableFree(void *data)
{
    free(data);
}
/******************************************************************************/
int main()
{
    int sock, e, cliLen, inMsgLen, outMsgLen, i;
    struct sockaddr_in servAddr, cliAddr;
    char inMsg[MAX_MSG_LEN], outMsg[MAX_MSG_LEN];
    RegMsg_t *regMsg;
    RegRespMsg_t *regRespMsg;
    struct timeval timeVal;
    unsigned long curtime;
    User_t dummyUser, *user;
    SkipListNode_t *node;
    
    /* open socket */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		printf("socket creation failed errno=%d\n", errno);
        exit(0);
	}
    bzero((char *)&servAddr,sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(SERV_PORT);
    e = bind(sock, (struct sockaddr *)&servAddr, sizeof(servAddr));
	if (e < 0) {
		printf("socket bind failed errno=%d\n", errno);
        exit(0);
	}
    /* create user table */
    serv.userTable = SkipListAlloc(userTableCmp, userTableFree);
    /* run the server main loop */
    while (1) {
        cliLen = sizeof(cliAddr);
        /* wait for msg arrival */
        inMsgLen = recvfrom(sock, inMsg, MAX_MSG_LEN, 0,
            (struct sockaddr *)&cliAddr, (socklen_t*)&cliLen);

        e = gettimeofday(&timeVal, NULL);
        curtime = timeVal.tv_sec;
        printf("[%ld] Rcvd pkt from %s:%d\n",
            curtime, inet_ntoa(cliAddr.sin_addr.s_addr),
            ntohs(cliAddr.sin_port));
        /* perform some checks */
        if (inMsgLen < 0) {
            printf("Error in recvfrom() errno=%d\n", errno);
            continue;
        }
        if (inMsgLen > MAX_MSG_LEN) {
            printf("Received message too long (size=%d)\n", inMsgLen);
            continue;
        }
        regMsg = (RegMsg_t *)inMsg;
        /* verify if passwd is correct */
        if (strlen(regMsg->passwd) > 16 || strcmp(regMsg->passwd,SERV_PASSWD)!=0) {
            printf("Passwd mismatch\n");
            continue;
        }
        if (strlen(regMsg->username) > 14) {
            printf("Username too long\n");
            continue;
        }
        /* determine if user already in database */
        strcpy(dummyUser.username, regMsg->username);
        user = (User_t *)SKIPLIST_NODE_VALUE(
            SkipListGetNode(serv.userTable, &dummyUser));
        if (user==NULL) { /* new user */
            user = (User_t *)malloc(sizeof(User_t)); assert(user);
            memset(user, 0x0, sizeof(User_t));
            strcpy(user->username, regMsg->username);
            user->tcpPort = regMsg->tcpPort;
            user->ipAddr = cliAddr.sin_addr.s_addr;
            user->timestamp = curtime + SERV_AGE_TIME;
            /* insert user in skiplist */
            SkipListInsert(serv.userTable, user, user, 0);
        }
        else { /* user already exists */
            user->tcpPort = regMsg->tcpPort;
            user->ipAddr = cliAddr.sin_addr.s_addr;
            user->timestamp = curtime + SERV_AGE_TIME;
        }
        /* remove aged-out entries */
        for (node = SKIPLIST_NODE_FIRST(serv.userTable); node; ) {
            user = SKIPLIST_NODE_VALUE(node);
            node = SKIPLIST_NODE_NEXT(node);
            if (user->timestamp < curtime) {
                printf("Aging out user=%s\n", user->username);
                SkipListDelete(serv.userTable, user);
            }
        }
        /* create reply */
        regRespMsg = (RegRespMsg_t *)outMsg;
        regRespMsg->nusers = htonl(SKIPLIST_NUM_NODES(serv.userTable));
        for (node = SKIPLIST_NODE_FIRST(serv.userTable), i=0, outMsgLen = 4;
             node && (i < 50);
             node = SKIPLIST_NODE_NEXT(node),
                 ++i, outMsgLen += sizeof(struct UserInfo_s)) {
            user = SKIPLIST_NODE_VALUE(node);
            strcpy(regRespMsg->user[i].username, user->username);
            regRespMsg->user[i].tcpPort = user->tcpPort;
            regRespMsg->user[i].ipAddr = user->ipAddr;
        }        
        /* send the reply */
        e = sendto(sock, outMsg, outMsgLen, 0,
            (struct sockaddr*)&cliAddr, cliLen);
        if (e < 0) {
            printf("sendto() failure errno=%d\n", errno);
            continue;
        }
    } /* end of forever loop */

    return 0;
}
