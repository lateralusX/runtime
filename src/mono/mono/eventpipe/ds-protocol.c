#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ds-rt-config.h"
#if !defined(DS_INCLUDE_SOURCE_FILES) || defined(DS_FORCE_INCLUDE_SOURCE_FILES)

#define DS_IMPL_PROTOCOL_GETTER_SETTER
#include "ds-protocol.h"
#include "ep.h"
#include "ep-stream.h"

static const DiagnosticsIpcHeader _ds_ipc_generic_success_header = {
	{ DOTNET_IPC_V1_MAGIC },
	(uint16_t)sizeof (DiagnosticsIpcHeader),
	(uint8_t)DS_SERVER_COMMANDSET_SERVER,
	(uint8_t)DS_SERVER_RESPONSEID_OK,
	(uint16_t)0x0000
};

static const DiagnosticsIpcHeader _ds_ipc_generic_error_header = {
	{ DOTNET_IPC_V1_MAGIC },
	(uint16_t)sizeof (DiagnosticsIpcHeader),
	(uint8_t)DS_SERVER_COMMANDSET_SERVER,
	(uint8_t)DS_SERVER_RESPONSEID_ERROR,
	(uint16_t)0x0000
};

typedef const uint8_t * (*ipc_parse_payload_func)(uint8_t *buffer, uint16_t buffer_len);

/*
 * Forward declares of all static functions.
 */

static
uint8_t *
ipc_advertise_cookie_v1_get (void);

static
bool
icp_advertise_v1_send (IpcStream *stream);

static
bool
ipc_message_try_parse_value (
	uint8_t **buffer,
	uint32_t *buffer_len,
	uint8_t *value,
	size_t value_len);

static
bool
ipc_message_try_parse_uint64_t (
	uint8_t **buffer,
	uint32_t *buffer_len,
	uint64_t *value);

static
inline
bool
ipc_message_try_parse_uint32_t (
	uint8_t **buffer,
	uint32_t *buffer_len,
	uint32_t *value);

static
inline
bool
ipc_message_try_parse_string_utf16_t (
	uint8_t **buffer,
	uint32_t *buffer_len,
	ep_char16_t **value);

static
bool
ipc_message_flatten_blitable_type (
	DiagnosticsIpcMessage *message,
	uint8_t *payload,
	size_t payload_size);

static
bool
ipc_message_initialize_header_uint32_t_payload (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcHeader *header,
	uint32_t payload);

static
bool
ipc_message_initialize_header_uint64_t_payload (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcHeader *header,
	uint64_t payload);

static
bool
ipc_message_try_parse (
	DiagnosticsIpcMessage *message,
	IpcStream *stream);

static
const uint8_t *
ipc_message_try_parse_payload (
	DiagnosticsIpcMessage *message,
	ipc_parse_payload_func parse_func);

static
bool
ipc_message_send (
	DiagnosticsIpcMessage *message,
	IpcStream *stream);

static
bool
ipc_message_send_stop_tracing_success (
	IpcStream *stream,
	EventPipeSessionID session_id);

static
bool
ipc_message_send_start_tracing_success (
	IpcStream *stream,
	EventPipeSessionID session_id);

static
bool
collect_tracing_command_try_parse_serialization_format (
	uint8_t **buffer,
	uint32_t *buffer_len,
	EventPipeSerializationFormat *format);

static
bool
collect_tracing_command_try_parse_circular_buffer_size (
	uint8_t **buffer,
	uint32_t *buffer_len,
	uint32_t *circular_buffer);

static
bool
collect_tracing_command_try_parse_rundown_requested (
	uint8_t **buffer,
	uint32_t *buffer_len,
	bool *rundown_requested);

static
bool
is_null_or_whitespace (ep_char8_t *str);

static
bool
collect_tracing_command_try_parse_config (
	uint8_t **buffer,
	uint32_t *buffer_len,
	ep_rt_provider_config_array_t *result);

static
const
uint8_t *
collect_tracing_command_try_parse_payload (
	uint8_t *buffer,
	uint16_t buffer_len);

