#pragma once

#if defined(_WIN32)
#include <malloc.h>
#define VORBIS_STACK_ALLOC(size) _malloca(size)
#define VORBIS_STACK_FREE(mem) _freea(mem)
#else
#include <alloca.h>
#define VORBIS_STACK_ALLOC(size) alloca(size)
#define VORBIS_STACK_FREE(mem)
#endif
