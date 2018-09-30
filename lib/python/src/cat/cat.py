#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Author: stdrickforce (Tengyuan Fan)
# Email: <stdrickforce@gmail.com> <fantengyuan@meituan.com>

__all__ = ['init', 'CAT_SUCCESS', 'CAT_ERROR']

import logging

from .const import *  # noqa

from .utils import (
    synchronized,
)

from .container import container

from .sdk import (
    catSdk as catSdkDefault,
    catSdkCoroutine,
)

log = logging.getLogger()


@synchronized
def init(appkey, **kwargs):
    if container.contains("catsdk"):
        log.warning("cat sdk has already been initialized!")

    if kwargs.get("logview", True) is False:
        sdk = catSdkCoroutine(appkey, **kwargs)
    else:
        sdk = catSdkDefault(appkey, **kwargs)
    container.put("catsdk", sdk)
