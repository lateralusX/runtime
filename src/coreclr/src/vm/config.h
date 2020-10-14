#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef FEATURE_PERFTRACING
#define ENABLE_PERFTRACING
#endif

#ifdef TARGET_WINDOWS
#define HOST_WIN32
#endif

#if defined(FEATURE_PROFAPI_ATTACH_DETACH) && defined(DACCESS_COMPILE)
#undef FEATURE_PROFAPI_ATTACH_DETACH
#endif

#endif /* __CONFIG_H__ */
