#ifndef __DIAGNOSTICS_IPC_H__
#define __DIAGNOSTICS_IPC_H__

#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ds-rt-config.h"
#include "ds-types.h"
#include "ds-rt.h"

#undef DS_IMPL_GETTER_SETTER
#ifdef DS_IMPL_IPC_GETTER_SETTER
#define DS_IMPL_GETTER_SETTER
#endif
#include "ds-getter-setter.h"

/*
 * DiagnosticsIpc.
 */

#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_IPC_GETTER_SETTER)
//TODO: Implement.
struct _DiagnosticsIpc {
#else
struct _DiagnosticsIpc_Internal {
#endif
	uint8_t dummy;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_IPC_GETTER_SETTER)
struct _DiagnosticsIpc {
	uint8_t _internal [sizeof (struct _DiagnosticsIpc_Internal)];
};
#endif

int32_t
ds_ipc_poll (
	ds_rt_ipc_poll_handle_array_t *poll_handles,
	uint32_t timeout_ms,
	ds_ipc_error_callback_func callback);

/*
 * DiagnosticsIpcPollHandle.
 */

// The bookeeping struct used for polling on server and client structs
#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_IPC_GETTER_SETTER)
//TODO: Implement.
struct _DiagnosticsIpcPollHandle {
#else
struct _DiagnosticsIpcPollHandle_Internal {
#endif
	// Only one of these will be non-null, treat as a union
	DiagnosticsIpc *ipc;
	IpcStream *stream;

	// contains some set of PollEvents
	// will be set by Poll
	// Any values here are ignored by Poll
	uint8_t events;

	// a cookie assignable by upstream users for additional bookkeeping
	void *user_data;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_IPC_GETTER_SETTER)
struct _DiagnosticsIpcPollHandle {
	uint8_t _internal [sizeof (struct _DiagnosticsIpcPollHandle_Internal)];
};
#endif

EP_DEFINE_GETTER(DiagnosticsIpcPollHandle *, ipc_poll_handle, uint8_t, events)
EP_DEFINE_GETTER(DiagnosticsIpcPollHandle *, ipc_poll_handle, void *, user_data)

DS_RT_DEFINE_ARRAY (ipc_poll_handle_array, ds_rt_ipc_poll_handle_array_t, DiagnosticsIpcPollHandle)
DS_RT_DEFINE_ARRAY_ITERATOR (ipc_poll_handle_array, ds_rt_ipc_poll_handle_array_t, ds_rt_ipc_poll_handle_array_iterator_t, DiagnosticsIpcPollHandle)

/*
 * IpcStreamFactory.
 */

void
ds_ipc_stream_factory_init (void);

void
ds_ipc_stream_factory_shutdown (ds_ipc_error_callback_func callback);

bool
ds_ipc_stream_factory_create_server (
	const ep_char8_t *const ipc_name,
	ds_ipc_error_callback_func callback);

bool
ds_ipc_stream_factory_create_client (
	const ep_char8_t *const ipc_name,
	ds_ipc_error_callback_func callback);

IpcStream *
ds_ipc_stream_factory_get_next_available_stream (ds_ipc_error_callback_func callback);

bool
ds_ipc_stream_factory_has_active_connections (void);

void
ds_ipc_stream_factory_close_connections (ds_ipc_error_callback_func callback);

/*
 * IpcStreamFactoryConnectionState.
 */

#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_IPC_GETTER_SETTER)
//TODO: Implement.
struct _IpcStreamFactoryConnectionState {
#else
struct _IpcStreamFactoryConnectionState_Internal {
#endif
//	DiagnosticsIpc *ipc;
	IpcStream *stream;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_IPC_GETTER_SETTER)
struct _IpcStreamFactoryConnectionState {
	uint8_t _internal [sizeof (struct _IpcStreamFactoryConnectionState_Internal)];
};
#endif

// returns a pollable handle and performs any preparation required
// e.g., as a side-effect, will connect and advertise on reverse connections
bool
ds_ipc_stream_factory_connection_state_get_ipc_poll_handle_vcall (
	IpcStreamFactoryConnectionState * connection_state,
	DiagnosticsIpcPollHandle *handle,
	ds_ipc_error_callback_func callback);

IpcStream *
ds_ipc_stream_factory_connection_state_get_connected_stream (
	IpcStreamFactoryConnectionState * connection_state,
	ds_ipc_error_callback_func callback);

void
ds_ipc_stream_factory_connection_state_reset (
	IpcStreamFactoryConnectionState * connection_state,
	ds_ipc_error_callback_func callback);

DS_RT_DEFINE_ARRAY (connection_state_array, ds_rt_connection_state_array_t, IpcStreamFactoryConnectionState *)
DS_RT_DEFINE_ARRAY_ITERATOR (connection_state_array, ds_rt_connection_state_array_t, ds_rt_connection_state_array_iterator_t, IpcStreamFactoryConnectionState *)

#endif /* ENABLE_PERFTRACING */
#endif /** __DIAGNOSTICS_IPC_H__ **/
