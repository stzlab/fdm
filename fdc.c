/*
 * Implementation for FDC raw command function
 *
 * Copyright (c) 2021 stzlab
 *
 * This software is released under the MIT License, see LICENSE.
 */

#include "fdc.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <sys/ioctl.h>
#include <linux/fd.h>
#include <linux/fdreg.h>

#define FD_DEVICE	"/dev/fd0"
#define GPL_SKIP	8

int FdFdc;
unsigned char DataRate;

int fdcInit(void)
{
	int parm;
	
	if ((FdFdc = open(FD_DEVICE, O_ACCMODE | O_NDELAY)) < 0)	{
		perror("fdcInit(open)");
		return -1;
	}
	
	parm = FD_RESET_ALWAYS;
	if (ioctl(FdFdc, FDRESET, &parm) < 0) {
		perror("fdcInit(FDRESET)");
		return -1;
	}
	
	return 0;
}

void fdcExit(void)
{
	close(FdFdc);
}

void fdcSetDataRate(unsigned char datarate)
{
	DataRate = datarate;
}

int fdcSenseDrive(unsigned char unit, struct fdc_res_sens *res)
{
	struct floppy_raw_cmd fdc;
	
	fdc.cmd[0] = FDC_CMD_SENSE_DRIVE;
	fdc.cmd[1] = unit;
	fdc.cmd_count = 2;
	fdc.flags = 0;
	
	if (ioctl(FdFdc, FDRAWCMD, &fdc) < 0) {
		perror("fdcSenseDrive");
		return -1;
	}
	
	memcpy(res, fdc.reply, sizeof(*res));
	return 0;
}

int fdcRecalibrate(unsigned char unit, struct fdc_res_intr *res)
{
	struct floppy_raw_cmd fdc;
	
	fdc.cmd[0] = FDC_CMD_RECALIBRATE;
	fdc.cmd[1] = unit;
	fdc.cmd_count = 2;
	fdc.flags = FD_RAW_INTR;
	
	if (ioctl(FdFdc, FDRAWCMD, &fdc) < 0) {
		perror("fdcRecalibrate");
		return -1;
	}
	
	memcpy(res, fdc.reply, sizeof(*res));
	return 0;
}

int fdcSeek(unsigned char unit, unsigned char cylinder, struct fdc_res_intr *res)
{
	struct floppy_raw_cmd fdc;
	
	fdc.cmd[0] = FDC_CMD_SEEK;
	fdc.cmd[1] = unit;
	fdc.cmd[2] = cylinder;
	fdc.cmd_count = 3;
	fdc.flags = FD_RAW_INTR;
	
	if (ioctl(FdFdc, FDRAWCMD, &fdc) < 0) {
		perror("fdcSeek");
		return -1;
	}
	
	memcpy(res, fdc.reply, sizeof(*res));
	return 0;
}

int fdcReadId(unsigned char unit, unsigned char head, unsigned char cmdopt, struct fdc_res_cmd *res)
{
	struct floppy_raw_cmd fdc;
	
	fdc.cmd[0] = FDC_CMD_READ_ID | cmdopt;
	fdc.cmd[1] = unit | (head << 2);
	fdc.cmd_count = 2;
	fdc.flags = FD_RAW_INTR;
	fdc.rate = DataRate;
	
	if (ioctl(FdFdc, FDRAWCMD, &fdc) < 0) {
		perror("fdcReadId");
		return -1;
	}
	
	memcpy(res, fdc.reply, sizeof(*res));
	return 0;
}

int fdcReadData(unsigned char unit, unsigned char head, unsigned char cmdopt,
	struct fdc_sector_id *id, int deleted, unsigned char *datPtr, struct fdc_res_cmd *res)
{
	struct floppy_raw_cmd fdc;
	
	fdc.cmd[0] = (deleted ? FDC_CMD_READ_DELETED_DATA : FDC_CMD_READ_DATA) | cmdopt;
	fdc.cmd[1] = unit | (head << 2);
	fdc.cmd[2] = id->c;
	fdc.cmd[3] = id->h;
	fdc.cmd[4] = id->r;
	fdc.cmd[5] = id->n;
	fdc.cmd[6] = 1;
	fdc.cmd[7] = GPL_SKIP;
	fdc.cmd[8] = 0xff;
	fdc.cmd_count = 9;
	fdc.flags = FD_RAW_INTR | FD_RAW_READ;
	fdc.data = datPtr;
	fdc.length = NSECSIZE(id->n);
	fdc.rate = DataRate;
	
	if (ioctl(FdFdc, FDRAWCMD, &fdc) < 0) {
		perror("fdcReadData");
		return -1;
	}
	