static
const
uint8_t *
collect_tracing2_command_try_parse_payload (
	uint8_t *buffer,
	uint16_t buffer_len);

static
void
eventpipe_protocol_helper_stop_tracing (
	DiagnosticsIpcMessage *message,
	IpcStream *stream);

static
void
eventpipe_protocol_helper_collect_tracing (
	DiagnosticsIpcMessage *message,
	IpcStream *stream);

static
void
eventpipe_protocol_helper_collect_tracing_2 (
	DiagnosticsIpcMessage *message,
	IpcStream *stream);

/*
* DiagnosticsIpc
*/

/**
* ==ADVERTISE PROTOCOL==
* Before standard IPC Protocol communication can occur on a client-mode connection
* the runtime must advertise itself over the connection.  ALL SUBSEQUENT COMMUNICATION 
* IS STANDARD DIAGNOSTICS IPC PROTOCOL COMMUNICATION.
* 
* See spec in: dotnet/diagnostics@documentation/design-docs/ipc-spec.md
* 
* The flow for Advertise is a one-way burst of 24 bytes consisting of
* 8 bytes  - "ADVR_V1\0" (ASCII chars + null byte)
* 16 bytes - random 128 bit number cookie (little-endian)
* 8 bytes  - PID (little-endian)
* 2 bytes  - unused 2 byte field for futureproofing
*/
static
uint8_t *
ipc_advertise_cookie_v1_get (void)
{
	static uint8_t advertise_cooike_v1_buffer [EP_ACTIVITY_ID_SIZE];
	static uint8_t *advertise_cooike_v1 = NULL;

	if (!advertise_cooike_v1) {
		ep_rt_create_activity_id (advertise_cooike_v1_buffer, EP_ACTIVITY_ID_SIZE);
		advertise_cooike_v1 = advertise_cooike_v1_buffer;
	}
	return advertise_cooike_v1;
}

bool
ds_icp_advertise_v1_send (IpcStream *stream)
{
	uint8_t advertise_buffer [DOTNET_IPC_V1_ADVERTISE_SIZE];
	uint8_t *cookie = ipc_advertise_cookie_v1_get ();
	uint64_t pid = DS_VAL64 (ep_rt_current_process_get_id ());
	uint64_t *buffer = (uint64_t *)advertise_buffer;
	bool result = false;

	ep_return_false_if_nok (stream != NULL);

	memcpy (buffer, DOTNET_IPC_V1_ADVERTISE_MAGIC, sizeof (uint64_t));
	buffer++;

	//Etract parts of guid buffer, convert elemnts to little endian.
	uint32_t data1;
	memcpy (&data1, cookie, sizeof (data1));
	data1 = DS_VAL32 (data1);
	cookie += sizeof (data1);

	uint16_t data2;
	memcpy (&data2, cookie, sizeof (data2));
	data2 = DS_VAL16 (data2);
	cookie += sizeof (data2);

	uint16_t data3;
	memcpy (&data3, cookie, sizeof (data3));
	data3 = DS_VAL16 (data3);
	cookie += sizeof (data3);

	uint64_t data4;
	memcpy (&data4, cookie, sizeof (data4));

	uint64_t swap = ((uint64_t)data1 << 32) | ((uint64_t)data2 << 16) | (uint64_t)data3;
	memcpy (buffer, &swap, sizeof (uint64_t));

	memcpy (buffer, &data4, sizeof (uint64_t));
	buffer++;

	memcpy (buffer, &pid, sizeof (uint64_t));
	buffer++;

	// zero out unused filed
	memset (buffer, 0, sizeof (uint16_t));

	uint32_t bytes_written = 0;
	ep_raise_error_if_nok (ep_ipc_stream_write (stream, advertise_buffer, sizeof (advertise_buffer), &bytes_written, 100 /*ms*/) == true);

	EP_ASSERT (bytes_written == sizeof (advertise_buffer));
	result = (bytes_written == sizeof (advertise_buffer));

ep_on_exit:
	return result;

ep_on_error:
	result = false;
	ep_exit_error_handler ();
}

