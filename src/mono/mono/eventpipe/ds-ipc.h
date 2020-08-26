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
 * DiagnosticPortBuilder.
 */

#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_IPC_GETTER_SETTER)
//TODO: Implement.
struct _DiagnosticPortBuilder {
#else
struct _DiagnosticPortBuilder_Internal {
#endif
	ep_char8_t *path;
	DiagnosticPortType type;
	DiagnosticPortSuspendMode suspend_mode;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_IPC_GETTER_SETTER)
struct _DiagnosticPortBuilder {
	uint8_t _internal [sizeof (struct _DiagnosticPortBuilder_Internal)];
};
#endif

/*
 * IpcStreamFactory.
 */

void
ds_ipc_stream_factory_init (void);

void
ds_ipc_stream_factory_shutdown (ds_ipc_error_callback_func callback);

//bool
//ds_ipc_stream_factory_create_server (
//	const ep_char8_t *const ipc_name,
//	ds_ipc_error_callback_func callback);
//
//bool
//ds_ipc_stream_factory_create_client (
//	const ep_char8_t *const ipc_name,
//	ds_ipc_error_callback_func callback);

bool
ds_ipc_stream_factory_configure (ds_ipc_error_callback_func callback);

IpcStream *
ds_ipc_stream_factory_get_next_available_stream (ds_ipc_error_callback_func callback);

void
ds_ipc_stream_factory_resume_current_port (void);

bool
ds_ipc_stream_factory_any_suspended_ports (void);

bool
ds_ipc_stream_factory_has_active_ports (void);

void
ds_ipc_stream_factory_close_ports (ds_ipc_error_callback_func callback);

void
ds_ipc_stream_factory_shutdown (ds_ipc_error_callback_func callback);

/*
 * IpcStreamFactoryDiagnosticPort.
 */

typedef bool (*IpcStreamFactoryDiagnosticPortGetIPCPollHandleFunc)(void *object, DiagnosticsIpcPollHandle *handle, ds_ipc_error_callback_func callback);
typedef IpcStream * (*IpcStreamFactoryDiagnosticPortGetConnectedStreamFunc)(void *object, ds_ipc_error_callback_func callback);
typedef void (*IpcStreamFactoryDiagnosticPortReset)(void *object, ds_ipc_error_callback_func callback);

struct _IpcStreamFactoryDiagnosticPortVtable {
	IpcStreamFactoryDiagnosticPortGetIPCPollHandleFunc get_ipc_poll_handle_func;
	IpcStreamFactoryDiagnosticPortGetConnectedStreamFunc get_connected_stream_func;
	IpcStreamFactoryDiagnosticPortReset reset_func;
};

#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_IPC_GETTER_SETTER)
struct _IpcStreamFactoryDiagnosticPort {
#else
struct _IpcStreamFactoryDiagnosticPort_Internal {
#endif
	IpcStreamFactoryDiagnosticPortVtable *vtable;
	DiagnosticsIpc *ipc;
	IpcStream *stream;
	bool has_resumed_runtime;
	DiagnosticPortSuspendMode suspend_mode;
	DiagnosticPortType type;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_IPC_GETTER_SETTER)
struct _IpcStreamFactoryDiagnosticPort {
	uint8_t _internal [sizeof (struct _IpcStreamFactoryDiagnosticPort_Internal)];
};
#endif

IpcStreamFactoryDiagnosticPort *
ds_ipc_stream_factory_diagnostic_port_init (
	IpcStreamFactoryDiagnosticPort *diagnostic_port,
	IpcStreamFactoryDiagnosticPortVtable *vtable,
	DiagnosticsIpc *ipc,
	IpcStreamFactoryDiagnosticPortBuilder *builder);

void
ds_ipc_stream_factory_diagnostic_port_fini (IpcStreamFactoryDiagnosticPort *diagnostic_port);

// returns a pollable handle and performs any preparation required
// e.g., as a side-effect, will connect and advertise on reverse connections
bool
ds_ipc_stream_factory_diagnostic_port_get_ipc_poll_handle_vcall (
	IpcStreamFactoryDiagnosticPort * diagnostic_port,
	DiagnosticsIpcPollHandle *handle,
	ds_ipc_error_callback_func callback);

// Returns the signaled stream in a usable state
IpcStream *
ds_ipc_stream_factory_diagnostic_port_get_connected_stream_vcall (
	IpcStreamFactoryDiagnosticPort * diagnostic_port,
	ds_ipc_error_callback_func callback);

// Resets the connection in the event of a hangup
void
ds_ipc_stream_factory_diagnostic_port_reset_vcall (
	IpcStreamFactoryDiagnosticPort * diagnostic_port,
	ds_ipc_error_callback_func callback);

// closes the underlying connections
// only performs minimal cleanup if isShutdown==true
void
ds_ipc_stream_factory_diagnostic_port_close (
	IpcStreamFactoryDiagnosticPort * diagnostic_port,
	bool is_shutdown,
	ds_ipc_error_callback_func callback);

/*
 * IpcStreamFactoryDiagnosticPortBuilder.
 */

#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_IPC_GETTER_SETTER)
struct _IpcStreamFactoryDiagnosticPortBuilder {
#else
struct _IpcStreamFactoryDiagnosticPortBuilder_Internal {
#endif
	ep_char8_t *path;
	DiagnosticPortSuspendMode suspend_mode;
	DiagnosticPortType type;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_IPC_GETTER_SETTER)
struct _IpcStreamFactoryDiagnosticPortBuilder {
	uint8_t _internal [sizeof (struct _IpcStreamFactoryDiagnosticPortBuilder_Internal)];
};
#endif

IpcStreamFactoryDiagnosticPortBuilder *
ds_ipc_stream_factory_diagnostic_port_builder_init (IpcStreamFactoryDiagnosticPortBuilder *builder);

void
ds_ipc_stream_factory_diagnostic_port_builder_fini (IpcStreamFactoryDiagnosticPortBuilder *builder);

void
ds_ipc_stream_factory_diagnostic_port_builder_set_tag (
	IpcStreamFactoryDiagnosticPortBuilder *builder,
	ep_char8_t *tag);

#endif /* ENABLE_PERFTRACING */
#endif /** __DIAGNOSTICS_IPC_H__ **/
