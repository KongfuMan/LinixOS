#include "types.h"

// set n int values start from physical addr `dst`
void*
memset(void* dst, int value, uint len){
    char *cdst = (char *) dst;
    for(int i = 0; i < len; i++){
        cdst[i] = value;
    }
    return dst;
}
