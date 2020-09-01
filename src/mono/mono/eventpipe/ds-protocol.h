#ifndef __DIAGNOSTICS_PROTOCOL_H__
#define __DIAGNOSTICS_PROTOCOL_H__

#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ds-rt-config.h"
#include "ds-types.h"
#include "ds-ipc.h"

#undef DS_IMPL_GETTER_SETTER
#ifdef DS_IMPL_PROTOCOL_GETTER_SETTER
#define DS_IMPL_GETTER_SETTER
#endif
#include "ds-getter-setter.h"

/*
* DiagnosticsIpc
*/

uint8_t *
ds_ipc_advertise_cookie_v1_get (void);

void
ds_ipc_advertise_cookie_v1_init (void);

bool
ds_icp_advertise_v1_send (DiagnosticsIpcStream *stream);

/*
* DiagnosticsIpcHeader
*/

// The header to be associated with every command and response
// to/from the diagnostics server
#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsIpcHeader {
#else
struct _DiagnosticsIpcHeader_Internal {
#endif
	// Magic Version number; a 0 terminated char array
	uint8_t magic [14];
	// The size of the incoming packet, size = header + payload size
	uint16_t size;
	// The scope of the Command.
	uint8_t commandset;
	// The command being sent
	uint8_t commandid;
	// reserved for future use
	uint16_t reserved;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsIpcHeader {
	uint8_t _internal [sizeof (struct _DiagnosticsIpcHeader_Internal)];
};
#endif

DS_DEFINE_GETTER_ARRAY_REF(DiagnosticsIpcHeader *, ipc_header, uint8_t *, const uint8_t *, magic, magic[0])
DS_DEFINE_GETTER(DiagnosticsIpcHeader *, ipc_header, uint8_t, commandset)
DS_DEFINE_GETTER(DiagnosticsIpcHeader *, ipc_header, uint8_t, commandid)

/*
* DiagnosticsIpcMessage
*/

#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsIpcMessage {
#else
struct _DiagnosticsIpcMessage_Internal {
#endif
	// header associated with this message
	DiagnosticsIpcHeader header;
	// Pointer to flattened buffer filled with:
	// incoming message: payload (could be empty which would be NULL)
	// outgoing message: header + payload
	uint8_t *data;
	// The total size of the message (header + payload)
	uint16_t size;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsIpcMessage {
	uint8_t _internal [sizeof (struct _DiagnosticsIpcMessage_Internal)];
};
#endif

DS_DEFINE_GETTER_REF(DiagnosticsIpcMessage *, ipc_message, DiagnosticsIpcHeader *, header)

DiagnosticsIpcMessage *
ds_ipc_message_init (DiagnosticsIpcMessage *message);

void
ds_ipc_message_fini (DiagnosticsIpcMessage *message);

// Initialize an incoming IpcMessage from a stream by parsing
// the header and payload.
//
// If either fail, this returns false, true otherwise
bool
ds_ipc_message_initialize_stream (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcStream *stream);

// Send an Error message across the pipe.
// Will return false on failure of any step (init or send).
// Regardless of success of this function, the spec
// dictates that the connection be closed on error,
// so the user is expected to delete the IpcStream
// after handling error cases.
bool
ds_ipc_message_send_error (
	DiagnosticsIpcStream *stream,
	uint32_t error);

bool
ds_ipc_message_send_success (
	DiagnosticsIpcStream *stream,
	uint32_t code);

/*
* EventPipeCollectTracingCommandPayload
*/

// Command = 0x0202
#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _EventPipeCollectTracingCommandPayload {
#else
struct _EventPipeCollectTracingCommandPayload_Internal {
#endif
	// The protocol buffer is defined as:
	// X, Y, Z means encode bytes for X followed by bytes for Y followed by bytes for Z
	// message = uint circularBufferMB, uint format, array<provider_config> providers
	// uint = 4 little endian bytes
	// wchar = 2 little endian bytes, UTF16 encoding
	// array<T> = uint length, length # of Ts
	// string = (array<char> where the last char must = 0) or (length = 0)
	// provider_config = ulong keywords, uint logLevel, string provider_name, string filter_data

