/*
 * Copyright (c) 2011-2018, Meituan Dianping. All Rights Reserved.
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements. See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef CCAT_MESSAGE_H
#define CCAT_MESSAGE_H

#include <string.h>
#include <stdlib.h>

#include "client.h"

#include "lib/cat_sds.h"
#include "lib/cat_static_queue.h"

#define CatMessageType_Trans 'T'
#define CatMessageType_Event 'E'
#define CatMessageType_HeartBeat 'H'
#define CatMessageType_Metric 'M'

#define CAT_SUCCESS_CHAR '0'

typedef void (* flusher)(CatMessage*);

typedef struct _CatMessageInner {

    union {
        char msgType;
        u_int32_t unused;
    } h;

    sds type;
    sds name;
    sds status;
    sds data;

    unsigned long long timestampInMillis; // created timestamp in milliseconds.

    int isCompleted;

    flusher flush;

} CatMessageInner;

typedef struct _CatTransactionInner {
    CATStaticQueue* children;
    unsigned long long durationStartInNano;     // nanoseconds
    unsigned long long durationInMicro;         // microseconds

    // TransactionInner should end up with a MessageInner
    CatMessageInner inner;
} CatTransactionInner;

/**
 * memory map
 * trans: [ CatTransactionInner (CatMessageInner) | (CatMessage) CatTransaction ]
 * other:                      [ CatMessageInner  |  CatMessage ]
 */
#define getInnerMsg(pMsg) ((CatMessageInner*)(((char*)(pMsg)) - sizeof(CatMessageInner)))

#define getInnerTrans(pMsg) ((CatTransactionInner*)(((char*)(pMsg)) - sizeof(CatTransactionInner)))

void* clearCatMessage(CatMessage* message);

void* clearCatTransaction(CatMessage* message);

void initCatMessage(CatMessage* pMsg, char msgType, const char* type, const char* name, flusher flush);

CatHeartBeat* createHeartBeat(const char* type, const char* name, flusher flush);

CatEvent* createEvent(const char* type, const char* name, flusher flush);

CatMetric* createMetric(const char* type, const char* name, flusher flush);

CatTransaction* createCatTransaction(const char* type, const char* name, flusher flush);

CatTransaction* copyCatTransaction(CatTransaction* pSrcTrans);

int isCatTransaction(CatMessage* message);

int isCatEvent(CatMessage* message);

int isCatMetric(CatMessage* message);

int isCatHeartBeat(CatMessage* message);

static inline void deleteCatMessage(CatMessage* message) {
    void* p;
    if (isCatTransaction(message)) {
        p = clearCatTransaction(message);
    } else {
        p = clearCatMessage(message);
    }
    free(p);
}

static inline int isCatMessageComplete(CatMessage* message) {
    CatMessageInner* pInner = getInnerMsg(message);
    return pInner->isCompleted;
}

static inline unsigned long long getCatMessageTimeStamp(CatMessage* message) {
    CatMessageInner* pInner = getInnerMsg(message);
    return pInner->timestampInMillis;
}

static inline sds getCatMessageType(CatMessage* message) {
    CatMessageInner* pInner = getInnerMsg(message);
    return pInner->type;
}

static inline int checkCatMessageSuccess(CatMessage* message) {
    CatMessageInner* pInner = getInnerMsg(message);
    if (pInner->status == NULL || catsdslen(pInner->status) != 1 || pInner->status[0] != CAT_SUCCESS_CHAR) {
        return 0;
    }
    return 1;
}

static inline void setMessageComplete(CatMessage* message) {
    CatMessageInner* inner = getInnerMsg(message);
    inner->isCompleted = 1;
    inner->flush(message);
}

unsigned long long getCatTransactionDurationUs(CatTransaction* trans);

static void inline setCatTransactionDurationUs(CatTransaction* trans, unsigned long long durationUs) {
    CatTransactionInner* pInner = getInnerTrans(trans);
    pInner->durationInMicro = durationUs;
}

CATStaticQueue* getCatTransactionChildren(CatTransaction* pSrcTrans);

static inline CatMessage* createMessage(char msgType, const char* type, const char* name, flusher flush) {
    CatMessageInner* pInner = malloc(sizeof(CatHeartBeat) + sizeof(CatMessageInner));
    if (pInner == NULL) {
        return NULL;
    }
    CatMessage* message = (CatMessage*) ((char*) pInner + sizeof(CatMessageInner));

    initCatMessage(message, msgType, type, name, flush);
    return message;
}

#endif //CCAT_MESSAGE_H
