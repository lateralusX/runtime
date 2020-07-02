#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-rt-config.h"
#if !defined(EP_INCLUDE_SOURCE_FILES) || defined(EP_FORCE_INCLUDE_SOURCE_FILES)

#define DS_IMPL_IPC_GETTER_SETTER
#include "ds-ipc.h"
#include "ep-rt.h"

#include "ds-server.h"

//TODO: Move into rt shim.

typedef struct _rt_mono_array_internal_t ep_rt_connection_state_array_t;
typedef struct _rt_mono_array_iterator_internal_t ep_rt_connection_state_array_iterator_t;

EP_RT_DECLARE_ARRAY (connection_state_array, ep_rt_connection_state_array_t, IpcStreamFactoryConnectionState *)
EP_RT_DECLARE_ARRAY_ITERATOR (connection_state_array, ep_rt_connection_state_array_t, ep_rt_connection_state_array_iterator_t, IpcStreamFactoryConnectionState *)

EP_RT_DEFINE_ARRAY (connection_state_array, ep_rt_connection_state_array_t, IpcStreamFactoryConnectionState *)
EP_RT_DEFINE_ARRAY_ITERATOR (connection_state_array, ep_rt_connection_state_array_t, ep_rt_connection_state_array_iterator_t, IpcStreamFactoryConnectionState *)

typedef struct _rt_mono_array_internal_t ep_rt_ipc_poll_handle_array_t;
typedef struct _rt_mono_array_iterator_internal_t ep_rt_ipc_poll_handle_array_iterator_t;

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

/*
 * IpcStreamFactory.
 */

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

	int32_t poll_timeout_ms = DS_IPC_POLL_TIMEOUT_INFINITE;
	bool connect_success = true;
	uint32_t poll_attempts = 0;
	
	ep_rt_ipc_poll_handle_array_alloc (&ipc_poll_handles);

	while (!stream) {
		connect_success = true;

		ep_rt_connection_state_array_t *connection_state = &_ds_ipc_stream_factory_connection_states_array;
		ep_rt_connection_state_array_iterator_t connection_state_iterator;
		ep_rt_connection_state_array_iterator_begin (connection_state, &connection_state_iterator);
		while (!ep_rt_connection_state_array_iterator_end (connection_state, &connection_state_iterator)) {
			IpcStreamFactoryConnectionState *state = ep_rt_connection_state_array_iterator_value (connection_state, &connection_state_iterator);
			DiagnosticsIpcPollHandle handle;
			if (ds_ipc_stream_factory_connection_state_get_ipc_poll_handle_vcall (&handle, callback))
				ep_rt_ipc_poll_handle_array_append (&ipc_poll_handles, handle);
			else
				connect_success = false;

			ep_rt_connection_state_array_iterator_next (connection_state, &connection_state_iterator);
		}

		poll_timeout_ms = connect_success ?
			DS_IPC_POLL_TIMEOUT_INFINITE :
			ds_ipc_stream_factory_get_next_timeout (poll_timeout_ms);

		int32_t ret_val = ds_ipc_poll (&ipc_poll_handles, poll_timeout_ms, callback);
		poll_attempts++;
		DS_LOG_INFO_2 ("IpcStreamFactory::GetNextAvailableStream - Poll attempt: %d, timeout: %dms.\n", poll_attempts, poll_timeout_ms);

		if (ret_val != 0) {
			ep_rt_ipc_poll_handle_array_iterator_t ipc_poll_handles_iterator;
			ep_rt_ipc_poll_handle_array_iterator_begin (&ipc_poll_handles, &ipc_poll_handles_iterator);
			while (!ep_rt_ipc_poll_handle_array_iterator_end (&ipc_poll_handles, &ipc_poll_handles_iterator)) {
				DiagnosticsIpcPollHandle poll_handle = ep_rt_ipc_poll_handle_array_iterator_value (&ipc_poll_handles, &ipc_poll_handles_iterator);
				IpcStreamFactoryConnectionState *state = (IpcStreamFactoryConnectionState *)ds_ipc_poll_handle_userdata_get (&poll_handle);
				switch (ds_ipc_poll_handle_events_get (&poll_handle)) {
				case DS_IPC_POLL_EVENTS_HANGUP:
					EP_ASSERT (state != NULL);
					ds_ipc_stream_factor_connection_state_reset (state, callback);

				}



				ep_rt_connection_state_array_iterator_next (connection_state, &connection_state_iterator);
			}
		}
	}

}

#endif /* !defined(EP_INCLUDE_SOURCE_FILES) || defined(EP_FORCE_INCLUDE_SOURCE_FILES) */
#endif /* ENABLE_PERFTRACING */

#ifndef EP_INCLUDE_SOURCE_FILES
extern const char quiet_linker_empty_file_warning_diagnostics_ipc;
const char quiet_linker_empty_file_warning_diagnostics_ipc = 0;
#endif
