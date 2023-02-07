// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#ifndef __DN_MALLOC_H__
#define __DN_MALLOC_H__

#ifndef DN_USE_CUSTOM_MALLOC_H
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
#else
#include DN_USE_CUSTOM_MALLOC_H
#endif
#endif /* __DN_MALLOC_H__ */
