/* Copyright (c) 2012, Microsoft Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "redis.h"
#ifdef _WIN32

DWORD WINAPI BkgdSaveThreadProc(LPVOID param);
void bkgdsave_cleanup();

FILE *bkgdfsave_fopen(const char *filename, const char *mode) {
    if (server.rdbbkgdfsave.background == 0) {
        return fopen(filename, mode);
    }

    if (server.rdbbkgdfsave.state != BKSAVE_IDLE) {
        errno = EEXIST;
        return NULL;
    }
    server.rdbbkgdfsave.curbuf = NULL;
    server.rdbbkgdfsave.filename = (char*)zmalloc(strlen(filename) + 1);
    strcpy(server.rdbbkgdfsave.filename, filename);
    server.rdbbkgdfsave.mode = (char*)zmalloc(strlen(mode) + 1);
    strcpy(server.rdbbkgdfsave.mode, mode);
    server.rdbbkgdfsave.filerename = NULL;
    server.rdbbkgdfsave.buffers = listCreate();
    server.rdbbkgdfsave.state = BKSAVE_BUFFERING;

    return (FILE *)&server.rdbbkgdfsave;
}

size_t bkgdfsave_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp) {
    size_t wsize;
    if (server.rdbbkgdfsave.background == 0) {
        return fwrite(ptr, size, nmemb, fp);
    }

    wsize = size * nmemb;

    if (server.rdbbkgdfsave.state != BKSAVE_BUFFERING) {
        errno = EINVAL;
        return 0;
    }

    if (server.rdbbkgdfsave.curbuf == NULL || wsize > server.rdbbkgdfsave.curbuf->rem) {
        /* need another buffer */
        server.rdbbkgdfsave.curbuf = (bkgdfsavehdr*)zmalloc(BKSAVE_BUFSIZE);
        if (server.rdbbkgdfsave.curbuf == NULL) {
            errno = EFBIG;
            server.rdbbkgdfsave.state = BKSAVE_FAILED;
            return 0;
        } else {
            server.rdbbkgdfsave.curbuf->pos = sizeof(bkgdfsavehdr);
            server.rdbbkgdfsave.curbuf->rem = BKSAVE_BUFSIZE - sizeof(bkgdfsavehdr);
            if (listAddNodeTail(server.rdbbkgdfsave.buffers, server.rdbbkgdfsave.curbuf) == NULL) {
                errno = EFBIG;
                server.rdbbkgdfsave.state = BKSAVE_FAILED;
                return 0;
            }
        }
    }

    /* we have space in buffer. copy data and adjust position */
    memcpy((char*)server.rdbbkgdfsave.curbuf + server.rdbbkgdfsave.curbuf->pos, ptr, wsize);
    server.rdbbkgdfsave.curbuf->pos += wsize;
    server.rdbbkgdfsave.curbuf->rem -= wsize;

    return nmemb;
}

int bkgdfsave_fclose(FILE *fp) {
    if (server.rdbbkgdfsave.background == 0) {
        return fclose(fp);
    }
    return 0;
}

int bkgdfsave_fflush(FILE *fp) {
    if (server.rdbbkgdfsave.background == 0) {
        return fflush(fp);
    }
    return 0;
}

int bkgdfsave_fileno(FILE *fp) {
    if (server.rdbbkgdfsave.background == 0) {
        return fileno(fp);
    }
    return 0;
}

int bkgdfsave_fsync(int fd) {
    if (server.rdbbkgdfsave.background == 0) {
        return fsync(fd);
    }
    return 0;
}

int bkgdfsave_rename(const char *src, const char *dst) {
    if (server.rdbbkgdfsave.background == 0) {
        return rename(src, dst);
    }
    server.rdbbkgdfsave.filerename = (char*)zmalloc(strlen(dst) + 1);
    strcpy(server.rdbbkgdfsave.filerename, dst);
    return 0;
}

int bkgdfsave_unlink(const char *src) {
    if (server.rdbbkgdfsave.background == 0) {
        return unlink(src);
    }
    return 0;
}

/* block command thread if not read only, and DB being written to buffers */
void bkgdsave_allowcmd(struct redisCommand *cmd) {
    /* if cmd is readonly, then allow */
    if (cmd != NULL && (cmd->flags & REDIS_CMD_READONLY) != 0)
        return;

    /* block if allowChanges event exists and is not set */
    if (server.rdbbkgdfsave.allowchanges != NULL) {
        if (WaitForSingleObject(server.rdbbkgdfsave.allowchanges, 60000) == WAIT_TIMEOUT) {
            redisLog(REDIS_WARNING,"FATAL: Wait on background save blocking too long.");
            exit(1);
        }
    }
}

/* test if saving data to buffers. Return 0 if DB updates should be blocked */
int bkgdsave_allowUpdates() {
    if (server.rdbbkgdfsave.background == 1)
        return 0;
    else
        return 1;
}

