#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ds-rt-config.h"
#if !defined(DS_INCLUDE_SOURCE_FILES) || defined(DS_FORCE_INCLUDE_SOURCE_FILES)

#define DS_IMPL_IPC_GETTER_SETTER
#include "ds-ipc.h"
#include "ds-protocol.h"
#include "ep-stream.h"

/*
 * Globals and volatile access functions.
 */

static volatile uint32_t _ds_shutting_down_state = 0;
static ds_rt_port_array_t _ds_port_array = { 0 };

// set this in get_next_available_stream, and then expose a callback that
// allows us to track which connections have sent their ResumeRuntime commands
static DiagnosticsPort *_ds_current_port = NULL;

static
inline
bool
load_shutting_down_state (void)
{
	return (ep_rt_volatile_load_uint32_t (&_ds_shutting_down_state) != 0) ? true : false;
}

static
inline
void
store_shutting_down_state (bool state)
{
	ep_rt_volatile_store_uint32_t (&_ds_shutting_down_state, state ? 1 : 0);
}

/*
 * Forward declares of all static functions.
 */

static
uint32_t
ipc_stream_factory_get_next_timeout (uint32_t current_timout_ms);

static
void
ipc_stream_factory_split_port_config (
	ep_char8_t *config,
	const ep_char8_t *delimiters,
	ds_rt_port_config_array_t *config_array);

static
bool
ipc_stream_factory_build_and_add_port (
	DiagnosticsPortBuilder *builder,
	ds_ipc_error_callback_func callback);

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

static
void
ipc_stream_factory_split_port_config (
	ep_char8_t *config,
	const ep_char8_t *delimiters,
	ds_rt_port_config_array_t *config_array)
{
	ep_char8_t *part = NULL;
	ep_char8_t *context = NULL;
	ep_char8_t *cursor = config;

	EP_ASSERT (config != NULL);
	EP_ASSERT (context != NULL);
	EP_ASSERT (cursor != NULL);

	part = ep_rt_utf8_string_strtok (cursor, delimiters, &context);
	while (part) {
		ds_rt_port_config_array_append (config_array, part);
		part = ep_rt_utf8_string_strtok (NULL, delimiters, &context);
	}
}

static
bool
ipc_stream_factory_build_and_add_port (
	DiagnosticsPortBuilder *builder,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (builder != NULL);
	EP_ASSERT (callback != NULL);

	bool success = false;
	DiagnosticsIpc *ipc = NULL;

	if (builder->type == DS_PORT_TYPE_LISTEN) {
		ipc = ds_ipc_alloc (builder->path, DS_IPC_CONNECTION_MODE_LISTEN, callback);
		ep_raise_error_if_nok (ipc != NULL);
		ep_raise_error_if_nok (ds_ipc_listen (ipc, callback) == true);
		ds_rt_port_array_append (&_ds_port_array, (DiagnosticsPort *)ds_listen_port_alloc (ipc, builder));
		success = true;
	} else if (builder->type == DS_PORT_TYPE_LISTEN) {
		ipc = ds_ipc_alloc (builder->path, DS_IPC_CONNECTION_MODE_CONNECT, callback);
		ep_raise_error_if_nok (ipc != NULL);
		ep_raise_error_if_nok (ds_ipc_listen (ipc, callback) == true);
		ds_rt_port_array_append (&_ds_port_array, (DiagnosticsPort *)ds_connect_port_alloc (ipc, builder));
		success = true;
	}

ep_on_exit:
	return success;

ep_on_error:
	ds_ipc_free (ipc);
	success = false;
	ep_exit_error_handler ();
}

void
ds_ipc_stream_factory_init (void)
{
	ds_rt_port_array_alloc (&_ds_port_array);
}

void
ds_ipc_stream_factory_fini (void)
{
	ds_rt_port_array_free (&_ds_port_array);
}

bool
ds_ipc_stream_factory_configure (ds_ipc_error_callback_func callback)
{
	bool success = true;

	ep_char8_t *ports = ds_rt_config_value_get_ports ();
	if (ports) {
		ds_rt_port_config_array_t port_configs;
		ds_rt_port_config_array_t port_config_parts;

		ds_rt_port_array_alloc (&port_configs);
		ds_rt_port_array_alloc (&port_config_parts);

		ipc_stream_factory_split_port_config (ports, ";", &port_configs);

		ep_char8_t **port_configs_data = ds_rt_port_config_array_data (&port_configs);
		size_t port_configs_size = ds_rt_port_config_array_size (&port_configs);
		for (size_t port_configs_index = 0; port_configs_index < port_configs_size; ++port_configs_index) {
			ep_char8_t *port_config = port_configs_data [port_configs_index];
			DS_LOG_INFO_1 ("ds_ipc_stream_factory_configure - Attempted to create Diagnostic Port from \"%s\".\n", port_config ? port_config : "");
			if (port_config) {
				ds_rt_port_config_array_clear (&port_config_parts, NULL);
				ipc_stream_factory_split_port_config (port_config, ",", &port_config_parts);

				ep_char8_t **port_config_parts_data = ds_rt_port_config_array_data (&port_config_parts);
				size_t port_config_parts_size = ds_rt_port_config_array_size (&port_config_parts);
				if (port_config_parts_size != 0) {
					DiagnosticsPortBuilder port_builder;
					ds_port_builder_init (&port_builder);
					for (size_t port_config_parts_index = 0; port_config_parts_index < port_config_parts_size; ++port_config_parts_index) {
						if (port_config_parts_index == 0)
							port_builder.path = port_config_parts_data [port_config_parts_index];
						else
							ds_port_builder_set_tag (&port_builder, port_config_parts_data [port_config_parts_index]);
					}
					if (!ep_rt_utf8_string_is_null_or_empty (port_builder.path)) {
						// Ignore listen type (see conversation in https://github.com/dotnet/runtime/pull/40499 for details)
						if (port_builder.type != DS_PORT_TYPE_LISTEN) {
							const bool build_success = ipc_stream_factory_build_and_add_port (&port_builder, callback);
							DS_LOG_INFO_1 ("ds_ipc_stream_factory_configure - Diagnostic Port creation succeeded? %d \n", build_success);
							success &= build_success;
						} else {
							DS_LOG_INFO_0 ("ds_ipc_stream_factory_configure - Ignoring LISTEN port configuration \n");
						}
					} else {
						DS_LOG_INFO_0("ds_ipc_stream_factory_configure - Ignoring port configuration with empty address\n");
					}
					ds_port_builder_fini (&port_builder);
				} else {
					success &= false;
				}
			}
		}

		ds_rt_port_array_free (&port_config_parts);
		ds_rt_port_array_free (&port_configs);
	}

	// create the default listen port
	int32_t port_suspend = ds_rt_config_value_get_default_port_suspend ();

	DiagnosticsPortBuilder default_port_builder;
	ds_port_builder_init (&default_port_builder);

	default_port_builder.path = NULL;
	default_port_builder.suspend_mode = port_suspend > 0 ? DS_PORT_SUSPEND_MODE_SUSPEND : DS_PORT_SUSPEND_MODE_NOSUSPEND;
	default_port_builder.type = DS_PORT_TYPE_LISTEN;

	success &= ipc_stream_factory_build_and_add_port (&default_port_builder, callback);

	ds_port_builder_fini (&default_port_builder);

	ep_rt_utf8_string_free (ports);

	return success;
}

