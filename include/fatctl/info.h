#ifndef _FATCTL_INFO_H_
#define _FATCTL_INFO_H_

/* BEGIN_DECLS */   
#ifdef __cplusplus
extern "C" {
#endif

/* See: https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fsa/7f79f01f-a025-42f3-a918-7930db28bbed */
struct fatctl_fsinfo {
    unsigned blksize;   /* Logical sector, typically 512 bytes. used as a standard storage size unit. */
    unsigned blksize_medium; /* Physical sector size -- depends on the underlying storage technology. */
    unsigned blksize_memory; /* The smaller of physical sector size (see above) and memory page size. */
    /* For most reasonable purposes, the use of `blksize_medium` is advised (performance, overwrites) */
};

int fatctl_fsinfo_query(int fd, struct fatctl_fsinfo* info);

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_INFO_H_ */