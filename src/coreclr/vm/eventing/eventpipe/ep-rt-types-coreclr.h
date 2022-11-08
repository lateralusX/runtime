// Implementation of ep-rt-types.h targeting CoreCLR runtime.
#ifndef __EVENTPIPE_RT_TYPES_CORECLR_H__
#define __EVENTPIPE_RT_TYPES_CORECLR_H__

#include <eventpipe/ep-rt-config.h>

#ifdef ENABLE_PERFTRACING

#ifdef DEBUG
#define EP_CHECKED_BUILD
#endif

#undef EP_ASSERT
#ifdef EP_CHECKED_BUILD
#define EP_ASSERT(expr) _ASSERTE(expr)
#else
#define EP_ASSERT(expr)
#endif

#undef EP_UNREACHABLE
#define EP_UNREACHABLE(msg) do { UNREACHABLE_MSG(msg); } while (0)

#undef EP_LIKELY
#define EP_LIKELY(expr) expr

#undef EP_UNLIKELY
#define EP_UNLIKELY(expr) expr

template<typename T>
struct _rt_coreclr_queue_internal_t {
	typedef struct SListElem<T> element_type_t;
	typedef class SList<element_type_t> queue_type_t;
	queue_type_t *queue;
};

typedef struct _rt_coreclr_table_callbacks_t {
	void (*key_free_func)(void *);
	void (*value_free_func)(void *);
} rt_coreclr_table_callbacks_t;

template<typename T1, typename T2>
struct _rt_coreclr_table_default_internal_t {
	typedef class SHash<NoRemoveSHashTraits< MapSHashTraits <T1, T2> > > table_type_t;
	rt_coreclr_table_callbacks_t callbacks;
	table_type_t *table;
};

template<typename T1, typename T2>
struct _rt_coreclr_table_remove_internal_t {
	typedef class SHash< MapSHashTraits <T1, T2> > table_type_t;
	rt_coreclr_table_callbacks_t callbacks;
	table_type_t *table;
};

class EventPipeCoreCLRStackHashTraits : public NoRemoveSHashTraits< MapSHashTraits<StackHashKey *, StackHashEntry *> >
{
public:
	typedef typename MapSHashTraits<StackHashKey *, StackHashEntry *>::element_t element_t;
	typedef typename MapSHashTraits<StackHashKey *, StackHashEntry *>::count_t count_t;

	typedef StackHashKey * key_t;

	static key_t GetKey (element_t e)
	{
		extern StackHashKey * ep_stack_hash_entry_get_key (StackHashEntry *);
		return ep_stack_hash_entry_get_key (e.Value ());
	}

	static BOOL Equals (key_t k1, key_t k2)
	{
		extern bool ep_stack_hash_key_equal (const void *, const void *);
		return ep_stack_hash_key_equal (k1, k2);
	}

	static count_t Hash (key_t k)
	{
		extern uint32_t ep_stack_hash_key_hash (const void *);
		return (count_t)ep_stack_hash_key_hash (k);
	}

	static element_t Null ()
	{
		return element_t (NULL, NULL);
	}

	static bool IsNull (const element_t &e)
	{
		return (e.Key () == NULL|| e.Value () == NULL);
	}
};

template<typename T1>
struct _rt_coreclr_table_custom_internal_t {
	typedef class SHash<T1> table_type_t;
	rt_coreclr_table_callbacks_t callbacks;
	table_type_t *table;
};

class CLREventStatic;
struct _rt_coreclr_event_internal_t {
	CLREventStatic *event;
};

class CrstStatic;
struct _rt_coreclr_lock_internal_t {
	CrstStatic *lock;
};

class SpinLock;
struct _rt_coreclr_spin_lock_internal_t {
	SpinLock *lock;
};

/*
 * EventPipeFile.
 */

#undef ep_rt_metadata_labels_hash_map_t
typedef struct _rt_coreclr_table_remove_internal_t<EventPipeEvent *, uint32_t> ep_rt_metadata_labels_hash_map_t;

#undef ep_rt_metadata_labels_hash_map_iterator_t
typedef class _rt_coreclr_table_remove_internal_t<EventPipeEvent *, uint32_t>::table_type_t::Iterator ep_rt_metadata_labels_hash_map_iterator_t;

#undef ep_rt_stack_hash_map_t
typedef struct _rt_coreclr_table_custom_internal_t<EventPipeCoreCLRStackHashTraits> ep_rt_stack_hash_map_t;

#undef ep_rt_stack_hash_map_iterator_t
typedef class _rt_coreclr_table_custom_internal_t<EventPipeCoreCLRStackHashTraits>::table_type_t::Iterator ep_rt_stack_hash_map_iterator_t;

/*
 * EventPipeProvider.
 */

#undef ep_rt_provider_callback_data_queue_t
typedef struct _rt_coreclr_queue_internal_t<EventPipeProviderCallbackData *> ep_rt_provider_callback_data_queue_t;

/*
 * EventPipe.
 */

#undef ep_rt_method_desc_t
typedef class MethodDesc ep_rt_method_desc_t;

/*
 * PAL.
 */

#undef ep_rt_file_handle_t
typedef class CFileStream * ep_rt_file_handle_t;

#undef ep_rt_wait_event_handle_t
typedef struct _rt_coreclr_event_internal_t ep_rt_wait_event_handle_t;

#undef ep_rt_lock_handle_t
typedef struct _rt_coreclr_lock_internal_t ep_rt_lock_handle_t;

#undef ep_rt_spin_lock_handle_t
typedef _rt_coreclr_spin_lock_internal_t ep_rt_spin_lock_handle_t;

/*
 * Thread.
 */

#undef ep_rt_thread_handle_t
typedef class Thread * ep_rt_thread_handle_t;

#undef ep_rt_thread_activity_id_handle_t
typedef class Thread * ep_rt_thread_activity_id_handle_t;

#undef ep_rt_thread_id_t
#ifndef TARGET_UNIX
typedef DWORD ep_rt_thread_id_t;
#else
typedef size_t ep_rt_thread_id_t;
#endif

#undef ep_rt_thread_start_func
typedef DWORD (WINAPI *ep_rt_thread_start_func)(LPVOID lpThreadParameter);

#undef ep_rt_thread_start_func_return_t
typedef DWORD ep_rt_thread_start_func_return_t;

#undef ep_rt_thread_params_t
typedef struct _rt_coreclr_thread_params_t {
	ep_rt_thread_handle_t thread;
	EventPipeThreadType thread_type;
	ep_rt_thread_start_func thread_func;
	void *thread_params;
} ep_rt_thread_params_t;

/*
 * ThreadSequenceNumberMap.
 */

#undef ep_rt_thread_sequence_number_hash_map_t
typedef struct _rt_coreclr_table_remove_internal_t<EventPipeThreadSessionState *, uint32_t> ep_rt_thread_sequence_number_hash_map_t;

#undef ep_rt_thread_sequence_number_hash_map_iterator_t
typedef class _rt_coreclr_table_remove_internal_t<EventPipeThreadSessionState *, uint32_t>::table_type_t::Iterator ep_rt_thread_sequence_number_hash_map_iterator_t;

#endif /* ENABLE_PERFTRACING */
#endif /* __EVENTPIPE_RT_TYPES_CORECLR_H__ */