DiagnosticsIpcStream *
ds_ipc_stream_factory_get_next_available_stream (ds_ipc_error_callback_func callback)
{
	DS_LOG_INFO_0 ("ds_ipc_stream_factory_get_next_available_stream - ENTER");

	DiagnosticsIpcStream *stream = NULL;
	ds_rt_ipc_poll_handle_array_t ipc_poll_handles;
	DiagnosticsIpcPollHandle ipc_poll_handle;
	ds_rt_port_array_t *ports = &_ds_port_array;
	DiagnosticsPort *port = NULL;

	int32_t poll_timeout_ms = DS_IPC_POLL_TIMEOUT_INFINITE;
	bool connect_success = true;
	uint32_t poll_attempts = 0;
	
	ds_rt_ipc_poll_handle_array_alloc (&ipc_poll_handles);

	while (!stream) {
		connect_success = true;
		ds_rt_port_array_iterator_t ports_iterator;
		ds_rt_port_array_iterator_begin (ports, &ports_iterator);
		while (!ds_rt_port_array_iterator_end (ports, &ports_iterator)) {
			port = ds_rt_port_array_iterator_value (&ports_iterator);
			DiagnosticsIpcPollHandle handle;
			if (ds_port_get_ipc_poll_handle_vcall (port, &ipc_poll_handle, callback))
				ds_rt_ipc_poll_handle_array_append (&ipc_poll_handles, ipc_poll_handle);
			else
				connect_success = false;

			ds_rt_port_array_iterator_next (ports, &ports_iterator);
		}

		poll_timeout_ms = connect_success ?
			DS_IPC_POLL_TIMEOUT_INFINITE :
			ipc_stream_factory_get_next_timeout (poll_timeout_ms);

		poll_attempts++;
		DS_LOG_INFO_2 ("ds_ipc_stream_factory_get_next_available_stream - Poll attempt: %d, timeout: %dms.\n", poll_attempts, poll_timeout_ms);
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
				port = (DiagnosticsPort *)ipc_poll_handle.user_data;
				switch (ipc_poll_handle.events) {
				case DS_IPC_POLL_EVENTS_HANGUP:
					EP_ASSERT (state != NULL);
					ds_port_reset_vcall (port, callback);
					DS_LOG_INFO_2 ("ds_ipc_stream_factory_get_next_available_stream - HUP :: Poll attempt: %d, connection %d hung up.\n", nPollAttempts, connection_id);
					poll_timeout_ms = DS_IPC_POLL_TIMEOUT_MIN_MS;
					break;
				case DS_IPC_POLL_EVENTS_SIGNALED:
					EP_ASSERT (state != NULL);
					if (!stream) {  // only use first signaled stream; will get others on subsequent calls
						stream = ds_port_get_connected_stream_vcall (port, callback);
						_ds_current_port = port;
					}
					DS_LOG_INFO_2 ("ds_ipc_stream_factory_get_next_available_stream - SIG :: Poll attempt: %d, connection %d signalled.\n", nPollAttempts, connection_id);
					break;
				case DS_IPC_POLL_EVENTS_ERR:
					DS_LOG_INFO_2 ("ds_ipc_stream_factory_get_next_available_stream - ERR :: Poll attempt: %d, connection %d errored.\n", nPollAttempts, connection_id);
					saw_error = true;
					break;
				case DS_IPC_POLL_EVENTS_NONE:
					DS_LOG_INFO_2 ("ds_ipc_stream_factory_get_next_available_stream - NON :: Poll attempt: %d, connection %d had no events.\n", nPollAttempts, connection_id);
					break;
				default:
					DS_LOG_INFO_2 ("ds_ipc_stream_factory_get_next_available_stream - UNK :: Poll attempt: %d, connection %d had invalid PollEvent.\n", nPollAttempts, connection_id);
					saw_error = true;
					break;
				}

				ds_rt_ipc_poll_handle_array_iterator_next (&ipc_poll_handles, &ipc_poll_handles_iterator);
				connection_id++;
			}
		}

		if (!stream && saw_error) {
			_ds_current_port = NULL;
			ep_raise_error ();
		}

		// clear the view.
		ds_rt_ipc_poll_handle_array_clear (&ipc_poll_handles, NULL);
	}

ep_on_exit:

