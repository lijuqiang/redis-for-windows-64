/*
 * Copyright (c) Microsoft Open Technologies, Inc.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING
 * WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE,
 * FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache License, Version 2.0 for specific language governing
 * permissions and limitations under the License.
 */

#ifndef WIN32WSIOCP_H
#define WIN32WSIOCP_H

#ifdef _WIN32
/* structs and functions for using IOCP with windows sockets */

/* structure used for async write requests.
 * contains overlapped, WSABuf, and callback info
 * NOTE: OVERLAPPED must be first member */
typedef struct asendreq {
    OVERLAPPED ov;
    WSABUF wbuf;
    aeWinSendReq req;
    aeFileProc *proc;
    aeEventLoop *eventLoop;
} asendreq;

/* structure used for async accept requests.
 * contains overlapped, accept socket, accept buffer
 * NOTE: OVERLAPPED must be first member */
typedef struct aacceptreq {
    OVERLAPPED ov;
    SOCKET accept;
    void *buf;
    struct aacceptreq *next;
} aacceptreq;


/* per socket information */
typedef struct aeSockState {
    int masks;
    int fd;
    aacceptreq *reqs;
    int wreqs;
    OVERLAPPED ov_read;
} aeSockState;

typedef aeSockState * fnGetSockState(void *apistate, int fd);
typedef void fnDelSockState(void *apistate, aeSockState *sockState);

#define READ_QUEUED         0x000100
#define SOCKET_ATTACHED     0x000400
#define ACCEPT_PENDING      0x000800
#define LISTEN_SOCK         0x001000

void aeWinInit(void *state, HANDLE iocp, fnGetSockState *getSockState, fnDelSockState *delSockState);
void aeWinCleanup();

#endif
#endif
