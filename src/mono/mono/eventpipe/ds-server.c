#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-rt-config.h"

#include "ds-server.h"
#include "ds-ipc.h"

// Option to include all internal source files into ds-server.c.
#ifdef EP_INCLUDE_SOURCE_FILES
#define EP_FORCE_INCLUDE_SOURCE_FILES

#else
#define EP_IMPL_EP_GETTER_SETTER
#include "ds-server.h"
#endif

/*
 * Globals and volatile access functions.
 */

static volatile uint32_t _ds_server_shutting_down_state = 0;
static ep_rt_wait_event_handle_t _ds_server_resume_runtime_startup_event = { 0 };

static
inline
bool
ds_server_volatile_load_shutting_down_state (void)
{
	return (ep_rt_volatile_load_uint32_t (&_ds_server_shutting_down_state) != 0) ? true : false;
}

static
inline
void
ds_server_volatile_store_shutting_down_state (bool state)
{
	ep_rt_volatile_store_uint32_t (&_ds_server_shutting_down_state, state ? 1 : 0);
}

/*
 * Forward declares of all static functions.
 */

static
void
error_callback_create (
	const ep_char8_t message,
	uint32_t code);

static
void
error_callback_close (
	const ep_char8_t message,
	uint32_t code);

static
void
warning_callback (
	const ep_char8_t message,
	uint32_t code);

/*
 * DiagnosticServer.
 */

static
void
error_callback_create (
	const ep_char8_t message,
	uint32_t code)
{
	EP_ASSERT (message != NULL);
	DS_LOG_ERROR_2 ("Failed to create diagnostic IPC: error (%d): %s.\n", code, message);
}

static
void
error_callback_close (
	const ep_char8_t message,
	uint32_t code)
{
	EP_ASSERT (message != NULL);
	DS_LOG_ERROR_2 ("Failed to close diagnostic IPC: error (%d): %s.\n", code, message);
}

static
void
warning_callback (
	const ep_char8_t message,
	uint32_t code)
{
	EP_ASSERT (message != NULL);
	DS_LOG_WARNING_2 ("warning (%d): %s.\n", code, message);
}

DS_RT_DEFINE_THREAD_FUNC (server_thread)
{
	EP_ASSERT (ds_server_volatile_load_shutting_down_state () == true || ds_ipc_stream_factory_has_active_connections () == true);

	if (!ds_ipc_stream_factory_has_active_connections ()) {
		DS_LOG_ERROR_0 ("Diagnostics IPC listener was undefined\n");
		return 1;
	}

	while (!ds_server_volatile_load_shutting_down_state ()) {
		IpcStream *stream = ds_ipc_stream_factor_get_next_available_stream (warning_callback);
		if (!stream)
			continue;

		DiagnosticsIpcMessage message;
		if (!ds_ipc_message_init (&message, stream)) {
			ds_ipc_message_send_error (stream, DS_IPC_E_BAD_ENCODING);
			ds_ipc_stream_free (stream);
			continue;
		}

		if (ep_rt_utf8_string_compare (
			(const ep_char8_t *)ds_ipc_message_header_get_magic (ds_ipc_message_get_header (&message)),
			(const ep_char8_t *)ds_ipc_magic_v1_get_magic ()) != 0) {

			ds_ipc_message_send_error (stream, DS_IPC_E_UNKNOWN_MAGIC);
			ep_ipc_stream_free (stream);
			continue;
		}

		switch ((DiagnosticServerCommandSet)ds_ipc_message_header_get_commandset (ds_ipc_message_get_header (&message))) {
		case DS_SERVER_COMMANDSET_SERVER:
			ds_protocol_helper_handle_ipc_message (message, stream);
			break;

		case DS_SERVER_COMMANDSET_EVENTPIPE:
			ep_protocol_helper_handle_ipc_message (message, stream);
			break;

		case DS_SERVER_COMMANDSET_DUMP:
#ifdef FEATURE_PROFAPI_ATTACH_DETACH
		case DS_SERVER_COMMAND_SET_PROFILER:
#endif // FEATURE_PROFAPI_ATTACH_DETACH
		default:
			DS_LOG_WARNING_1 ("Received unknown request type (%d)\n", ds_ipc_message_header_get_commandset (ds_ipc_message_get_header (&message)));
			ds_ipc_message_send_error (stream, DS_IPC_E_UNKNOWN_COMMAND);
			ep_ipc_stream_free (stream);
			break;
		}
	}

	return (ds_rt_thread_start_func_return_t)0;
}

