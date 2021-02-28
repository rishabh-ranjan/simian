#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#ifdef DEBUG_MEM
#ifdef __cplusplus
extern "C" {
#endif
void *malloc2(size_t size, int line, const char *file) {
    void *result = malloc(size);
    printf("Malloc: %p, Line: %d, File: %s\n", result, line, file);
    return result;
}
void *strdup2(char *str, int line, const char *file) {
    void *result = strdup(str);
    printf("Malloc: %p, Line: %d, File: %s\n", result, line, file);
    return result;
}
void free2(void *address, int line, const char *file) {
    free(address);
    printf("Free: %p, Line: %d, File: %s\n", address, line, file);
}
#ifdef __cplusplus
}
#endif

#elif defined(DEBUG_PROF_MEM)

#include <map>
#include <utility>

std::map<void*, std::pair<int,char*> > mem;

#ifdef __cplusplus
extern "C" {
#endif
void *malloc2(size_t size, int line, const char *file) {
    void *result = malloc(size);
    mem.insert(std::make_pair(result, std::make_pair(line,file)));
    //printf("Malloc: %p, Line: %d, File: %s\n", result, line, file);
    return result;
}
void *strdup2(char *str, int line, const char *file) {
    void *result = strdup(str);
    mem.insert(std::make_pair(result, std::make_pair(line,file)));
    //printf("Malloc: %p, Line: %d, File: %s\n", result, line, file);
    return result;
}
void free2(void *address, int line, const char *file) {
    free(address);
    mem.erase(address);
    //printf("Free: %p, Line: %d, File: %s\n", address, line, file);
}

void memcheck() {
    std::map<void*, std::pair<int, char*> >::iterator mi = mem.begin();
    std::map<void*, std::pair<int, char*> >::iterator mie = mem.end();
    fprintf(stderr,"[memory leaks begin]\n");
    for(; mi!=mie; mi++) {
        fprintf(stderr,"%s:%d\t%x\n", mi->second.second, mi->second.first, mi->first);
    }
    fprintf(stderr,"[memory leaks end]\n");
}
#ifdef __cplusplus
}
#endif
#endif
