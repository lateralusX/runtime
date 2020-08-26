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

static
void
ipc_stream_factory_split_diagnostic_port_config (
	ep_char8_t *config,
	const ep_char8_t *delimiters,
	ds_rt_diagnostic_port_config_array_t *config_array);

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
ipc_stream_factory_split_diagnostic_port_config (
	ep_char8_t *config,
	const ep_char8_t *delimiters,
	ds_rt_diagnostic_port_config_array_t *config_array)
{
	ep_char8_t *part = NULL;
	ep_char8_t *context = NULL;
	ep_char8_t *cursor = config;

	EP_ASSERT (config != NULL);
	EP_ASSERT (context != NULL);
	EP_ASSERT (cursor != NULL);

	part = ep_rt_utf8_string_strtok (cursor, delimiters, &context);
	while (part) {
		ds_rt_diagnostic_port_config_array_append (config_array, part);
		part = ep_rt_utf8_string_strtok (NULL, delimiters, &context);
	}
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
ds_ipc_stream_factory_configure (ds_ipc_error_callback_func callback)
{
	bool success = true;

	ds_rt_diagnostic_port_config_array_t port_configs;
	ds_rt_diagnostic_port_config_array_t port_config_parts;

	ep_char8_t *ports = ds_rt_config_value_get_diagnostic_ports ();
	if (ports) {
		ds_rt_diagnostic_port_array_alloc (&port_configs);
		ds_rt_diagnostic_port_array_alloc (&port_config_parts);

		ipc_stream_factory_split_diagnostic_port_config (ports, ";", &port_configs);

		ep_char8_t **port_configs_data = ds_rt_diagnostic_port_config_array_data ((&port_configs));
		size_t port_configs_size = ds_rt_diagnostic_port_config_array_size (&port_configs);
		for (size_t port_configs_index = 0; port_configs_index < port_configs_size; ++port_configs_index) {
			ep_char8_t *port_config = port_configs_data [port_configs_index];
			DS_LOG_INFO_1 ("IpcStreamFactory::Configure - Attempted to create Diagnostic Port from \"%s\".\n", port_config ? port_config : "");
			if (port_config) {
				ds_rt_diagnostic_port_config_array_clear (&port_config_parts, NULL);
				ipc_stream_factory_split_diagnostic_port_config (port_config, ",", &port_config_parts);

				ep_char8_t **port_config_parts_data = ds_rt_diagnostic_port_config_array_data ((&port_config_parts));
				size_t port_config_parts_size = ds_rt_diagnostic_port_config_array_size (&port_config_parts);
				if (port_config_parts_size != 0) {
					IpcStreamFactoryDiagnosticPortBuilder port_builder;
					ds_ipc_stream_factory_diagnostic_port_builder_init (&port_builder);
					for (size_t port_config_parts_index = 0; port_config_parts_index < port_config_parts_size; ++port_config_parts_index) {
						if (port_config_parts_index == 0)
							port_builder.path = port_config_parts_data [port_config_parts_index];
						else
							ds_ipc_stream_factory_diagnostic_port_builder_set_tag (&port_builder, port_config_parts_data [port_config_parts_index]);
					}
					if (!ep_rt_utf8_string_is_null_or_empty (port_builder.path)) {
						// Ignore listen type (see conversation in https://github.com/dotnet/runtime/pull/40499 for details)
						if (port_builder.type != DS_PORT_TYPE_LISTEN) {
							const bool build_success = ipc_stream_factory_build_and_add_port (&port_builder, callback);
							DS_LOG_INFO_1 ("IpcStreamFactory::Configure - Diagnostic Port creation succeeded? %d \n", build_success);
							success &= build_success;
						} else {
							DS_LOG_INFO_0 ("IpcStreamFactory::Configure - Ignoring LISTEN port configuration \n");
						}
					} else {
						DS_LOG_INFO_0("IpcStreamFactory::Configure - Ignoring port configuration with empty address\n");
					}
					ds_ipc_stream_factory_diagnostic_port_builder_fini (&port_builder);
				} else {
					success &= false;
				}
			}
		}
	}

	// create the default listen port
	int32_t port_suspend = ds_rt_config_value_get_default_diagnostic_port_suspend ();
	IpcStreamFactoryDiagnosticPortBuilder default_port_builder;
	ds_ipc_stream_factory_diagnostic_port_builder_init (&default_port_builder);

	default_port_builder.path = NULL;
	default_port_builder.suspend_mode = port_suspend > 0 ? DS_PORT_SUSPEND_MODE_SUSPEND : DS_PORT_SUSPEND_MODE_NOSUSPEND;
	default_port_builder.type = DS_PORT_TYPE_LISTEN;

	success &= ipc_stream_factory_build_and_add_port (&default_port_builder, callback);

	ds_ipc_stream_factory_diagnostic_port_builder_fini (&default_port_builder);

	ds_rt_diagnostic_port_array_free (&port_config_parts);
	ds_rt_diagnostic_port_array_free (&port_configs);

	ep_rt_utf8_string_free (ports);

	return success;
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
					ds_ipc_stream_factory_diagnostic_port_reset_vcall (diagnostic_port, callback);
					DS_LOG_INFO_2 ("IpcStreamFactory::GetNextAvailableStream - HUP :: Poll attempt: %d, connection %d hung up.\n", nPollAttempts, connection_id);
					poll_timeout_ms = DS_IPC_POLL_TIMEOUT_MIN_MS;
					break;
				case DS_IPC_POLL_EVENTS_SIGNALED:
					EP_ASSERT (state != NULL);
					if (!stream) {  // only use first signaled stream; will get others on subsequent calls
						stream = ds_ipc_stream_factory_diagnostic_port_get_connected_stream_vcall (diagnostic_port, callback);
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

/*
 * IpcStreamFactoryDiagnosticPort.
 */

IpcStreamFactoryDiagnosticPort *
ds_ipc_stream_factory_diagnostic_port_init (
	IpcStreamFactoryDiagnosticPort *diagnostic_port,
	IpcStreamFactoryDiagnosticPortVtable *vtable,
	DiagnosticsIpc *ipc,
	IpcStreamFactoryDiagnosticPortBuilder *builder)
{
	EP_ASSERT (diagnostic_port != NULL);
	EP_ASSERT (vtable != NULL);
	EP_ASSERT (ipc != NULL);
	EP_ASSERT (builder != NULL);

	diagnostic_port->vtable = vtable;
	diagnostic_port->suspend_mode = builder->suspend_mode;
	diagnostic_port->type = builder->type;
	diagnostic_port->ipc = ipc;
	diagnostic_port->stream = NULL;
	diagnostic_port->has_resumed_runtime = false;

	return diagnostic_port;
}

void
ds_ipc_stream_factory_diagnostic_port_fini (IpcStreamFactoryDiagnosticPort *diagnostic_port)
{
	return;
}

bool
ds_ipc_stream_factory_diagnostic_port_get_ipc_poll_handle_vcall (
	IpcStreamFactoryDiagnosticPort * diagnostic_port,
	DiagnosticsIpcPollHandle *handle,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (diagnostic_port != NULL);
	EP_ASSERT (diagnostic_port->vtable != NULL);

	IpcStreamFactoryDiagnosticPortVtable *vtable = diagnostic_port->vtable;

	EP_ASSERT (vtable->get_ipc_poll_handle_func != NULL);
	return vtable->get_ipc_poll_handle_func (diagnostic_port, handle, callback);
}

IpcStream *
ds_ipc_stream_factory_diagnostic_port_get_connected_stream_vcall (
	IpcStreamFactoryDiagnosticPort * diagnostic_port,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (diagnostic_port != NULL);
	EP_ASSERT (diagnostic_port->vtable != NULL);

	IpcStreamFactoryDiagnosticPortVtable *vtable = diagnostic_port->vtable;

	EP_ASSERT (vtable->get_connected_stream_func != NULL);
	return vtable->get_connected_stream_func (diagnostic_port, callback);
}

void
ds_ipc_stream_factory_diagnostic_port_reset_vcall (
	IpcStreamFactoryDiagnosticPort * diagnostic_port,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (diagnostic_port != NULL);
	EP_ASSERT (diagnostic_port->vtable != NULL);

	IpcStreamFactoryDiagnosticPortVtable *vtable = diagnostic_port->vtable;

	EP_ASSERT (vtable->reset_func != NULL);
	vtable->reset_func (diagnostic_port, callback);
}

void
ds_ipc_stream_factory_diagnostic_port_close (
	IpcStreamFactoryDiagnosticPort * diagnostic_port,
	bool is_shutdown,
	ds_ipc_error_callback_func callback)
{
	EP_ASSERT (diagnostic_port != NULL);
	if (diagnostic_port->ipc)
		ds_ipc_close (is_shutdown, callback);
	if (diagnostic_port->stream && !is_shutdown)
		ep_ipc_stream_close (callback); //TODO: Move ipc stream into ds.
}

IpcStreamFactoryDiagnosticPortBuilder *
ds_ipc_stream_factory_diagnostic_port_builder_init (IpcStreamFactoryDiagnosticPortBuilder *builder)
{
	EP_ASSERT (builder != NULL);
	builder->path = NULL;
	builder->suspend_mode = DS_PORT_SUSPEND_MODE_SUSPEND;
	builder->type = DS_PORT_TYPE_CONNECT;

	return builder;
}

void
ds_ipc_stream_factory_diagnostic_port_builder_fini (IpcStreamFactoryDiagnosticPortBuilder *builder)
{
	return;
}

void
ds_ipc_stream_factory_diagnostic_port_builder_set_tag (
	IpcStreamFactoryDiagnosticPortBuilder *builder,
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
		DS_LOG_INFO_1 ("IpcStreamFactory::DiagnosticPortBuilder::WithTag - Unknown tag '%s'.\n", tag);
}

#endif /* !defined(DS_INCLUDE_SOURCE_FILES) || defined(DS_FORCE_INCLUDE_SOURCE_FILES) */
#endif /* ENABLE_PERFTRACING */

#ifndef EP_INCLUDE_SOURCE_FILES
extern const char quiet_linker_empty_file_warning_diagnostics_ipc;
const char quiet_linker_empty_file_warning_diagnostics_ipc = 0;
#endif