//TODO: FIX when having PAL.
//#ifdef TARGET_UNIX
//	STRESS_LOG2(LF_DIAGNOSTICS_PORT, LL_INFO10, "ds_ipc_stream_factory_get_next_available_stream - EXIT :: Poll attempt: %d, stream using handle %d.\n", nPollAttempts, pStream->_clientSocket);
//#else
//	STRESS_LOG2(LF_DIAGNOSTICS_PORT, LL_INFO10, "ds_ipc_stream_factory_get_next_available_stream - EXIT :: Poll attempt: %d, stream using handle %d.\n", nPollAttempts, pStream->_hPipe);
//#endif

	return stream;

ep_on_error:
	stream = NULL;
	ep_exit_error_handler ();
}

void
ds_ipc_stream_factory_resume_current_port (void)
{
	if (_ds_current_port != NULL)
		_ds_current_port->has_resumed_runtime = true;
}

bool
ds_ipc_stream_factory_any_suspended_ports (void)
{
	bool any_suspended_ports = false;
	ds_rt_port_array_iterator_t iterator;
	ds_rt_port_array_iterator_begin (&_ds_port_array, &iterator);
	while (!ds_rt_port_array_iterator_end (&_ds_port_array, &iterator)) {
		DiagnosticsPort *port = ds_rt_port_array_iterator_value (&iterator);
		any_suspended_ports |= !(port->suspend_mode == DS_PORT_SUSPEND_MODE_NOSUSPEND || port->has_resumed_runtime);
		ds_rt_port_array_iterator_next (&_ds_port_array, &iterator);
	}
	return any_suspended_ports;
}

bool
ds_ipc_stream_factory_has_active_ports (void)
{
	return !load_shutting_down_state () &&
		ds_rt_port_array_size (&_ds_port_array) > 0;
}

void
ds_ipc_stream_factory_close_ports (ds_ipc_error_callback_func callback)
{
	ds_rt_port_array_iterator_t iterator;
	ds_rt_port_array_iterator_begin (&_ds_port_array, &iterator);
	while (!ds_rt_port_array_iterator_end (&_ds_port_array, &iterator)) {
		ds_port_close (ds_rt_port_array_iterator_value (&iterator), false, callback);
		ds_rt_port_array_iterator_next (&_ds_port_array, &iterator);
	}
}

void
ds_ipc_stream_factory_shutdown (ds_ipc_error_callback_func callback)
{
	if (load_shutting_down_state ())
		return;

	store_shutting_down_state (true);

	ds_rt_port_array_iterator_t iterator;
	ds_rt_port_array_iterator_begin (&_ds_port_array, &iterator);
	while (!ds_rt_port_array_iterator_end (&_ds_port_array, &iterator)) {
		ds_port_close (ds_rt_port_array_iterator_value (&iterator), true, callback);
		ds_port_free_vcall (ds_rt_port_array_iterator_value (&iterator));
		ds_rt_port_array_iterator_next (&_ds_port_array, &iterator);
	}

	_ds_current_port = NULL;
}

/*
 * DiagnosticsPort.
 */

DiagnosticsPort *
ds_port_init (
	DiagnosticsPort *port,
	DiagnosticsPortVtable *vtable,
	DiagnosticsIpc *ipc,
	DiagnosticsPortBuilder *builder)
{
	EP_ASSERT (port != NULL);
	EP_ASSERT (vtable != NULL);
	EP_ASSERT (ipc != NULL);
	EP_ASSERT (builder != NULL);

	port->vtable = vtable;
	port->suspend_mode = builder->suspend_mode;
	port->type = builder->type;
	port->ipc = ipc;
	port->stream = NULL;
	port->has_resumed_runtime = false;

	return port;
}

void
ds_port_fini (DiagnosticsPort *port)
{
	return;
}

void
ds_port_free_vcall (DiagnosticsPort *port)
{
	ep_return_void_if_nok (port != NULL);

	EP_ASSERT (port->vtable != NULL);
	DiagnosticsPortVtable *vtable = port->vtable;

	EP_ASSERT (vtable->free_func != NULL);
	vtable->free_func (port);
}

bool
ds_port_get_ipc_poll_handle_vcall (
	DiagnosticsPort *port,
	DiagnosticsIpcPollHandle *handle,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (port != NULL);
	EP_ASSERT (port->vtable != NULL);

	DiagnosticsPortVtable *vtable = port->vtable;

	EP_ASSERT (vtable->get_ipc_poll_handle_func != NULL);
	return vtable->get_ipc_poll_handle_func (port, handle, callback);
}

DiagnosticsIpcStream *
ds_port_get_connected_stream_vcall (
	DiagnosticsPort *port,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (port != NULL);
	EP_ASSERT (port->vtable != NULL);

	DiagnosticsPortVtable *vtable = port->vtable;

	EP_ASSERT (vtable->get_connected_stream_func != NULL);
	return vtable->get_connected_stream_func (port, callback);
}

void
ds_port_reset_vcall (
	DiagnosticsPort *port,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (port != NULL);
	EP_ASSERT (port->vtable != NULL);

	DiagnosticsPortVtable *vtable = port->vtable;

	EP_ASSERT (vtable->reset_func != NULL);
	vtable->reset_func (port, callback);
}

void
ds_port_close (
	DiagnosticsPort *port,
	bool is_shutdown,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (port != NULL);
	if (port->ipc)
		ds_ipc_close (port->ipc, is_shutdown, callback);
	if (port->stream && !is_shutdown)
		ds_ipc_stream_close (port->stream, callback);
}

DiagnosticsPortBuilder *
ds_port_builder_init (DiagnosticsPortBuilder *builder)
{
	EP_ASSERT (builder != NULL);
	builder->path = NULL;
	builder->suspend_mode = DS_PORT_SUSPEND_MODE_SUSPEND;
	builder->type = DS_PORT_TYPE_CONNECT;

	return builder;
}

void
ds_port_builder_fini (DiagnosticsPortBuilder *builder)
{
	return;
}

