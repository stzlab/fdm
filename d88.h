/*
 * Definition for D88 disk image file
 *
 * Copyright (c) 2021 stzlab
 *
 * This software is released under the MIT License, see LICENSE.
 */

/* Media Typee */
#define D88_TYPE_2D		0x00
#define D88_TYPE_2DD		0x10
#define D88_TYPE_2HD		0x20
#define D88_TYPE_1D		0x30
#define D88_TYPE_1DD		0x40

/* Disk Write Protect */
#define D88_PROTECT_OFF		0x00		/* Disk No Write Protect */
#define D88_PROTECT_ON		0x10		/* Disk Write Protected */

/* PC-9801 INT 1BH Status */
#define D88_STATUS_CM		0x10		/* Control Mark */
#define D88_STATUS_WP		0x10		/* Write Protect (Sense Command) */
#define D88_STATUS_DB		0x20		/* DMA Boundary */
#define D88_STATUS_EN		0x30		/* End of Cylinder */
#define D88_STATUS_EC		0x40		/* Equipment Check */
#define D88_STATUS_OR		0x50		/* OverRun */
#define D88_STATUS_NR		0x60		/* Not Ready */
#define D88_STATUS_NW		0x70		/* Not Writable */
#define D88_STATUS_ER		0x80		/* ERror */
#define D88_STATUS_TO		0x90		/* TimeOut */
#define D88_STATUS_DE		0xA0		/* DataError(ID) */
#define D88_STATUS_DD		0xB0		/* DataError(Data) */
#define D88_STATUS_ND		0xC0		/* No Data */
#define D88_STATUS_BC		0xD0		/* Bad Cylinder */
#define D88_STATUS_MA		0xE0		/* Missing Address mark(ID) */
#define D88_STATUS_MD		0xF0		/* Missing Address mark(Data) */

/* Encode Type */
#define D88_ENCODE_MFM		0x00
#define D88_ENCODE_FM		0x40

/* Data Address Mark */
#define D88_DAM_NORMAL		0x00
#define D88_DAM_DELETED		0x10

struct __attribute__ ((__packed__)) D88_HEADER {
	unsigned char szTitle[17];
	unsigned char abReserved[9];
	unsigned char bWriteProtect;
	unsigned char bMediaType;
	unsigned int dwDiskSize;
	unsigned int adwTrackOffsets[164];
};

struct __attribute__ ((__packed__)) D88_SECTOR {
	unsigned char c;
	unsigned char h;
	unsigned char r;
	unsigned char n;
	unsigned short wSectors;
	unsigned char bEncoding;
	unsigned char bDataAddressMark;
	unsigned char bStatus;
	unsigned char abReserved[5];
	unsigned short wLength;
};