/*
* DiagnosticsIpcMessage
*/

static
inline
bool
ipc_message_try_parse_value (
	uint8_t **buffer,
	uint32_t *buffer_len,
	uint8_t *value,
	size_t value_len)
{
	EP_ASSERT (buffer != NULL);
	EP_ASSERT (buffer_len != NULL);
	EP_ASSERT (value != NULL);
	EP_ASSERT ((buffer_len - value_len) >= 0);

	memcpy (value, *buffer, value_len);
	*buffer = *buffer + value_len;
	*buffer_len = *buffer_len - value_len;
	return true;
}

static
inline
bool
ipc_message_try_parse_uint64_t (
	uint8_t **buffer,
	uint32_t *buffer_len,
	uint64_t *value)
{
	EP_ASSERT (buffer != NULL);
	EP_ASSERT (buffer_len != NULL);
	EP_ASSERT (value != NULL);

	bool result = ipc_message_try_parse_value (buffer, buffer_len, (uint8_t *)value, sizeof (uint64_t));
	if (result)
		value = DS_VAL64 (value);
	return result;
}

static
inline
bool
ipc_message_try_parse_uint32_t (
	uint8_t **buffer,
	uint32_t *buffer_len,
	uint32_t *value)
{
	EP_ASSERT (buffer != NULL);
	EP_ASSERT (buffer_len != NULL);
	EP_ASSERT (value != NULL);

	bool result = ipc_message_try_parse_value (buffer, buffer_len, (uint8_t*)value, sizeof (uint32_t));
	if (result)
		value = DS_VAL32 (value);
	return result;
}

// TODO: Strings are in little endian format in buffer.
static
inline
bool
ipc_message_try_parse_string_utf16_t (
	uint8_t **buffer,
	uint32_t *buffer_len,
	ep_char16_t **value)
{
	EP_ASSERT (buffer_cursor != NULL);
	EP_ASSERT (buffer_len != NULL);
	EP_ASSERT (value != NULL);

	bool result = false;

	uint32_t string_len = 0;
	ep_raise_error_if_nok (ipc_message_try_parse_uint32_t (buffer, buffer_len, &string_len) == true);

	if (string_len != 0) {
		if (string_len > (*buffer_len / sizeof (ep_char16_t)))
			ep_raise_error ();

		if (((const ep_char16_t *)*buffer) [string_len - 1] != 0)
			ep_raise_error ();

		*value = (ep_char16_t *)*buffer;

	} else {
		*value = NULL;
	}

	*buffer = *buffer + (string_len * sizeof (ep_char16_t));
	*buffer_len = *buffer_len + (string_len * sizeof (ep_char16_t));

	result = true;

ep_on_exit:
	return result;

ep_on_error:
	EP_ASSERT (result == false);
	ep_exit_error_handler ();
}

static
bool
ipc_message_flatten_blitable_type (
	DiagnosticsIpcMessage *message,
	uint8_t *payload,
	size_t payload_size)
{
	EP_ASSERT (message != NULL);
	EP_ASSERT (payload != NULL);

	if (message->data != NULL)
		return true;

	bool result = false;
	uint8_t *buffer = NULL;
	uint8_t *buffer_cursor = NULL;

	EP_ASSERT (sizeof (message->header) + payload_size <= UINT16_MAX);
	message->size = sizeof (message->header) + payload_size;

	buffer = ep_rt_byte_array_alloc (message->size);
	ep_raise_error_if_nok (buffer != NULL);

	buffer_cursor = buffer;
	message->header.size = message->size;

	memcpy (buffer_cursor, &message->header, sizeof (message->header));
	buffer_cursor += sizeof (message->header);

	memcpy (buffer_cursor, payload, payload_size);

	EP_ASSERT (message->data == NULL);
	message->data = buffer;

	buffer = NULL;
	result = true;

ep_on_exit:
	return result;

ep_on_error:
	ep_rt_byte_array_free (buffer);
	result = false;
	ep_exit_error_handler ();
}

