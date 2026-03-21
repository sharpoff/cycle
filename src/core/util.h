#pragma once

#define SAFE_FREE(obj)   \
    if ((obj)) {         \
        (obj) = nullptr; \
        delete (obj);    \
    }