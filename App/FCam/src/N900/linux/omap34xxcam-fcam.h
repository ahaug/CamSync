#ifndef OMAP34XXCAM_FCAM_H
#define OMAP34XXCAM_FCAM_H

#include <linux/videodev2.h>

/* VIDIOCS, for kernel space and user space */
#define VIDIOC_FCAM_INSTALL _IO('V', BASE_VIDIOC_PRIVATE + 25)
#define VIDIOC_FCAM_WAIT_FOR_HS_VS _IOWR('V', BASE_VIDIOC_PRIVATE + 26, struct timeval)

/* Extra sensor controls */
#define V4L2_CID_FRAME_TIME (V4L2_CTRL_CLASS_CAMERA | 0x10ff)
#define V4L2_CID_GAIN_EXACT (V4L2_CTRL_CLASS_CAMERA | 0x10fe)

/* Kernel-side structures */

#ifdef __KERNEL__

struct omap34xxcam_fcam_client {
	/* The file handle of the fcam process (or NULL if there isn't one) */
	struct file *file; 

	/* A semaphore to use for waiting for HS_VS */
	struct semaphore sem; 

	/* Timestamp of most recent HS_VS */
	struct timeval timestamp;

	/* TODO: Add a next pointer, to make a linked list in order to handle multiple fcam clients */
};

#endif

#endif