static
bool
ipc_message_initialize_header_uint32_t_payload (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcHeader *header,
	uint32_t payload)
{
	EP_ASSERT (message);
	EP_ASSERT (header);

	message->header = *header;
	return ipc_message_flatten_blitable_type (message, (uint8_t *)&payload, sizeof (payload));
}

static
bool
ipc_message_initialize_header_uint64_t_payload (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcHeader *header,
	uint64_t payload)
{
	EP_ASSERT (message);
	EP_ASSERT (header);

	message->header = *header;
	return ipc_message_flatten_blitable_type (message, (uint8_t *)&payload, sizeof (payload));
}

// Attempt to populate header and payload from a buffer.
// Payload is left opaque as a flattened buffer in m_pData
static
bool
ipc_message_try_parse (
	DiagnosticsIpcMessage *message,
	IpcStream *stream)
{
	EP_ASSERT (message != NULL);
	EP_ASSERT (stream != NULL);

	uint8_t *buffer = NULL;
	bool success = false;

	// Read out header first
	uint32_t bytes_read;
	success = ep_ipc_stream_read (stream, (uint8_t *)&message->header, sizeof (message->header), &bytes_read, EP_INFINITE_WAIT);
	if (!success || (bytes_read < sizeof (message->header)))
		ep_raise_error ();

	if (message->header.size < sizeof (message->header))
		ep_raise_error ();

	message->size = message->header.size;

	// Then read out payload to buffer.
	uint16_t payload_size = message->header.size - sizeof (message->header);
	if (payload_size != 0) {
		uint8_t *buffer = ep_rt_byte_array_alloc (payload_size);
		ep_raise_error_if_nok (buffer != NULL);

		success = ep_ipc_stream_read (stream, buffer, payload_size, &bytes_read, EP_INFINITE_WAIT);
		if (!success || (bytes_read < payload_size))
			ep_raise_error ();

		message->data = buffer;
		buffer = NULL;
	}

ep_on_exit:
	return success;

ep_on_error:
	ep_rt_byte_array_free (buffer);
	success = false;
	ep_exit_error_handler ();
}

static
const uint8_t *
ipc_message_try_parse_payload (
	DiagnosticsIpcMessage *message,
	ipc_parse_payload_func parse_func)
{
	ep_return_false_if_nok (message != NULL);

	EP_ASSERT (message->data);

	uint8_t *payload = NULL;

	if (parse_func) {
		EP_ASSERT (!parse_func);
	} else {
		payload = message->data;
		message->data = NULL; // user is expected to clean up buffer when finished with it
	}

	return payload;
}

static
bool
ipc_message_send (
	DiagnosticsIpcMessage *message,
	IpcStream *stream)
{
	EP_ASSERT (message != NULL);
	EP_ASSERT (message->data != NULL);
	EP_ASSERT (stream != NULL);

	uint32_t bytes_written;
	bool success = ep_ipc_stream_write (stream, message->data, message->size, &bytes_written, EP_INFINITE_WAIT);
	return (bytes_written == message->size) && success;
}

static
bool
ipc_message_send_stop_tracing_success (
	IpcStream *stream,
	EventPipeSessionID session_id)
{
	EP_ASSERT (stream != NULL);

	DiagnosticsIpcMessage success_message;
	ds_ipc_message_init (&success_message);
	bool success = ipc_message_initialize_header_uint64_t_payload (&success_message, &_ds_ipc_generic_success_header, (uint64_t)session_id);
	if (success)
		ipc_message_send (&success_message, stream);
	ds_ipc_message_fini (&success_message);
	return success;
}

static
bool
ipc_message_send_start_tracing_success (
	IpcStream *stream,
	EventPipeSessionID session_id)
{
	EP_ASSERT (stream != NULL);

	DiagnosticsIpcMessage success_message;
	ds_ipc_message_init (&success_message);
	bool success = ipc_message_initialize_header_uint64_t_payload (&success_message, &_ds_ipc_generic_success_header, (uint64_t)session_id);
	if (success)
		ipc_message_send (&success_message, stream);
	ds_ipc_message_fini (&success_message);
	return success;
}

