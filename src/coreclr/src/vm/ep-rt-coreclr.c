#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-rt-config.h"
#include "ep-types.h"
#include "ep-rt.h"

ep_rt_lock_handle_t _ep_rt_coreclr_config_lock_handle;
CrstStatic _ep_rt_coreclr_config_lock;

thread_local EventPipeCoreCLRThreadHolderTLS EventPipeCoreCLRThreadHolderTLS::g_threadHolderTLS;

ep_char8_t *_ep_rt_coreclr_diagnostics_cmd_line;

#ifndef TARGET_UNIX
uint32_t *_ep_rt_coreclr_proc_group_offsets;
#endif

#endif /* ENABLE_PERFTRACING */

