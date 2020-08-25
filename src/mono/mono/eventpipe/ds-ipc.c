#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ds-rt-config.h"
#if !defined(DS_INCLUDE_SOURCE_FILES) || defined(DS_FORCE_INCLUDE_SOURCE_FILES)

#define DS_IMPL_IPC_GETTER_SETTER
#include "ds-ipc.h"

/*
 * Globals and volatile access functions.
 */

static volatile uint32_t _ds_ipc_stream_factory_shutting_down_state = 0;
static ds_rt_diagnostic_port_array_t _ds_ipc_stream_factory_diagnostic_port_array = { 0 };

// set this in get_next_available_stream, and then expose a callback that
// allows us to track which connections have sent their ResumeRuntime commands
static IpcStreamFactoryDiagnosticPort *_ds_ipc_stream_factory_current_port = NULL;

static
inline
bool
ipc_stream_factory_load_shutting_down_state (void)
{
	return (ep_rt_volatile_load_uint32_t (&_ds_ipc_stream_factory_shutting_down_state) != 0) ? true : false;
}

static
inline
void
ipc_stream_factory_store_shutting_down_state (bool state)
{
	ep_rt_volatile_store_uint32_t (&_ds_ipc_stream_factory_shutting_down_state, state ? 1 : 0);
}

/*
 * Forward declares of all static functions.
 */

static
uint32_t
ipc_stream_factory_get_next_timeout (uint32_t current_timout_ms);

/*
 * IpcStreamFactory.
 */

static
inline
uint32_t
ipc_stream_factory_get_next_timeout (uint32_t current_timeout_ms)
{
	if (current_timeout_ms == DS_IPC_POLL_TIMEOUT_INFINITE)
		return DS_IPC_POLL_TIMEOUT_MIN_MS;
	else
		return (current_timeout_ms >= DS_IPC_POLL_TIMEOUT_MAX_MS) ?
			DS_IPC_POLL_TIMEOUT_MAX_MS :
			(uint32_t)((float)current_timeout_ms * DS_IPC_POLL_TIMEOUT_FALLOFF_FACTOR);
}

void
ds_ipc_stream_factory_init (void)
{
	ds_rt_diagnostic_port_array_alloc (&_ds_ipc_stream_factory_diagnostic_port_array);
}

void
ds_ipc_stream_factory_shutdown (ds_ipc_error_callback_func callback)
{
	//TODO: Implement.
	EP_ASSERT (callback == NULL);
	ds_rt_diagnostic_port_array_free (&_ds_ipc_stream_factory_diagnostic_port_array);
}

bool
ds_ipc_stream_factory_has_active_ports (void)
{
	return !ipc_stream_factory_load_shutting_down_state () &&
		ds_rt_diagnostic_port_array_size (&_ds_ipc_stream_factory_diagnostic_port_array) > 0;
}