DiagnosticsIpcMessage *
ds_ipc_message_init (DiagnosticsIpcMessage *message)
{
	ep_return_null_if_nok (message != NULL);

	message->data = NULL;
	message->size = 0;
	memset (&message->header, 0 , sizeof (message->header));

	return message;
}

void
ds_ipc_message_fini (DiagnosticsIpcMessage *message)
{
	ep_return_void_if_nok (message != NULL);
	ep_rt_byte_array_free (message->data);
}

bool
ds_ipc_message_initialize (
	DiagnosticsIpcMessage *message,
	IpcStream *stream)
{
	return ipc_message_try_parse (message, stream);
}

bool
ds_ipc_message_send_error (
	IpcStream *stream,
	uint32_t error)
{
	ep_return_false_if_nok (stream != NULL);

	DiagnosticsIpcMessage error_message;
	ds_ipc_message_init (&error_message);
	bool success = ipc_message_initialize_header_uint32_t_payload (&error_message, &_ds_ipc_generic_error_header, error);
	if (success)
		ipc_message_send (&error_message, stream);
	ds_ipc_message_fini (&error_message);
	return success;
}

bool
ds_ipc_message_send_success (
	IpcStream *stream,
	uint32_t code)
{
	ep_return_false_if_nok (stream != NULL);

	DiagnosticsIpcMessage success_message;
	ds_ipc_message_init (&success_message);
	bool success = ipc_message_initialize_header_uint32_t_payload (&success_message, &_ds_ipc_generic_success_header, code);
	if (success)
		ipc_message_send (&success_message, stream);
	ds_ipc_message_fini (&success_message);
	return success;
}

/*
* EventPipeCollectTracingCommandPayload
*/

static
inline
bool
collect_tracing_command_try_parse_serialization_format (
	uint8_t **buffer,
	uint32_t *buffer_len,
	EventPipeSerializationFormat *format)
{
	EP_ASSERT (buffer != NULL);
	EP_ASSERT (buffer_len != NULL);
	EP_ASSERT (format != NULL);

	uint32_t serialization_format;
	bool can_parse = ipc_message_try_parse_uint32_t (buffer, buffer_len, &serialization_format);

	*format = (EventPipeSerializationFormat)serialization_format;
	return can_parse && (0 <= (int32_t)serialization_format) && ((int32_t)serialization_format < (int32_t)EP_SERIALIZATION_FORMAT_COUNT);
}

static
inline
bool
collect_tracing_command_try_parse_circular_buffer_size (
	uint8_t **buffer,
	uint32_t *buffer_len,
	uint32_t *circular_buffer)
{
	EP_ASSERT (buffer != NULL);
	EP_ASSERT (buffer_len != NULL);
	EP_ASSERT (circular_buffer != NULL);

	bool can_parse = ipc_message_try_parse_uint32_t (buffer, buffer_len, circular_buffer);
	return can_parse && (*circular_buffer > 0);
}

static
inline
bool
collect_tracing_command_try_parse_rundown_requested (
	uint8_t **buffer,
	uint32_t *buffer_len,
	bool *rundown_requested)
{
	EP_ASSERT (buffer != NULL);
	EP_ASSERT (buffer_len != NULL);
	EP_ASSERT (rundown_requested != NULL);

	return ipc_message_try_parse_value (buffer, buffer_len, (uint8_t *)rundown_requested, sizeof (bool));
}

static
bool
is_null_or_whitespace (ep_char8_t *str)
{
	if (str == NULL)
		return true;

	while (*str) {
		if (!isspace(*str))
			return false;
		str++;
	}
	return true;
}

