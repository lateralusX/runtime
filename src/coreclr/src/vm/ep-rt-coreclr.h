// Implementation of ep-rt.h targeting CoreCLR runtime.
#ifndef __EVENTPIPE_RT_CORECLR_H__
#define __EVENTPIPE_RT_CORECLR_H__

#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-rt-config.h"
#include "ep-thread.h"
#include "ep-types.h"
#include "ep-provider.h"
#include "ep-session-provider.h"
#include "fstream.h"
#include "typestring.h"

#undef EP_ARRAY_SIZE
#define EP_ARRAY_SIZE(expr) (sizeof(expr) / sizeof ((expr) [0]))

#undef EP_INFINITE_WAIT
#define EP_INFINITE_WAIT INFINITE

#undef EP_GCX_PREEMP_ENTER
#define EP_GCX_PREEMP_ENTER { GCX_PREEMP();

#undef EP_GCX_PREEMP_EXIT
#define EP_GCX_PREEMP_EXIT }

#undef EP_ALWAYS_INLINE
#define EP_ALWAYS_INLINE FORCEINLINE

#undef EP_NEVER_INLINE
#define EP_NEVER_INLINE NOINLINE

#undef EP_ALIGN_UP
#define EP_ALIGN_UP(val,align) ALIGN_UP(val,align)

#ifndef EP_RT_BUILD_TYPE_FUNC_NAME
#define EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, type_name, func_name) \
prefix_name ## _rt_ ## type_name ## _ ## func_name
#endif

#define EP_RT_DEFINE_LIST_PREFIX(prefix_name, list_name, list_type, item_type) \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, list_name, alloc) (list_type *list) { \
		list->list = new (nothrow) list_type::list_type_t (); \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, list_name, free) (list_type *list, void (*callback)(void *)) { \
		if (list->list) { \
			while (!list->list->IsEmpty ()) { \
				list_type::element_type_t *current = list->list->RemoveHead (); \
				if (callback) \
					callback (reinterpret_cast<void *>(current->GetValue ())); \
				delete current; \
			} \
			delete list->list; \
		} \
		list->list = NULL; \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, list_name, clear) (list_type *list, void (*callback)(void *)) { \
		EP_ASSERT (list->list != NULL); \
		while (!list->list->IsEmpty ()) { \
			list_type::element_type_t *current = list->list->RemoveHead (); \
			if (callback) \
				callback (reinterpret_cast<void *>(current->GetValue ())); \
			delete current; \
		} \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, list_name, append) (list_type *list, item_type item) { \
		EP_ASSERT (list->list != NULL); \
		list->list->InsertTail (new (nothrow) list_type::element_type_t (item)); \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, list_name, remove) (list_type *list, const item_type item) { \
		EP_ASSERT (list->list != NULL); \
		list_type::element_type_t *current = list->list->GetHead (); \
		while (current) { \
			if (current->GetValue () == item) { \
				if (list->list->FindAndRemove (current)) \
					delete current; \
				break; \
			} \
			current = list->list->GetNext (current); \
		} \
	} \
	static inline bool EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, list_name, find) (const list_type *list, const item_type item_to_find, item_type *found_item) { \
		EP_ASSERT (list->list != NULL); \
		bool found = false; \
		list_type::element_type_t *current = list->list->GetHead (); \
		while (current) { \
			if (current->GetValue () == item_to_find) { \
				*found_item = current->GetValue (); \
				found = true; \
				break; \
			} \
			current = list->list->GetNext (current); \
		} \
		return found; \
	} \
	static inline bool EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, list_name, is_empty) (const list_type *list) { \
		return (list->list == NULL) || list->list->IsEmpty ();\
	}

#define EP_RT_DEFINE_LIST(list_name, list_type, item_type) \
	EP_RT_DEFINE_LIST_PREFIX(ep, list_name, list_type, item_type)

#define EP_RT_DEFINE_LIST_ITERATOR_PREFIX(prefix_name, list_name, list_type, iterator_type, item_type) \
	static inline iterator_type EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, list_name, iterator_begin) (const list_type *list) { \
		return list->list->begin (); \
	} \
	static inline bool EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, list_name, iterator_end) (const list_type *list, const iterator_type *iterator) { \
		return (*iterator == list->list->end ()); \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, list_name, iterator_next) (iterator_type *iterator) { \
		(*iterator)++; \
	} \
	static inline item_type EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, list_name, iterator_value) (const iterator_type *iterator) { \
		return const_cast<iterator_type *>(iterator)->operator*(); \
	}

#define EP_RT_DEFINE_LIST_ITERATOR(list_name, list_type, iterator_type, item_type) \
	EP_RT_DEFINE_LIST_ITERATOR_PREFIX(ep, list_name, list_type, iterator_type, item_type)

#define EP_RT_DEFINE_QUEUE_PREFIX(prefix_name, queue_name, queue_type, item_type) \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, queue_name, alloc) (queue_type *queue) { \
		queue->queue = new (nothrow) SList<SListElem<item_type>> (); \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, queue_name, free) (queue_type *queue) { \
		if (queue->queue) \
			delete queue->queue; \
		queue->queue = NULL; \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, queue_name, pop_head) (queue_type *queue, item_type *item) { \
		EP_ASSERT (queue->queue != NULL); \
		SListElem<item_type> *node = queue->queue->RemoveHead (); \
		if (node) { \
			*item = node->m_Value; \
			delete node; \
		} else { \
			*item = NULL; \
		} \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, queue_name, push_head) (queue_type *queue, item_type item) { \
		EP_ASSERT (queue->queue != NULL); \
		SListElem<item_type> *node = new (nothrow) SListElem<item_type> (item); \
		queue->queue->InsertHead (node); \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, queue_name, push_tail) (queue_type *queue, item_type item) { \
		EP_ASSERT (queue->queue != NULL); \
		SListElem<item_type> *node = new (nothrow) SListElem<item_type> (item); \
		queue->queue->InsertTail (node); \
	} \
	static inline bool EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, queue_name, is_empty) (const queue_type *queue) { \
		EP_ASSERT (queue->queue != NULL); \
		return queue->queue->IsEmpty (); \
	}

#define EP_RT_DEFINE_QUEUE(queue_name, queue_type, item_type) \
	EP_RT_DEFINE_QUEUE_PREFIX(ep, queue_name, queue_type, item_type)

#define EP_RT_DEFINE_ARRAY_PREFIX(prefix_name, array_name, array_type, iterator_type, item_type) \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, alloc) (array_type *ep_array) { \
		ep_array->array = new (nothrow) CQuickArrayList<item_type> (); \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, alloc_capacity) (array_type *ep_array, size_t capacity) { \
		ep_array->array = new (nothrow) CQuickArrayList<item_type> (); \
		if (ep_array->array) \
			ep_array->array->AllocNoThrow (capacity);\
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, free) (array_type *ep_array) { \
		if (ep_array) \
			delete ep_array->array; \
		ep_array->array = NULL; \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, append) (array_type *ep_array, item_type item) { \
		EP_ASSERT (ep_array->array != NULL); \
		EX_TRY \
		{ \
			ep_array->array->Push (item); \
		} \
		EX_CATCH {} \
		EX_END_CATCH(SwallowAllExceptions); \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, clear) (array_type *ep_array) { \
		EP_ASSERT (ep_array->array != NULL); \
		while (ep_array->array->Size () > 0) \
			item_type item = ep_array->array->Pop (); \
		ep_array->array->Shrink (); \
	} \
	static inline size_t EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, size) (const array_type *ep_array) { \
		EP_ASSERT (ep_array->array != NULL); \
		return ep_array->array->Size (); \
	} \
	static inline item_type * EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, data) (const array_type *ep_array) { \
		EP_ASSERT (ep_array->array != NULL); \
		return ep_array->array->Ptr (); \
	}

