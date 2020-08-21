#ifndef __DIAGNOSTICS_IPC_H__
#define __DIAGNOSTICS_IPC_H__

#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-rt-config.h"
#include "ep-types.h"

#undef EP_IMPL_GETTER_SETTER
#ifdef EP_IMPL_STREAM_GETTER_SETTER
#define EP_IMPL_GETTER_SETTER
#endif
#define EP_GETTER_SETTER_PREFIX ds_
#include "ep-getter-setter.h"

// Polling timeout semantics
// If client connection is opted in
//   and connection succeeds => set timeout to infinite
//   and connection fails => set timeout to minimum and scale by falloff factor
// else => set timeout to -1 (infinite)
//
// If an agent closes its socket while we're still connected,
// Poll will return and let us know which connection hung up
#define DS_IPC_POLL_TIMEOUT_FALLOFF_FACTOR (float)1.25
#define DS_IPC_POLL_TIMEOUT_INFINITE (int32_t)-1
#define DS_IPC_POLL_TIMEOUT_MIN_MS (int32_t)10
#define DS_IPC_POLL_TIMEOUT_MAX_MS (int32_t)500

typedef void (*ds_ipc_error_callback_func)(
	const ep_char8_t *message,
	uint32_t code);

typedef enum {
	DS_IPC_CONNECTION_MODE_CLIENT,
	DS_IPC_CONNECTION_MODE_SERVER
} DiagnosticsIpcConnectionMode;

typedef enum {
	DS_IPC_POLL_EVENTS_TIMEOUT = 0x00, // implies timeout
	DS_IPC_POLL_EVENTS_SIGNALED = 0x01, // ready for use
	DS_IPC_POLL_EVENTS_HANGUP = 0x02, // connection remotely closed
	DS_IPC_POLL_EVENTS_ERR = 0x04 // other error
} DiagnosticsIpcPollEvents;

typedef struct _DiagnosticsIpc DiagnosticsIpc;
typedef struct _DiagnosticsIpcPollHandle DiagnosticsIpcPollHandle;
typedef struct _IpcStreamFactoryConnectionState IpcStreamFactoryConnectionState;

//TODO: Move into rt shim.

typedef struct _rt_mono_array_internal_t ep_rt_connection_state_array_t;
typedef struct _rt_mono_array_iterator_internal_t ep_rt_connection_state_array_iterator_t;

typedef struct _rt_mono_array_internal_t ep_rt_ipc_poll_handle_array_t;
typedef struct _rt_mono_array_iterator_internal_t ep_rt_ipc_poll_handle_array_iterator_t;

/*
 * DiagnosticsIpc.
 */

#if defined(EP_INLINE_GETTER_SETTER) || defined(DS_IMPL_IPC_GETTER_SETTER)
//TODO: Implement.
struct _DiagnosticsIpc {
#else
struct _DiagnosticsIpc_Internal {
#endif
	uint8_t dummy;
};

#if !defined(EP_INLINE_GETTER_SETTER) && !defined(DS_IMPL_IPC_GETTER_SETTER)
struct _DiagnosticsIpc {
	uint8_t _internal [sizeof (struct _DiagnosticsIpc_Internal)];
};
#endif

int32_t
ds_ipc_poll (ep_rt_ipc_poll_handle_array_t *poll_handles, uint32_t timeout_ms, ds_ipc_error_callback_func callback);

/*
 * DiagnosticsIpcPollHandle.
 */

// The bookeeping struct used for polling on server and client structs
#if defined(EP_INLINE_GETTER_SETTER) || defined(DS_IMPL_IPC_GETTER_SETTER)
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

#if !defined(EP_INLINE_GETTER_SETTER) && !defined(DS_IMPL_IPC_GETTER_SETTER)
struct _DiagnosticsIpcPollHandle {
	uint8_t _internal [sizeof (struct _DiagnosticsIpcPollHandle_Internal)];
};
#endif

EP_DEFINE_GETTER(DiagnosticsIpcPollHandle *, ipc_poll_handle, uint8_t, events)
EP_DEFINE_GETTER(DiagnosticsIpcPollHandle *, ipc_poll_handle, void *, user_data)

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
	ds_ipc_error_callback_func callback
);

bool
ds_ipc_stream_factory_create_client (
	const ep_char8_t *const ipc_name,
	ds_ipc_error_callback_func callback
);

IpcStream *
ds_ipc_stream_factory_get_next_available_stream (ds_ipc_error_callback_func callback);

bool
ds_ipc_stream_factory_has_active_connections (void);

void
ds_ipc_stream_factory_close_connections (ds_ipc_error_callback_func callback);

/*
 * IpcStreamFactoryConnectionState.
 */

#if defined(EP_INLINE_GETTER_SETTER) || defined(DS_IMPL_IPC_GETTER_SETTER)
//TODO: Implement.
struct _IpcStreamFactoryConnectionState {
#else
struct _IpcStreamFactoryConnectionState_Internal {
#endif
//	DiagnosticsIpc *ipc;
	IpcStream *stream;
};

#if !defined(EP_INLINE_GETTER_SETTER) && !defined(DS_IMPL_IPC_GETTER_SETTER)
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

#undef EP_GETTER_SETTER_PREFIX

#endif /* ENABLE_PERFTRACING */
#endif /** __DIAGNOSTICS_IPC_H__ **/
