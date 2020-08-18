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

#endif /* ENABLE_PERFTRACING */
#endif /** __DIAGNOSTICS_IPC_H__ **/