#define EP_RT_DEFINE_ARRAY(array_name, array_type, iterator_type, item_type) \
	EP_RT_DEFINE_ARRAY_PREFIX(ep, array_name, array_type, iterator_type, item_type)

#define EP_RT_DEFINE_ARRAY_ITERATOR_PREFIX(prefix_name, array_name, array_type, iterator_type, item_type) \
	static inline iterator_type EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, iterator_begin) (const array_type *ep_array) { \
		iterator_type temp; \
		temp.array = ep_array->array; \
		temp.index = 0; \
		return temp; \
	} \
	static inline bool EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, iterator_end) (const array_type *ep_array, const iterator_type *iterator) { \
		EP_ASSERT (iterator->array != NULL); \
		return (iterator->index >= iterator->array->Size ()); \
	} \
	static void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, iterator_next) (iterator_type *iterator) { \
		iterator->index++; \
	} \
	static item_type EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, iterator_value) (const iterator_type *iterator) { \
		EP_ASSERT (iterator->array != NULL); \
		EP_ASSERT (iterator->index < iterator->array->Size ()); \
		return iterator->array->operator[] (iterator->index); \
	}

#define EP_RT_DEFINE_ARRAY_REVERSE_ITERATOR_PREFIX(prefix_name, array_name, array_type, iterator_type, item_type) \
	static inline iterator_type EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, reverse_iterator_begin) (const array_type *ep_array) { \
		iterator_type temp; \
		temp.array = ep_array->array; \
		temp.index = static_cast<int32_t>(ep_array->array->Size () - 1); \
		return temp; \
	} \
	static inline bool EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, reverse_iterator_end) (const array_type *ep_array, const iterator_type *iterator) { \
		EP_ASSERT (iterator->array != NULL); \
		return (iterator->index < 0); \
	} \
	static void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, reverse_iterator_next) (iterator_type *iterator) { \
		iterator->index--; \
	} \
	static item_type EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, array_name, reverse_iterator_value) (const iterator_type *iterator) { \
		EP_ASSERT (iterator->array != NULL); \
		EP_ASSERT (iterator->index >= 0); \
		return iterator->array->operator[] (iterator->index); \
	}

#define EP_RT_DEFINE_ARRAY_ITERATOR(array_name, array_type, iterator_type, item_type) \
	EP_RT_DEFINE_ARRAY_ITERATOR_PREFIX(ep, array_name, array_type, iterator_type, item_type)

#define EP_RT_DEFINE_ARRAY_REVERSE_ITERATOR(array_name, array_type, iterator_type, item_type) \
	EP_RT_DEFINE_ARRAY_REVERSE_ITERATOR_PREFIX(ep, array_name, array_type, iterator_type, item_type)

#define EP_RT_DEFINE_HASH_MAP_PREFIX(prefix_name, hash_map_name, hash_map_type, key_type, value_type) \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, hash_map_name, alloc) (hash_map_type *hash_map, uint32_t (*hash_callback)(const void *), bool (*eq_callback)(const void *, const void *), void (*key_free_callback)(void *), void (*value_free_callback)(void *)) { \
		EP_ASSERT (key_free_callback == NULL); \
		hash_map->table = new (nothrow) hash_map_type::table_type_t (); \
		hash_map->callbacks.key_free_func = key_free_callback; \
		hash_map->callbacks.value_free_func = value_free_callback; \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, hash_map_name, free) (hash_map_type *hash_map) { \
		if (hash_map->table) { \
			if (hash_map->callbacks.value_free_func) { \
				for (hash_map_type::table_type_t::Iterator iterator = hash_map->table->Begin (); iterator != hash_map->table->End (); ++iterator) \
						hash_map->callbacks.value_free_func (reinterpret_cast<void *>((ptrdiff_t)(iterator->Value ()))); \
			} \
			delete hash_map->table; \
		} \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, hash_map_name, add) (hash_map_type *hash_map, key_type key, value_type value) { \
		EP_ASSERT (hash_map->table != NULL); \
		EX_TRY \
		{ \
			hash_map->table->Add (hash_map_type::table_type_t::element_t (key, value)); \
		} \
		EX_CATCH {} \
		EX_END_CATCH(SwallowAllExceptions); \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, hash_map_name, remove_all) (hash_map_type *hash_map) { \
		EP_ASSERT (hash_map->table != NULL); \
		if (hash_map->callbacks.value_free_func) { \
			for (hash_map_type::table_type_t::Iterator iterator = hash_map->table->Begin (); iterator != hash_map->table->End (); ++iterator) \
				hash_map->callbacks.value_free_func (reinterpret_cast<void *>((ptrdiff_t)(iterator->Value ()))); \
		} \
		hash_map->table->RemoveAll (); \
	} \
	static inline bool EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, hash_map_name, lookup) (const hash_map_type *hash_map, const key_type key, value_type *value) { \
		EP_ASSERT (hash_map->table != NULL); \
		const hash_map_type::table_type_t::element_t *ret = hash_map->table->LookupPtr ((key_type)key); \
		if (ret == NULL) \
			return false; \
		*value = ret->Value (); \
		return true; \
	} \
	static inline uint32_t EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, hash_map_name, count) (const hash_map_type *hash_map) { \
		EP_ASSERT (hash_map->table != NULL); \
		return hash_map->table->GetCount (); \
	}

#define EP_RT_DEFINE_HASH_MAP_REMOVE_PREFIX(prefix_name, hash_map_name, hash_map_type, key_type, value_type) \
		EP_RT_DEFINE_HASH_MAP_PREFIX(prefix_name, hash_map_name, hash_map_type, key_type, value_type) \
		static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, hash_map_name, remove) (hash_map_type *hash_map, const key_type key) { \
			EP_ASSERT (hash_map->table != NULL); \
			const hash_map_type::table_type_t::element_t *ret = NULL; \
			if (hash_map->callbacks.value_free_func) \
				ret = hash_map->table->LookupPtr ((key_type)key); \
			hash_map->table->Remove ((key_type)key); \
			if (ret) \
				hash_map->callbacks.value_free_func (reinterpret_cast<void *>(static_cast<ptrdiff_t>(ret->Value ()))); \
		} \

#define EP_RT_DEFINE_HASH_MAP(hash_map_name, hash_map_type, key_type, value_type) \
	EP_RT_DEFINE_HASH_MAP_PREFIX(ep, hash_map_name, hash_map_type, key_type, value_type)

#define EP_RT_DEFINE_HASH_MAP_REMOVE(hash_map_name, hash_map_type, key_type, value_type) \
	EP_RT_DEFINE_HASH_MAP_REMOVE_PREFIX(ep, hash_map_name, hash_map_type, key_type, value_type)

#define EP_RT_DEFINE_HASH_MAP_ITERATOR_PREFIX(prefix_name, hash_map_name, hash_map_type, iterator_type, key_type, value_type) \
	static inline iterator_type EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, hash_map_name, iterator_begin) (const hash_map_type *hash_map) { \
		return hash_map->table->Begin (); \
	} \
	static inline bool EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, hash_map_name, iterator_end) (const hash_map_type *hash_map, const iterator_type *iterator) { \
		return (hash_map->table->End () == *iterator); \
	} \
	static inline void EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, hash_map_name, iterator_next) (iterator_type *iterator) { \
		(*iterator)++; \
	} \
	static inline key_type EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, hash_map_name, iterator_key) (const iterator_type *iterator) { \
		return (*iterator)->Key (); \
	} \
	static inline value_type EP_RT_BUILD_TYPE_FUNC_NAME(prefix_name, hash_map_name, iterator_value) (const iterator_type *iterator) { \
		return (*iterator)->Value (); \
	}

#define EP_RT_DEFINE_HASH_MAP_ITERATOR(hash_map_name, hash_map_type, iterator_type, key_type, value_type) \
	EP_RT_DEFINE_HASH_MAP_ITERATOR_PREFIX(ep, hash_map_name, hash_map_type, iterator_type, key_type, value_type)

