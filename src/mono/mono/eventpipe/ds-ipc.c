#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-rt-config.h"
#if !defined(EP_INCLUDE_SOURCE_FILES) || defined(EP_FORCE_INCLUDE_SOURCE_FILES)

#define DS_IMPL_IPC_GETTER_SETTER
#include "ds-ipc.h"
#include "ep-rt.h"

#include "ds-server.h"

EP_RT_DECLARE_ARRAY (connection_state_array, ep_rt_connection_state_array_t, IpcStreamFactoryConnectionState *)
EP_RT_DECLARE_ARRAY_ITERATOR (connection_state_array, ep_rt_connection_state_array_t, ep_rt_connection_state_array_iterator_t, IpcStreamFactoryConnectionState *)

EP_RT_DEFINE_ARRAY (connection_state_array, ep_rt_connection_state_array_t, IpcStreamFactoryConnectionState *)
EP_RT_DEFINE_ARRAY_ITERATOR (connection_state_array, ep_rt_connection_state_array_t, ep_rt_connection_state_array_iterator_t, IpcStreamFactoryConnectionState *)

EP_RT_DECLARE_ARRAY (ipc_poll_handle_array, ep_rt_ipc_poll_handle_array_t, DiagnosticsIpcPollHandle)
EP_RT_DECLARE_ARRAY_ITERATOR (ipc_poll_handle_array, ep_rt_ipc_poll_handle_array_t, ep_rt_ipc_poll_handle_array_iterator_t, DiagnosticsIpcPollHandle)

EP_RT_DEFINE_ARRAY (ipc_poll_handle_array, ep_rt_ipc_poll_handle_array_t, DiagnosticsIpcPollHandle)
EP_RT_DEFINE_ARRAY_ITERATOR (ipc_poll_handle_array, ep_rt_ipc_poll_handle_array_t, ep_rt_ipc_poll_handle_array_iterator_t, DiagnosticsIpcPollHandle)

/*
 * Globals and volatile access functions.
 */

static volatile uint32_t _ds_ipc_stream_factory_shutting_down_state = 0;
static ep_rt_connection_state_array_t _ds_ipc_stream_factory_connection_states_array = { 0 };

static
inline
bool
ds_ipc_stream_factory_load_shutting_down_state (void)
{
	return (ep_rt_volatile_load_uint32_t (&_ds_ipc_stream_factory_shutting_down_state) != 0) ? true : false;
}

static
inline
void
ds_ipc_stream_factory_store_shutting_down_state (bool state)
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
	ep_rt_connection_state_array_alloc (&_ds_ipc_stream_factory_connection_states_array);
}

void
ds_ipc_stream_factory_shutdown (ds_ipc_error_callback_func callback)
{
	//TODO: Implement.
	EP_ASSERT (callback == NULL);
	ep_rt_connection_state_array_free (&_ds_ipc_stream_factory_connection_states_array);
}

bool
ds_ipc_stream_factory_has_active_connections (void)
{
	return !ds_ipc_stream_factory_load_shutting_down_state () &&
		ep_rt_connection_state_array_size (&_ds_ipc_stream_factory_connection_states_array) > 0;
}

IpcStream *
ds_ipc_stream_factory_get_next_available_stream (ds_ipc_error_callback_func callback)
{
	IpcStream *stream = NULL;
	ep_rt_ipc_poll_handle_array_t ipc_poll_handles;
	DiagnosticsIpcPollHandle ipc_poll_handle;
	ep_rt_connection_state_array_t *connection_states = &_ds_ipc_stream_factory_connection_states_array;
	IpcStreamFactoryConnectionState *connection_state = NULL;

	int32_t poll_timeout_ms = DS_IPC_POLL_TIMEOUT_INFINITE;
	bool connect_success = true;
	uint32_t poll_attempts = 0;
	
	ep_rt_ipc_poll_handle_array_alloc (&ipc_poll_handles);

	while (!stream) {
		connect_success = true;
		ep_rt_connection_state_array_iterator_t connection_states_iterator;
		ep_rt_connection_state_array_iterator_begin (connection_states, &connection_states_iterator);
		while (!ep_rt_connection_state_array_iterator_end (connection_states, &connection_states_iterator)) {
			connection_state = ep_rt_connection_state_array_iterator_value (&connection_states_iterator);
			DiagnosticsIpcPollHandle handle;
			if (ds_ipc_stream_factory_connection_state_get_ipc_poll_handle_vcall (connection_state, &ipc_poll_handle, callback))
				ep_rt_ipc_poll_handle_array_append (&ipc_poll_handles, ipc_poll_handle);
			else
				connect_success = false;

			ep_rt_connection_state_array_iterator_next (connection_states, &connection_states_iterator);
		}

		poll_timeout_ms = connect_success ?
			DS_IPC_POLL_TIMEOUT_INFINITE :
			ipc_stream_factory_get_next_timeout (poll_timeout_ms);

		int32_t ret_val = ds_ipc_poll (&ipc_poll_handles, poll_timeout_ms, callback);
		poll_attempts++;
		DS_LOG_INFO_2 ("IpcStreamFactory::GetNextAvailableStream - Poll attempt: %d, timeout: %dms.\n", poll_attempts, poll_timeout_ms);

		if (ret_val != 0) {
			ep_rt_ipc_poll_handle_array_iterator_t ipc_poll_handles_iterator;
			ep_rt_ipc_poll_handle_array_iterator_begin (&ipc_poll_handles, &ipc_poll_handles_iterator);
			while (!ep_rt_ipc_poll_handle_array_iterator_end (&ipc_poll_handles, &ipc_poll_handles_iterator)) {
				ipc_poll_handle = ep_rt_ipc_poll_handle_array_iterator_value (&ipc_poll_handles_iterator);
				connection_state = (IpcStreamFactoryConnectionState *)ds_ipc_poll_handle_get_user_data (&ipc_poll_handle);
				switch (ds_ipc_poll_handle_get_events (&ipc_poll_handle)) {
				case DS_IPC_POLL_EVENTS_HANGUP:
					EP_ASSERT (state != NULL);
					ds_ipc_stream_factory_connection_state_reset (connection_state, callback);
					DS_LOG_INFO_1 ("IpcStreamFactory::GetNextAvailableStream - Poll attempt: %d, connection hung up.\n", nPollAttempts);
					poll_timeout_ms = DS_IPC_POLL_TIMEOUT_MIN_MS;
					break;
				case DS_IPC_POLL_EVENTS_SIGNALED:
					EP_ASSERT (state != NULL);
					if (!stream)  // only use first signaled stream; will get others on subsequent calls
						stream = ds_ipc_stream_factory_connection_state_get_connected_stream (connection_state, callback);
					break;
				case DS_IPC_POLL_EVENTS_ERR:
					ep_raise_error ();
				default:
					// TODO: Error handling
					break;
				}

				ep_rt_ipc_poll_handle_array_iterator_next (&ipc_poll_handles, &ipc_poll_handles_iterator);
			}
		}

		// clear the view.
		ep_rt_ipc_poll_handle_array_clear (&ipc_poll_handles, NULL);
	}

ep_on_exit:
	return stream;

ep_on_error:
	stream = NULL;
	ep_exit_error_handler ();
}

#endif /* !defined(EP_INCLUDE_SOURCE_FILES) || defined(EP_FORCE_INCLUDE_SOURCE_FILES) */
#endif /* ENABLE_PERFTRACING */

#ifndef EP_INCLUDE_SOURCE_FILES
extern const char quiet_linker_empty_file_warning_diagnostics_ipc;
const char quiet_linker_empty_file_warning_diagnostics_ipc = 0;
#endif
