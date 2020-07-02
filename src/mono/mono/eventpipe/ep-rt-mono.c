#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-rt-config.h"
#include "ep-types.h"
#include "ep-rt.h"
#include <mono/utils/mono-lazy-init.h>

ep_rt_spin_lock_handle_t _ep_rt_mono_config_lock = {0};
EventPipeMonoFuncTable _ep_rt_mono_func_table = {0};

mono_lazy_init_t _ep_rt_mono_os_cmd_line_init = MONO_LAZY_INIT_STATUS_NOT_INITIALIZED;
char *_ep_rt_mono_os_cmd_line = NULL;

mono_lazy_init_t _ep_rt_mono_managed_cmd_line_init = MONO_LAZY_INIT_STATUS_NOT_INITIALIZED;
char *_ep_rt_mono_managed_cmd_line = NULL;

#endif /* ENABLE_PERFTRACING */

extern const char quiet_linker_empty_file_warning_eventpipe_rt_mono;
const char quiet_linker_empty_file_warning_eventpipe_rt_mono = 0;
