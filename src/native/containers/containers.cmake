set (SHARED_CONTAINER_SOURCES "")
set (SHARED_CONTAINER_HEADERS "")

list(APPEND SHARED_CONTAINER_SOURCES
    dn-allocator.c
    dn-array.c
    dn-byte-array.c
    dn-ptr-array.c
    dn-slist.c
    dn-list.c
    dn-queue.c
)

list(APPEND SHARED_CONTAINER_HEADERS
    dn-allocator.h
    dn-array.h
    dn-array-ex.h
    dn-byte-array.h
    dn-byte-array-ex.h
    dn-malloc.h
    dn-ptr-array.h
    dn-ptr-array-ex.h
    dn-slist.h
    dn-slist-ex.h
    dn-list.h
    dn-list-ex.h
    dn-queue.h
    dn-queue-ex.h
    dn-sort-frag.h
    dn-utils.h
)