	uint8_t *incoming_buffer;
	ep_rt_provider_config_array_t provider_configs;
	uint32_t circular_buffer_size_in_mb;
	EventPipeSerializationFormat serialization_format;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _EventPipeCollectTracingCommandPayload {
	uint8_t _internal [sizeof (struct _EventPipeCollectTracingCommandPayload_Internal)];
};
#endif

EventPipeCollectTracingCommandPayload *
ep_collect_tracing_command_payload_alloc (void);

void
ep_collect_tracing_command_payload_free (EventPipeCollectTracingCommandPayload *payload);

/*
* EventPipeCollectTracing2CommandPayload
*/

// Command = 0x0202
#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _EventPipeCollectTracing2CommandPayload {
#else
struct _EventPipeCollectTracing2CommandPayload_Internal {
#endif
	// The protocol buffer is defined as:
	// X, Y, Z means encode bytes for X followed by bytes for Y followed by bytes for Z
	// message = uint circularBufferMB, uint format, array<provider_config> providers
	// uint = 4 little endian bytes
	// wchar = 2 little endian bytes, UTF16 encoding
	// array<T> = uint length, length # of Ts
	// string = (array<char> where the last char must = 0) or (length = 0)
	// provider_config = ulong keywords, uint logLevel, string provider_name, string filter_data

	uint8_t *incoming_buffer;
	ep_rt_provider_config_array_t provider_configs;
	uint32_t circular_buffer_size_in_mb;
	EventPipeSerializationFormat serialization_format;
	bool rundown_requested;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _EventPipeCollectTracing2CommandPayload {
	uint8_t _internal [sizeof (struct _EventPipeCollectTracing2CommandPayload_Internal)];
};
#endif

EventPipeCollectTracing2CommandPayload *
ep_collect_tracing2_command_payload_alloc (void);

void
ep_collect_tracing2_command_payload_free (EventPipeCollectTracing2CommandPayload *payload);

/*
* EventPipeStopTracingCommandPayload
*/

// Command = 0x0201
#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _EventPipeStopTracingCommandPayload {
#else
struct _EventPipeStopTracingCommandPayload_Internal {
#endif
	EventPipeSessionID session_id;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _EventPipeStopTracingCommandPayload {
	uint8_t _internal [sizeof (struct _EventPipeStopTracingCommandPayload_Internal)];
};
#endif

void
ep_stop_tracing_command_payload_free (EventPipeStopTracingCommandPayload *payload);

/*
* DiagnosticsProcessInfoPayload
*/

// command = 0x0400
#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsProcessInfoPayload {
#else
struct _DiagnosticsProcessInfoPayload_Internal {
#endif
	// The protocol buffer is defined as:
	// X, Y, Z means encode bytes for X followed by bytes for Y followed by bytes for Z
	// uint = 4 little endian bytes
	// long = 8 little endian bytes
	// GUID = 16 little endian bytes
	// wchar = 2 little endian bytes, UTF16 encoding
	// array<T> = uint length, length # of Ts
	// string = (array<char> where the last char must = 0) or (length = 0)

	// ProcessInfo = long pid, string cmdline, string OS, string arch, GUID runtimeCookie
	uint64_t process_id;
	const ep_char16_t *command_line;
	const ep_char16_t *os;
	const ep_char16_t *arch;
	uint8_t runtime_cookie [EP_ACTIVITY_ID_SIZE];
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsProcessInfoPayload {
	uint8_t _internal [sizeof (struct _DiagnosticsProcessInfoPayload_Internal)];
};
#endif

DiagnosticsProcessInfoPayload *
ds_process_info_payload_init (
	DiagnosticsProcessInfoPayload *payload,
	const ep_char16_t *command_line,
	const ep_char16_t *os,
	const ep_char16_t *arch,
	uint32_t process_id,
	const uint8_t *runtime_cookie);

void
ds_process_info_payload_fini (DiagnosticsProcessInfoPayload *payload);

/*
* EventPipeProtocolHelper
*/

void
ep_protocol_helper_handle_ipc_message (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcStream *stream);

/*
 * DiagnosticsProcessProtocolHelper.
 */

void
ds_process_protocol_helper_handle_ipc_message (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcStream *stream);

void
ds_process_protocol_helper_get_process_info (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcStream *stream);

void
ds_process_protocol_helper_resume_runtime_startup (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcStream *stream);


#endif /* ENABLE_PERFTRACING */
#endif /** __DIAGNOSTICS_PROTOCOL_H__ **/
