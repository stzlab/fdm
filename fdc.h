/*
 * Definition for FDC raw command function
 *
 * Copyright (c) 2021 stzlab
 *
 * This software is released under the MIT License, see LICENSE.
 */

/* Command complete with interrupt signal */
#define FDC_CMD_READ_DATA		0x06	/* Read sector data */
#define FDC_CMD_READ_TRACK		0x02	/* Read sector data entire data field continuously */
#define FDC_CMD_READ_ID			0x0A	/* Prints the header of a sector */
#define FDC_CMD_READ_DELETED_DATA	0x0C	/* Read sector data with deleted address mark */
#define FDC_CMD_WRITE_DATA		0x05	/* Write sector data */
#define FDC_CMD_FORMAT_TRACK		0x0D	/* Format track */
#define FDC_CMD_WRITE_DELETED_DATA	0x09	/* Write sector data with deleted address mark */
#define FDC_CMD_VERIFY			0x16	/* Verify sector (no data transfer) */
#define FDC_CMD_SEEK			0x0F	/* Seek track */
#define FDC_CMD_RECALIBRATE		0x07	/* Head retracted to track 0 */

/* Command complete without interrupt signal */
#define FDC_CMD_SENSE_INTERRUPT		0x08	/* Sense interrupt status */
#define FDC_CMD_SENSE_DRIVE		0x04	/* Sense drive status */
#define FDC_CMD_SPECIFY			0x03	/* Specify SRT, HUT HLT, ND */
#define FDC_CMD_VERSION			0x10	/* Get version code */
#define FDC_CMD_PART_ID			0x18	/* Get partid */
#define FDC_CMD_SCAN_EQUAL		0x10	/* Scan equal */
#define FDC_CMD_SCAN_LOW_OR_EQUAL	0x19	/* Scan low or equal */
#define FDC_CMD_SCAN_HIGH_OR_EQUAL	0x1D	/* Scan high or euqal */

/* Command option flag */
#define FDC_OPT_NONE	0x00
#define FDC_OPT_MT	0x80	/* Multi-Track */
#define FDC_OPT_MFM	0x40	/* MFM or FM Mode */
#define FDC_OPT_SK	0x20	/* Skip deleted data address mark */

/* Select target */
#define FDC_SEL_US0	0x01	/* Unit Select 0 */
#define FDC_SEL_US1	0x02	/* Unit Select 1 */
#define FDC_SEL_HS	0x04	/* Head Select */

/* Bits of FD_ST0 */
#define FDC_ST0_IC	0xC0	/* Interrupt Code */
#define FDC_ST0_SE	0x20	/* Seek End */
#define FDC_ST0_EC	0x10	/* Equipment Check */
#define FDC_ST0_NR	0x08	/* Not Ready */
#define FDC_ST0_HS	0x04	/* Head Select */
#define FDC_ST0_US1	0x02	/* Unit Select 1 */
#define FDC_ST0_US0	0x01	/* Unit Select 0 */

/* Bits of FD_ST1 */
#define FDC_ST1_EN	0x80	/* End of Cylinder */
#define FDC_ST1_DE	0x20	/* Data Error */
#define FDC_ST1_OR	0x10	/* OverRun */
#define FDC_ST1_ND	0x04	/* No Data */
#define FDC_ST1_NW	0x02	/* Not Writable */
#define FDC_ST1_MA	0x01	/* Missing Address Mark */

/* Bits of FD_ST2 */
#define FDC_ST2_CM	0x40	/* Control Mark */
#define FDC_ST2_DD	0x20	/* Data Error in Data Field */
#define FDC_ST2_WC	0x10	/* Wrong Cylinder */
#define FDC_ST2_SH	0x08	/* Scan Equal Hit */
#define FDC_ST2_SN	0x04	/* Scan Not Satisfied */
#define FDC_ST2_BC	0x02	/* Bad Cylinder */
#define FDC_ST2_MD	0x01	/* Missing Address Mark in Data Field */

/* Bits of FD_ST3 */
#define FDC_ST3_FT	0x80	/* Fault */
#define FDC_ST3_WP	0x40	/* Write Protect */
#define FDC_ST3_RY	0x20	/* Ready */
#define FDC_ST3_T0	0x10	/* Track Zero */
#define FDC_ST3_TS	0x08	/* Two Side */
#define FDC_ST3_HS	0x04	/* Head Select */
#define FDC_ST3_US1	0x02	/* Unit Select 1 */
#define FDC_ST3_US0	0x01	/* Unit Select 0 */

#define NSECSIZE(num)	(128 << (num < 8 ? num : 8))

struct __attribute__ ((__packed__)) fdc_res_cmd {
	unsigned char st0;
	unsigned char st1;
	unsigned char st2;
	unsigned char c;
	unsigned char h;
	unsigned char r;
	unsigned char n;
};

struct __attribute__ ((__packed__)) fdc_res_intr {
	unsigned char st0;
	unsigned char pcn;
};

struct __attribute__ ((__packed__)) fdc_res_sens {
	unsigned char st3;
};

struct __attribute__ ((__packed__)) fdc_sector_id {
	unsigned char c;
	unsigned char h;
	unsigned char r;
	unsigned char n;
};

int fdcInit(void);
void fdcExit(void);
void fdcSetDataRate(unsigned char drate);

int fdcSenseDrive(unsigned char unit, struct fdc_res_sens *res);
int fdcRecalibrate(unsigned char unit, struct fdc_res_intr *res);
int fdcSeek(unsigned char unit, unsigned char cylinder, struct fdc_res_intr *res);
int fdcReadId(unsigned char unit, unsigned char head, unsigned char cmdopt,
	struct fdc_res_cmd *res);
int fdcReadData(unsigned char unit, unsigned char head, unsigned char cmdopt,
	struct fdc_sector_id *id, int deleted, unsigned char *datBuf, struct fdc_res_cmd *res);
int fdcVerify(unsigned char unit, unsigned char head, unsigned char cmdopt,
	struct fdc_sector_id *id, struct fdc_res_cmd *res);
int fdcWriteData(unsigned char unit, unsigned char head, unsigned char cmdopt,
	struct fdc_sector_id *id, int deleted, unsigned char *datBuf, struct fdc_res_cmd *res);
int fdcFormat(unsigned char unit, unsigned char head, unsigned char cmdopt,
	unsigned char sizeN, unsigned char countR, unsigned char formatGpl, unsigned char dataPtn,
	struct fdc_sector_id *idBuf, struct fdc_res_cmd *res);
int fdcReadDiag(unsigned char unit, unsigned char head, unsigned char cmdopt,
	struct fdc_sector_id *id, unsigned char sizeN, unsigned char *datBuf, struct fdc_res_cmd *res);