void
ds_port_builder_set_tag (
	DiagnosticsPortBuilder *builder,
	ep_char8_t *tag)
{
	EP_ASSERT (builder != NULL);
	EP_ASSERT (tag != NULL);

	if (ep_rt_utf8_string_compare_ignore_case (tag, "listen") == 0)
		builder->type = DS_PORT_TYPE_LISTEN;
	else if (ep_rt_utf8_string_compare_ignore_case (tag, "connect") == 0)
		builder->type = DS_PORT_TYPE_CONNECT;
	else if (ep_rt_utf8_string_compare_ignore_case (tag, "nosuspend") == 0)
		builder->suspend_mode = DS_PORT_SUSPEND_MODE_NOSUSPEND;
	else if (ep_rt_utf8_string_compare_ignore_case (tag, "suspend") == 0)
		builder->suspend_mode = DS_PORT_SUSPEND_MODE_SUSPEND;
	else
		// don't mutate if it's not a valid option
		DS_LOG_INFO_1 ("ds_port_builder_set_tag - Unknown tag '%s'.\n", tag);
}

/*
 * DiagnosticsConnectPort.
 */

static
void
connect_port_free_func (void *object)
{
	EP_ASSERT (object != NULL);
	ds_connect_port_free ((DiagnosticsConnectPort *)object);
}

static
bool
connect_port_get_ipc_poll_handle_func (
	void *object,
	DiagnosticsIpcPollHandle *handle,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (object != NULL);
	EP_ASSERT (handle != NULL);

	bool success = false;
	DiagnosticsConnectPort *connect_port = (DiagnosticsConnectPort *)object;
	DiagnosticsIpcStream *connection = NULL;

	DS_LOG_INFO_0 ("connect_port_get_ipc_poll_handle - ENTER.\n");

	if (!connect_port->port.stream) {
		DS_LOG_INFO_0 ("connect_port_get_ipc_poll_handle - cache was empty!\n");
		// cache is empty, reconnect, e.g., there was a disconnect
		connection = ds_ipc_connect (connect_port->port.ipc, callback);
		if (!connection) {
			if (callback)
				callback("Failed to connect to client connection", -1);
			ep_raise_error ();
		}
	//TODO: Fix in PAL.
	//#ifdef TARGET_UNIX
	//	STRESS_LOG1(LF_DIAGNOSTICS_PORT, LL_INFO10, "IpcStreamFactory::ClientConnectionState::GetIpcPollHandle - returned connection { _clientSocket = %d }\n", pConnection->_clientSocket);
	//#else
	//	STRESS_LOG2(LF_DIAGNOSTICS_PORT, LL_INFO10, "IpcStreamFactory::ClientConnectionState::GetIpcPollHandle - returned connection { _hPipe = %d, _oOverlap.hEvent = %d }\n", pConnection->_hPipe, pConnection->_oOverlap.hEvent);
	//#endif
		if (!ds_icp_advertise_v1_send (connection)) {
			if (callback)
				callback("Failed to send advertise message", -1);
			ep_raise_error ();
		}

		//Transfer ownership.
		connect_port->port.stream = connection;
		connection = NULL;
	}

	handle->ipc = NULL;
	handle->stream = connect_port->port.stream;
	handle->events = 0;
	handle->user_data = object;

	success = true;

ep_on_exit:
	DS_LOG_INFO_0 ("connect_port_get_ipc_poll_handle - EXIT.\n");
	return success;

ep_on_error:
	ds_ipc_stream_free (connection);
	success = false;
	ep_exit_error_handler ();
}

static
DiagnosticsIpcStream *
connect_port_get_connected_stream_func (
	void *object,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (object != NULL);

	DiagnosticsConnectPort *connect_port = (DiagnosticsConnectPort *)object;
	DiagnosticsIpcStream *stream = connect_port->port.stream;
	connect_port->port.stream = NULL;
	return stream;
}

static
void
connect_port_reset (
	void *object,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (object != NULL);

	DiagnosticsConnectPort *connect_port = (DiagnosticsConnectPort *)object;
	ds_ipc_stream_free (connect_port->port.stream);
	connect_port->port.stream = NULL;
}

static DiagnosticsPortVtable connect_port_vtable = {
	connect_port_free_func,
	connect_port_get_ipc_poll_handle_func,
	connect_port_get_connected_stream_func,
	connect_port_reset };

DiagnosticsConnectPort *
ds_connect_port_alloc (
	DiagnosticsIpc *ipc,
	DiagnosticsPortBuilder *builder)
{
	DiagnosticsConnectPort * instance = ep_rt_object_alloc (DiagnosticsConnectPort);
	ep_raise_error_if_nok (instance != NULL);

	ep_raise_error_if_nok (ds_port_init (
		(DiagnosticsPort *)instance,
		&connect_port_vtable,
		ipc,
		builder) != NULL);

ep_on_exit:
	return instance;

ep_on_error:
	ds_connect_port_free (instance);
	instance = NULL;
	ep_exit_error_handler ();
}

void
ds_connect_port_free (DiagnosticsConnectPort *connect_port)
{
	ep_return_void_if_nok (connect_port != NULL);
	ds_port_fini (&connect_port->port);
	ep_rt_object_free (connect_port);
}

/*
 * DiagnosticsListenPort.
 */

static
void
listen_port_free_func (void *object)
{
	EP_ASSERT (object != NULL);
	ds_listen_port_free ((DiagnosticsListenPort *)object);
}

static
bool
listen_port_get_ipc_poll_handle_func (
	void *object,
	DiagnosticsIpcPollHandle *handle,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (object != NULL);
	EP_ASSERT (handle != NULL);

	DiagnosticsListenPort *listen_port = (DiagnosticsListenPort *)object;

	handle->ipc = listen_port->port.ipc;
	handle->stream = NULL;
	handle->events = 0;
	handle->user_data = object;

	return true;
}

static
DiagnosticsIpcStream *
listen_port_get_connected_stream_func (
	void *object,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (object != NULL);

	DiagnosticsListenPort *listen_port = (DiagnosticsListenPort *)object;
	return ds_ipc_accept (listen_port->port.ipc, callback);
}

