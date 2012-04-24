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

#ifndef __W32BKSAV_H__
#define __W32BKSAV_H__

#ifdef _WIN32

#include "adlist.h"

#define BKSAVE_IDLE         0
#define BKSAVE_BUFFERING    1
#define BKSAVE_WRITING      3
#define BKSAVE_SUCCESS      4
#define BKSAVE_FAILED       5
#define BKSAVE_WAITTHREAD   6

#define BKSAVE_BUFSIZE      (1024 * 1024)


/* each buffer has a current postion and remaining space */
typedef struct bkgdfsavehdr {
    size_t pos;
    size_t rem;
} bkgdfsavehdr;

typedef struct bkgdfsave {
    int background;
    int state;
    HANDLE allowchanges;
    HANDLE dosaveevent;
    HANDLE terminateevent;
    HANDLE thread;
    char *filename;
    char *mode;
    char *filerename;
    list *buffers;
    bkgdfsavehdr *curbuf;
    int (*bkgdfsave_serialize)(char *);
} bkgdfsave;

int bkgdsave_start(char *filename, int (*bkgdfsave_serialize)(char *));
FILE *bkgdfsave_fopen(const char *filename, const char *mode);
size_t bkgdfsave_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp);
int bkgdfsave_fclose(FILE *fp);
int bkgdfsave_fflush(FILE *fp);
int bkgdfsave_fsync(int fd);
int bkgdfsave_rename(const char *src, const char *dst);
int bkgdfsave_unlink(const char *src);
int bkgdfsave_fileno(FILE *fp);
int bkgdsave_termthread();
void bkgdsave_init();
int bkgdsave_complete(int result);
void bkgdsave_allowcmd(struct redisCommand *cmd);
int bkgdsave_allowUpdates();


#endif

#endif
