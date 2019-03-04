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
#include "message.h"

#include "lib/cat_clog.h"
#include "lib/cat_time_util.h"

int isCatTransaction(CatMessage* message) {
    CatMessageInner* pInner = getInnerMsg(message);
    return pInner->h.msgType == CatMessageType_Trans;
}

int isCatEvent(CatMessage* message) {
    CatMessageInner* pInner = getInnerMsg(message);
    return pInner->h.msgType == CatMessageType_Event;
}

int isCatMetric(CatMessage* message) {
    CatMessageInner* pInner = getInnerMsg(message);
    return pInner->h.msgType == CatMessageType_Metric;
}

int isCatHeartBeat(CatMessage* message) {
    CatMessageInner* pInner = getInnerMsg(message);
    return pInner->h.msgType == CatMessageType_HeartBeat;
}

static void addChild(CatTransaction* message, CatMessage* childMsg) {
    CatTransactionInner* pInner = getInnerTrans(message);
    int pushRst = pushBackCATStaticQueue(pInner->children, childMsg);
    if (CATSTATICQUEUE_ERR == pushRst) {
        INNER_LOG(CLOG_ERROR, "Transaction Add Child Error！%d", getCATStaticQueueSize(pInner->children));
    }
}

void* clearCatTransaction(CatMessage* message) {
    clearCatMessage(message);

    CatTransactionInner* pInner = getInnerTrans(message);
    size_t i = 0;
    for (; i < getCATStaticQueueSize(pInner->children); ++i) {
        CatMessage* pMessage = getCATStaticQueueByIndex(pInner->children, i);
        deleteCatMessage(pMessage);
    }
    destroyCATStaticQueue(pInner->children);
    return pInner;
}

static void setTransactionComplete(CatTransaction* message) {
    CatTransactionInner* pInner = getInnerTrans(message);
    if (pInner->inner.isCompleted) {
        // do nothing.
    } else {
        if (pInner->durationInMicro == 0) {
            pInner->durationInMicro = GetTime64() * 1000 - pInner->durationStartInNano / 1000;
        }
        pInner->inner.isCompleted = 1;
        pInner->inner.flush((CatMessage*) message);
    }
}

CATStaticQueue* getCatTransactionChildren(CatTransaction* pSrcTrans) {
    CatTransactionInner* pInner = getInnerTrans(pSrcTrans);
    return pInner->children;
}

static void setDurationInMillis(CatTransaction* trans, unsigned long long durationMs) {
    setCatTransactionDurationUs(trans, durationMs * 1000);
}

static void setDurationStart(CatTransaction* trans, unsigned long long durationStartMs) {
    CatTransactionInner* pInner = getInnerTrans(trans);
    pInner->durationStartInNano = durationStartMs * 1000 * 1000;
}

CatTransaction* createCatTransaction(const char* type, const char* name, flusher flush) {
    CatTransactionInner* pTransInner = malloc(sizeof(CatTransaction) + sizeof(CatTransactionInner));
    if (pTransInner == NULL) {
        return NULL;
    }
    CatTransaction* pTrans = (CatTransaction*) (((char*) pTransInner + sizeof(CatTransactionInner)));

    initCatMessage((CatMessage*) pTrans, CatMessageType_Trans, type, name, flush);

    pTransInner->children = createCATStaticQueue(2048); // TODO need a auto scale list.
    pTransInner->durationStartInNano = GetTime64() * 1000 * 1000;
    pTransInner->durationInMicro = 0;

    pTrans->complete = setTransactionComplete;

    pTrans->addChild = addChild;
    pTrans->setDurationInMillis = setDurationInMillis;
    pTrans->setDurationStart = setDurationStart;

    pTrans->setStatus(pTrans, CAT_SUCCESS); // status default success
    return pTrans;
}

unsigned long long getCatTransactionDurationUs(CatTransaction* trans) {
    CatTransactionInner* pInner = getInnerTrans(trans);

    if (pInner->durationInMicro > 0) {
        return pInner->durationInMicro;
    } else {
        unsigned long long tmpDuration = 0;
        size_t len = pInner->children == NULL ? 0 : getCATStaticStackSize(pInner->children);

        if (len > 0 && pInner->children != NULL) {
            CatMessage* lastChild = getCATStaticStackByIndex(pInner->children, len - 1);
            CatMessageInner* lastChildInner = getInnerMsg(lastChild);

            if (isCatTransaction(lastChild)) {
                CatTransactionInner* pInner = getInnerTrans(lastChild);
                tmpDuration = (lastChildInner->timestampInMillis - pInner->inner.timestampInMillis) * 1000 +
                              pInner->durationInMicro;
            } else {
                tmpDuration = (lastChildInner->timestampInMillis - pInner->inner.timestampInMillis) * 1000;
            }
        }
        return tmpDuration;
    }

}

CatTransaction* copyCatTransaction(CatTransaction* pSrcTrans) {
    CatTransactionInner* pSrcTransInner = getInnerTrans(pSrcTrans);
    CatTransaction* clonedTrans = createCatTransaction(
            pSrcTransInner->inner.type,
            pSrcTransInner->inner.name,
            pSrcTransInner->inner.flush
    );
    CatTransactionInner* clonedTransInner = getInnerTrans(clonedTrans);

    clonedTransInner->inner.timestampInMillis = pSrcTransInner->inner.timestampInMillis;
    clonedTransInner->durationInMicro = getCatTransactionDurationUs(pSrcTrans);
    clonedTransInner->inner.data = catsdsdup(pSrcTransInner->inner.data);
    return clonedTrans;
}