typedef DWORD (WINAPI *ep_rt_thread_start_func)(LPVOID lpThreadParameter);
typedef DWORD ep_rt_thread_start_func_return_t;
typedef DWORD ep_rt_thread_id_t;

#define EP_RT_DEFINE_THREAD_FUNC(name) static ep_rt_thread_start_func_return_t WINAPI name (LPVOID data)

static
inline
char *
diagnostics_command_line_get (void)
{
	return ep_rt_utf16_to_utf8_string (reinterpret_cast<const ep_char16_t *>(GetCommandLineForDiagnostics ()), -1);
}

static
inline
ep_char8_t **
diagnostics_command_line_get_ref (void)
{
	extern ep_char8_t *_ep_rt_coreclr_diagnostics_cmd_line;
	return &_ep_rt_coreclr_diagnostics_cmd_line;
}

static
inline
void
diagnostics_command_line_lazy_init (void)
{
	if (!*diagnostics_command_line_get_ref ())
		*diagnostics_command_line_get_ref () = diagnostics_command_line_get ();
}

static
inline
void
diagnostics_command_line_lazy_clean (void)
{
	ep_rt_utf8_string_free (*diagnostics_command_line_get_ref ());
	*diagnostics_command_line_get_ref () = NULL;
}

static
inline
ep_rt_lock_handle_t *
ep_rt_coreclr_config_lock_get (void)
{
	extern ep_rt_lock_handle_t _ep_rt_coreclr_config_lock_handle;
	return &_ep_rt_coreclr_config_lock_handle;
}

/*
* Atomics.
*/

static
inline
uint32_t
ep_rt_atomic_inc_uint32_t (volatile uint32_t *value)
{
	return static_cast<uint32_t>(InterlockedIncrement ((volatile LONG *)(value)));
}

static
inline
uint32_t
ep_rt_atomic_dec_uint32_t (volatile uint32_t *value)
{
	return static_cast<uint32_t>(InterlockedDecrement ((volatile LONG *)(value)));
}

static
inline
int32_t
ep_rt_atomic_inc_int32_t (volatile int32_t *value)
{
	return static_cast<int32_t>(InterlockedIncrement ((volatile LONG *)(value)));
}

static
inline
int32_t
ep_rt_atomic_dec_int32_t (volatile int32_t *value)
{
	return static_cast<int32_t>(InterlockedDecrement ((volatile LONG *)(value)));
}

static
inline
int64_t
ep_rt_atomic_inc_int64_t (volatile int64_t *value)
{
	return static_cast<int64_t>(InterlockedIncrement64 ((volatile LONG64 *)(value)));
}

static
inline
int64_t
ep_rt_atomic_dec_int64_t (volatile int64_t *value)
{
	return static_cast<int64_t>(InterlockedDecrement64 ((volatile LONG64 *)(value)));
}

/*
 * EventPipe.
 */

EP_RT_DEFINE_ARRAY (session_id_array, ep_rt_session_id_array_t, ep_rt_session_id_array_iterator_t, EventPipeSessionID)
EP_RT_DEFINE_ARRAY_ITERATOR (session_id_array, ep_rt_session_id_array_t, ep_rt_session_id_array_iterator_t, EventPipeSessionID)

static
inline
EventPipeThreadHolder *
thread_holder_alloc_func (void)
{
	return ep_thread_holder_alloc (ep_thread_alloc());
}

static
inline
void
thread_holder_free_func (EventPipeThreadHolder * thread_holder)
{
	ep_thread_holder_free (thread_holder);
}

static
inline
void
ep_rt_init (void)
{
	extern ep_rt_lock_handle_t _ep_rt_coreclr_config_lock_handle;
	extern CrstStatic _ep_rt_coreclr_config_lock;

	_ep_rt_coreclr_config_lock_handle.lock = &_ep_rt_coreclr_config_lock;
	_ep_rt_coreclr_config_lock_handle.lock->InitNoThrow (CrstEventPipe, (CrstFlags)(CRST_REENTRANCY | CRST_TAKEN_DURING_SHUTDOWN | CRST_HOST_BREAKABLE));

	if (CLRConfig::GetConfigValue (CLRConfig::INTERNAL_EventPipeProcNumbers) != 0) {
#ifndef TARGET_UNIX
		// setup the windows processor group offset table
		uint16_t groups = ::GetActiveProcessorGroupCount ();
		extern uint32_t *_ep_rt_coreclr_proc_group_offsets;
		_ep_rt_coreclr_proc_group_offsets = new (nothrow) uint32_t [groups];
		if (_ep_rt_coreclr_proc_group_offsets) {
			uint32_t procs = 0;
			for (uint16_t i = 0; i < procs; ++i) {
				_ep_rt_coreclr_proc_group_offsets [i] = procs;
				procs += GetActiveProcessorCount (i);
			}
		}
#endif
	}
}

static
inline
void
ep_rt_shutdown (void)
{
	diagnostics_command_line_lazy_clean ();
}

static
inline
bool
ep_rt_config_aquire (void)
{
	return ep_rt_lock_aquire (ep_rt_coreclr_config_lock_get ());
}

static
inline
bool
ep_rt_config_release (void)
{
	return ep_rt_lock_release (ep_rt_coreclr_config_lock_get ());
}

#ifdef EP_CHECKED_BUILD
static
inline
void
ep_rt_config_requires_lock_held (void)
{
	ep_rt_lock_requires_lock_held (ep_rt_coreclr_config_lock_get ());
}

static
inline
void
ep_rt_config_requires_lock_not_held (void)
{
	ep_rt_lock_requires_lock_not_held (ep_rt_coreclr_config_lock_get ());
}
#endif

static
inline
bool
ep_rt_walk_managed_stack_for_thread (
	ep_rt_thread_handle_t thread,
	EventPipeStackContents *stack_contents)
{
	extern bool ep_rt_coreclr_walk_managed_stack_for_thread (ep_rt_thread_handle_t thread, EventPipeStackContents *stack_contents);
	return ep_rt_coreclr_walk_managed_stack_for_thread (thread, stack_contents);
}

static
inline
bool
ep_rt_method_get_simple_assembly_name (
	ep_rt_method_desc_t *method,
	ep_char8_t *name,
	size_t name_len)
{
	EP_ASSERT (method != NULL);
	EP_ASSERT (name != NULL);

	name [0] = 0;

	const ep_char8_t *assembly_name = method->GetLoaderModule ()->GetAssembly ()->GetSimpleName ();
	if (!assembly_name)
		return false;

	size_t assembly_name_len = strlen (assembly_name) + 1;
	memcpy (name, assembly_name, (assembly_name_len < name_len) ? assembly_name_len : name_len);

	return true;
}

static
inline
bool
ep_rt_method_get_full_name (
	ep_rt_method_desc_t *method,
	ep_char8_t *name,
	size_t name_len)
{
	EP_ASSERT (method != NULL);
	EP_ASSERT (name != NULL);

	SString method_name;
	StackScratchBuffer conversion;

	TypeString::AppendMethodInternal (method_name, method, TypeString::FormatNamespace | TypeString::FormatSignature);
	const ep_char8_t *method_name_utf8 = method_name.GetUTF8 (conversion);
	if (!method_name_utf8)
		return false;

	size_t method_name_utf8_len = strlen (method_name_utf8) + 1;
	memcpy (name, method_name_utf8, (method_name_utf8_len < name_len) ? method_name_utf8_len : name_len);

	return true;
}

