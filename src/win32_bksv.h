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


/* each buffer has a current postion and remaining space */
typedef struct bkgdfsavehdr {
    size_t pos;
    size_t rem;
} bkgdfsavehdr;

typedef struct bkgdfsave {
    int background;
    int state;
    HANDLE dosaveevent;
    HANDLE terminateevent;
    HANDLE thread;
    char *filename;
    char *tmpname;
    int (*bkgdfsave_serialize)(char *);
} bkgdfsave;

void bkgdsave_init();
int bkgdsave_start(const char *filename, int (*bkgdfsave_serialize)(char *));
int bkgdsave_termthread();

#endif

#endif
