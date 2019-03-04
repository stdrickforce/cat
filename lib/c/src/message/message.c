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

#include "lib/cat_time_util.h"

static void addData(CatMessage* message, const char* data) {
    CatMessageInner* pInner = getInnerMsg(message);
    if (NULL == pInner->data) {
        pInner->data = catsdsnew(data);
    } else {
        pInner->data = catsdscat(pInner->data, "&");
        pInner->data = catsdscat(pInner->data, data);
    }
}

static void addKV(CatMessage* message, const char* dataKey, const char* dataValue) {
    CatMessageInner* pInner = getInnerMsg(message);
    if (NULL == pInner->data) {
        pInner->data = catsdsnew(dataKey);
        pInner->data = catsdscat(pInner->data, "=");
        pInner->data = catsdscat(pInner->data, dataValue);
    } else {
        pInner->data = catsdscat(pInner->data, "&");
        pInner->data = catsdscat(pInner->data, dataKey);
        pInner->data = catsdscat(pInner->data, "=");
        pInner->data = catsdscat(pInner->data, dataValue);
    }
}

static void setStatus(CatMessage* message, const char* status) {
    CatMessageInner* pInner = getInnerMsg(message);
    if (pInner->status == NULL) {
        pInner->status = catsdsnew(status);
    } else {
        pInner->status = catsdscpy(pInner->status, status);
    }
}

static void setTimestamp(CatMessage* message, unsigned long long timeMs) {
    CatMessageInner* pInner = getInnerMsg(message);
    pInner->timestampInMillis = timeMs;
}

void* clearCatMessage(CatMessage* message) {
    CatMessageInner* pInner = getInnerMsg(message);
    if (pInner->status != NULL) {
        catsdsfree(pInner->status);
        pInner->status = NULL;
    }
    if (pInner->data != NULL) {
        catsdsfree(pInner->data);
        pInner->data = NULL;
    }
    if (pInner->type != NULL) {
        catsdsfree(pInner->type);
        pInner->type = NULL;
    }
    if (pInner->name != NULL) {
        catsdsfree(pInner->name);
        pInner->name = NULL;
    }
    return pInner;
}

CatHeartBeat* createHeartBeat(const char* type, const char* name, flusher flush) {
    return createMessage(CatMessageType_HeartBeat, type, name, flush);
}

CatHeartBeat* createEvent(const char* type, const char* name, flusher flush) {
    return createMessage(CatMessageType_Event, type, name, flush);
}

CatMetric* createMetric(const char* type, const char* name, flusher flush) {
    return createMessage(CatMessageType_Metric, type, name, flush);
}

void initCatMessage(CatMessage* pMsg, char msgType, const char* type, const char* name, flusher flush) {
    CatMessageInner* pInner = getInnerMsg(pMsg);
    memset(pInner, 0, sizeof(CatMessage) + sizeof(CatMessageInner));
    pInner->h.msgType = msgType;
    pInner->timestampInMillis = GetTime64();
    pInner->type = catsdsnew(type);
    pInner->name = catsdsnew(name);
    pInner->flush = flush;

    pMsg->addData = addData;
    pMsg->addKV = addKV;
    pMsg->complete = setMessageComplete;
    pMsg->setTimestamp = setTimestamp;
    pMsg->setStatus = setStatus;
}
