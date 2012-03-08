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
