#ifndef __DIAGNOSTICS_RT_H__
#define __DIAGNOSTICS_RT_H__

#include "ep-rt.h"

#define DS_LOG_ALWAYS_0(msg) ds_rt_redefine
#define DS_LOG_ALWAYS_1(msg, data1) ds_rt_redefine
#define DS_LOG_ALWAYS_2(msg, data1, data2) ds_rt_redefine
#define DS_LOG_INFO_0(msg) ds_rt_redefine
#define DS_LOG_INFO_1(msg, data1) ds_rt_redefine
#define DS_LOG_INFO_2(msg, data1, data2) ds_rt_redefine
#define DS_LOG_ERROR_0(msg) ds_rt_redefine
#define DS_LOG_ERROR_1(msg, data1) ds_rt_redefine
#define DS_LOG_ERROR_2(msg, data1, data2) ds_rt_redefine
#define DS_LOG_WARNING_0(msg) ds_rt_redefine
#define DS_LOG_WARNING_1(msg, data1) ds_rt_redefine
#define DS_LOG_WARNING_2(msg, data1, data2) ds_rt_redefine

static
bool
ds_rt_config_value_get_enable (void);

static
ep_char8_t *
ds_rt_config_value_get_monitor_address (void);

static
bool
ds_rt_config_value_get_diagnostics_monitor_pause_on_start (void);

#include "ds-rt-mono.h"

#endif /* __DIAGNOSTICS_RT_H__ */