static
void
listen_port_reset (
	void *object,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (object != NULL);
	return;
}

static DiagnosticsPortVtable listen_port_vtable = {
	listen_port_free_func,
	listen_port_get_ipc_poll_handle_func,
	listen_port_get_connected_stream_func,
	listen_port_reset };

DiagnosticsListenPort *
ds_listen_port_alloc (
	DiagnosticsIpc *ipc,
	DiagnosticsPortBuilder *builder)
{
	DiagnosticsListenPort * instance = ep_rt_object_alloc (DiagnosticsListenPort);
	ep_raise_error_if_nok (instance != NULL);

	ep_raise_error_if_nok (ds_port_init (
		(DiagnosticsPort *)instance,
		&listen_port_vtable,
		ipc,
		builder) != NULL);

ep_on_exit:
	return instance;

ep_on_error:
	ds_listen_port_free (instance);
	instance = NULL;
	ep_exit_error_handler ();
}

void
ds_listen_port_free (DiagnosticsListenPort *listen_port)
{
	ep_return_void_if_nok (listen_port != NULL);
	ds_port_fini (&listen_port->port);
	ep_rt_object_free (listen_port);
}

//PAL
//TODO: Move into corresponding ds-ipc-win32 and ds-ipc-unix.

#include "windows.h"

DiagnosticsIpc *
ds_ipc_alloc (
	const char *pipe_name,
	DiagnosticsIpcConnectionMode mode,
	ds_ipc_error_callback_func callback)
{
	int32_t characters_written = -1;

	DiagnosticsIpc *instance = ep_rt_object_alloc (DiagnosticsIpc);
	ep_raise_error_if_nok (instance != NULL);

	instance->mode = mode;
	instance->is_listening = false;

	// All memory zeroed on alloc.
	//memset (&instance->overlap, 0, sizeof (instance->overlap));

	instance->overlap.hEvent = INVALID_HANDLE_VALUE;
	instance->pipe = INVALID_HANDLE_VALUE;

	if (pipe_name) {
		characters_written = ep_rt_utf8_string_snprintf (
			&instance->pipe_name,
			DS_IPC_WIN32_MAX_NAMED_PIPE_LEN,
			"\\\\.\\pipe\\%s",
			pipe_name);
	} else {
		characters_written = ep_rt_utf8_string_snprintf (
			&instance->pipe_name,
			DS_IPC_WIN32_MAX_NAMED_PIPE_LEN,
			"\\\\.\\pipe\\dotnet-diagnostic-%d",
			ep_rt_current_process_get_id ());
	}

	if (characters_written == -1) {
		if (callback)
			callback("Failed to generate the named pipe name", characters_written);
		ep_raise_error ();
	}

ep_on_exit:
	return instance;

ep_on_error:
	ds_ipc_free (instance);
	instance = NULL;
	ep_exit_error_handler ();
}

void
ds_ipc_free (DiagnosticsIpc *ipc)
{
	ep_return_void_if_nok (ipc != NULL);

	ds_ipc_close (ipc, false, NULL);
	ep_rt_object_free (ipc);
}

int32_t
ds_ipc_poll (
	ds_rt_ipc_poll_handle_array_t *poll_handles,
	uint32_t timeout_ms,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (poll_handles);

	int32_t result = 1;
	DiagnosticsIpcPollHandle * poll_handles_data = ds_rt_ipc_poll_handle_array_data (poll_handles);
	size_t poll_handles_data_len = ds_rt_ipc_poll_handle_array_size (poll_handles);

	HANDLE handles [MAXIMUM_WAIT_OBJECTS];
	for (size_t i = 0; i < poll_handles_data_len; ++i) {
		poll_handles_data [i].events = 0; // ignore any input on events.
		if (poll_handles_data [i].ipc) {
			// SERVER
			EP_ASSERT (poll_handles_data [i].ipc->mode == DS_IPC_CONNECTION_MODE_LISTEN);
			handles [i] = poll_handles_data [i].ipc->overlap.hEvent;
		} else {
			// CLIENT
			bool success = true;
			DWORD bytes_read = 1;
			if (!poll_handles_data [i].stream->is_test_reading) {
				// check for data by doing an asynchronous 0 byte read.
				// This will signal if the pipe closes (hangup) or the server
				// sends new data
				success = ReadFile (
					poll_handles_data [i].stream->pipe,         // handle
					NULL,                                       // null buffer
					0,                                          // read 0 bytesd
					&bytes_read,                                // dummy variable
					&poll_handles_data [i].stream->overlap);    // overlap object to use

				poll_handles_data [i].stream->is_test_reading = true;
				if (!success) {
					DWORD error = GetLastError ();
					switch (error) {
					case ERROR_IO_PENDING:
						handles [i] = poll_handles_data [i].stream->overlap.hEvent;
						break;
					case ERROR_PIPE_NOT_CONNECTED:
						poll_handles_data [i].events = (uint8_t)DS_IPC_POLL_EVENTS_HANGUP;
						result = -1;
						ep_raise_error ();
					default:
						if (callback)
							callback("0 byte async read on client connection failed", error);
						result = -1;
						ep_raise_error ();
					}
				} else {
					// there's already data to be read
					handles [i] = poll_handles_data [i].stream->overlap.hEvent;
				}
			} else {
				handles [i] = poll_handles_data [i].stream->overlap.hEvent;
			}
		}
	}

	// call wait for multiple obj
	DWORD wait = WaitForMultipleObjects (
		poll_handles_data_len,      // count
		handles,                    // handles
		false,                      // don't wait all
		timeout_ms);

	if (wait == WAIT_TIMEOUT) {
		// we timed out
		result = 0;
		ep_raise_error ();
	}

	if (wait == WAIT_FAILED) {
		// we errored
		if (callback)
			callback ("WaitForMultipleObjects failed", GetLastError());
		result = -1;
		ep_raise_error ();
	}

	// determine which of the streams signaled
	DWORD index = wait - WAIT_OBJECT_0;
	// error check the index
	if (index < 0 || index > (poll_handles_data_len - 1)) {
		// check if we abandoned something
		DWORD abandonedIndex = wait - WAIT_ABANDONED_0;
		if (abandonedIndex > 0 || abandonedIndex < (poll_handles_data_len - 1)) {
			poll_handles_data [abandonedIndex].events = (uint8_t)DS_IPC_POLL_EVENTS_HANGUP;
			result = -1;
			ep_raise_error ();
		} else {
			if (callback)
				callback ("WaitForMultipleObjects failed", GetLastError());
			result = -1;
			ep_raise_error ();
		}
	}

	// Set revents depending on what signaled the stream
	if (poll_handles_data [index].ipc == NULL) {
		// CLIENT
		// check if the connection got hung up
		DWORD dummy = 0;
		bool success = GetOverlappedResult(
			poll_handles_data [index].stream->pipe,
			&poll_handles_data [index].stream->overlap,
			&dummy,
			true);
		poll_handles_data [index].stream->is_test_reading = false;
		if (!success) {
			DWORD error = GetLastError();
			if (error == ERROR_PIPE_NOT_CONNECTED) {
				poll_handles_data [index].events = (uint8_t)DS_IPC_POLL_EVENTS_HANGUP;
			} else {
				if (callback)
					callback ("Client connection error", -1);
				poll_handles_data [index].events = (uint8_t)DS_IPC_POLL_EVENTS_ERR;
				result = -1;
				ep_raise_error ();
			}
		} else {
			poll_handles_data [index].events = (uint8_t)DS_IPC_POLL_EVENTS_SIGNALED;
		}
	} else {
		// SERVER
		poll_handles_data [index].events = (uint8_t)DS_IPC_POLL_EVENTS_SIGNALED;
	}

	result = 1;

ep_on_exit:
	return result;

ep_on_error:

	if (result == 1)
		result = -1;

	ep_exit_error_handler ();
}

