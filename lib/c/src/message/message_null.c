//
// Created by Terence Fan on 3/4/19.
//

#include "message.h"

static void addDataNull(CatMessage *message, const char *data) {}

static void addKVNull(CatMessage *message, const char *dataKey, const char *dataValue) {};

static void setStatusNull(CatMessage *message, const char *status) {}

static void setTimestampNull(CatMessage *message, unsigned long long timestamp) {}

static void setCompleteNull(CatMessage *message) {}

CatMessage g_cat_nullMsg = {
        addDataNull,
        addKVNull,
        setStatusNull,
        setTimestampNull,
        setCompleteNull
};

static void _addDataNull(CatTransaction* trans, const char* data) {}

static void _addKVNull(CatTransaction* trans, const char* dataKey, const char* dataValue) {}

static void _setStatusNull(CatTransaction* trans, const char* status) {}

static void _setTimestampNull(CatTransaction* trans, unsigned long long timestamp) {}

static void _setCompleteNull(CatTransaction* trans) {}

static void _addChildNull(CatTransaction* trans, CatMessage* childMsg) {}

static void _setDurationInMillisNull(CatTransaction* trans, unsigned long long duration) {}

static void _setDurationStartNull(CatTransaction* trans, unsigned long long durationStart) {}

CatTransaction g_cat_nullTrans = {
        _addDataNull,
        _addKVNull,
        _setStatusNull,
        _setTimestampNull,
        _setCompleteNull,
        _addChildNull,
        _setDurationInMillisNull,
        _setDurationStartNull,
};

