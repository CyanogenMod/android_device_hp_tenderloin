/*
 * This is a userspace power management driver for the digitizer in the HP
 * Touchpad to turn the digitizer on and off.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 *
 * Copyright (c) 2012 CyanogenMod Touchpad Project.
 *
 *
 */

// Maximum number of times to retry powering on the digitizer
#define MAX_DIGITIZER_RETRY 3

void touchscreen_power(int enable);

void init_digitizer_fd(void);
#ifndef __FD_SET
#define __FD_SET(fd, fdsetp)   (((fd_set *)(fdsetp))->fds_bits[(fd) >> 5] |= (1<<((fd) & 31)))
#endif

#ifndef __FD_CLR
#define __FD_CLR(fd, fdsetp)   (((fd_set *)(fdsetp))->fds_bits[(fd) >> 5] &= ~(1<<((fd) & 31)))
#endif

#ifndef __FD_ISSET
#define __FD_ISSET(fd, fdsetp)   ((((fd_set *)(fdsetp))->fds_bits[(fd) >> 5] & (1<<((fd) & 31))) != 0)
#endif

#ifndef __FD_ZERO
#define __FD_ZERO(fdsetp)   (memset (fdsetp, 0, sizeof (*(fd_set *)(fdsetp))))
#endif

#ifndef NFDBITS
#define NFDBITS __NFDBITS
#endif

#ifndef FD_SETSIZE
#define FD_SETSIZE __FD_SETSIZE
#endif

#ifndef FD_SET
#define FD_SET(fd,fdsetp) __FD_SET(fd,fdsetp)
#endif

#ifndef FD_CLR
#define FD_CLR(fd,fdsetp) __FD_CLR(fd,fdsetp)
#endif

#ifndef FD_ISSET
#define FD_ISSET(fd,fdsetp) __FD_ISSET(fd,fdsetp)
#endif

#ifndef FD_ZERO
#define FD_ZERO(fdsetp) __FD_ZERO(fdsetp)
#endif
