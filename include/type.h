#ifndef ERR_H
#define ERR_H

typedef enum linko_type TYPE;

enum linko_err {
    LINKO_NO_ERR = 0,
    LINKO_ERR,
};

enum linko_type {
    LINKO_OBJECT,
    LINKO_EXEC,
};

#endif