static
bool
collect_tracing_command_try_parse_config (
	uint8_t **buffer,
	uint32_t *buffer_len,
	ep_rt_provider_config_array_t *result)
{
	EP_ASSERT (buffer != NULL);
	EP_ASSERT (buffer_len != NULL);
	EP_ASSERT (result != NULL);

	// Picking an arbitrary upper bound,
	// This should be larger than any reasonable client request.
	// TODO: This might be too large.
	const uint32_t max_count_configs = 1000;
	uint32_t count_configs = 0;

	ep_char8_t *provider_name_utf8 = NULL;
	ep_char8_t *filter_data_utf8 = NULL;

	ep_raise_error_if_nok (ipc_message_try_parse_uint32_t (buffer, buffer_len, &count_configs) == true);
	ep_raise_error_if_nok (count_configs <= max_count_configs);

	ep_rt_provider_config_array_alloc_capacity (result, count_configs);

	for (uint32_t i = 0; i < count_configs; ++i) {
		uint64_t keywords = 0;
		ep_raise_error_if_nok (ipc_message_try_parse_uint64_t (buffer, buffer_len, &keywords) == true);

		uint32_t log_level = 0;
		ep_raise_error_if_nok (ipc_message_try_parse_uint32_t (buffer, buffer_len, &log_level) == true);
		ep_raise_error_if_nok (log_level <= EP_EVENT_LEVEL_VERBOSE);

		const ep_char16_t *provider_name = NULL;
		ep_raise_error_if_nok (ipc_message_try_parse_string_utf16_t (buffer, buffer_len, &provider_name) == true);

		provider_name_utf8 = ep_rt_utf16_to_utf8_string (provider_name, -1);
		ep_raise_error_if_nok (provider_name_utf8 != NULL);

		ep_raise_error_if_nok (is_null_or_whitespace (provider_name_utf8) == false);

		const ep_char16_t *filter_data = NULL; // This parameter is optional.
		ipc_message_try_parse_string_utf16_t (buffer, buffer_len, &filter_data);

		if (filter_data) {
			filter_data_utf8 = ep_rt_utf16_to_utf8_string (filter_data, -1);
			ep_raise_error_if_nok (filter_data_utf8 != NULL);
		}

		EventPipeProviderConfiguration provider_config;
		ep_provider_config_init (&provider_config, provider_name_utf8, keywords, (EventPipeEventLevel)log_level, filter_data_utf8);
		ep_rt_provider_config_array_append (result, provider_config);

		provider_name_utf8 = NULL;
		filter_data_utf8 = NULL;
	}

ep_on_exit:
	return (count_configs > 0);

ep_on_error:
	count_configs = 0;
	ep_rt_utf8_string_free (provider_name_utf8);
	ep_rt_utf8_string_free (filter_data_utf8);
	ep_exit_error_handler ();
}

static
const
uint8_t *
collect_tracing_command_try_parse_payload (
	uint8_t *buffer,
	uint16_t buffer_len)
{
	EP_ASSERT (buffer != NULL);

	uint8_t * buffer_cursor = buffer;
	uint32_t buffer_cursor_len = buffer_len;

	EventPipeCollectTracingCommandPayload * instance = ds_collect_tracing_command_payload_alloc ();
	ep_raise_error_if_nok (instance != NULL);

	instance->incoming_buffer = buffer;

	if (!collect_tracing_command_try_parse_circular_buffer_size (&buffer_cursor, &buffer_cursor_len, &instance->circular_buffer_size_in_mb ) ||
		!collect_tracing_command_try_parse_serialization_format (&buffer_cursor, &buffer_cursor_len, &instance->serialization_format) ||
		!collect_tracing_command_try_parse_config (&buffer_cursor, &buffer_cursor_len, &instance->provider_configs))
		ep_raise_error ();

ep_on_exit:
	return (uint8_t *)instance;

ep_on_error:
	ds_collect_tracing_command_payload_free (instance);
	instance = NULL;
	ep_exit_error_handler ();
}

EventPipeCollectTracingCommandPayload *
ds_collect_tracing_command_payload_alloc (void)
{
	return ep_rt_object_alloc (EventPipeCollectTracingCommandPayload);
}

void
ds_collect_tracing_command_payload_free (EventPipeCollectTracingCommandPayload *payload)
{
	ep_return_void_if_nok (payload != NULL);
	ep_rt_byte_array_free (payload->incoming_buffer);

	EventPipeProviderConfiguration *config = ep_rt_provider_config_array_data (&payload->provider_configs);
	size_t config_len = ep_rt_provider_config_array_size (&payload->provider_configs);
	for (size_t i = 0; i < config_len; ++i) {
		ep_rt_utf8_string_free (config [i].provider_name);
		ep_rt_utf8_string_free (config [i].filter_data);
	}

	ep_rt_object_free (payload);
}

