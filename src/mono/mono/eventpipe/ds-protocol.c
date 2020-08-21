#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-rt-config.h"
#if !defined(EP_INCLUDE_SOURCE_FILES) || defined(EP_FORCE_INCLUDE_SOURCE_FILES)

#define DS_IMPL_PROTOCOL_GETTER_SETTER
#include "ds-protocol.h"
#include "ep-stream.h"

#if BIGENDIAN
#define DS_VAL16(x)    (((x) >> 8) | ((x) << 8))
#define DS_VAL32(y)    (((y) >> 24) | (((y) >> 8) & 0x0000FF00L) | (((y) & 0x0000FF00L) << 8) | ((y) << 24))
#define DS_VAL64(z)    (((uint64_t)DS_VAL32(z) << 32) | DS_VAL32((z) >> 32))
#else
#define DS_VAL16(x) x
#define DS_VAL32(x) x
#define DS_VAL64(x) x
#endif // BIGENDIAN

#define DOTNET_IPC_V1_ADVERTISE_MAGIC "ADVR_V1"
#define DOTNET_IPC_V1_ADVERTISE_SIZE 34

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
ipc_message_try_parse (
	DiagnosticsIpcMessage *message,
	IpcStream *stream);

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

static
bool
ds_icp_advertise_v1_send (IpcStream *stream)
{
	uint8_t advertise_buffer [DOTNET_IPC_V1_ADVERTISE_SIZE];
	uint8_t *cookie = ipc_advertise_cookie_v1_get ();
	uint64_t pid = DS_VAL64 (ep_rt_current_process_get_id ());
	uint64_t *buffer = (uint64_t *)advertise_buffer;
	bool result = false;

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

DiagnosticsIpcMessage *
ds_ipc_message_init (DiagnosticsIpcMessage *message)
{
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


// Initialize an incoming IpcMessage from a stream by parsing
// the header and payload.
//
// If either fail, this returns false, true otherwise
bool
ds_ipc_message_initialize (
	DiagnosticsIpcMessage *message,
	IpcStream *stream)
{
	return ipc_message_try_parse (message, stream);
}

bool
ds_ipc_message_initialize_error (
	DiagnosticsIpcMessage *message,
	uint32_t error
)
{
	return ds_ipc_message_initialize_header_uint32_t_payload (message, &_ds_ipc_generic_error_header, error);
}

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

bool
ds_ipc_message_initialize_header_uint32_t_payload (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcHeader *header,
	uint32_t payload)
{
	EP_ASSERT (message);
	EP_ASSERT (header);

	message->header = *header;
	return ipc_message_flatten_blitable_type (message, (uint8_t *)&payload, sizeof (payload));
}

// Send an Error message across the pipe.
// Will return false on failure of any step (init or send).
// Regardless of success of this function, the spec
// dictates that the connection be closed on error,
// so the user is expected to delete the IpcStream
// after handling error cases.
bool
ds_ipc_message_send_error_message (
	IpcStream *stream,
	uint32_t error)
{
	ep_return_false_if_nok (stream != NULL);

	DiagnosticsIpcMessage error_message;
	ds_ipc_message_init (&error_message);
	bool success = ds_ipc_message_initialize_error (&error_message, error);
	if (success)
		ds_ipc_message_send (&error_message, stream);
	ds_ipc_message_fini (&error_message);
	return success;
}

bool
ds_ipc_message_send_success_message (
	IpcStream *stream,
	uint32_t code)
{
	ep_return_false_if_nok (stream != NULL);

	DiagnosticsIpcMessage success_message;
	ds_ipc_message_init (&success_message);
	bool success = ds_ipc_message_initialize_header_uint32_t_payload (&success_message, &_ds_ipc_generic_success_header, code);
	if (success)
		ds_ipc_message_send (&success_message, stream);
	ds_ipc_message_fini (&success_message);
	return success;
}

bool
ds_ipc_message_send (
	DiagnosticsIpcMessage *message,
	IpcStream *stream
)
{
	ep_return_false_if_nok (message != NULL && stream != NULL);

	EP_ASSERT (message->data != NULL);

	uint32_t bytes_written;
	bool success = ep_ipc_stream_write (stream, message->data, message->size, &bytes_written, EP_INFINITE_WAIT);
	return (bytes_written == message->size) && success;
}

#endif /* !defined(EP_INCLUDE_SOURCE_FILES) || defined(EP_FORCE_INCLUDE_SOURCE_FILES) */
#endif /* ENABLE_PERFTRACING */

#ifndef EP_INCLUDE_SOURCE_FILES
extern const char quiet_linker_empty_file_warning_diagnostics_protocol;
const char quiet_linker_empty_file_warning_diagnostics_protocol = 0;
#endif