bool
ds_ipc_listen (
	DiagnosticsIpc *ipc,
	ds_ipc_error_callback_func callback)
{

	bool result = false;

	EP_ASSERT (ipc != NULL);
	EP_ASSERT (ipc->mode == DS_IPC_CONNECTION_MODE_LISTEN);
	if (ipc->mode != DS_IPC_CONNECTION_MODE_LISTEN) {
		if (callback)
			callback ("Cannot call Listen on a client connection", -1);
		return false;
	}

	if (ipc->is_listening)
		return true;

	EP_ASSERT (ipc->pipe == INVALID_HANDLE_VALUE);

	const uint32_t in_buffer_size = 16 * 1024;
	const uint32_t out_buffer_size = 16 * 1024;
	ipc->pipe = CreateNamedPipeA (
		ipc->pipe_name,                                             // pipe name
		PIPE_ACCESS_DUPLEX |                                        // read/write access
		FILE_FLAG_OVERLAPPED,                                       // async listening
		PIPE_TYPE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,    // message type pipe, message-read and blocking mode
		PIPE_UNLIMITED_INSTANCES,                                   // max. instances
		out_buffer_size,                                            // output buffer size
		in_buffer_size,                                             // input buffer size
		0,                                                          // default client time-out
		NULL);                                                      // default security attribute

	if (ipc->pipe == INVALID_HANDLE_VALUE) {
		if (callback)
			callback ("Failed to create an instance of a named pipe.", GetLastError());
		ep_raise_error ();
	}

	EP_ASSERT (ipc->overlap.hEvent == INVALID_HANDLE_VALUE);

	ipc->overlap.hEvent = CreateEvent (NULL, true, false, NULL);
	if (!ipc->overlap.hEvent) {
		if (callback)
			callback ("Failed to create overlap event", GetLastError());
		ep_raise_error ();
	}

	if (ConnectNamedPipe (ipc->pipe, &ipc->overlap) == 0) {
		const DWORD error_code = GetLastError ();
		switch (error_code) {
		case ERROR_IO_PENDING:
			// There was a pending connection that can be waited on (will happen in poll)
		case ERROR_PIPE_CONNECTED:
			// Occurs when a client connects before the function is called.
			// In this case, there is a connection between client and
			// server, even though the function returned zero.
			break;

		default:
			if (callback)
				callback ("A client process failed to connect.", error_code);
			ep_raise_error ();
		}
	}

	ipc->is_listening = true;
	result = true;

ep_on_exit:
	return result;

ep_on_error:
	ds_ipc_close (ipc, false, callback);
	result = false;
	ep_exit_error_handler ();
}

DiagnosticsIpcStream *
ds_ipc_accept (
	DiagnosticsIpc *ipc,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (ipc != NULL);
	EP_ASSERT (ipc->mode == DS_IPC_CONNECTION_MODE_LISTEN);

	DiagnosticsIpcStream *stream = NULL;

	DWORD dummy = 0;
	bool success = GetOverlappedResult (
		ipc->pipe,      // handle
		&ipc->overlap,  // overlapped
		&dummy,         // throw-away dword
		true);          // wait till event signals

	if (!success) {
		if (callback)
			callback ("Failed to GetOverlappedResults for NamedPipe server", GetLastError());
		ep_raise_error ();
	}

	// create new IpcStream using handle and reset the Server object so it can listen again
	stream = ds_ipc_stream_alloc (ipc->pipe, DS_IPC_CONNECTION_MODE_LISTEN);
	ep_raise_error_if_nok (stream != NULL);

	// reset the server
	ipc->pipe = INVALID_HANDLE_VALUE;
	ipc->is_listening = false;
	CloseHandle (ipc->overlap.hEvent);
	memset(&ipc->overlap, 0, sizeof(OVERLAPPED)); // clear the overlapped objects state
	ipc->overlap.hEvent = INVALID_HANDLE_VALUE;

	ep_raise_error_if_nok (ds_ipc_listen (ipc, callback) == true);

ep_on_exit:
	return stream;

ep_on_error:
	ds_ipc_stream_free (stream);
	stream = NULL;
	ep_exit_error_handler ();
}

