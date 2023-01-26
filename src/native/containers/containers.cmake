set (SHARED_CONTAINER_SOURCES "")
set (SHARED_CONTAINER_HEADERS "")

list(APPEND SHARED_CONTAINER_SOURCES
    dn-allocator.c
    dn-vector.c
    dn-slist.c
    dn-list.c
    dn-queue.c
    dn-unordered-map.c
)

list(APPEND SHARED_CONTAINER_HEADERS
    dn-allocator.h
    dn-vector.h
    dn-malloc.h
    dn-slist.h
    dn-slist-ex.h
    dn-list.h
    dn-list-ex.h
    dn-queue.h
    dn-queue-ex.h
    dn-unordered-map.h
    dn-unordered-map-ex.h
    dn-sort-frag.h
    dn-utils.h
)
