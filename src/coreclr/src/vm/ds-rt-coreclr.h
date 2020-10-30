// Implementation of ds-rt.h targeting Mono runtime.
#ifndef __DIAGNOSTICS_RT_MONO_H__
#define __DIAGNOSTICS_RT_MONO_H__

#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-rt-coreclr.h"
#include "ds-profiler-protocol.h"
#include "ds-dump-protocol.h"

#undef DS_LOG_ALWAYS_0
#define DS_LOG_ALWAYS_0(msg) STRESS_LOG0(LF_DIAGNOSTICS_PORT, LL_ALWAYS, msg)

#undef DS_LOG_ALWAYS_1
#define DS_LOG_ALWAYS_1(msg, data1) STRESS_LOG1(LF_DIAGNOSTICS_PORT, LL_ALWAYS, msg, data1)

#undef DS_LOG_ALWAYS_2
#define DS_LOG_ALWAYS_2(msg, data1, data2) STRESS_LOG2(LF_DIAGNOSTICS_PORT, LL_ALWAYS, msg, data1, data2)

#undef DS_LOG_INFO_0
#define DS_LOG_INFO_0(msg) STRESS_LOG0(LF_DIAGNOSTICS_PORT, LL_INFO10, msg)

#undef DS_LOG_INFO_1
#define DS_LOG_INFO_1(msg, data1) STRESS_LOG1(LF_DIAGNOSTICS_PORT, LL_INFO10, msg, data1)

#undef DS_LOG_INFO_2
#define DS_LOG_INFO_2(msg, data1, data2) STRESS_LOG2(LF_DIAGNOSTICS_PORT, LL_INFO10, msg, data1, data2)

#undef DS_LOG_ERROR_0
#define DS_LOG_ERROR_0(msg) STRESS_LOG0(LF_DIAGNOSTICS_PORT, LL_ERROR, msg)

#undef DS_LOG_ERROR_1
#define DS_LOG_ERROR_1(msg, data1) STRESS_LOG1(LF_DIAGNOSTICS_PORT, LL_ERROR, msg, data1)

#undef DS_LOG_ERROR_2
#define DS_LOG_ERROR_2(msg, data1, data2) STRESS_LOG2(LF_DIAGNOSTICS_PORT, LL_ERROR, msg, data1, data2)

#undef DS_LOG_WARNING_0
#define DS_LOG_WARNING_0(msg) STRESS_LOG0(LF_DIAGNOSTICS_PORT, LL_WARNING, msg)

#undef DS_LOG_WARNING_1
#define DS_LOG_WARNING_1(msg, data1) STRESS_LOG1(LF_DIAGNOSTICS_PORT, LL_WARNING, msg, data1)

#undef DS_LOG_WARNING_2
#define DS_LOG_WARNING_2(msg, data1, data2) STRESS_LOG2(LF_DIAGNOSTICS_PORT, LL_WARNING, msg, data1, data2)

#undef DS_LOG_DEBUG_0
#define DS_LOG_DEBUG_0(msg) STRESS_LOG0(LF_DIAGNOSTICS_PORT, LL_INFO1000, msg)

#undef DS_ENTER_BLOCKING_PAL_SECTION
#define DS_ENTER_BLOCKING_PAL_SECTION

#undef DS_EXIT_BLOCKING_PAL_SECTION
#define DS_EXIT_BLOCKING_PAL_SECTION

#define DS_RT_DEFINE_ARRAY(array_name, array_type, iterator_type, item_type) \
	EP_RT_DEFINE_ARRAY_PREFIX(ds, array_name, array_type, iterator_type, item_type)

#define DS_RT_DEFINE_ARRAY_ITERATOR(array_name, array_type, iterator_type, item_type) \
	EP_RT_DEFINE_ARRAY_ITERATOR_PREFIX(ds, array_name, array_type, iterator_type, item_type)

/*
* AutoTrace.
*/

#ifdef FEATURE_AUTO_TRACE
#include "autotrace.h"
#endif

static
inline
void
ds_rt_auto_trace_init (void)
{
#ifdef FEATURE_AUTO_TRACE
	auto_trace_init ();
#endif
}

static
inline
void
ds_rt_auto_trace_launch (void)
{
#ifdef FEATURE_AUTO_TRACE
	auto_trace_launch ();
#endif
}

static
inline
void
ds_rt_auto_trace_signal (void)
{
#ifdef FEATURE_AUTO_TRACE
	auto_trace_signal ();
#endif
}

static
inline
void
ds_rt_auto_trace_wait (void)
{
#ifdef FEATURE_AUTO_TRACE
	auto_trace_wait ();
#endif
}

/*
 * DiagnosticsConfiguration.
 */

static
inline
bool
ds_rt_config_value_get_enable (void)
{
	return CLRConfig::GetConfigValue (CLRConfig::EXTERNAL_EnableDiagnostics) != 0;
}