static
inline
void
ep_rt_provider_config_init (EventPipeProviderConfiguration *provider_config)
{
	if (!ep_rt_utf8_string_compare (ep_config_get_rundown_provider_name_utf8 (), ep_provider_config_get_provider_name (provider_config))) {
		MICROSOFT_WINDOWS_DOTNETRUNTIME_RUNDOWN_PROVIDER_DOTNET_Context.EventPipeProvider.Level = ep_provider_config_get_logging_level (provider_config);
		MICROSOFT_WINDOWS_DOTNETRUNTIME_RUNDOWN_PROVIDER_DOTNET_Context.EventPipeProvider.EnabledKeywordsBitmask = ep_provider_config_get_keywords (provider_config);
		MICROSOFT_WINDOWS_DOTNETRUNTIME_RUNDOWN_PROVIDER_DOTNET_Context.EventPipeProvider.IsEnabled = true;
	}
}

static
inline
void
ep_rt_init_providers_and_events (void)
{
	extern void InitProvidersAndEvents ();
	InitProvidersAndEvents ();
}

static
inline
bool
ep_rt_providers_validate_all_disabled (void)
{
	return (!MICROSOFT_WINDOWS_DOTNETRUNTIME_PROVIDER_DOTNET_Context.EventPipeProvider.IsEnabled &&
		!MICROSOFT_WINDOWS_DOTNETRUNTIME_PRIVATE_PROVIDER_DOTNET_Context.EventPipeProvider.IsEnabled &&
		!MICROSOFT_WINDOWS_DOTNETRUNTIME_RUNDOWN_PROVIDER_DOTNET_Context.EventPipeProvider.IsEnabled);
}

static
inline
void
ep_rt_prepare_provider_invoke_callback (EventPipeProviderCallbackData *provider_callback_data)
{
#if defined(HOST_OSX) && defined(HOST_ARM64)
	auto jitWriteEnableHolder = PAL_JITWriteEnable(false);
#endif // defined(HOST_OSX) && defined(HOST_ARM64)
}

/*
 * EventPipeBuffer.
 */

EP_RT_DEFINE_ARRAY (buffer_array, ep_rt_buffer_array_t, ep_rt_buffer_array_iterator_t, EventPipeBuffer *)
EP_RT_DEFINE_ARRAY_ITERATOR (buffer_array, ep_rt_buffer_array_t, ep_rt_buffer_array_iterator_t, EventPipeBuffer *)

/*
 * EventPipeBufferList.
 */

EP_RT_DEFINE_ARRAY (buffer_list_array, ep_rt_buffer_list_array_t, ep_rt_buffer_list_array_iterator_t, EventPipeBufferList *)
EP_RT_DEFINE_ARRAY_ITERATOR (buffer_list_array, ep_rt_buffer_list_array_t, ep_rt_buffer_list_array_iterator_t, EventPipeBufferList *)

/*
 * EventPipeEvent.
 */

EP_RT_DEFINE_LIST (event_list, ep_rt_event_list_t, EventPipeEvent *)
EP_RT_DEFINE_LIST_ITERATOR (event_list, ep_rt_event_list_t, ep_rt_event_list_iterator_t, EventPipeEvent *)

/*
 * EventPipeFile.
 */

EP_RT_DEFINE_HASH_MAP_REMOVE(metadata_labels_hash, ep_rt_metadata_labels_hash_map_t, EventPipeEvent *, uint32_t)
EP_RT_DEFINE_HASH_MAP(stack_hash, ep_rt_stack_hash_map_t, StackHashKey *, StackHashEntry *)
EP_RT_DEFINE_HASH_MAP_ITERATOR(stack_hash, ep_rt_stack_hash_map_t, ep_rt_stack_hash_map_iterator_t, StackHashKey *, StackHashEntry *)

/*
 * EventPipeProvider.
 */

EP_RT_DEFINE_LIST (provider_list, ep_rt_provider_list_t, EventPipeProvider *)
EP_RT_DEFINE_LIST_ITERATOR (provider_list, ep_rt_provider_list_t, ep_rt_provider_list_iterator_t, EventPipeProvider *)

EP_RT_DEFINE_QUEUE (provider_callback_data_queue, ep_rt_provider_callback_data_queue_t, EventPipeProviderCallbackData *)

static
inline
EventPipeProvider *
ep_rt_provider_list_find_by_name (
	const ep_rt_provider_list_t *list,
	const ep_char8_t *name)
{
	// The provider list should be non-NULL, but can be NULL on shutdown.
	if (list){
		SList<SListElem<EventPipeProvider *>> *provider_list = list->list;
		SListElem<EventPipeProvider *> *element = provider_list->GetHead ();
		while (element) {
			EventPipeProvider *provider = element->GetValue ();
			if (ep_rt_utf8_string_compare (ep_provider_get_provider_name (element->GetValue ()), name) == 0)
				return provider;

			element = provider_list->GetNext (element);
		}
	}

	return NULL;
}

/*
 * EventPipeProviderConfiguration.
 */

EP_RT_DEFINE_ARRAY (provider_config_array, ep_rt_provider_config_array_t, ep_rt_provider_config_array_iterator_t, EventPipeProviderConfiguration)
EP_RT_DEFINE_ARRAY_ITERATOR (provider_config_array, ep_rt_provider_config_array_t, ep_rt_provider_config_array_iterator_t, EventPipeProviderConfiguration)

static
inline
bool
ep_rt_config_value_get_enable (void)
{
	return CLRConfig::GetConfigValue (CLRConfig::INTERNAL_EnableEventPipe) != 0;
}

static
inline
ep_char8_t *
ep_rt_config_value_get_config (void)
{
	CLRConfigStringHolder value(CLRConfig::GetConfigValue (CLRConfig::INTERNAL_EventPipeConfig));
	return ep_rt_utf16_to_utf8_string (reinterpret_cast<ep_char16_t *>(value.GetValue ()), -1);
}

static
inline
ep_char8_t *
ep_rt_config_value_get_output_path (void)
{
	CLRConfigStringHolder value(CLRConfig::GetConfigValue (CLRConfig::INTERNAL_EventPipeOutputPath));
	return ep_rt_utf16_to_utf8_string (reinterpret_cast<ep_char16_t *>(value.GetValue ()), -1);
}

static
inline
uint32_t
ep_rt_config_value_get_circular_mb (void)
{
	return CLRConfig::GetConfigValue (CLRConfig::INTERNAL_EventPipeCircularMB);
}

/*
 * EventPipeSampleProfiler.
 */

static
inline
void
ep_rt_sample_profiler_write_sampling_event_for_threads (ep_rt_thread_handle_t sampling_thread, EventPipeEvent *sampling_event)
{

	extern void ep_rt_coreclr_sample_profiler_write_sampling_event_for_threads (ep_rt_thread_handle_t sampling_thread, EventPipeEvent *sampling_event);
	ep_rt_coreclr_sample_profiler_write_sampling_event_for_threads (sampling_thread, sampling_event);
}

static
void
ep_rt_notify_profiler_provider_created (EventPipeProvider *provider)
{
#ifndef DACCESS_COMPILE
		// Let the profiler know the provider has been created so it can register if it wants to
		BEGIN_PIN_PROFILER (CORProfilerIsMonitoringEventPipe ());
		g_profControlBlock.pProfInterface->EventPipeProviderCreated (provider);
		END_PIN_PROFILER ();
#endif // DACCESS_COMPILE
}

/*
 * EventPipeSessionProvider.
 */

EP_RT_DEFINE_LIST (session_provider_list, ep_rt_session_provider_list_t, EventPipeSessionProvider *)
EP_RT_DEFINE_LIST_ITERATOR (session_provider_list, ep_rt_session_provider_list_t, ep_rt_session_provider_list_iterator_t, EventPipeSessionProvider *)

/*
 * EventPipeSequencePoint.
 */

EP_RT_DEFINE_LIST (sequence_point_list, ep_rt_sequence_point_list_t, EventPipeSequencePoint *)
EP_RT_DEFINE_LIST_ITERATOR (sequence_point_list, ep_rt_sequence_point_list_t, ep_rt_sequence_point_list_iterator_t, EventPipeSequencePoint *)

/*
 * EventPipeThread.
 */