DiagnosticsIpcStream *
ds_ipc_connect (
	DiagnosticsIpc *ipc,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (ipc != NULL);
	EP_ASSERT (ipc->mode == DS_IPC_CONNECTION_MODE_CONNECT);

	DiagnosticsIpcStream *stream = NULL;
	HANDLE pipe = INVALID_HANDLE_VALUE;

	if (ipc->mode != DS_IPC_CONNECTION_MODE_CONNECT) {
		if (callback)
			callback ("Cannot call connect on a server connection", 0);
		ep_raise_error ();
	}

	pipe = CreateFileA(
		ipc->pipe_name,         // pipe name
		PIPE_ACCESS_DUPLEX,     // read/write access
		0,                      // no sharing
		NULL,                   // default security attributes
		OPEN_EXISTING,          // opens existing pipe
		FILE_FLAG_OVERLAPPED,   // overlapped
		NULL);                  // no template file

	if (pipe == INVALID_HANDLE_VALUE) {
		if (callback)
			callback ("Failed to connect to named pipe.", GetLastError ());
		ep_raise_error ();
	}

	stream = ds_ipc_stream_alloc (pipe, ipc->mode);
	ep_raise_error_if_nok (stream);

	pipe = INVALID_HANDLE_VALUE;

ep_on_exit:
	return stream;

ep_on_error:
	ds_ipc_stream_free (stream);
	stream = NULL;

	if (pipe != INVALID_HANDLE_VALUE) {
		CloseHandle (pipe);
	}

	ep_exit_error_handler ();
}

void
ds_ipc_close (
	DiagnosticsIpc *ipc,
	bool is_shutdown,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (ipc != NULL);

	// don't attempt cleanup on shutdown and let the OS handle it
	if (is_shutdown) {
		if (callback)
			callback ("Closing without cleaning underlying handles", 100);
		return;
	}

	if (ipc->pipe != INVALID_HANDLE_VALUE) {
		if (ipc->mode == DS_IPC_CONNECTION_MODE_LISTEN) {
			const bool success_disconnect = DisconnectNamedPipe (ipc->pipe);
			if (success_disconnect != TRUE && callback)
				callback ("Failed to disconnect NamedPipe", GetLastError());
		}

		const BOOL success_close_pipe = CloseHandle (ipc->pipe);
		if (success_close_pipe != TRUE && callback)
			callback ("Failed to close pipe handle", GetLastError());
		ipc->pipe = INVALID_HANDLE_VALUE;
	}

	if (ipc->overlap.hEvent != INVALID_HANDLE_VALUE) {
		const BOOL success_close_event = CloseHandle (ipc->overlap.hEvent);
		if (success_close_event != TRUE && callback)
			callback ("Failed to close overlap event handle", GetLastError());
		memset(&ipc->overlap, 0, sizeof(OVERLAPPED)); // clear the overlapped objects state
		ipc->overlap.hEvent = INVALID_HANDLE_VALUE;
	}
}

/*
 * DiagnosticsIpcStream.
 */

static
void
ipc_stream_free_func (void *object)
{
	EP_ASSERT (object != NULL);
	DiagnosticsIpcStream *ipc_stream = (DiagnosticsIpcStream *)object;
	ds_ipc_stream_free (ipc_stream);
}

static
bool
ipc_stream_read_func (
	void *object,
	uint8_t *buffer,
	uint32_t bytes_to_read,
	uint32_t *bytes_read,
	uint32_t timeout_ms)
{
	EP_ASSERT (object != NULL);
	DiagnosticsIpcStream *ipc_stream = (DiagnosticsIpcStream *)object;
	return ds_ipc_stream_read (
		ipc_stream,
		buffer,
		bytes_to_read,
		bytes_read,
		timeout_ms);
}

static
bool
ipc_stream_write_func (
	void *object,
	const uint8_t *buffer,
	uint32_t bytes_to_write,
	uint32_t *bytes_written,
	uint32_t timeout_ms)
{
	EP_ASSERT (object != NULL);
	DiagnosticsIpcStream *ipc_stream = (DiagnosticsIpcStream *)object;
	return ds_ipc_stream_write (
		ipc_stream,
		buffer,
		bytes_to_write,
		bytes_written,
		timeout_ms);
}

static
bool
ipc_stream_flush_func (void *object)
{
	EP_ASSERT (object != NULL);
	DiagnosticsIpcStream *ipc_stream = (DiagnosticsIpcStream *)object;
	return ds_ipc_stream_flush (ipc_stream);
}
static
bool
ipc_stream_close_func (void *object)
{
	EP_ASSERT (object != NULL);
	DiagnosticsIpcStream *ipc_stream = (DiagnosticsIpcStream *)object;
	return ds_ipc_stream_close (ipc_stream, NULL);
}

static IpcStreamVtable ipc_stream_vtable = {
	ipc_stream_free_func,
	ipc_stream_read_func,
	ipc_stream_write_func,
	ipc_stream_flush_func,
	ipc_stream_close_func };

DiagnosticsIpcStream *
ds_ipc_stream_alloc (
	HANDLE pipe,
	DiagnosticsIpcConnectionMode mode)
{
	DiagnosticsIpcStream *instance = ep_rt_object_alloc (DiagnosticsIpcStream);
	ep_raise_error_if_nok (instance != NULL);

	ep_raise_error_if_nok (ep_ipc_stream_init (&instance->stream, &ipc_stream_vtable) != NULL);

	instance->pipe = pipe;
	instance->mode = mode;

	// All memory zeroed on alloc.
	//memset (&instance->overlap, 0, sizeof (OVERLAPPED));

	instance->overlap.hEvent = CreateEvent (NULL, true, false, NULL);

ep_on_exit:
	return instance;

ep_on_error:
	ds_ipc_stream_free (instance);
	instance = NULL;
	ep_exit_error_handler ();
}