/* start a background save 
 * The serialize function could be rdbsave or aofsave */
int bkgdsave_start(char *filename, int (*bkgdfsave_serialize)(char *)) {
    if (server.rdbbkgdfsave.state != BKSAVE_IDLE) {
        /* only one background activity at a time is allowed */
        errno = EINVAL;
        return -1;
    }

    if (server.rdbbkgdfsave.thread == NULL) {
        /* create handles and thread */
        server.rdbbkgdfsave.dosaveevent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (server.rdbbkgdfsave.dosaveevent == NULL) {
            goto failed;
        }

        server.rdbbkgdfsave.terminateevent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (server.rdbbkgdfsave.terminateevent == NULL) {
            goto failed;
        }

        server.rdbbkgdfsave.thread = CreateThread(NULL, 0, BkgdSaveThreadProc, &server.rdbbkgdfsave, 0, NULL);
        if (server.rdbbkgdfsave.thread == NULL) {
            goto failed;
        }

        server.rdbbkgdfsave.allowchanges = CreateEvent(NULL, TRUE, TRUE, NULL);
        if (server.rdbbkgdfsave.allowchanges == NULL) {
            goto failed;
        }
    }

    /* start saving data into buffers */
    server.rdbbkgdfsave.background = 1;
    server.rdbbkgdfsave.filename = (char*)zmalloc(strlen(filename) + 1);
    strcpy(server.rdbbkgdfsave.filename, filename);
    server.rdbbkgdfsave.bkgdfsave_serialize = bkgdfsave_serialize;

    /* stop data changes until thread has made a copy */
    ResetEvent(server.rdbbkgdfsave.allowchanges);
    /* signal background thread to run */
    SetEvent(server.rdbbkgdfsave.dosaveevent);
    return REDIS_OK;

failed:
    bkgdsave_cleanup();
    errno = EINVAL;
    return -1;
}

int bkgdsave_termthread() {
    if (server.rdbbkgdfsave.terminateevent != NULL && server.rdbbkgdfsave.thread != NULL) {
        SetEvent(server.rdbbkgdfsave.terminateevent);
        WaitForSingleObject(server.rdbbkgdfsave.thread, INFINITE);
    }
    bkgdsave_cleanup();
    return 0;
}

int bkgdsave_complete(int result) {
    /* allow data changes again */
    if (server.rdbbkgdfsave.allowchanges != NULL) {
        SetEvent(server.rdbbkgdfsave.allowchanges);
    }
    return REDIS_OK;
}


/* cleanup state for thread termination */
void bkgdsave_cleanup() {
    if (server.rdbbkgdfsave.dosaveevent != NULL) {
        CloseHandle(server.rdbbkgdfsave.dosaveevent);
        server.rdbbkgdfsave.dosaveevent = NULL;
    }
    if (server.rdbbkgdfsave.terminateevent != NULL) {
        CloseHandle(server.rdbbkgdfsave.terminateevent);
        server.rdbbkgdfsave.terminateevent = NULL;
    }
    if (server.rdbbkgdfsave.thread != NULL) {
        CloseHandle(server.rdbbkgdfsave.thread);
        server.rdbbkgdfsave.thread = NULL;
    }
    if (server.rdbbkgdfsave.allowchanges != NULL) {
        CloseHandle(server.rdbbkgdfsave.allowchanges);
        server.rdbbkgdfsave.allowchanges = NULL;
    }
    if (server.rdbbkgdfsave.filename != NULL) {
        zfree(server.rdbbkgdfsave.filename);
        server.rdbbkgdfsave.filename = NULL;
    }
    if (server.rdbbkgdfsave.mode != NULL) {
        zfree(server.rdbbkgdfsave.mode);
        server.rdbbkgdfsave.mode = NULL;
    }
    if (server.rdbbkgdfsave.filerename != NULL) {
        zfree(server.rdbbkgdfsave.filerename);
        server.rdbbkgdfsave.filerename = NULL;
    }
    if (server.rdbbkgdfsave.buffers != NULL) {
        listSetFreeMethod(server.rdbbkgdfsave.buffers, zfree);
        listRelease(server.rdbbkgdfsave.buffers);
        server.rdbbkgdfsave.buffers = NULL;
    }
}

void bkgdsave_init() {
    server.rdbbkgdfsave.dosaveevent = NULL;
    server.rdbbkgdfsave.terminateevent = NULL;
    server.rdbbkgdfsave.allowchanges = NULL;
    server.rdbbkgdfsave.thread = NULL;
    server.rdbbkgdfsave.background = 0;
    server.rdbbkgdfsave.state = BKSAVE_IDLE;
    server.rdbbkgdfsave.filename = NULL;
    server.rdbbkgdfsave.mode = NULL;
    server.rdbbkgdfsave.filerename = NULL;
    server.rdbbkgdfsave.buffers = NULL;
}