EP_RT_DEFINE_LIST (thread_list, ep_rt_thread_list_t, EventPipeThread *)
EP_RT_DEFINE_LIST_ITERATOR (thread_list, ep_rt_thread_list_t, ep_rt_thread_list_iterator_t, EventPipeThread *)

EP_RT_DEFINE_ARRAY (thread_array, ep_rt_thread_array_t, ep_rt_thread_array_iterator_t, EventPipeThread *)
EP_RT_DEFINE_ARRAY_ITERATOR (thread_array, ep_rt_thread_array_t, ep_rt_thread_array_iterator_t, EventPipeThread *)

/*
 * EventPipeThreadSessionState.
 */

EP_RT_DEFINE_LIST (thread_session_state_list, ep_rt_thread_session_state_list_t, EventPipeThreadSessionState *)
EP_RT_DEFINE_LIST_ITERATOR (thread_session_state_list, ep_rt_thread_session_state_list_t, ep_rt_thread_session_state_list_iterator_t, EventPipeThreadSessionState *)

EP_RT_DEFINE_ARRAY (thread_session_state_array, ep_rt_thread_session_state_array_t, ep_rt_thread_session_state_array_iterator_t, EventPipeThreadSessionState *)
EP_RT_DEFINE_ARRAY_ITERATOR (thread_session_state_array, ep_rt_thread_session_state_array_t, ep_rt_thread_session_state_array_iterator_t, EventPipeThreadSessionState *)

static
inline
EventPipeSessionProvider *
ep_rt_session_provider_list_find_by_name (
	const ep_rt_session_provider_list_t *list,
	const ep_char8_t *name)
{
	SList<SListElem<EventPipeSessionProvider *>> *provider_list = list->list;
	EventPipeSessionProvider *session_provider = NULL;
	SListElem<EventPipeSessionProvider *> *element = provider_list->GetHead ();
	while (element) {
		EventPipeSessionProvider *candidate = element->GetValue ();
		if (ep_rt_utf8_string_compare (ep_session_provider_get_provider_name (candidate), name) == 0) {
			session_provider = candidate;
			break;
		}
		element = provider_list->GetNext (element);
	}

	return session_provider;
}

/*
 * Arrays.
 */

static
inline
uint8_t *
ep_rt_byte_array_alloc (size_t len)
{
	return new (nothrow) uint8_t [len];
}

static
inline
void
ep_rt_byte_array_free (uint8_t *ptr)
{
	if (ptr)
		delete [] ptr;
}

/*
 * Event.
 */

static
inline
void
ep_rt_wait_event_alloc (
	ep_rt_wait_event_handle_t *wait_event,
	bool manual,
	bool initial)
{
	EP_ASSERT (wait_event != NULL);
	EP_ASSERT (wait_event->event == NULL);

	wait_event->event = new (nothrow) CLREventStatic ();
	if (manual)
		wait_event->event->CreateManualEvent (initial);
	else
		wait_event->event->CreateAutoEvent (initial);
}

static
inline
void
ep_rt_wait_event_free (ep_rt_wait_event_handle_t *wait_event)
{
	if (wait_event != NULL && wait_event->event != NULL) {
		wait_event->event->CloseEvent ();
		delete wait_event->event;
		wait_event->event = NULL;
	}
}

static
inline
bool
ep_rt_wait_event_set (ep_rt_wait_event_handle_t *wait_event)
{
	EP_ASSERT (wait_event != NULL && wait_event->event != NULL);
	return wait_event->event->Set ();
}

static
inline
int32_t
ep_rt_wait_event_wait (
	ep_rt_wait_event_handle_t *wait_event,
	uint32_t timeout,
	bool alertable)
{
	EP_ASSERT (wait_event != NULL && wait_event->event != NULL);
	return wait_event->event->Wait (timeout, alertable);
}

static
inline
EventPipeWaitHandle
ep_rt_wait_event_get_wait_handle (ep_rt_wait_event_handle_t *wait_event)
{
	EP_ASSERT (wait_event != NULL && wait_event->event != NULL);
	return reinterpret_cast<EventPipeWaitHandle>(wait_event->event->GetHandleUNHOSTED());
}

static
inline
bool
ep_rt_wait_event_is_valid (ep_rt_wait_event_handle_t *wait_event)
{
	if (wait_event == NULL || wait_event->event == NULL)
		return false;

	return wait_event->event->IsValid ();
}

/*
 * Misc.
 */

static
inline
int
ep_rt_get_last_error (void)
{
	return ::GetLastError ();
}

static
inline
bool
ep_rt_process_detach (void)
{
	return (bool)g_fProcessDetach;
}

static
inline
bool
ep_rt_process_shutdown (void)
{
	return (bool)g_fEEShutDown;
}

static
inline
void
ep_rt_create_activity_id (
	uint8_t *activity_id,
	uint32_t activity_id_len)
{
	EP_ASSERT (activity_id != NULL);
	EP_ASSERT (activity_id_len == EP_ACTIVITY_ID_SIZE);

	CoCreateGuid (reinterpret_cast<GUID *>(activity_id));
}

static
inline
bool
ep_rt_is_running (void)
{
	return (bool)g_fEEStarted;
}

static
inline
void
ep_rt_execute_rundown (void)
{
	if (CLRConfig::GetConfigValue (CLRConfig::INTERNAL_EventPipeRundown) > 0) {
		// Ask the runtime to emit rundown events.
		if (g_fEEStarted && !g_fEEShutDown)
			ETW::EnumerationLog::EndRundown ();
	}
}

/*
 * Objects.
 */

#undef ep_rt_object_alloc
#define ep_rt_object_alloc(obj_type) (new (nothrow) obj_type())

#undef ep_rt_object_array_alloc
#define ep_rt_object_array_alloc(obj_type,size) (new (nothrow) obj_type [size])

#undef ep_rt_object_array_free
#define ep_rt_object_array_free(obj_ptr) do { if (obj_ptr) delete [] obj_ptr; } while(0)

#undef ep_rt_object_free
#define ep_rt_object_free(obj_ptr) do { if (obj_ptr) delete obj_ptr; } while(0)

/*
 * PAL.
 */

typedef struct ep_rt_thread_params_t {
	ep_rt_thread_handle_t thread;
	EventPipeThreadType thread_type;
	ep_rt_thread_start_func thread_func;
	void *thread_params;
} ep_rt_thread_params_t;

typedef struct _rt_coreclr_thread_params_internal_t {
	ep_rt_thread_params_t thread_params;
} rt_coreclr_thread_params_internal_t;

static
DWORD WINAPI ep_rt_thread_coreclr_start_func (LPVOID params)
{
	rt_coreclr_thread_params_internal_t *thread_params = reinterpret_cast<rt_coreclr_thread_params_internal_t *>(params);
	DWORD result = thread_params->thread_params.thread_func (thread_params);
	if (thread_params->thread_params.thread)
		::DestroyThread (thread_params->thread_params.thread);
	delete thread_params;
	return result;
}

static
inline
bool
ep_rt_thread_create (
	void *thread_func,
	void *params,
	EventPipeThreadType thread_type,
	void *id)
{
	bool result = false;

	rt_coreclr_thread_params_internal_t *thread_params = new (nothrow) rt_coreclr_thread_params_internal_t ();
	if (thread_params) {
		thread_params->thread_params.thread_type = thread_type;
		if (thread_type == EP_THREAD_TYPE_SESSION || thread_type == EP_THREAD_TYPE_SAMPLING) {
			thread_params->thread_params.thread = SetupUnstartedThread ();
			thread_params->thread_params.thread_func = reinterpret_cast<LPTHREAD_START_ROUTINE>(thread_func);
			thread_params->thread_params.thread_params = params;
			if (thread_params->thread_params.thread->CreateNewThread (0, ep_rt_thread_coreclr_start_func, thread_params)) {
				thread_params->thread_params.thread->SetBackground (TRUE);
				thread_params->thread_params.thread->StartThread ();
				*reinterpret_cast<DWORD *>(id) = thread_params->thread_params.thread->GetThreadId ();
				result = true;
			}
		} else if (thread_type == EP_THREAD_TYPE_SERVER) {
			DWORD thread_id = 0;
			HANDLE server_thread = ::CreateThread (nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(thread_func), nullptr, 0, &thread_id);
			if (server_thread != NULL) {
				::CloseHandle (server_thread);
				*reinterpret_cast<DWORD *>(id) = thread_id;
				result = true;
			}
		}
	}

	return result;
}

