#include "types.h"

// set n int values start from physical addr `dst`
void* memset(void* dst, int value, uint len){
    char *cdst = (char *) dst;
    for(int i = 0; i < len; i++){
        cdst[i] = value;
    }
    return dst;
}

// void* memmove(void* dst, const void* src, uint len){
//     char *cdst = (char *) dst;
//     char *csrc = (char *) src;
//     for(int i = 0; i < len; i++){
//         cdst[i] = csrc[i];
//     }
//     return dst;
// }

void*
memmove(void *dst, const void *src, uint n)
{
  const char *s;
  char *d;

  if(n == 0)
    return dst;
  
  s = src;
  d = dst;
  if(s < d && s + n > d){
    s += n;
    d += n;
    while(n-- > 0)
      *--d = *--s;
  } else
    while(n-- > 0)
      *d++ = *s++;

  return dst;
}

char* safestrcpy(char *s, const char *t, int n){
    char *os;

    os = s;
    if(n <= 0)
        return os;
    while(--n > 0 && (*s++ = *t++) != 0)
    ;
    *s = 0;
    return os;
}