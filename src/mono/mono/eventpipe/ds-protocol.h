#ifndef __DIAGNOSTICS_PROTOCOL_H__
#define __DIAGNOSTICS_PROTOCOL_H__

#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-rt-config.h"
#include "ep-rt.h"
#include "ep-stream.h"

#undef EP_IMPL_GETTER_SETTER
#ifdef DS_IMPL_PROTOCOL_GETTER_SETTER
#define EP_IMPL_GETTER_SETTER
#endif
#include "ep-getter-setter.h"

#if BIGENDIAN
#define DS_VAL16(x)    (((x) >> 8) | ((x) << 8))
#define DS_VAL32(y)    (((y) >> 24) | (((y) >> 8) & 0x0000FF00L) | (((y) & 0x0000FF00L) << 8) | ((y) << 24))
#define DS_VAL64(z)    (((uint64_t)DS_VAL32(z) << 32) | DS_VAL32((z) >> 32))
#else
#define DS_VAL16(x) x
#define DS_VAL32(x) x
#define DS_VAL64(x) x
#endif // BIGENDIAN

#define DOTNET_IPC_V1_MAGIC "DOTNET_IPC_V1"
#define DOTNET_IPC_V1_ADVERTISE_MAGIC "ADVR_V1"
#define DOTNET_IPC_V1_ADVERTISE_SIZE 34

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
#else
struct _DiagnosticsIpcHeader_Internal {
#endif

#if !defined(EP_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsIpcHeader {
	uint8_t _internal [sizeof (struct _DiagnosticsIpcHeader_Internal)];
};
#endif

typedef struct _DiagnosticsIpcHeader DiagnosticsIpcHeader;

static const DiagnosticsIpcHeader ds_ipc_generic_success_header = {
	{ DOTNET_IPC_V1_MAGIC },
	(uint16_t)sizeof (DiagnosticsIpcHeader),
	(uint8_t)DS_SERVER_COMMANDSET_SERVER,
	(uint8_t)DS_SERVER_RESPONSEID_OK,
	(uint16_t)0x0000
};

static const DiagnosticsIpcHeader ds_ipc_generic_error_header = {
	{ DOTNET_IPC_V1_MAGIC },
	(uint16_t)sizeof (DiagnosticsIpcHeader),
	(uint8_t)DS_SERVER_COMMANDSET_SERVER,
	(uint8_t)DS_SERVER_RESPONSEID_ERROR,
	(uint16_t)0x0000
};

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
inline
uint8_t *
ds_ipc_advertise_cookie_v1_get (void)
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
inline
bool
ds_icp_advertise_v1_send (IpcStream *stream)
{
	uint8_t advertise_buffer [DOTNET_IPC_V1_ADVERTISE_SIZE];
	uint8_t *cookie = ds_ipc_advertise_cookie_v1_get ();
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

#if defined(EP_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsIpcMessage {
	// header associated with this message
	DiagnosticsIpcHeader header;
	// Pointer to flattened buffer filled with:
	// incoming message: payload (could be empty which would be NULL)
	// outgoing message: header + payload
	uint8_t *data;
	// The total size of the message (header + payload)
	uint16_t size;
};
#else
struct _DiagnosticsIpcMessage_Internal {
#endif

#if !defined(EP_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsIpcMessage {
	uint8_t _internal [sizeof (struct _DiagnosticsIpcMessage_Internal)];
};
#endif

typedef struct _DiagnosticsIpcMessage DiagnosticsIpcMessage;

#endif /* ENABLE_PERFTRACING */
#endif /** __DIAGNOSTICS_PROTOCOL_H__ **/