static
inline
void
ep_rt_thread_sleep (uint64_t ns)
{
#ifdef TARGET_UNIX
	PAL_nanosleep (ns);
#else  //TARGET_UNIX
	const uint32_t NUM_NANOSECONDS_IN_1_MS = 1000000;
	ClrSleepEx (static_cast<DWORD>(ns / NUM_NANOSECONDS_IN_1_MS), FALSE);
#endif //TARGET_UNIX
}

static
inline
uint32_t
ep_rt_current_process_get_id (void)
{
	return static_cast<uint32_t>(GetCurrentProcessId ());
}

static
inline
uint32_t
ep_rt_current_processor_get_number (void)
{
#ifndef TARGET_UNIX
	extern uint32_t *_ep_rt_coreclr_proc_group_offsets;
	if (_ep_rt_coreclr_proc_group_offsets) {
		PROCESSOR_NUMBER proc;
		GetCurrentProcessorNumberEx (&proc);
		return _ep_rt_coreclr_proc_group_offsets [proc.Group] + proc.Number;
	}
#endif
	return 0xFFFFFFFF;
}

static
inline
uint32_t
ep_rt_processors_get_count (void)
{
	SYSTEM_INFO sys_info = {};
	GetSystemInfo (&sys_info);
	return static_cast<uint32_t>(sys_info.dwNumberOfProcessors);
}

static
inline
size_t
ep_rt_current_thread_get_id (void)
{
#ifdef TARGET_UNIX
	return static_cast<size_t>(::PAL_GetCurrentOSThreadId ());
#else
	return static_cast<size_t>(::GetCurrentThreadId ());
#endif
}

static
inline
int64_t
ep_rt_perf_counter_query (void)
{
	LARGE_INTEGER value;
	if (QueryPerformanceCounter (&value))
		return static_cast<int64_t>(value.QuadPart);
	else
		return 0;
}

static
inline
int64_t
ep_rt_perf_frequency_query (void)
{
	LARGE_INTEGER value;
	if (QueryPerformanceFrequency (&value))
		return static_cast<int64_t>(value.QuadPart);
	else
		return 0;
}

static
inline
void
ep_rt_system_time_get (EventPipeSystemTime *system_time)
{
	SYSTEMTIME value;
	GetSystemTime (&value);

	EP_ASSERT(system_time != NULL);
	ep_system_time_set (
		system_time,
		value.wYear,
		value.wMonth,
		value.wDayOfWeek,
		value.wDay,
		value.wHour,
		value.wMinute,
		value.wSecond,
		value.wMilliseconds);
}

static
inline
int64_t
ep_rt_system_timestamp_get (void)
{
	FILETIME value;
	GetSystemTimeAsFileTime (&value);
	return static_cast<int64_t>(((static_cast<uint64_t>(value.dwHighDateTime)) << 32) | static_cast<uint64_t>(value.dwLowDateTime));
}

static
inline
int32_t
ep_rt_system_get_alloc_granularity (void)
{
	return static_cast<int32_t>(g_SystemInfo.dwAllocationGranularity);
}

static
inline
const ep_char8_t *
ep_rt_os_command_line_get (void)
{
	EP_ASSERT (!"Can not reach here");
	return NULL;
}

static
inline
ep_rt_file_handle_t
ep_rt_file_open_write (const ep_char8_t *path)
{
	ep_char16_t *path_utf16 = ep_rt_utf8_to_utf16_string (path, -1);
	ep_return_null_if_nok (path_utf16 != NULL);

	CFileStream *file_stream = new (nothrow) CFileStream();
	if (FAILED (file_stream->OpenForWrite (reinterpret_cast<LPWSTR>(path_utf16))))
	{
		delete file_stream;
		file_stream = NULL;
	}

	ep_rt_utf16_string_free (path_utf16);
	return static_cast<ep_rt_file_handle_t>(file_stream);
}

static
inline
bool
ep_rt_file_close (ep_rt_file_handle_t file_handle)
{
	// Closed in destructor.
	if (file_handle)
		delete file_handle;
	return true;
}

static
inline
bool
ep_rt_file_write (
	ep_rt_file_handle_t file_handle,
	const uint8_t *buffer,
	uint32_t bytes_to_write,
	uint32_t *bytes_written)
{
	ep_return_false_if_nok (file_handle != NULL);
	EP_ASSERT (buffer != NULL);

	ULONG out_count;
	HRESULT result = reinterpret_cast<CFileStream *>(file_handle)->Write (buffer, bytes_to_write, &out_count);
	*bytes_written = static_cast<uint32_t>(out_count);
	return result == S_OK;
}

static
inline
uint8_t *
ep_rt_valloc0 (size_t buffer_size)
{
	return reinterpret_cast<uint8_t *>(ClrVirtualAlloc (NULL, buffer_size, MEM_COMMIT, PAGE_READWRITE));
}

static
inline
void
ep_rt_vfree (
	uint8_t *buffer,
	size_t buffer_size)
{
	if (buffer)
		ClrVirtualFree (buffer, 0, MEM_RELEASE);
}

static
inline
uint32_t
ep_rt_temp_path_get (
	ep_char8_t *buffer,
	uint32_t buffer_len)
{
	EP_ASSERT (!"Can not reach here");
	return 0;
}

EP_RT_DEFINE_ARRAY (env_array_utf16, ep_rt_env_array_utf16_t, ep_rt_env_array_utf16_iterator_t, ep_char16_t *)
EP_RT_DEFINE_ARRAY_ITERATOR (env_array_utf16, ep_rt_env_array_utf16_t, ep_rt_env_array_utf16_iterator_t, ep_char16_t *)

static
inline
void
ep_rt_os_environment_get_utf16 (ep_rt_env_array_utf16_t *env_array)
{
	EP_ASSERT (env_array != NULL);
	LPWSTR envs = GetEnvironmentStringsW ();
	if (envs) {
		LPWSTR next = envs;
		while (*next) {
			ep_rt_env_array_utf16_append (env_array, ep_rt_utf16_string_dup (reinterpret_cast<const ep_char16_t *>(next)));
			next += ep_rt_utf16_string_len (reinterpret_cast<const ep_char16_t *>(next)) + 1;
		}
		FreeEnvironmentStringsW (envs);
	}
}

/*
* Lock.
*/

static
inline
bool
ep_rt_lock_aquire (ep_rt_lock_handle_t *lock)
{
	// NOTHROW
	bool result = true;
	EX_TRY
	{
		if (lock) {
			CrstBase::CrstHolderWithState holder(lock->lock);
			holder.SuppressRelease ();
		}
	}
	EX_CATCH
	{
		result = false;
	}
	EX_END_CATCH(SwallowAllExceptions);

	return result;
}

static
inline
bool
ep_rt_lock_release (ep_rt_lock_handle_t *lock)
{
	// NOTHROW
	EX_TRY
	{
		if (lock) {
			CrstBase::UnsafeCrstInverseHolder holder(lock->lock);
			holder.SuppressRelease ();
		}
	}
	EX_CATCH {}
	EX_END_CATCH(SwallowAllExceptions);

	return true;
}