/*
* EventPipeCollectTracing2CommandPayload
*/

static
const
uint8_t *
collect_tracing2_command_try_parse_payload (
	uint8_t *buffer,
	uint16_t buffer_len)
{
	EP_ASSERT (buffer != NULL);

	uint8_t * buffer_cursor = buffer;
	uint32_t buffer_cursor_len = buffer_len;

	EventPipeCollectTracing2CommandPayload * instance = ds_collect_tracing2_command_payload_alloc ();
	ep_raise_error_if_nok (instance != NULL);

	instance->incoming_buffer = buffer;

	if (!collect_tracing_command_try_parse_circular_buffer_size (&buffer_cursor, &buffer_cursor_len, &instance->circular_buffer_size_in_mb ) ||
		!collect_tracing_command_try_parse_serialization_format (&buffer_cursor, &buffer_cursor_len, &instance->serialization_format) ||
		!collect_tracing_command_try_parse_rundown_requested (&buffer_cursor, &buffer_cursor_len, &instance->rundown_requested) ||
		!collect_tracing_command_try_parse_config (&buffer_cursor, &buffer_cursor_len, &instance->provider_configs))
		ep_raise_error ();

ep_on_exit:
	return (uint8_t *)instance;

ep_on_error:
	ds_collect_tracing2_command_payload_free (instance);
	instance = NULL;
	ep_exit_error_handler ();
}

EventPipeCollectTracing2CommandPayload *
ds_collect_tracing2_command_payload_alloc (void)
{
	return ep_rt_object_alloc (EventPipeCollectTracing2CommandPayload);
}

void
ds_collect_tracing2_command_payload_free (EventPipeCollectTracing2CommandPayload *payload)
{
	ep_return_void_if_nok (payload != NULL);
	ep_rt_byte_array_free (payload->incoming_buffer);

	EventPipeProviderConfiguration *config = ep_rt_provider_config_array_data (&payload->provider_configs);
	size_t config_len = ep_rt_provider_config_array_size (&payload->provider_configs);
	for (size_t i = 0; i < config_len; ++i) {
		ep_rt_utf8_string_free (config [i].provider_name);
		ep_rt_utf8_string_free (config [i].filter_data);
	}

	ep_rt_object_free (payload);
}

/*
* EventPipeProtocolHelper
*/

static
void
eventpipe_protocol_helper_stop_tracing (
	DiagnosticsIpcMessage *message,
	IpcStream *stream)
{
	ep_return_void_if_nok (message != NULL && stream != NULL);

	const EventPipeStopTracingCommandPayload *payload;
	payload = (const EventPipeStopTracingCommandPayload *)ipc_message_try_parse_payload (message, NULL);

	if (!payload) {
		ds_ipc_message_send_error (stream, DS_IPC_E_BAD_ENCODING);
		ep_raise_error ();
	}

	ep_disable (payload->session_id);

	ipc_message_send_stop_tracing_success (stream, payload->session_id);
	ep_ipc_stream_flush (stream);

ep_on_exit:
	ds_stop_tracing_command_payload_free (payload);
	ep_ipc_stream_free (stream);
	return;

ep_on_error:
	ep_exit_error_handler ();
}

