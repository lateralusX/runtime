#ifndef __DIAGNOSTIC_SERVER_H__
#define __DIAGNOSTIC_SERVER_H__

#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-rt-config.h"
#include "ep-rt.h"

#include "ds-protocol.h"

//Error codes used in logging.
//TODO: Convert into hex.
#define DS_IPC_S_OK ((unsigned long)0L)
#define DS_IPC_E_BAD_ENCODING (((unsigned long)(1)<<31) | ((unsigned long)(19)<<16) | ((unsigned long)(0x1384)))
#define DS_IPC_E_UNKNOWN_COMMAND (((unsigned long)(1)<<31) | ((unsigned long)(19)<<16) | ((unsigned long)(0x1385)))
#define DS_IPC_E_UNKNOWN_MAGIC (((unsigned long)(1)<<31) | ((unsigned long)(19)<<16) | ((unsigned long)(0x1386)))

//TODO: MOVE all this into rt shim.
//Logging macros used in diagnostic server.
#define DS_LOG_ALWAYS_0(msg)
#define DS_LOG_ALWAYS_1(msg, data1)
#define DS_LOG_ALWAYS_2(msg, data1, data2)
#define DS_LOG_INFO_0(msg)
#define DS_LOG_INFO_1(msg, data1)
#define DS_LOG_INFO_2(msg, data1, data2)
#define DS_LOG_ERROR_0(msg)
#define DS_LOG_ERROR_1(msg, data1)
#define DS_LOG_ERROR_2(msg, data1, data2)
#define DS_LOG_WARNING_0(msg)
#define DS_LOG_WARNING_1(msg, data1)
#define DS_LOG_WARNING_2(msg, data1, data2)

typedef void (*ds_error_callback_func)(
	const ep_char8_t *message,
	uint32_t code);

typedef MonoThreadStart ds_rt_thread_start_func;
typedef mono_thread_start_return_t ds_rt_thread_start_func_return_t;
#define DS_RT_DEFINE_THREAD_FUNC(name) static mono_thread_start_return_t WINAPI ## name ## (gpointer data)

static
inline
int
ds_rt_get_last_error (void)
{
#ifdef HOST_WIN32
	return GetLastError ();
#else
	return errno;
#endif
}

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
ds_rt_config_value_get_diagnostic_monitor_pause_on_start (void)
{
	bool enable = false;
	gchar *value = g_getenv ("DOTNET_DiagnosticsMonitorPauseOnStart");
	if (value && atoi (value) != 0)
		enable = true;
	g_free (value);
	return enable;
}

/*
 * DiagnosticServer.
 */

// Initialize the event pipe (Creates the EventPipe IPC server).
bool
ds_server_init (void);

// Shutdown the event pipe.
bool
ds_server_shutdown (void);

// Pauses runtime startup after the Diagnostics Server has been started
// allowing a Diagnostics Monitor to attach perform tasks before
// Startup is completed
EP_NEVER_INLINE
void
ds_server_pause_for_diagnostics_monitor (void);

// Sets event to resume startup in runtime
// This is a no-op if not configured to pause or runtime has already resumed
void
ds_server_resume_runtime_startup (void);

/*
 * DiagnosticServerProtocolHelper.
 */

void
ds_server_protocol_helper_handle_ipc_message (
	DiagnosticsIpcMessage *message,
	IpcStream *stream);

void
ds_server_protocol_helper_resume_runtime_startup (
	DiagnosticsIpcMessage *message,
	IpcStream *stream);

#endif /* ENABLE_PERFTRACING */
#endif /** __DIAGNOSTIC_SERVER_H__ **/