#ifdef EP_CHECKED_BUILD
static
inline
void
ep_rt_lock_requires_lock_held (const ep_rt_lock_handle_t *lock)
{
	// NOTHROW
	EP_ASSERT (((ep_rt_lock_handle_t *)lock)->lock->OwnedByCurrentThread ());
}

static
inline
void
ep_rt_lock_requires_lock_not_held (const ep_rt_lock_handle_t *lock)
{
	// NOTHROW
	EP_ASSERT (lock->lock == NULL || !((ep_rt_lock_handle_t *)lock)->lock->OwnedByCurrentThread ());
}
#endif

/*
* SpinLock.
*/

static
inline
void
ep_rt_spin_lock_alloc (ep_rt_spin_lock_handle_t *spin_lock)
{
	// NOTHROW
	EX_TRY
	{
		spin_lock->lock = new (nothrow) SpinLock();
		spin_lock->lock->Init (LOCK_TYPE_DEFAULT);
	}
	EX_CATCH {}
	EX_END_CATCH(SwallowAllExceptions);
}

static
inline
void
ep_rt_spin_lock_free (ep_rt_spin_lock_handle_t *spin_lock)
{
	// NOTHROW
	if (spin_lock && spin_lock->lock) {
		delete spin_lock->lock;
		spin_lock->lock = NULL;
	}
}

static
inline
bool
ep_rt_spin_lock_aquire (ep_rt_spin_lock_handle_t *spin_lock)
{
	// NOTHROW
	if (spin_lock && spin_lock->lock)
		SpinLock::AcquireLock (spin_lock->lock);
	return true;
}

static
inline
bool
ep_rt_spin_lock_release (ep_rt_spin_lock_handle_t *spin_lock)
{
	// NOTHROW
	if (spin_lock && spin_lock->lock)
		SpinLock::ReleaseLock (spin_lock->lock);
	return true;
}

#ifdef EP_CHECKED_BUILD
static
inline
void
ep_rt_spin_lock_requires_lock_held (const ep_rt_spin_lock_handle_t *spin_lock)
{
	// NOTHROW
	EP_ASSERT (spin_lock->lock->OwnedByCurrentThread ());
}

static
inline
void
ep_rt_spin_lock_requires_lock_not_held (const ep_rt_spin_lock_handle_t *spin_lock)
{
	// NOTHROW
	EP_ASSERT (spin_lock->lock == NULL || !spin_lock->lock->OwnedByCurrentThread ());
}
#endif

/*
 * String.
 */

static
inline
int
ep_rt_utf8_string_compare (
	const ep_char8_t *str1,
	const ep_char8_t *str2)
{
	return strcmp (reinterpret_cast<const char *>(str1), reinterpret_cast<const char *>(str2));
}

static
inline
int
ep_rt_utf8_string_compare_ignore_case (
	const ep_char8_t *str1,
	const ep_char8_t *str2)
{
	return _stricmp (reinterpret_cast<const char *>(str1), reinterpret_cast<const char *>(str2));
}

static
inline
bool
ep_rt_utf8_string_is_null_or_empty (const ep_char8_t *str)
{
	if (str == NULL)
		return true;

	while (*str) {
		if (!isspace (*str))
			return false;
		str++;
	}
	return true;
}

static
inline
ep_char16_t *
ep_rt_utf8_to_utf16_string (
	const ep_char8_t *str,
	size_t len)
{
	COUNT_T length = WszMultiByteToWideChar (CP_UTF8, 0, str, static_cast<int>(len), 0, 0);
	if (length == 0)
		return NULL;

	if (static_cast<int>(len) != -1)
		length += 1;

	ep_char16_t *str_utf16 = reinterpret_cast<ep_char16_t *>(malloc (length * sizeof (ep_char16_t)));
	if (!str_utf16)
		return NULL;

	length = WszMultiByteToWideChar (CP_UTF8, 0, str, static_cast<int>(len), reinterpret_cast<LPWSTR>(str_utf16), length);
	if (length == 0) {
		free (str_utf16);
		return NULL;
	}
	str_utf16 [length - 1] = 0;
	return str_utf16;
}

static
inline
ep_char8_t *
ep_rt_utf8_string_dup (const ep_char8_t *str)
{
	return _strdup (str);
}

static
inline
ep_char8_t *
ep_rt_utf8_string_strtok (
	ep_char8_t *str,
	const ep_char8_t *delimiter,
	ep_char8_t **context)
{
	return strtok_s (str, delimiter, context);
}

#undef ep_rt_utf8_string_snprintf
#define ep_rt_utf8_string_snprintf( \
	str, \
	str_len, \
	format, ...) \
sprintf_s (reinterpret_cast<char *>(str), static_cast<size_t>(str_len), reinterpret_cast<const char *>(format), __VA_ARGS__)

static
inline
ep_char16_t *
ep_rt_utf16_string_dup (const ep_char16_t *str)
{
	size_t str_size = (ep_rt_utf16_string_len (str) + 1) * sizeof (ep_char16_t);
	ep_char16_t *str_dup = reinterpret_cast<ep_char16_t *>(malloc (str_size));
	if (str_dup)
		memcpy (str_dup, str, str_size);
	return str_dup;
}

static
inline
void
ep_rt_utf8_string_free (ep_char8_t *str)
{
	if (str)
		free (str);
}

static
inline
size_t
ep_rt_utf16_string_len (const ep_char16_t *str)
{
	return wcslen (reinterpret_cast<const wchar_t *>(str));
}

static
inline
ep_char8_t *
ep_rt_utf16_to_utf8_string (
	const ep_char16_t *str,
	size_t len)
{
	COUNT_T length = WszWideCharToMultiByte (CP_UTF8, 0, reinterpret_cast<LPCWSTR>(str), static_cast<int>(len), NULL, 0, NULL, NULL);
	if (length == 0)
		return NULL;

	if (static_cast<int>(len) != -1)
		length += 1;

	ep_char8_t *str_utf8 = reinterpret_cast<ep_char8_t *>(malloc (length));
	if (!str_utf8)
		return NULL;

	length = WszWideCharToMultiByte (CP_UTF8, 0, reinterpret_cast<LPCWSTR>(str), static_cast<int>(len), reinterpret_cast<LPSTR>(str_utf8), length, NULL, NULL);
	if (length == 0) {
		free (str_utf8);
		return NULL;
	}
	str_utf8 [length - 1] = 0;
	return str_utf8;
}

static
inline
void
ep_rt_utf16_string_free (ep_char16_t *str)
{
	if (str)
		free (str);
}

static
inline
wchar_t *
ep_rt_utf8_to_wcs_string (
	const ep_char8_t *str,
	size_t len)
{
#if WCHAR_MAX == 0xFFFF
	return reinterpret_cast<wchar_t *>(ep_rt_utf8_to_utf16_string (str, len));
#else
	//TODO: This needs a utf8 to utf32 conversion routine.
	EP_ASSERT (!"Can not reach here");
#endif
}

static
inline
ep_char8_t *
ep_rt_wcs_to_utf8_string (
	const wchar_t *str,
	size_t len)
{
#if WCHAR_MAX == 0xFFFF
	return ep_rt_utf16_to_utf8_string (reinterpret_cast<const ep_char16_t *>(str), len);
#else
	//TODO: This needs a utf32 to utf8 conversion routine.
	EP_ASSERT (!"Can not reach here");
#endif
}

static
inline
void
ep_rt_wcs_string_free (wchar_t *str)
{
	if (str)
		free (str);
}

static
inline
const ep_char8_t *
ep_rt_managed_command_line_get (void)
{
	EP_ASSERT (!"Can not reach here");
	return NULL;
}

static
const ep_char8_t *
ep_rt_diagnostics_command_line_get (void)
{
	// TODO: Real lazy init implementation.
	diagnostics_command_line_lazy_init ();
	return diagnostics_command_line_get ();
}

/*
 * Thread.
 */

class EventPipeCoreCLRThreadHolderTLS {
public:
	EventPipeCoreCLRThreadHolderTLS ()
	{
		;
	}

