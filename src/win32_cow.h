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

#ifdef _WIN32
/************************************************************************
 * This module defines copy on write to support
 * saving on a background thread in Windows.
 ************************************************************************/

/* collections are converted to read only arrays */
typedef struct cowListArray {
    int numele;
    listNode le[];
} cowListArray;

typedef struct cowDictArray {
    int numele;
    dictEntry de[];
} cowDictArray;

typedef struct dictZEntry {
    dictEntry de;
    double score;
} dictZEntry;

typedef struct cowDictZArray {
    int numele;
    dictZEntry zde[];
} cowDictZArray;

/* Special read only iterator for hash dictionary can iterate over
 * regular hash table encoding or array of entries.
 * Used only for background save with in process copy on write.
 * If the hash needs to be copied, it is converted to a readonly array */
typedef struct roDictIter {
    cowDictArray *ar;
    dict *hdict;
    dictIterator *di;
    int pos;
} roDictIter;

/* Special read only iterator for zset hash dictionary can iterate over
 * regular hash table encoding or array of entries.
 * Used only for background save copy on write.
 * If the hash needs to be copied, it is converted to a readonly array */
typedef struct roZDictIter {
    cowDictZArray *ar;
    dict *hdict;
    dictIterator *di;
    int pos;
} roZDictIter;

/* Special read only iterator for list can iterate over
 * regular list encoding or array of entries.
 * Used only for background save with in process copy on write.
 * If the list needs to be copied, it is converted to a readonly array */
typedef struct roListIter {
    cowListArray *ar;
    list *olist;
    listIter li;
    int pos;
} roListIter;

/* current iterators in use.
 * If the current object is converted to an array
 * then the current iterator must be converted as well */
typedef struct bkgdIters {
    roDictIter *curDbDictIter;
    roDictIter *curObjDictIter;
    roZDictIter *curObjZDictIter;
    roListIter *curObjListIter;
    CRITICAL_SECTION csMigrate;
} bkgditers;


/* structure for top level DB dictionary extensions
   used to change and restore destructor type,
   and to track read only array snapshot */
typedef struct bkgdDbExt {
    dictType *savedType;
    dictType *cowType;
    dictType *readonlyType;
    cowDictArray *dictArray;
    int id;
} bkgdDbExt;

/* wincow functions */
void cowInit();
void cowBkgdSaveStart();
void cowBkgdSaveStop();
void cowLock();
void cowUnlock();
int deferFreeObject(void *obj);
int roDBDictSize(int id);
roDictIter *roDBGetIterator(int id);
roDictIter *roDictGetIterator(dict *d, cowDictArray *ro);
dictEntry *roDictNext(roDictIter *iter);
void roDictReleaseIterator(roDictIter *iter);
roZDictIter *roZDictGetIterator(dict *d, cowDictZArray *ro);
dictEntry *roZDictNext(roZDictIter *iter);
void roZDictReleaseIterator(roZDictIter *iter);
roListIter *roListGetIterator(list *l, cowListArray *ro);
void roListRewind(list *l, cowListArray *ro, roListIter *iter);
listNode *roListNext(roListIter *iter);
void roListReleaseIterator(roListIter *iter);
void *getRoConvertedObj(void *key, void *o);
void cowReleaseListArray(cowListArray *ar);
void cowReleaseDictArray(cowDictArray *ar);
void cowReleaseDictZArray(cowDictZArray *ar);


/* redis.c functions used in wincow */
int dictEncObjKeyCompare(void *privdata, const void *key1, const void *key2);
unsigned int dictEncObjHash(const void *key);

#else
/* define read only iterator types and methods as normal iterator types and methods */
#define roDictIter             dictIterator
#define roZDictIter            dictIterator
#define roListIter             listIter
#define roDictGetIterator(a,b) dictGetIterator((a))
#define roZDictGetIterator(a,b) dictGetIterator((a))
#define roListRewind(a,b,c)    listRewind((a),(c))
#define roDictNext             dictNext
#define roZDictNext            dictNext
#define roListNext             listNext
#define roDictReleaseIterator  dictReleaseIterator
#define roZDictReleaseIterator dictReleaseIterator
#define cowLock()
#define cowUnlock()
#endif
