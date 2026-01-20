#define DRIVER_EXTERNAL_TESTS
#define DRIVER_NAME "eventpipe-tests"

#include "minipal/time.h"

static double __frequency = 0.0;

double get_timestamp (void)
{
	if (__frequency == 0.0)
		__frequency = (double)minipal_hires_tick_frequency () / (double)1000.0;
	return (double)minipal_hires_ticks () / __frequency;
}

#include "ep-tests.h"
#include "mono/eglib/test/driver.c"