IpcStream *
ds_ipc_stream_factory_get_next_available_stream (ds_ipc_error_callback_func callback)
{
	DS_LOG_INFO_0 ("IpcStreamFactory::GetNextAvailableStream - ENTER");

	IpcStream *stream = NULL;
	ds_rt_ipc_poll_handle_array_t ipc_poll_handles;
	DiagnosticsIpcPollHandle ipc_poll_handle;
	ds_rt_diagnostic_port_array_t *diagnostic_ports = &_ds_ipc_stream_factory_diagnostic_port_array;
	IpcStreamFactoryDiagnosticPort *diagnostic_port = NULL;

	int32_t poll_timeout_ms = DS_IPC_POLL_TIMEOUT_INFINITE;
	bool connect_success = true;
	uint32_t poll_attempts = 0;
	
	ds_rt_ipc_poll_handle_array_alloc (&ipc_poll_handles);

	while (!stream) {
		connect_success = true;
		ds_rt_diagnostic_port_array_iterator_t diagnostic_ports_iterator;
		ds_rt_diagnostic_port_array_iterator_begin (diagnostic_ports, &diagnostic_ports_iterator);
		while (!ds_rt_diagnostic_port_array_iterator_end (diagnostic_ports, &diagnostic_ports_iterator)) {
			diagnostic_port = ds_rt_diagnostic_port_array_iterator_value (&diagnostic_ports_iterator);
			DiagnosticsIpcPollHandle handle;
			if (ds_ipc_stream_factory_diagnostic_port_get_ipc_poll_handle_vcall (diagnostic_port, &ipc_poll_handle, callback))
				ds_rt_ipc_poll_handle_array_append (&ipc_poll_handles, ipc_poll_handle);
			else
				connect_success = false;

			ds_rt_diagnostic_port_array_iterator_next (diagnostic_ports, &diagnostic_ports_iterator);
		}

		poll_timeout_ms = connect_success ?
			DS_IPC_POLL_TIMEOUT_INFINITE :
			ipc_stream_factory_get_next_timeout (poll_timeout_ms);

		poll_attempts++;
		DS_LOG_INFO_2 ("IpcStreamFactory::GetNextAvailableStream - Poll attempt: %d, timeout: %dms.\n", poll_attempts, poll_timeout_ms);
//TODO: FIX when having PAL.
//		for (uint32_t i = 0; i < rgIpcPollHandles.Size(); i++)
//		{
//			if (rgIpcPollHandles[i].pIpc != nullptr)
//			{
//#ifdef TARGET_UNIX
//				STRESS_LOG2(LF_DIAGNOSTICS_PORT, LL_INFO10, "\tSERVER IpcPollHandle[%d] = { _serverSocket = %d }\n", i, rgIpcPollHandles[i].pIpc->_serverSocket);
//#else
//				STRESS_LOG3(LF_DIAGNOSTICS_PORT, LL_INFO10, "\tSERVER IpcPollHandle[%d] = { _hPipe = %d, _oOverlap.hEvent = %d }\n", i, rgIpcPollHandles[i].pIpc->_hPipe, rgIpcPollHandles[i].pIpc->_oOverlap.hEvent);
//#endif
//			}
//			else
//			{
//#ifdef TARGET_UNIX
//				STRESS_LOG2(LF_DIAGNOSTICS_PORT, LL_INFO10, "\tCLIENT IpcPollHandle[%d] = { _clientSocket = %d }\n", i, rgIpcPollHandles[i].pStream->_clientSocket);
//#else
//				STRESS_LOG3(LF_DIAGNOSTICS_PORT, LL_INFO10, "\tCLIENT IpcPollHandle[%d] = { _hPipe = %d, _oOverlap.hEvent = %d }\n", i, rgIpcPollHandles[i].pStream->_hPipe, rgIpcPollHandles[i].pStream->_oOverlap.hEvent);
//#endif
//			}
//		}
		int32_t ret_val = ds_ipc_poll (&ipc_poll_handles, poll_timeout_ms, callback);
		bool saw_error = false;

		if (ret_val != 0) {
			uint32_t connection_id = 0;
			ds_rt_ipc_poll_handle_array_iterator_t ipc_poll_handles_iterator;
			ds_rt_ipc_poll_handle_array_iterator_begin (&ipc_poll_handles, &ipc_poll_handles_iterator);
			while (!ds_rt_ipc_poll_handle_array_iterator_end (&ipc_poll_handles, &ipc_poll_handles_iterator)) {
				ipc_poll_handle = ds_rt_ipc_poll_handle_array_iterator_value (&ipc_poll_handles_iterator);
				diagnostic_port = (IpcStreamFactoryDiagnosticPort *)ipc_poll_handle.user_data;
				switch (ipc_poll_handle.events) {
				case DS_IPC_POLL_EVENTS_HANGUP:
					EP_ASSERT (state != NULL);
					ds_ipc_stream_factory_diagnostic_port_reset (diagnostic_port, callback);
					DS_LOG_INFO_2 ("IpcStreamFactory::GetNextAvailableStream - HUP :: Poll attempt: %d, connection %d hung up.\n", nPollAttempts, connection_id);
					poll_timeout_ms = DS_IPC_POLL_TIMEOUT_MIN_MS;
					break;
				case DS_IPC_POLL_EVENTS_SIGNALED:
					EP_ASSERT (state != NULL);
					if (!stream) {  // only use first signaled stream; will get others on subsequent calls
						stream = ds_ipc_stream_factory_diagnostic_port_get_connected_stream (diagnostic_port, callback);
						_ds_ipc_stream_factory_current_port = diagnostic_port;
					}
					DS_LOG_INFO_2 ("IpcStreamFactory::GetNextAvailableStream - SIG :: Poll attempt: %d, connection %d signalled.\n", nPollAttempts, connection_id);
					break;
				case DS_IPC_POLL_EVENTS_ERR:
					DS_LOG_INFO_2 ("IpcStreamFactory::GetNextAvailableStream - ERR :: Poll attempt: %d, connection %d errored.\n", nPollAttempts, connection_id);
					saw_error = true;
					break;
				case DS_IPC_POLL_EVENTS_NONE:
					DS_LOG_INFO_2 ("IpcStreamFactory::GetNextAvailableStream - NON :: Poll attempt: %d, connection %d had no events.\n", nPollAttempts, connection_id);
					break;
				default:
					DS_LOG_INFO_2 ("IpcStreamFactory::GetNextAvailableStream - UNK :: Poll attempt: %d, connection %d had invalid PollEvent.\n", nPollAttempts, connection_id);
					saw_error = true;
					break;
				}

				ds_rt_ipc_poll_handle_array_iterator_next (&ipc_poll_handles, &ipc_poll_handles_iterator);
				connection_id++;
			}
		}

		if (!stream && saw_error) {
			_ds_ipc_stream_factory_current_port = NULL;
			ep_raise_error ();
		}

		// clear the view.
		ds_rt_ipc_poll_handle_array_clear (&ipc_poll_handles, NULL);
	}

ep_on_exit:

//TODO: FIX when having PAL.
//#ifdef TARGET_UNIX
//	STRESS_LOG2(LF_DIAGNOSTICS_PORT, LL_INFO10, "IpcStreamFactory::GetNextAvailableStream - EXIT :: Poll attempt: %d, stream using handle %d.\n", nPollAttempts, pStream->_clientSocket);
//#else
//	STRESS_LOG2(LF_DIAGNOSTICS_PORT, LL_INFO10, "IpcStreamFactory::GetNextAvailableStream - EXIT :: Poll attempt: %d, stream using handle %d.\n", nPollAttempts, pStream->_hPipe);
//#endif

	return stream;

ep_on_error:
	stream = NULL;
	ep_exit_error_handler ();
}

#endif /* !defined(DS_INCLUDE_SOURCE_FILES) || defined(DS_FORCE_INCLUDE_SOURCE_FILES) */
#endif /* ENABLE_PERFTRACING */

#ifndef EP_INCLUDE_SOURCE_FILES
extern const char quiet_linker_empty_file_warning_diagnostics_ipc;
const char quiet_linker_empty_file_warning_diagnostics_ipc = 0;
#endif