static
inline
ep_char8_t *
ds_rt_config_value_get_ports (void)
{
	CLRConfigStringHolder value(CLRConfig::GetConfigValue (CLRConfig::EXTERNAL_DOTNET_DiagnosticPorts));
	return ep_rt_utf16_to_utf8_string (reinterpret_cast<ep_char16_t *>(value.GetValue ()), -1);
}

static
inline
uint32_t
ds_rt_config_value_get_default_port_suspend (void)
{
	return static_cast<uint32_t>(CLRConfig::GetConfigValue (CLRConfig::EXTERNAL_DOTNET_DefaultDiagnosticPortSuspend));
}

/*
* DiagnosticsDump.
*/

static
inline
uint32_t
ds_rt_generate_core_dump (DiagnosticsGenerateCoreDumpCommandPayload *payload)
{
	uint32_t result = DS_IPC_E_FAIL;
#ifdef HOST_WIN32
	if (GenerateCrashDump (reinterpret_cast<LPCWSTR>(ds_generate_core_dump_command_payload_get_dump_name (payload)),
		static_cast<int32_t>(ds_generate_core_dump_command_payload_get_dump_type (payload)),
		(ds_generate_core_dump_command_payload_get_diagnostics (payload) != 0) ? true : false))
		result = DS_IPC_S_OK;
#else
	MAKE_UTF8PTR_FROMWIDE_NOTHROW (dump_name, payload->dumpName);
	if (dump_name != nullptr) {
		if (PAL_GenerateCoreDump (dump_name,
			static_cast<int32_t>(ds_generate_core_dump_command_payload_get_dump_type (payload)),
			(ds_generate_core_dump_command_payload_get_diagnostics (payload) != 0) ? true : false))
			result = DS_IPC_S_OK;
	}
#endif
	return result;
}

/*
 * DiagnosticsIpc.
 */

static
inline
void
ds_rt_transport_get_default_name (
	ep_char8_t *name,
	int32_t name_len,
	const ep_char8_t *prefix,
	int32_t id,
	const ep_char8_t *group_id,
	const ep_char8_t *suffix)
{
#ifdef TARGET_UNIX
	PAL_GetTransportName (name_len, name, prefix, id, group_id, suffix);
#endif
}

/*
 * DiagnosticsIpcPollHandle.
 */

DS_RT_DEFINE_ARRAY (ipc_poll_handle_array, ds_rt_ipc_poll_handle_array_t, ds_rt_ipc_poll_handle_array_iterator_t, DiagnosticsIpcPollHandle)
DS_RT_DEFINE_ARRAY_ITERATOR (ipc_poll_handle_array, ds_rt_ipc_poll_handle_array_t, ds_rt_ipc_poll_handle_array_iterator_t, DiagnosticsIpcPollHandle)

/*
 * DiagnosticsPort.
 */

DS_RT_DEFINE_ARRAY (port_array, ds_rt_port_array_t, ds_rt_port_array_iterator_t, DiagnosticsPort *)
DS_RT_DEFINE_ARRAY_ITERATOR (port_array, ds_rt_port_array_t, ds_rt_port_array_iterator_t, DiagnosticsPort *)

DS_RT_DEFINE_ARRAY (port_config_array, ds_rt_port_config_array_t, ds_rt_port_config_array_iterator_t, ep_char8_t *)
DS_RT_DEFINE_ARRAY_ITERATOR (port_config_array, ds_rt_port_config_array_t, ds_rt_port_config_array_iterator_t, ep_char8_t *)

/*
* DiagnosticsProfiler.
*/

#ifdef FEATURE_PROFAPI_ATTACH_DETACH
#include "profilinghelper.h"
#include "profilinghelper.inl"

static
inline
uint32_t
ds_rt_profiler_attach (DiagnosticsAttachProfilerCommandPayload *payload)
{
	if (!g_profControlBlock.fProfControlBlockInitialized)
		return DS_IPC_E_RUNTIME_UNINITIALIZED;

	// Certain actions are only allowable during attach, and this flag is how we track it.
	ClrFlsSetThreadType (ThreadType_ProfAPI_Attach);

	HRESULT hr;
	EX_TRY {
		hr = ProfilingAPIUtility::LoadProfilerForAttach (reinterpret_cast<const CLSID *>(ds_attach_profiler_command_payload_get_profiler_guid_cref (payload)),
			reinterpret_cast<LPCWSTR>(ds_attach_profiler_command_payload_get_profiler_path (payload)),
			reinterpret_cast<LPVOID>(ds_attach_profiler_command_payload_get_client_data (payload)),
			static_cast<UINT>(ds_attach_profiler_command_payload_get_client_data_len (payload)),
			static_cast<DWORD>(ds_attach_profiler_command_payload_get_attach_timeout (payload)));
	}
	EX_CATCH_HRESULT (hr);

	// Clear the flag so this thread isn't permanently marked as the attach thread.
	ClrFlsClearThreadType (ThreadType_ProfAPI_Attach);

	return hr;
}
#endif // FEATURE_PROFAPI_ATTACH_DETACH

#endif /* ENABLE_PERFTRACING */
#endif /* __DIAGNOSTICS_RT_MONO_H__ */
