/*
 * System API: errno — see system/*.c for implementation.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __SYSTEM_ERRNO_H__
#define __SYSTEM_ERRNO_H__

/* Semantics of these constants is similar to their Unix counterparts. */

#define ENOENT -1  /* No such file */
#define EBADF -2   /* Bad file handle */
#define EINVAL -3  /* Invalid argument */
#define ENOTSUP -4 /* Operation not supported */
#define EIO -5     /* Input/output error */

#endif /* !__SYSTEM_ERRNO_H__ */
