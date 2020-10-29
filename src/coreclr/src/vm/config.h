#ifndef __CONFIG_H__
#define __CONFIG_H__

//#define FEATURE_PERFTRACING_C_LIB

#if defined(FEATURE_PERFTRACING) && defined(FEATURE_PERFTRACING_C_LIB)
#define ENABLE_PERFTRACING
#endif

#ifdef TARGET_WINDOWS
#define HOST_WIN32
#endif

#if defined(FEATURE_PROFAPI_ATTACH_DETACH) && defined(DACCESS_COMPILE)
#undef FEATURE_PROFAPI_ATTACH_DETACH
#endif

#endif /* __CONFIG_H__ */
