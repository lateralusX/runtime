set (SHARED_CONTAINER_SOURCES "")
set (SHARED_CONTAINER_HEADERS "")

list(APPEND SHARED_CONTAINER_SOURCES
    dn-allocator.c
    dn-vector.c
    dn-fwd-list.c
    dn-list.c
    dn-queue.c
    dn-unordered-map.c
)

list(APPEND SHARED_CONTAINER_HEADERS
    dn-allocator.h
    dn-vector.h
    dn-vector-t.h
    dn-malloc.h
    dn-fwd-list.h
    dn-list.h
    dn-queue.h
    dn-unordered-map.h
    dn-unordered-map-ex.h
    dn-sort-frag.inc
    dn-utils.h
)
