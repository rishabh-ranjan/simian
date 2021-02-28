#if defined(DEBUG_MEM) || defined(DEBUG_PROF_MEM)
#ifdef __cplusplus
extern "C" {
#endif
void *malloc2(size_t size, int line, const char *file);
void *strdup2(char *str, int line, const char *file);
void free2(void *address, int line, const char *file);
#ifdef __cplusplus
}
#endif

#ifdef DEBUG_PROF_MEM
#include <map>
#include <utility>

extern std::map<void*, std::pair<int,char*> > mem;

#ifdef __cplusplus
extern "C" {
#endif
void memcheck();
#ifdef __cplusplus
}
#endif
#endif

#define malloc(size) malloc2(size, __LINE__, __FILE__)
#define strdup(str) strdup2(str, __LINE__, __FILE__)
#define free(address) free2(address, __LINE__, __FILE__)
#endif
