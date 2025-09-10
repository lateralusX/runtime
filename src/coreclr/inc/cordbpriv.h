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

    DBCF_USER_MASK                  = 0x00FF,
    DBCF_GENERATE_DEBUG_CODE        = 0x0001,
    DBCF_ALLOW_JIT_OPT              = 0x0008,
    DBCF_PROFILER_ENABLED           = 0x0020,
//    DBCF_ACTIVATE_REMOTE_DEBUGGING  = 0x0040,  Deprecated.  DO NOT USE

    DBCF_INTERNAL_MASK              = 0xFF00,
    DBCF_PENDING_ATTACH             = 0x0100,
    DBCF_ATTACHED                   = 0x0200,
    DBCF_FIBERMODE                  = 0x0400
};

//
// Flags used to control the debuggable state of modules and
// assemblies.
//
enum DebuggerAssemblyControlFlags
{
    DACF_NONE                       = 0x00,
    DACF_USER_OVERRIDE              = 0x01,
    DACF_ALLOW_JIT_OPTS             = 0x02,
    DACF_OBSOLETE_TRACK_JIT_INFO    = 0x04, // obsolete in V2.0, we're always tracking.
    DACF_ENC_ENABLED                = 0x08,
    DACF_IGNORE_PDBS                = 0x20,
    DACF_CONTROL_FLAGS_MASK         = 0x2F,

    DACF_PDBS_COPIED                = 0x10,
    DACF_MISC_FLAGS_MASK            = 0x10,
};

enum DebuggerSWBreakpointType
{
    DSWB_MIN = 0,
    DSWB_PRE_STUB = DSWB_MIN,
    DSWB_EXTERNAL_METHOD_FIXUP,
    DSWB_MULTICAST_DELEGATE,
    DSWB_MAX
};

struct DebuggerSWBreakpointArgs
{
public:
    DebuggerSWBreakpointArgs(DebuggerSWBreakpointType type) { this->type = type; }
    DebuggerSWBreakpointType type;
};

template<typename T1>
struct DebuggerSWBreakpointArgsT1 : public DebuggerSWBreakpointArgs
{
public:
    DebuggerSWBreakpointArgsT1(DebuggerSWBreakpointType type, T1 arg1)
        : DebuggerSWBreakpointArgs(type), arg1(arg1) {}
    T1 arg1;
};

template<typename T1, typename T2>
struct DebuggerSWBreakpointArgsT2 : public DebuggerSWBreakpointArgsT1<T1>
{
public:
    DebuggerSWBreakpointArgsT2(DebuggerSWBreakpointType type, T1 arg1, T2 arg2)
        : DebuggerSWBreakpointArgsT1(type, arg1), arg2(arg2) {}
    T2 arg2;
};

#ifndef DACCESS_COMPILE

GARY_DECL(DWORD, g_debuggerSWBreakpoints, DSWB_MAX);

class DebuggerSWBreakpointHelpers
{
public:

    static const char * ToString(DebuggerSWBreakpointType type)
    {
        switch (type)
        {
            case DSWB_PRE_STUB: return "DSWB_PRE_STUB";
            case DSWB_EXTERNAL_METHOD_FIXUP: return "DSWB_EXTERNAL_METHOD_FIXUP";
            case DSWB_MULTICAST_DELEGATE: return "DSWB_MULTICAST_DELEGATE";
            default: return "Unknown DebuggerSWBreakpointType";
        }
    }

    static DWORD_PTR GetSWBreakpointAddress(DebuggerSWBreakpointType type)
    {
        _ASSERTE(type >= 0 && type < DSWB_MAX);
        return (DWORD_PTR)(g_debuggerSWBreakpoints + type);
    }

    static bool IsEnabled(DebuggerSWBreakpointType type)
    {
        return *(PTR_DWORD)(g_debuggerSWBreakpoints + type) != 0;
    }
};

#endif // !DACCESS_COMPILE

#endif /* _cordbpriv_h_ */
