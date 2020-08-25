#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ds-rt-config.h"
#if !defined(DS_INCLUDE_SOURCE_FILES) || defined(DS_FORCE_INCLUDE_SOURCE_FILES)

#define DS_IMPL_PROTOCOL_GETTER_SETTER
#include "ds-protocol.h"
#include "ds-server.h"
#include "ep.h"
#include "ep-stream.h"
#include "ep-event-source.h"

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

static uint8_t _ds_ipc_advertise_cooike_v1 [EP_ACTIVITY_ID_SIZE] = { 0 };

typedef bool (ipc_flatten_payload_func)(void *payload, uint8_t **buffer, uint16_t *buffer_len);
typedef const uint8_t * (*ipc_parse_payload_func)(uint8_t *buffer, uint16_t buffer_len);

/*
 * Forward declares of all static functions.
 */

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
	size_t payload_len);

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
ipc_message_try_write_string_utf16_t (
	uint8_t **buffer,
	uint16_t *buffer_len,
	const ep_char16_t *value);

static
bool
ipc_message_try_send_string_utf16_t (
	IpcStream *stream,
	const ep_char16_t *value);

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
ipc_message_flatten (
	DiagnosticsIpcMessage *message,
	void *payload,
	uint16_t payload_len,
	ipc_flatten_payload_func flatten_payload);

static
bool
ipc_message_initialize_buffer (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcHeader *header,
	void *payload,
	uint16_t payload_len,
	ipc_flatten_payload_func flatten_payload);

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

static
uint16_t
process_info_payload_get_size (DiagnosticsProcessInfoPayload *payload);

static
bool
process_info_payload_flatten (
	DiagnosticsProcessInfoPayload *payload,
	uint8_t **buffer,
	uint16_t *size);

/*
* DiagnosticsIpc
*/

uint8_t *
ds_ipc_advertise_cookie_v1_get (void)
{
	return _ds_ipc_advertise_cooike_v1;
}

void
ds_ipc_advertise_cookie_v1_init (void)
{
	ep_rt_create_activity_id ((uint8_t *)&_ds_ipc_advertise_cooike_v1, EP_ACTIVITY_ID_SIZE);
}

