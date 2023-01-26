
#ifndef __DN_MALLOC_H__
#define __DN_MALLOC_H__

#ifndef DN_USE_CUSTOM_MALLOC_H
#include "dn-utils.h"

#include <stdlib.h>
#include <memory.h>

static inline void
dn_free (void *block)
{
	free (block);
}

static inline void *
dn_malloc (size_t size)
{
	if (!size)
		return malloc (1);

	return malloc (size);
}

static inline void *
dn_realloc (void *block, size_t size)
{
	if (!size)
		return realloc (block, 1);

	return realloc (block, size);
}

static inline void *
dn_malloc0 (size_t size)
{
	void * const block = dn_malloc (size);
	if (block)
		memset (block, 0, size);
	return block;
}
#else
#include DN_USE_CUSTOM_MALLOC_H
#endif

#define dn_new(type,size) \
	((type *) dn_malloc (sizeof (type) * (size)))

#define dn_new0(type,size) \
	((type *) dn_malloc0 (sizeof (type)* (size)))
#endif /* __DN_MALLOC_H__ */
