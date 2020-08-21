#ifndef __DIAGNOSTICS_TYPES_H__
#define __DIAGNOSTICS_TYPES_H__

#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-types.h"

/*
 * Diagnostic Structs.
 */

typedef struct _DiagnosticsIpc DiagnosticsIpc;
typedef struct _DiagnosticsIpcHeader DiagnosticsIpcHeader;
typedef struct _DiagnosticsIpcMessage DiagnosticsIpcMessage;
typedef struct _DiagnosticsIpcPollHandle DiagnosticsIpcPollHandle;
typedef struct _EventPipeCollectTracingCommandPayload EventPipeCollectTracingCommandPayload;
typedef struct _EventPipeCollectTracing2CommandPayload EventPipeCollectTracing2CommandPayload;
typedef struct _EventPipeStopTracingCommandPayload EventPipeStopTracingCommandPayload;
typedef struct _IpcStreamFactoryConnectionState IpcStreamFactoryConnectionState;

/*
 * Diagnostics Enums.
 */

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
} DiagnosticsServerCommandSet;

// Overlaps with DiagnosticsServerResponseId
// DON'T create overlapping values
typedef enum {
	// 0x00 used in DiagnosticsServerResponseId
	DS_SERVER_COMMANDID_RESUME_RUNTIME = 0x01,
	// 0xFF used DiagnosticsServerResponseId
} DiagnosticsServerCommandId;

// Overlaps with DiagnosticsServerCommandId
// DON'T create overlapping values
typedef enum {
	DS_SERVER_RESPONSEID_OK = 0x00,
	DS_SERVER_RESPONSEID_ERROR = 0xFF,
} DiagnosticsServerResponseId;

// The event pipe command set is 0x02
// see ds-ipc.h and ds-server.h for more details
typedef enum {
	EP_COMMANDID_STOP_TRACING = 0x01,
	EP_COMMANDID_COLLECT_TRACING  = 0x02,
	EP_COMMANDID_COLLECT_TRACING_2 = 0x03,
	// future
} EventPipeCommandId;

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

#define DOTNET_IPC_V1_MAGIC "DOTNET_IPC_V1"
#define DOTNET_IPC_V1_ADVERTISE_MAGIC "ADVR_V1"
#define DOTNET_IPC_V1_ADVERTISE_SIZE 34

#if BIGENDIAN
#define DS_VAL16(x)    (((x) >> 8) | ((x) << 8))
#define DS_VAL32(y)    (((y) >> 24) | (((y) >> 8) & 0x0000FF00L) | (((y) & 0x0000FF00L) << 8) | ((y) << 24))
#define DS_VAL64(z)    (((uint64_t)DS_VAL32(z) << 32) | DS_VAL32((z) >> 32))
#else
#define DS_VAL16(x) x
#define DS_VAL32(x) x
#define DS_VAL64(x) x
#endif // BIGENDIAN

//TODO: Convert into hex.
#define DS_IPC_S_OK ((uint32_t)0L)
#define DS_IPC_E_BAD_ENCODING (((uint32_t)(1)<<31) | ((uint32_t)(19)<<16) | ((uint32_t)(0x1384)))
#define DS_IPC_E_UNKNOWN_COMMAND (((uint32_t)(1)<<31) | ((uint32_t)(19)<<16) | ((uint32_t)(0x1385)))
#define DS_IPC_E_UNKNOWN_MAGIC (((uint32_t)(1)<<31) | ((uint32_t)(19)<<16) | ((uint32_t)(0x1386)))
#define DS_IPC_E_FAIL (uint32_t)(0x80004005L)

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

#endif /* ENABLE_PERFTRACING */
#endif /* __DIAGNOSTICS_TYPES_H__ */