bool
ds_server_init (void)
{
	if (!ds_rt_config_value_get_enable ())
		return true;

	bool success = false;
	ep_char8_t *address = NULL;

	address = ds_rt_config_value_get_monitor_address ();
	if (address) {
		// By default, opts in to Pause on Start.
		ep_rt_wait_event_alloc (&_ds_server_resume_runtime_startup_event, true, false);
		// Create the client mode connection.
		ep_raise_error_if_nok (ds_ipc_stream_factory_create_client (address, error_callback_create) == true);
	}

	ep_raise_error_if_nok (ds_ipc_stream_factory_create_server (NULL, error_callback_create) == true);

	if (ds_ipc_stream_factory_has_active_connections ()) {
		size_t thread_id = 0;

		if (!ep_rt_thread_create (NULL, NULL, &thread_id)) {
			// Failed to create IPC thread.
			ds_ipc_stream_factory_close_connections ();
			DS_LOG_ERROR_1 ("Failed to create diagnostic server thread (%d).\n", ds_rt_get_last_error ());
			ep_raise_error ();
		}
	}

ep_on_exit:
	ep_rt_utf8_string_free (address);
	return success;

ep_on_error:
	success = false;
	ep_exit_error_handler ();
}

bool
ds_server_shutdown (void)
{
	ds_server_volatile_store_shutting_down_state (true);

	if (ds_ipc_stream_factory_has_active_connections ())
		ds_ipc_stream_factory_shutdown (error_callback_close);

	return true;
}

// This method will block runtime bring-up IFF DOTNET_DiagnosticsMonitorAddress != NULL and DOTNET_DiagnosticsMonitorPauseOnStart!=0 (it's default state)
// The _ds_resume_runtime_startup_event event will be signaled when the Diagnostics Monitor uses the ResumeRuntime Diagnostics IPC Command
void
ds_server_pause_for_diagnostics_monitor (void)
{
	ep_char8_t *address = NULL;
	wchar_t *address_wcs = NULL;

	address = ds_rt_config_value_get_monitor_address ();
	if (address) {
		if (ds_rt_config_value_get_diagnostic_monitor_pause_on_start ()) {
			EP_ASSERT (ep_rt_wait_event_is_valid (&_ds_resume_runtime_startup_event) == true);
			address_wcs = ep_rt_utf8_to_wcs_string (address, -1);
			if (address_wcs) {
				printf ("The runtime has been configured to pause during startup and is awaiting a Diagnostics IPC ResumeStartup command from a server at '%ls'.\n", address_wcs);
				fflush (stdout);
			}
			DS_LOG_ALWAYS_0 ("The runtime has been configured to pause during startup and is awaiting a Diagnostics IPC ResumeStartup command.");
			if (ep_rt_wait_event_wait (&_ds_server_resume_runtime_startup_event, 5000, false) != 0) {
				DS_LOG_ALWAYS_0 ("The runtime has been configured to pause during startup and is awaiting a Diagnostics IPC ResumeStartup command and has waitied 5 seconds.");
				ep_rt_wait_event_wait (&_ds_server_resume_runtime_startup_event, EP_INFINITE_WAIT, false);
			}
		}
	}

	ep_rt_wcs_string_free (address_wcs);
	ep_rt_utf8_string_free (address);
}

void
ds_server_resume_runtime_startup (void)
{
	if (ep_rt_wait_event_is_valid (&_ds_server_resume_runtime_startup_event))
		ep_rt_wait_event_set (&_ds_server_resume_runtime_startup_event);
}

/*
 * DiagnosticServerProtocolHelper.
 */

void
ds_server_protocol_helper_handle_ipc_message (
	DiagnosticsIpcMessage *message,
	IpcStream *stream)
{
	EP_ASSERT (message != NULL);
	EP_ASSERT (stream != NULL);

	switch ((DiagnosticServerCommandId)ds_ipc_message_header_get_commandid (ds_ipc_message_get_header (&message))) {
	case DS_IPC_COMMAND_ID_RESUME_RUNTIME:
		ds_protocol_helper_resume_runtime_startup (message, stream);
		break;
	default:
		DS_LOG_WARNING_1 ("Received unknown request type (%d)\n", ds_ipc_message_header_get_commandset (ds_ipc_message_get_header (&message)));
		ds_ipc_message_send_error (stream, DS_IPC_E_UNKNOWN_COMMAND);
		ep_ipc_stream_free (stream);
		break;
	}
}

void
ds_server_protocol_helper_resume_runtime_startup (
	DiagnosticsIpcMessage *message,
	IpcStream *stream)
{
	EP_ASSERT (message != NULL);
	EP_ASSERT (stream != NULL);

	// no payload
	ds_resume_runtime_startup ();
	DiagnosticIpcMessage success_response;
	if (ds_ipc_message_init (&success_response, ds_ipc_generic_success_header_get (), DS_IPC_S_OK))
		ds_ipc_message_send (&success_response, stream);
}

#endif /* ENABLE_PERFTRACING */

extern const char quiet_linker_empty_file_warning_diagnostic_server;
const char quiet_linker_empty_file_warning_diagnostic_server = 0;
