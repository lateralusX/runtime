#include "mono/eventpipe/ep.h"
#include "eglib/test/test.h"

#ifdef _CRTDBG_MAP_ALLOC
static _CrtMemState eventpipe_memory_start_snapshot;
static _CrtMemState eventpipe_memory_end_snapshot;
static _CrtMemState eventpipe_memory_diff_snapshot;
#endif

static RESULT
test_rt_setup (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&eventpipe_memory_start_snapshot);
#endif
	return NULL;
}

static RESULT
test_rt_perf_frequency (void)
{
	return (ep_perf_frequency_query () > 0) ? NULL : FAILED ("Frequency to low");
}

static RESULT
test_rt_perf_timestamp (void)
{
	RESULT result = NULL;
	uint32_t test_location = 0;
	int64_t frequency = 0;
	double elapsed_time_ms = 0;

	ep_timestamp_t start = ep_perf_timestamp_get ();
	g_usleep (10 * 1000);
	ep_timestamp_t stop = ep_perf_timestamp_get ();

	test_location = 1;

	ep_raise_error_if_nok (stop > start);

	test_location = 2;

	frequency = ep_perf_frequency_query ();
	ep_raise_error_if_nok (frequency > 0);

	test_location = 3;

	elapsed_time_ms = ((double)(stop - start) / (double)frequency) * 1000;

	ep_raise_error_if_nok (elapsed_time_ms > 0);

	test_location = 4;

	ep_raise_error_if_nok (elapsed_time_ms > 10);

ep_on_exit:
	return result;

ep_on_error:
	if (!result)
		result = FAILED ("Failed at test location=%i", test_location);
	ep_exit_error_handler ();
}

static RESULT
test_rt_system_time (void)
{
	RESULT result = NULL;
	uint32_t test_location = 0;
	EventPipeSystemTime time1;
	EventPipeSystemTime time2;
	bool time_diff = false;

	ep_system_time_get (&time1);

	ep_raise_error_if_nok (time1.year > 1600 && time1.year < 30828);
	test_location = 1;

	ep_raise_error_if_nok (time1.month > 0 && time1.month < 13);
	test_location = 2;

	ep_raise_error_if_nok (time1.day > 0 && time1.day < 32);
	test_location = 3;

	ep_raise_error_if_nok (time1.day_of_week >= 0 && time1.day_of_week < 7);
	test_location = 4;

	ep_raise_error_if_nok (time1.hour >= 0 && time1.hour < 24);
	test_location = 5;

	ep_raise_error_if_nok (time1.minute >= 0 && time1.minute < 60);
	test_location = 6;

	ep_raise_error_if_nok (time1.second >= 0 && time1.second < 60);
	test_location = 7;

	ep_raise_error_if_nok (time1.milliseconds >= 0 && time1.milliseconds < 1000);
	test_location = 8;

	g_usleep (1000 * 1000);

	ep_system_time_get (&time2);

	time_diff |= time1.year != time2.year;
	time_diff |= time1.month != time2.month;
	time_diff |= time1.day != time2.day;
	time_diff |= time1.day_of_week != time2.day_of_week;
	time_diff |= time1.hour != time2.hour;
	time_diff |= time1.minute != time2.minute;
	time_diff |= time1.second != time2.second;
	time_diff |= time1.milliseconds != time2.milliseconds;

	ep_raise_error_if_nok (time_diff == true);

ep_on_exit:
	return result;

ep_on_error:
	if (!result)
		result = FAILED ("Failed at test location=%i", test_location);
	ep_exit_error_handler ();
}

static RESULT
test_rt_system_file_time (void)
{
	RESULT result = NULL;
	uint32_t test_location = 0;
	int64_t frequency = 0;
	double elapsed_time_ms = 0;

	ep_filetime_t start = ep_system_file_time_get ();
	g_usleep (10 * 1000);
	ep_filetime_t stop = ep_system_file_time_get ();

	test_location = 1;

	ep_raise_error_if_nok (stop > start);

ep_on_exit:
	return result;

ep_on_error:
	if (!result)
		result = FAILED ("Failed at test location=%i", test_location);
	ep_exit_error_handler ();
}

static RESULT
test_rt_teardown (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&eventpipe_memory_end_snapshot);
	if ( _CrtMemDifference( &eventpipe_memory_diff_snapshot, &eventpipe_memory_start_snapshot, &eventpipe_memory_end_snapshot) ) {
		_CrtMemDumpStatistics( &eventpipe_memory_diff_snapshot );
		return FAILED ("Memory leak detected!");
	}
#endif
	return NULL;
}

static Test ep_rt_tests [] = {
	{"test_rt_setup", test_rt_setup},
	{"test_rt_perf_frequency", test_rt_perf_frequency},
	{"test_rt_perf_timestamp", test_rt_perf_timestamp},
	{"test_rt_system_time", test_rt_system_time},
	{"test_rt_system_file_time", test_rt_system_file_time},
	{"test_rt_teardown", test_rt_teardown},
	{NULL, NULL}
};

DEFINE_TEST_GROUP_INIT(ep_rt_tests_init, ep_rt_tests)