/* background thread to write buffers to disk */
DWORD WINAPI BkgdSaveThreadProc(LPVOID param) {
    bkgdfsave *bkgddata = (bkgdfsave *)param;
    listIter *iter;
    listNode *node;
    FILE *fp = NULL;
    int rc;
    char *fname;
    HANDLE workorterm[2];

    workorterm[0] = server.rdbbkgdfsave.terminateevent;
    workorterm[1] = server.rdbbkgdfsave.dosaveevent;

    while (1) {

        DWORD ev = WaitForMultipleObjects(2, workorterm, FALSE, INFINITE);
        if (ev != (WAIT_OBJECT_0 + 1)) {
            /* terminate or unexpected return, do exit */
            bkgdsave_cleanup();
            return 0;
        }

        /* start saving data into buffers */
        server.rdbbkgdfsave.background = 1;
        /* filename will be overwritten by bkgdfsave_fopen. Save copy to free later */
        fname = server.rdbbkgdfsave.filename;
        server.rdbbkgdfsave.filename = NULL;
        rc = server.rdbbkgdfsave.bkgdfsave_serialize(fname);
        zfree(fname);
        server.rdbbkgdfsave.background = 0;
        /* call complete to ensure write commands allowed */
        bkgdsave_complete(REDIS_OK);

        if (WaitForSingleObject(server.rdbbkgdfsave.terminateevent, 0) == WAIT_OBJECT_0) {
            /* terminated early */
            return 1;
        }

        /* now write buffers to disk */
        server.rdbbkgdfsave.state = BKSAVE_WRITING;
        fp = fopen(server.rdbbkgdfsave.filename, server.rdbbkgdfsave.mode);
        if (fp == NULL) {
            redisLog(REDIS_WARNING,"Error opening temp DB file on the final destination: %s", strerror(errno));
            server.rdbbkgdfsave.state = BKSAVE_FAILED;
        }

        if (server.rdbbkgdfsave.state == BKSAVE_WRITING) {
            iter = listGetIterator(server.rdbbkgdfsave.buffers, AL_START_HEAD);
            if (iter != NULL) {
                while ((node = listNext(iter))) {
                    bkgdfsavehdr *curbuf = (bkgdfsavehdr *)listNodeValue(node);

                    if (WaitForSingleObject(server.rdbbkgdfsave.terminateevent, 0) == WAIT_OBJECT_0) {
                        /* terminate early */
                        return 1;
                    }
                    if (curbuf != NULL) {
                        rc = fwrite((char*)curbuf + sizeof(bkgdfsavehdr), curbuf->pos - sizeof(bkgdfsavehdr), 1, fp);
                        if (rc == 0) {
                            redisLog(REDIS_WARNING,"Error writing temp DB file on the final destination: %s", strerror(errno));
                            server.rdbbkgdfsave.state = BKSAVE_FAILED;
                            break;
                        }
                    }
                }
            }
        }
        if (fp != NULL) {
            if (server.rdbbkgdfsave.state == BKSAVE_WRITING) {
                fflush(fp);
                fsync(fileno(fp));
            }
            fclose(fp);
        }

        if (server.rdbbkgdfsave.state == BKSAVE_WRITING) {
            if (server.rdbbkgdfsave.filerename != NULL) {
                int renrc = 0;
                if (rename(server.rdbbkgdfsave.filename, server.rdbbkgdfsave.filerename) == -1) {
                    redisLog(REDIS_WARNING,"Error moving temp DB file on the final destination: %s. rename from %s to %s",
                                strerror(errno), server.rdbbkgdfsave.filename, server.rdbbkgdfsave.filerename);
                    unlink(server.rdbbkgdfsave.filename);
                    server.rdbbkgdfsave.state = BKSAVE_FAILED;
                }
            }
        } else {
            unlink(server.rdbbkgdfsave.filename);
        }

        if (server.rdbbkgdfsave.buffers != NULL ) {
            listSetFreeMethod(server.rdbbkgdfsave.buffers, zfree);
            listRelease(server.rdbbkgdfsave.buffers);
            server.rdbbkgdfsave.buffers = NULL;
        }
        if (server.rdbbkgdfsave.filename != NULL) {
            zfree(server.rdbbkgdfsave.filename);
            server.rdbbkgdfsave.filename = NULL;
        }
        if (server.rdbbkgdfsave.mode != NULL) {
            zfree(server.rdbbkgdfsave.mode);
            server.rdbbkgdfsave.mode = NULL;
        }
        if (server.rdbbkgdfsave.filerename != NULL) {
            zfree(server.rdbbkgdfsave.filerename);
            server.rdbbkgdfsave.filerename = NULL;
        }

        if (server.rdbbkgdfsave.state != BKSAVE_FAILED) {
            server.rdbbkgdfsave.state = BKSAVE_SUCCESS;
        }
        redisLog(REDIS_WARNING,"Winthread: completed background writing");
    }

    redisLog(REDIS_WARNING,"Winthread: exiting");
    return 0;
}

#endif