/**
* ==ADVERTISE PROTOCOL==
* Before standard IPC Protocol communication can occur on a client-mode connection
* the runtime must advertise itself over the connection.  ALL SUBSEQUENT COMMUNICATION 
* IS STANDARD DIAGNOSTICS IPC PROTOCOL COMMUNICATION.
* 
* See spec in: dotnet/diagnostics@documentation/design-docs/ipc-spec.md
* 
* The flow for Advertise is a one-way burst of 34 bytes consisting of
* 8 bytes  - "ADVR_V1\0" (ASCII chars + null byte)
* 16 bytes - random 128 bit number cookie (little-endian)
* 8 bytes  - PID (little-endian)
* 2 bytes  - unused 2 byte field for futureproofing
*/
bool
ds_icp_advertise_v1_send (IpcStream *stream)
{
	uint8_t advertise_buffer [DOTNET_IPC_V1_ADVERTISE_SIZE];
	uint8_t *cookie = ds_ipc_advertise_cookie_v1_get ();
	uint64_t pid = DS_VAL64 (ep_rt_current_process_get_id ());
	uint64_t *buffer = (uint64_t *)advertise_buffer;
	bool result = false;

	ep_return_false_if_nok (stream != NULL);

	memcpy (buffer, DOTNET_IPC_V1_ADVERTISE_MAGIC, sizeof (uint64_t));
	buffer++;

	// fills buffer[1] and buffer[2]
	memcpy (buffer, cookie, EP_ACTIVITY_ID_SIZE);
	buffer +=2;

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
ipc_message_try_write_string_utf16_t (
	uint8_t **buffer,
	uint16_t *buffer_len,
	const ep_char16_t *value)
{
	EP_ASSERT (buffer != NULL);
	EP_ASSERT (*buffer != NULL);
	EP_ASSERT (buffer_len != NULL);
	EP_ASSERT (value != NULL);

	bool result = true;
	uint32_t string_len = (uint32_t)(ep_rt_utf16_string_len (value) + 1);
	size_t total_bytes = (string_len * sizeof (ep_char16_t)) + sizeof(uint32_t);

	EP_ASSERT (total_bytes <= UINT16_MAX);
	EP_ASSERT (*buffer_len >= (uint16_t)total_bytes);
	if (*buffer_len < (uint16_t)total_bytes || total_bytes > UINT16_MAX)
		ep_raise_error ();

	memcpy (*buffer, &string_len, sizeof (string_len));
	*buffer += sizeof (string_len);

	memcpy (*buffer, value, string_len * sizeof (ep_char16_t));
	*buffer += (string_len * sizeof (ep_char16_t));

	*buffer_len -= (uint16_t)total_bytes;

ep_on_exit:
	return result;

ep_on_error:
	result = false;
	ep_exit_error_handler ();
}

static
bool
ipc_message_try_send_string_utf16_t (
	IpcStream *stream,
	const ep_char16_t *value)
{
	uint32_t string_len = (uint32_t)(ep_rt_utf16_string_len (value) + 1);
	uint32_t string_bytes = (uint32_t)(string_len * sizeof (ep_char16_t));
	uint32_t total_bytes = (uint32_t)(string_bytes + sizeof (uint32_t));

	uint32_t total_written = 0;
	uint32_t written = 0;

	bool success = ep_ipc_stream_write (stream, (const uint8_t *)&string_len, (uint32_t)sizeof (string_len), &written, EP_INFINITE_WAIT);
	total_written += written;

	if (success) {
		success &= ep_ipc_stream_write (stream, (const uint8_t *)value, string_bytes, &written, EP_INFINITE_WAIT);
		total_written += written;
	}

	EP_ASSERT (total_bytes == total_written);
	return success && (total_bytes == total_written);
}

static
bool
ipc_message_flatten_blitable_type (
	DiagnosticsIpcMessage *message,
	uint8_t *payload,
	size_t payload_len)
{
	EP_ASSERT (message != NULL);
	EP_ASSERT (payload != NULL);

	if (message->data != NULL)
		return true;

	bool result = false;
	uint8_t *buffer = NULL;
	uint8_t *buffer_cursor = NULL;

	EP_ASSERT (sizeof (message->header) + payload_len <= UINT16_MAX);
	message->size = sizeof (message->header) + payload_len;

	buffer = ep_rt_byte_array_alloc (message->size);
	ep_raise_error_if_nok (buffer != NULL);

	buffer_cursor = buffer;
	message->header.size = message->size;

	memcpy (buffer_cursor, &message->header, sizeof (message->header));
	buffer_cursor += sizeof (message->header);

	memcpy (buffer_cursor, payload, payload_len);

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
	uint16_t payload_len = message->header.size - sizeof (message->header);
	if (payload_len != 0) {
		uint8_t *buffer = ep_rt_byte_array_alloc (payload_len);
		ep_raise_error_if_nok (buffer != NULL);

		success = ep_ipc_stream_read (stream, buffer, payload_len, &bytes_read, EP_INFINITE_WAIT);
		if (!success || (bytes_read < payload_len))
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

	if (parse_func)
		payload = parse_func (message->data, message->size - sizeof (message->header));
	else
		payload = message->data;

	message->data = NULL; // user is expected to clean up buffer when finished with it
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

static
bool
ipc_message_flatten (
	DiagnosticsIpcMessage *message,
	void *payload,
	uint16_t payload_len,
	ipc_flatten_payload_func flatten_payload)
{
	EP_ASSERT (message != NULL);
	EP_ASSERT (payload != NULL);

	if (message->data)
		return true;

	bool result = true;
	uint8_t *buffer = NULL;

	uint16_t total_len = 0;
	total_len += sizeof (DiagnosticsIpcHeader) + payload_len;

	uint16_t remaining_len = total_len;
	message->size = total_len;

	buffer = ep_rt_byte_array_alloc (message->size);
	ep_raise_error_if_nok (buffer != NULL);

	uint8_t * buffer_cursor = buffer;
	message->header.size = message->size;

	memcpy (buffer_cursor, &message->header, sizeof (DiagnosticsIpcHeader));
	buffer_cursor += sizeof (DiagnosticsIpcHeader);
	remaining_len -= sizeof (DiagnosticsIpcHeader);

	if (flatten_payload)
		result = flatten_payload (payload, &buffer_cursor, &remaining_len);
	else
		memcpy (buffer_cursor, payload, payload_len);

	EP_ASSERT (message->data == NULL);

	//Transfer ownership.
	message->data = buffer;
	buffer = NULL;

ep_on_exit:
	ep_rt_byte_array_free (buffer);
	return result;

ep_on_error:
	result = false;
	ep_exit_error_handler ();
}

static
bool
ipc_message_initialize_buffer (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcHeader *header,
	void *payload,
	uint16_t payload_len,
	ipc_flatten_payload_func flatten_payload)
{
	message->header = *header;
	return ipc_message_flatten (message, payload, payload_len, flatten_payload);
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
ds_ipc_message_initialize_stream (
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
		NULL);

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
		NULL);

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
	case DS_COMMANDID_COLLECT_TRACING:
		eventpipe_protocol_helper_collect_tracing (message, stream);
		break;
	case DS_COMMANDID_COLLECT_TRACING_2:
		eventpipe_protocol_helper_collect_tracing_2 (message, stream);
		break;
	case DS_COMMANDID_STOP_TRACING:
		eventpipe_protocol_helper_stop_tracing (message, stream);
		break;
	default:
		DS_LOG_WARNING_1 ("Received unknown request type (%d)\n", ds_ipc_header_get_commandset (ds_ipc_message_get_header_cref (message)));
		ds_ipc_message_send_error (stream, DS_IPC_E_UNKNOWN_COMMAND);
		ep_ipc_stream_free (stream);
		break;
	}
}

/*
 * DiagnosticsProcessInfoPayload.
 */

static
uint16_t
process_info_payload_get_size (DiagnosticsProcessInfoPayload *payload)
{
	// see IPC spec @ https://github.com/dotnet/diagnostics/blob/master/documentation/design-docs/ipc-protocol.md
	// for definition of serialization format

	// uint64_t ProcessId;  -> 8 bytes
	// GUID RuntimeCookie;  -> 16 bytes
	// LPCWSTR CommandLine; -> 4 bytes + strlen * sizeof(WCHAR)
	// LPCWSTR OS;          -> 4 bytes + strlen * sizeof(WCHAR)
	// LPCWSTR Arch;        -> 4 bytes + strlen * sizeof(WCHAR)

	EP_ASSERT (payload != NULL);

	size_t size = 0;
	size += sizeof(payload->process_id);
	size += sizeof(payload->runtime_cookie);

	size += sizeof(uint32_t);
	size += (payload->command_line != NULL) ?
		(ep_rt_utf16_string_len (payload->command_line) + 1) * sizeof(ep_char16_t) : 0;

	size += sizeof(uint32_t);
	size += (payload->os != NULL) ?
		(ep_rt_utf16_string_len (payload->os) + 1) * sizeof(ep_char16_t) : 0;

	size += sizeof(uint32_t);
	size += (payload->arch != NULL) ?
		(ep_rt_utf16_string_len (payload->arch) + 1) * sizeof(ep_char16_t) : 0;

	EP_ASSERT (size <= UINT16_MAX);
	return (uint16_t)size;
}

static
bool
process_info_payload_flatten (
	DiagnosticsProcessInfoPayload *payload,
	uint8_t **buffer,
	uint16_t *size)
{
	EP_ASSERT (payload != NULL);
	EP_ASSERT (buffer != NULL);
	EP_ASSERT (*buffer != NULL);
	EP_ASSERT (size != NULL);
	EP_ASSERT (process_info_payload_get_size (payload) == *size);

	// see IPC spec @ https://github.com/dotnet/diagnostics/blob/master/documentation/design-docs/ipc-protocol.md
	// for definition of serialization format

	bool success = true;

	// uint64_t ProcessId;
	memcpy (*buffer, &payload->process_id, sizeof (payload->process_id));
	*buffer += sizeof (payload->process_id);
	*size -= sizeof (payload->process_id);

	// GUID RuntimeCookie;
	memcpy(*buffer, &payload->runtime_cookie, sizeof (payload->runtime_cookie));
	*buffer += sizeof (payload->runtime_cookie);
	*size -= sizeof (payload->runtime_cookie);

	// LPCWSTR CommandLine;
	success &= ipc_message_try_write_string_utf16_t (buffer, size, payload->command_line);

	// LPCWSTR OS;
	if (success)
		success &= ipc_message_try_write_string_utf16_t (buffer, size, payload->os);

	// LPCWSTR Arch;
	if (success)
		success &= ipc_message_try_write_string_utf16_t (buffer, size, payload->arch);

	// Assert we've used the whole buffer we were given
	EP_ASSERT(*size == 0);

	return success;
}

DiagnosticsProcessInfoPayload *
ds_process_info_payload_init (
	DiagnosticsProcessInfoPayload *payload,
	const ep_char16_t *command_line,
	const ep_char16_t *os,
	const ep_char16_t *arch,
	uint32_t process_id,
	const uint8_t *runtime_cookie)
{
	ep_return_null_if_nok (payload != NULL);

	payload->command_line = command_line;
	payload->os = os;
	payload->arch = arch;
	payload->process_id = process_id;

	if (runtime_cookie)
		memcpy (&payload->runtime_cookie, runtime_cookie, EP_ACTIVITY_ID_SIZE);

	return payload;
}

void
ds_process_info_payload_fini (DiagnosticsProcessInfoPayload *payload)
{
	;
}

/*
 * DiagnosticsProcessProtocolHelper.
 */

void
ds_process_protocol_helper_get_process_info (
	DiagnosticsIpcMessage *message,
	IpcStream *stream)
{
	EP_ASSERT (message != NULL);
	EP_ASSERT (stream != NULL);

	ep_char16_t *command_line = NULL;
	ep_char16_t *os_info = NULL;
	ep_char16_t *arch_info = NULL;

	if (ep_rt_managed_command_line_get ())
		command_line = ep_rt_utf8_to_utf16_string (ep_rt_managed_command_line_get (), -1);

	// Checkout https://github.com/dotnet/coreclr/pull/24433 for more information about this fall back.
	if (!command_line)
		// Use the result from ep_rt_command_line_get() instead
		command_line = ep_rt_utf8_to_utf16_string (ep_rt_command_line_get (), -1);

	// get OS + Arch info
	os_info = ep_rt_utf8_to_utf16_string (ep_event_source_get_os_info (), -1);
	arch_info = ep_rt_utf8_to_utf16_string (ep_event_source_get_arch_info (), -1);

	DiagnosticsProcessInfoPayload payload;
	ds_process_info_payload_init (
		&payload,
		command_line,
		os_info,
		arch_info,
		ep_rt_current_process_get_id (),
		ds_ipc_advertise_cookie_v1_get ());

	ep_raise_error_if_nok (ipc_message_initialize_buffer (
		message,
		&_ds_ipc_generic_success_header,
		&payload,
		process_info_payload_get_size (&payload),
		process_info_payload_flatten) == true);

	ipc_message_send (message, stream);

ep_on_exit:
	ds_process_info_payload_fini (&payload);
	ep_rt_utf16_string_free (arch_info);
	ep_rt_utf16_string_free (os_info);
	ep_rt_utf16_string_free (command_line);
	ep_ipc_stream_free (stream);
	return;

ep_on_error:
	ds_ipc_message_send_error (stream, DS_IPC_E_FAIL);
	DS_LOG_WARNING_0 ("Failed to send DiagnosticsIPC response");
	ep_exit_error_handler ();
}

void
ds_process_protocol_helper_resume_runtime_startup (
	DiagnosticsIpcMessage *message,
	IpcStream *stream)
{
	EP_ASSERT (message != NULL);
	EP_ASSERT (stream != NULL);

	// no payload
	ds_server_resume_runtime_startup ();
	bool success = ds_ipc_message_send_success (stream, DS_IPC_S_OK);
	if (!success) {
		ds_ipc_message_send_error (stream, DS_IPC_E_FAIL);
		DS_LOG_WARNING_0 ("Failed to send DiagnosticsIPC response");
	}

	ep_ipc_stream_free (stream);
}

void
ds_process_protocol_helper_handle_ipc_message (
	DiagnosticsIpcMessage *message,
	IpcStream *stream)
{
	EP_ASSERT (message != NULL);
	EP_ASSERT (stream != NULL);

	switch ((DiagnosticsProcessCommandId)ds_ipc_header_get_commandid (ds_ipc_message_get_header_ref (message))) {
	case DS_PROCESS_COMMANDID_GET_PROCESS_INFO:
		ds_process_protocol_helper_get_process_info (message, stream);
		break;
	default:
		DS_LOG_WARNING_1 ("Received unknown request type (%d)\n", ds_ipc_message_header_get_commandset (ds_ipc_message_get_header (&message)));
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
