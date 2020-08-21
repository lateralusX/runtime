#ifndef __DIAGNOSTICS_PROTOCOL_H__
#define __DIAGNOSTICS_PROTOCOL_H__

#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-rt-config.h"
#include "ep-rt.h"

#undef EP_IMPL_GETTER_SETTER
#ifdef DS_IMPL_PROTOCOL_GETTER_SETTER
#define EP_IMPL_GETTER_SETTER
#endif
#define EP_GETTER_SETTER_PREFIX ds_
#include "ep-getter-setter.h"

#define DOTNET_IPC_V1_MAGIC "DOTNET_IPC_V1"

typedef enum {
	DS_IPC_MAGIC_VERSION_DOTNET_IPC_V1 = 0x01,
	// FUTURE
} DiagnosticsIpcMagicVersion;

typedef enum {
	// reserved   = 0x00,
	DS_SERVER_COMMANDSET_DUMP = 0x01,
	DS_SERVER_COMMANDSET_EVENTPIPE = 0x02,
	DS_SERVER_COMMANDSET_PROFILER = 0x03,
	DS_SERVER_COMMANDSET_SERVER = 0xFF
} DiagnosticServerCommandSet;

// Overlaps with DiagnosticServerResponseId
// DON'T create overlapping values
typedef enum {
	// 0x00 used in DiagnosticServerResponseId
	DS_SERVER_COMMANDID_RESUME_RUNTIME = 0x01,
	// 0xFF used DiagnosticServerResponseId
} DiagnosticServerCommandId;

// Overlaps with DiagnosticServerCommandId
// DON'T create overlapping values
typedef enum {
	DS_SERVER_RESPONSEID_OK = 0x00,
	DS_SERVER_RESPONSEID_ERROR = 0xFF,
} DiagnosticServerResponseId;

// The header to be associated with every command and response
// to/from the diagnostics server
#if defined(EP_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
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

#if !defined(EP_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsIpcHeader {
	uint8_t _internal [sizeof (struct _DiagnosticsIpcHeader_Internal)];
};
#endif

typedef struct _DiagnosticsIpcHeader DiagnosticsIpcHeader;

EP_DEFINE_GETTER_ARRAY_REF(DiagnosticsIpcHeader *, ipc_header, uint8_t *, const uint8_t *, magic, magic[0])
EP_DEFINE_GETTER(DiagnosticsIpcHeader *, ipc_header, uint8_t, commandset)
EP_DEFINE_GETTER(DiagnosticsIpcHeader *, ipc_header, uint8_t, commandid)

#if defined(EP_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
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

#if !defined(EP_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsIpcMessage {
	uint8_t _internal [sizeof (struct _DiagnosticsIpcMessage_Internal)];
};
#endif

typedef struct _DiagnosticsIpcMessage DiagnosticsIpcMessage;

EP_DEFINE_GETTER_REF(DiagnosticsIpcMessage *, ipc_message, DiagnosticsIpcHeader *, header)

DiagnosticsIpcMessage *
ds_ipc_message_init (DiagnosticsIpcMessage *message);

void
ds_ipc_message_fini (DiagnosticsIpcMessage *message);

bool
ds_ipc_message_initialize (
	DiagnosticsIpcMessage *message,
	IpcStream *stream);

bool
ds_ipc_message_initialize_error (
	DiagnosticsIpcMessage *message,
	uint32_t error);

bool
ds_ipc_message_initialize_header_uint32_t_payload (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcHeader *header,
	uint32_t payload);

bool
ds_ipc_message_send_error_message (
	IpcStream *stream,
	uint32_t error);

bool
ds_ipc_message_send_success_message (
	IpcStream *stream,
	uint32_t code);

bool
ds_ipc_message_send (
	DiagnosticsIpcMessage *message,
	IpcStream *stream
);

#undef EP_GETTER_SETTER_PREFIX

#endif /* ENABLE_PERFTRACING */
#endif /** __DIAGNOSTICS_PROTOCOL_H__ **/