static
void
eventpipe_protocol_helper_collect_tracing (
	DiagnosticsIpcMessage *message,
	IpcStream *stream)
{
	ep_return_void_if_nok (message != NULL && stream != NULL);

	const EventPipeCollectTracingCommandPayload *payload;
	payload = (const EventPipeCollectTracingCommandPayload *)ipc_message_try_parse_payload (message, collect_tracing_command_try_parse_payload);

	if (!payload) {
		ds_ipc_message_send_error (stream, DS_IPC_E_BAD_ENCODING);
		ep_raise_error ();
	}

	EventPipeSessionID session_id = ep_enable (
		NULL,
		payload->circular_buffer_size_in_mb,
		ep_rt_provider_config_array_data (&payload->provider_configs),
		(uint32_t)ep_rt_provider_config_array_size (&payload->provider_configs),
		EP_SESSION_TYPE_IPCSTREAM,
		payload->serialization_format,
		true,
		stream,
		true);

	if (session_id == 0) {
		ds_ipc_message_send_error (stream, DS_IPC_E_FAIL);
		ep_raise_error ();
	} else {
		ipc_message_send_start_tracing_success (stream, session_id);
		ep_start_streaming (session_id);
	}

ep_on_exit:
	ds_collect_tracing_command_payload_free (payload);
	return;

ep_on_error:
	ep_ipc_stream_free (stream);
	ep_exit_error_handler ();
}

static
void
eventpipe_protocol_helper_collect_tracing_2 (
	DiagnosticsIpcMessage *message,
	IpcStream *stream)
{
	ep_return_void_if_nok (message != NULL && stream != NULL);

	const EventPipeCollectTracing2CommandPayload *payload;
	payload = (const EventPipeCollectTracing2CommandPayload *)ipc_message_try_parse_payload (message, collect_tracing2_command_try_parse_payload);

	if (!payload) {
		ds_ipc_message_send_error (stream, DS_IPC_E_BAD_ENCODING);
		ep_raise_error ();
	}

	EventPipeSessionID session_id = ep_enable (
		NULL,
		payload->circular_buffer_size_in_mb,
		ep_rt_provider_config_array_data (&payload->provider_configs),
		(uint32_t)ep_rt_provider_config_array_size (&payload->provider_configs),
		EP_SESSION_TYPE_IPCSTREAM,
		payload->serialization_format,
		payload->rundown_requested,
		stream,
		true);

	if (session_id == 0) {
		ds_ipc_message_send_error (stream, DS_IPC_E_FAIL);
		ep_raise_error ();
	} else {
		ipc_message_send_start_tracing_success (stream, session_id);
		ep_start_streaming (session_id);
	}

ep_on_exit:
	ds_collect_tracing2_command_payload_free (payload);
	return;

ep_on_error:
	ep_ipc_stream_free (stream);
	ep_exit_error_handler ();
}

void
ds_stop_tracing_command_payload_free (EventPipeStopTracingCommandPayload *payload)
{
	ep_return_void_if_nok (payload != NULL);
	ep_rt_byte_array_free ((uint8_t *)payload);
}

void
ds_eventpipe_protocol_helper_handle_ipc_message (
	DiagnosticsIpcMessage *message,
	IpcStream *stream)
{
	ep_return_void_if_nok (message != NULL && stream != NULL);

	switch ((EventPipeCommandId)ds_ipc_header_get_commandid (ds_ipc_message_get_header_cref (message))) {
	case EP_COMMANDID_COLLECT_TRACING:
		eventpipe_protocol_helper_collect_tracing (message, stream);
		break;
	case EP_COMMANDID_COLLECT_TRACING_2:
		eventpipe_protocol_helper_collect_tracing_2 (message, stream);
		break;
	case EP_COMMANDID_STOP_TRACING:
		eventpipe_protocol_helper_stop_tracing (message, stream);
		break;
	default:
		DS_LOG_WARNING_1 ("Received unknown request type (%d)\n", ds_ipc_header_get_commandset (ds_ipc_message_get_header_cref (message)));
		ds_ipc_message_send_error (stream, DS_IPC_E_UNKNOWN_COMMAND);
		ep_ipc_stream_free (stream);
		break;
	}
}

#endif /* !defined(EP_INCLUDE_SOURCE_FILES) || defined(EP_FORCE_INCLUDE_SOURCE_FILES) */
#endif /* ENABLE_PERFTRACING */

#ifndef EP_INCLUDE_SOURCE_FILES
extern const char quiet_linker_empty_file_warning_diagnostics_protocol;
const char quiet_linker_empty_file_warning_diagnostics_protocol = 0;
#endif
