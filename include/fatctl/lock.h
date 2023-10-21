#ifndef _FATCTL_LOCK_H_
#define _FATCTL_LOCK_H_

/* BEGIN_DECLS */   
#ifdef __cplusplus
extern "C" {
#endif

/* TODO struct flock { ... } */
int flock(int fd, int operation);

/* END_DECLS */
#ifdef __cplusplus
}
#endif

#endif /* _FATCTL_LOCK_H_ */