void
ds_ipc_stream_free (DiagnosticsIpcStream *ipc_stream)
{
	ep_return_void_if_nok (ipc_stream != NULL);
	ds_ipc_stream_close (ipc_stream, NULL);
	ep_rt_object_free (ipc_stream);
}

bool
ds_ipc_stream_read (
	DiagnosticsIpcStream *ipc_stream,
	uint8_t *buffer,
	uint32_t bytes_to_read,
	uint32_t *bytes_read,
	uint32_t timeout_ms)
{
	EP_ASSERT (ipc_stream != NULL);
	EP_ASSERT (buffer != NULL);
	EP_ASSERT (bytes_read != NULL);

	DWORD read = 0;
	LPOVERLAPPED overlap = &ipc_stream->overlap;
	bool success = ReadFile (
		ipc_stream->pipe,	// handle to pipe
		buffer,				// buffer to receive data
		bytes_to_read,		// size of buffer
		&read,				// number of bytes read
		overlap) != 0;		// overlapped I/O

	if (!success) {
		if (timeout_ms == DS_IPC_WIN32_INFINITE_TIMEOUT) {
			success = GetOverlappedResult (
				ipc_stream->pipe,
				overlap,
				&read,
				true) != 0;
		} else {
			DWORD error = GetLastError ();
			if (error == ERROR_IO_PENDING) {
				DWORD wait = WaitForSingleObject (ipc_stream->overlap.hEvent, (DWORD)timeout_ms);
				if (wait == WAIT_OBJECT_0) {
					// get the result
					success = GetOverlappedResult (
						ipc_stream->pipe,
						overlap,
						&read,
						true) != 0;
				} else {
					// cancel IO and ensure the cancel happened
					if (CancelIo (ipc_stream->pipe)) {
						// check if the async write beat the cancellation
						success = GetOverlappedResult (
							ipc_stream->pipe,
							overlap,
							&read,
							true) != 0;
					}
				}
			}
		}
		// TODO: Add error handling.
	}

	*bytes_read = (uint32_t)read;
	return success;
}

bool
ds_ipc_stream_write (
	DiagnosticsIpcStream *ipc_stream,
	const uint8_t *buffer,
	uint32_t bytes_to_write,
	uint32_t *bytes_written,
	uint32_t timeout_ms)
{
	EP_ASSERT (ipc_stream != NULL);
	EP_ASSERT (buffer != NULL);
	EP_ASSERT (bytes_written != NULL);

	DWORD written = 0;
	LPOVERLAPPED overlap = &ipc_stream->overlap;
	bool success = WriteFile (
		ipc_stream->pipe,   // handle to pipe
		buffer,             // buffer to write from
		bytes_to_write,     // number of bytes to write
		&written,           // number of bytes written
		overlap) != 0;      // overlapped I/O

	if (!success) {
		DWORD error = GetLastError ();
		if (error == ERROR_IO_PENDING) {
			if (timeout_ms == DS_IPC_WIN32_INFINITE_TIMEOUT) {
				// if we're waiting infinitely, don't bother with extra kernel call
				success = GetOverlappedResult (
					ipc_stream->pipe,
					overlap,
					&written,
					true) != 0;
			} else {
				DWORD wait = WaitForSingleObject (ipc_stream->overlap.hEvent, (DWORD)timeout_ms);
				if (wait == WAIT_OBJECT_0) {
					// get the result
					success = GetOverlappedResult (
						ipc_stream->pipe,
						overlap,
						&written,
						true) != 0;
				} else {
					// cancel IO and ensure the cancel happened
					if (CancelIo (ipc_stream->pipe)) {
						// check if the async write beat the cancellation
						success = GetOverlappedResult (
							ipc_stream->pipe,
							overlap,
							&written,
							true) != 0;
					}
				}
			}
		}
		// TODO: Add error handling.
	}

	*bytes_written = (uint32_t)written;
	return success;
}

bool
ds_ipc_stream_flush (const DiagnosticsIpcStream *ipc_stream)
{
	EP_ASSERT (ipc_stream != NULL);
	const bool success = FlushFileBuffers (ipc_stream->pipe) != 0;

	//TODO: Add error handling.
	return success;
}

bool
ds_ipc_stream_close (
	DiagnosticsIpcStream *ipc_stream,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (ipc_stream != NULL);

	if (ipc_stream->pipe != INVALID_HANDLE_VALUE) {
		ds_ipc_stream_flush (ipc_stream);
		if (ipc_stream->mode == DS_IPC_CONNECTION_MODE_LISTEN) {
			const bool success_disconnect = DisconnectNamedPipe (ipc_stream->pipe);
			if (success_disconnect != TRUE && callback)
				callback ("Failed to disconnect NamedPipe", GetLastError());
		}

		const BOOL success_close_pipe = CloseHandle (ipc_stream->pipe);
		if (success_close_pipe != TRUE && callback)
			callback ("Failed to close pipe handle", GetLastError());
		ipc_stream->pipe = INVALID_HANDLE_VALUE;
	}

	if (ipc_stream->overlap.hEvent != INVALID_HANDLE_VALUE) {
		const BOOL success_close_event = CloseHandle (ipc_stream->overlap.hEvent);
		if (success_close_event != TRUE && callback)
			callback ("Failed to close overlapped event handle", GetLastError());
		memset(&ipc_stream->overlap, 0, sizeof(OVERLAPPED)); // clear the overlapped objects state
		ipc_stream->overlap.hEvent = INVALID_HANDLE_VALUE;
	}

	ipc_stream->is_test_reading = false;

	return true;
}

#endif /* !defined(DS_INCLUDE_SOURCE_FILES) || defined(DS_FORCE_INCLUDE_SOURCE_FILES) */
#endif /* ENABLE_PERFTRACING */

#ifndef EP_INCLUDE_SOURCE_FILES
extern const char quiet_linker_empty_file_warning_diagnostics_ipc;
const char quiet_linker_empty_file_warning_diagnostics_ipc = 0;
#endif