	memcpy(res, fdc.reply, sizeof(*res));
	return 0;
}

int fdcVerify(unsigned char unit, unsigned char head, unsigned char cmdopt,
	struct fdc_sector_id *id, struct fdc_res_cmd *res)
{
	struct floppy_raw_cmd fdc;
	
	fdc.cmd[0] = FDC_CMD_VERIFY | cmdopt;
	fdc.cmd[1] = unit | (head << 2);
	fdc.cmd[2] = id->c;
	fdc.cmd[3] = id->h;
	fdc.cmd[4] = id->r;
	fdc.cmd[5] = id->n;
	fdc.cmd[6] = 1;
	fdc.cmd[7] = GPL_SKIP;
	fdc.cmd[8] = 0xff;
	fdc.cmd_count = 9;
	fdc.flags = FD_RAW_INTR;
	fdc.rate = DataRate;
	
	if (ioctl(FdFdc, FDRAWCMD, &fdc) < 0) {
		perror("fdcVerify");
		return -1;
	}
	
	memcpy(res, fdc.reply, sizeof(*res));
	return 0;
}

int fdcWriteData(unsigned char unit, unsigned char head, unsigned char cmdopt,
	struct fdc_sector_id *id, int deleted, unsigned char *datPtr, struct fdc_res_cmd *res)
{
	struct floppy_raw_cmd fdc;
	
	fdc.cmd[0] = (deleted ? FDC_CMD_WRITE_DELETED_DATA : FDC_CMD_WRITE_DATA) | cmdopt;
	fdc.cmd[1] = unit | (head << 2);
	fdc.cmd[2] = id->c;
	fdc.cmd[3] = id->h;
	fdc.cmd[4] = id->r;
	fdc.cmd[5] = id->n;
	fdc.cmd[6] = 1;
	fdc.cmd[7] = GPL_SKIP;
	fdc.cmd[8] = 0xff;
	fdc.cmd_count = 9;
	fdc.flags = FD_RAW_INTR | FD_RAW_WRITE;
	fdc.data = datPtr;
	fdc.length = NSECSIZE(id->n);
	fdc.rate = DataRate;
	
	if (ioctl(FdFdc, FDRAWCMD, &fdc) < 0) {
		perror("fdcWriteData");
		return -1;
	}
	
	memcpy(res, fdc.reply, sizeof(*res));
	return 0;
}

int fdcFormat(unsigned char unit, unsigned char head, unsigned char cmdopt,
	unsigned char sizeN, unsigned char countR, unsigned char formatGpl, unsigned char dataPtn, struct fdc_sector_id *idBuf,
	struct fdc_res_cmd *res)
{
	struct floppy_raw_cmd fdc;
	
	fdc.cmd[0] = FDC_CMD_FORMAT_TRACK | cmdopt;
	fdc.cmd[1] = unit | (head << 2);
	fdc.cmd[2] = sizeN;
	fdc.cmd[3] = countR;
	fdc.cmd[4] = formatGpl;
	fdc.cmd[5] = dataPtn;
	fdc.cmd_count = 6;
	fdc.flags = FD_RAW_INTR | FD_RAW_WRITE;
	fdc.data = idBuf;
	fdc.length = countR * 4; /* C H R N */
	fdc.rate = DataRate;
	
	if (ioctl(FdFdc, FDRAWCMD, &fdc) < 0) {
		perror("fdcFormat");
		return -1;
	}
	
	memcpy(res, fdc.reply, sizeof(*res));
	return 0;
}

int fdcReadDiag(unsigned char unit, unsigned char head, unsigned char cmdopt,
	struct fdc_sector_id *id, unsigned char sizeN, unsigned char *datPtr, struct fdc_res_cmd *res)
{
	struct floppy_raw_cmd fdc;
	
	fdc.cmd[0] = FDC_CMD_READ_TRACK | cmdopt;
	fdc.cmd[1] = unit | (head << 2);
	fdc.cmd[2] = id->c;
	fdc.cmd[3] = id->h;
	fdc.cmd[4] = id->r;
	fdc.cmd[5] = id->n;
	fdc.cmd[6] = 1;
	fdc.cmd[7] = GPL_SKIP;
	fdc.cmd[8] = 0xff;
	fdc.cmd_count = 9;
	fdc.flags = FD_RAW_INTR | FD_RAW_READ;
	fdc.data = datPtr;
	fdc.length = NSECSIZE(sizeN);
	fdc.rate = DataRate;
	
	if (ioctl(FdFdc, FDRAWCMD, &fdc) < 0) {
		perror("fdcReadDiag");
		return -1;
	}
	
	memcpy(res, fdc.reply, sizeof(*res));
	return 0;
}
