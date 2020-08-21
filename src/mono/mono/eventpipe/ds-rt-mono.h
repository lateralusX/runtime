// Implementation of ds-rt.h targeting Mono runtime.
#ifndef __DIAGNOSTICS_RT_MONO_H__
#define __DIAGNOSTICS_RT_MONO_H__

#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-rt-mono.h"

#undef DS_LOG_ALWAYS_0
#define DS_LOG_ALWAYS_0(msg)

#undef DS_LOG_ALWAYS_1
#define DS_LOG_ALWAYS_1(msg, data1)

#undef DS_LOG_ALWAYS_2
#define DS_LOG_ALWAYS_2(msg, data1, data2)

#undef DS_LOG_INFO_0
#define DS_LOG_INFO_0(msg)

#undef DS_LOG_INFO_1
#define DS_LOG_INFO_1(msg, data1)

#undef DS_LOG_INFO_2
#define DS_LOG_INFO_2(msg, data1, data2)

#undef DS_LOG_ERROR_0
#define DS_LOG_ERROR_0(msg)

#undef DS_LOG_ERROR_1
#define DS_LOG_ERROR_1(msg, data1)

#undef DS_LOG_ERROR_2
#define DS_LOG_ERROR_2(msg, data1, data2)

#undef DS_LOG_WARNING_0
#define DS_LOG_WARNING_0(msg)

#undef DS_LOG_WARNING_1
#define DS_LOG_WARNING_1(msg, data1)

#undef DS_LOG_WARNING_2
#define DS_LOG_WARNING_2(msg, data1, data2)

typedef struct _rt_mono_array_internal_t ds_rt_connection_state_array_t;
typedef struct _rt_mono_array_iterator_internal_t ds_rt_connection_state_array_iterator_t;

typedef struct _rt_mono_array_internal_t ds_rt_ipc_poll_handle_array_t;
typedef struct _rt_mono_array_iterator_internal_t ds_rt_ipc_poll_handle_array_iterator_t;

#define DS_RT_DEFINE_ARRAY(array_name, array_type, item_type) \
	EP_RT_DEFINE_ARRAY_PREFIX(ds, array_name, array_type, item_type)

#define DS_RT_DEFINE_ARRAY_ITERATOR(array_name, array_type, iterator_type, item_type) \
	EP_RT_DEFINE_ARRAY_ITERATOR_PREFIX(ds, array_name, array_type, iterator_type, item_type)

static
inline
bool
ds_rt_config_value_get_enable (void)
{
	bool enable = false;
	gchar *value = g_getenv ("COMPlus_EnableDiagnostics");
	if (value && atoi (value) == 1)
		enable = true;
	g_free (value);
	return enable;
}

static
inline
ep_char8_t *
ds_rt_config_value_get_monitor_address (void)
{
	return g_getenv ("DOTNET_DiagnosticsMonitorAddress");
}

static
inline
bool
ds_rt_config_value_get_diagnostics_monitor_pause_on_start (void)
{
	bool enable = false;
	gchar *value = g_getenv ("DOTNET_DiagnosticsMonitorPauseOnStart");
	if (value && atoi (value) != 0)
		enable = true;
	g_free (value);
	return enable;
}

#endif /* ENABLE_PERFTRACING */
#endif /* __DIAGNOSTICS_RT_MONO_H__ */