	~EventPipeCoreCLRThreadHolderTLS ()
	{
		if (m_threadHolder)
		{
			ep_thread_holder_free (m_threadHolder);
			m_threadHolder = NULL;
		}
	}

	static inline EventPipeThreadHolder * getThreadHolder ()
	{
		return g_threadHolderTLS.m_threadHolder;
	}

	static inline void setThreadHolder (EventPipeThreadHolder *threadHolder)
	{
		if (g_threadHolderTLS.m_threadHolder)
		{
			ep_thread_holder_free (g_threadHolderTLS.m_threadHolder);
			g_threadHolderTLS.m_threadHolder = NULL;
		}
		g_threadHolderTLS.m_threadHolder = threadHolder;
	}

private:
	EventPipeThreadHolder *m_threadHolder;
	static thread_local EventPipeCoreCLRThreadHolderTLS g_threadHolderTLS;
};

static
inline
void
ep_rt_thread_setup (void)
{
	SetupThread ();
}

static
inline
EventPipeThread *
ep_rt_thread_get (void)
{
	EventPipeThreadHolder *thread_holder = EventPipeCoreCLRThreadHolderTLS::getThreadHolder ();
	return thread_holder ? ep_thread_holder_get_thread (thread_holder) : NULL;
}

static
inline
EventPipeThread *
ep_rt_thread_get_or_create (void)
{
	EventPipeThreadHolder *thread_holder = EventPipeCoreCLRThreadHolderTLS::getThreadHolder ();
	if (!thread_holder) {
		thread_holder = thread_holder_alloc_func ();
		EventPipeCoreCLRThreadHolderTLS::setThreadHolder (thread_holder);
	}
	return ep_thread_holder_get_thread (thread_holder);
}

static
inline
ep_rt_thread_handle_t
ep_rt_thread_get_handle (void)
{
	return GetThread ();
}

static
inline
size_t
ep_rt_thread_get_id (ep_rt_thread_handle_t thread_handle)
{
	EP_ASSERT (thread_handle != NULL);
	return thread_handle->GetOSThreadId64 ();
}

static
inline
bool
ep_rt_thread_has_started (ep_rt_thread_handle_t thread_handle)
{
	return thread_handle != NULL && thread_handle->HasStarted ();
}

static
inline
ep_rt_thread_activity_id_handle_t
ep_rt_thread_get_activity_id_handle (void)
{
	return GetThread ();
}

static
inline
const uint8_t *
ep_rt_thread_get_activity_id_cref (ep_rt_thread_activity_id_handle_t activity_id_handle)
{
	EP_ASSERT (activity_id_handle != NULL);
	return reinterpret_cast<const uint8_t *>(activity_id_handle->GetActivityId ());
}

static
inline
void
ep_rt_thread_get_activity_id (
	ep_rt_thread_activity_id_handle_t activity_id_handle,
	uint8_t *activity_id,
	uint32_t activity_id_len)
{
	EP_ASSERT (activity_id_handle != NULL);
	EP_ASSERT (activity_id != NULL);
	EP_ASSERT (activity_id_len == EP_ACTIVITY_ID_SIZE);

	memcpy (activity_id, ep_rt_thread_get_activity_id_cref (activity_id_handle), EP_ACTIVITY_ID_SIZE);
}

static
inline
void
ep_rt_thread_set_activity_id (
	ep_rt_thread_activity_id_handle_t activity_id_handle,
	const uint8_t *activity_id,
	uint32_t activity_id_len)
{
	EP_ASSERT (activity_id_handle != NULL);
	EP_ASSERT (activity_id != NULL);
	EP_ASSERT (activity_id_len == EP_ACTIVITY_ID_SIZE);

	activity_id_handle->SetActivityId (reinterpret_cast<LPCGUID>(activity_id));
}

#undef EP_YIELD_WHILE
#define EP_YIELD_WHILE(condition) YIELD_WHILE(condition)

/*
 * ThreadSequenceNumberMap.
 */

EP_RT_DEFINE_HASH_MAP(thread_sequence_number_map, ep_rt_thread_sequence_number_hash_map_t, EventPipeThreadSessionState *, uint32_t)
EP_RT_DEFINE_HASH_MAP_ITERATOR(thread_sequence_number_map, ep_rt_thread_sequence_number_hash_map_t, ep_rt_thread_sequence_number_hash_map_iterator_t, EventPipeThreadSessionState *, uint32_t)

/*
 * Volatile.
 */

static
inline
uint32_t
ep_rt_volatile_load_uint32_t (const volatile uint32_t *ptr)
{
	return VolatileLoad<uint32_t> ((const uint32_t *)ptr);
}

static
inline
uint32_t
ep_rt_volatile_load_uint32_t_without_barrier (const volatile uint32_t *ptr)
{
	return VolatileLoadWithoutBarrier<uint32_t> ((const uint32_t *)ptr);
}

static
inline
void
ep_rt_volatile_store_uint32_t (
	volatile uint32_t *ptr,
	uint32_t value)
{
	VolatileStore<uint32_t> ((uint32_t *)ptr, value);
}

static
inline
void
ep_rt_volatile_store_uint32_t_without_barrier (
	volatile uint32_t *ptr,
	uint32_t value)
{
	VolatileStoreWithoutBarrier<uint32_t>((uint32_t *)ptr, value);
}

static
inline
uint64_t
ep_rt_volatile_load_uint64_t (const volatile uint64_t *ptr)
{
	return VolatileLoad<uint64_t> ((const uint64_t *)ptr);
}

static
inline
uint64_t
ep_rt_volatile_load_uint64_t_without_barrier (const volatile uint64_t *ptr)
{
	return VolatileLoadWithoutBarrier<uint64_t> ((const uint64_t *)ptr);
}

static
inline
void
ep_rt_volatile_store_uint64_t (
	volatile uint64_t *ptr,
	uint64_t value)
{
	VolatileStore<uint64_t> ((uint64_t *)ptr, value);
}

static
inline
void
ep_rt_volatile_store_uint64_t_without_barrier (
	volatile uint64_t *ptr,
	uint64_t value)
{
	VolatileStoreWithoutBarrier ((uint64_t *)ptr, value);
}

static
inline
int64_t
ep_rt_volatile_load_int64_t (const volatile int64_t *ptr)
{
	return VolatileLoad<int64_t> ((int64_t *)ptr);
}

static
inline
int64_t
ep_rt_volatile_load_int64_t_without_barrier (const volatile int64_t *ptr)
{
	return VolatileLoadWithoutBarrier<int64_t> ((int64_t *)ptr);
}

static
inline
void
ep_rt_volatile_store_int64_t (
	volatile int64_t *ptr,
	int64_t value)
{
	VolatileStore<int64_t> ((int64_t *)ptr, value);
}

static
inline
void
ep_rt_volatile_store_int64_t_without_barrier (
	volatile int64_t *ptr,
	int64_t value)
{
	VolatileStoreWithoutBarrier ((int64_t *)ptr, value);
}

static
inline
void *
ep_rt_volatile_load_ptr (volatile void **ptr)
{
	return VolatileLoad<void *> ((void **)ptr);
}

static
inline
void *
ep_rt_volatile_load_ptr_without_barrier (volatile void **ptr)
{
	return VolatileLoadWithoutBarrier<void *> ((void **)ptr);
}

static
inline
void
ep_rt_volatile_store_ptr (
	volatile void **ptr,
	void *value)
{
	VolatileStore<void *> ((void **)ptr, value);
}

static
inline
void
ep_rt_volatile_store_ptr_without_barrier (
	volatile void **ptr,
	void *value)
{
	VolatileStoreWithoutBarrier<void *> ((void **)ptr, value);
}

#endif /* ENABLE_PERFTRACING */
#endif /* __EVENTPIPE_RT_CORECLR_H__ */
