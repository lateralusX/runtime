// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

/* ------------------------------------------------------------------------- *
 * cordbpriv.h -- header file for private Debugger data shared by various
 *                Runtime components.
 * ------------------------------------------------------------------------- */

#ifndef _cordbpriv_h_
#define _cordbpriv_h_

#include "corhdr.h"
#include <unknwn.h>

//
// Initial value for EnC versions
//
#define CorDB_DEFAULT_ENC_FUNCTION_VERSION    1
#define CorDB_UNKNOWN_ENC_FUNCTION_VERSION    ((SIZE_T)(-1))

enum DebuggerLaunchSetting
{
    DLS_ASK_USER          = 0,
    DLS_ATTACH_DEBUGGER   = 1
};


//
// Flags used to control the Runtime's debugging modes. These indicate to
// the Runtime that it needs to load the Runtime Controller, track data
// during JIT's, etc.
//
enum DebuggerControlFlag
{
    DBCF_NORMAL_OPERATION           = 0x0000,
    DBCF_PENDING_ATTACH             = 0x0100, // [cDAC] [Debugger] : Contract depends on this value.
    DBCF_ATTACHED                   = 0x0200, // [cDAC] [Debugger] : Contract depends on this value.
};

//
// Flags used to control the debuggable state of modules and
// assemblies.
//
// [cDAC] [Loader]: Contract depends on DACF_NONE, DACF_ALLOW_JIT_OPTS, DACF_ENC_ENABLED, DACF_IGNORE_PDBS, DACF_CONTROL_FLAGS_MASK.
enum DebuggerAssemblyControlFlags
{
    DACF_NONE                       = 0x00,
    DACF_ALLOW_JIT_OPTS             = 0x02,
    DACF_OBSOLETE_TRACK_JIT_INFO    = 0x04, // obsolete in V2.0, we're always tracking.
    DACF_ENC_ENABLED                = 0x08,
    DACF_IGNORE_PDBS                = 0x20,
    DACF_CONTROL_FLAGS_MASK         = 0x2E,

    DACF_PDBS_COPIED                = 0x10,
    DACF_MISC_FLAGS_MASK            = 0x10,
};

enum DebuggerTracepointType
{
    DEBUGGER_TRACEPOINT_MIN = 0,
    DEBUGGER_TRACEPOINT_PRE_STUB = DEBUGGER_TRACEPOINT_MIN,
    DEBUGGER_TRACEPOINT_EXTERNAL_METHOD_FIXUP,
    DEBUGGER_TRACEPOINT_MULTICAST_DELEGATE,
    DEBUGGER_TRACEPOINT_PINVOKE_IMPORT,
    DEBUGGER_TRACEPOINT_MAX
};

struct DebuggerTracepointArgs
{
    DebuggerTracepointArgs(DebuggerTracepointType type) : type(type) {}
    DebuggerTracepointType type;
};

template<typename T1>
struct DebuggerTracepointArgsT1 : public DebuggerTracepointArgs
{
    DebuggerTracepointArgsT1(DebuggerTracepointType type, T1 arg1)
        : DebuggerTracepointArgs(type), arg1(arg1) {}
    T1 arg1;
};

template<typename T1, typename T2>
struct DebuggerTracepointArgsT2 : public DebuggerTracepointArgsT1<T1>
{
    DebuggerTracepointArgsT2(DebuggerTracepointType type, T1 arg1, T2 arg2)
        : DebuggerTracepointArgsT1<T1>(type, arg1), arg2(arg2) {}
    T2 arg2;
};

#if defined(DEBUGGING_SUPPORTED) && !defined(DACCESS_COMPILE)
GARY_DECL(DWORD, g_debuggerTracepointCounters, DEBUGGER_TRACEPOINT_MAX);
#endif // DEBUGGING_SUPPORTED && !DACCESS_COMPILE

class DebuggerTracepointHelpers
{
public:
    static const char* ToString(DebuggerTracepointType type)
    {
        LIMITED_METHOD_CONTRACT;
        switch (type)
        {
            case DEBUGGER_TRACEPOINT_PRE_STUB: return "DEBUGGER_TRACEPOINT_PRE_STUB";
            case DEBUGGER_TRACEPOINT_EXTERNAL_METHOD_FIXUP: return "DEBUGGER_TRACEPOINT_EXTERNAL_METHOD_FIXUP";
            case DEBUGGER_TRACEPOINT_MULTICAST_DELEGATE: return "DEBUGGER_TRACEPOINT_MULTICAST_DELEGATE";
            case DEBUGGER_TRACEPOINT_PINVOKE_IMPORT: return "DEBUGGER_TRACEPOINT_PINVOKE_IMPORT";
            default: return "Unknown DebuggerTracepointType";
        }
    }

#if defined(DEBUGGING_SUPPORTED) && !defined(DACCESS_COMPILE)
    static DWORD* GetCounterAddress(DebuggerTracepointType type)
    {
        LIMITED_METHOD_CONTRACT;
        _ASSERTE(type >= DEBUGGER_TRACEPOINT_MIN && type < DEBUGGER_TRACEPOINT_MAX);
        return g_debuggerTracepointCounters + type;
    }

    static bool IsEnabled(DebuggerTracepointType type)
    {
        LIMITED_METHOD_CONTRACT;
        return VolatileLoad(GetCounterAddress(type)) != 0;
    }
#endif // DEBUGGING_SUPPORTED && !DACCESS_COMPILE
};

#endif /* _cordbpriv_h_ */
