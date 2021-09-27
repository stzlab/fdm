/*
 * Floppy Disk Management Tool (dump/restore) for Linux
 *
 * Copyright (c) 2021 stzlab
 *
 * This software is released under the MIT License, see LICENSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fdc.h"
#include "d88.h"

#define FD_DEVNUM			0
#define MAXTRKLEN			12500		/* Maximum length of track(2HD@300rpm, Unformatted) */
#define MAXSECNUM			66			/* Maximum number of sectors(2HD@300rpm, 128 bytes/sector,No GAP3,No GAP4b) */

#define GETENCFDC(enc)		(enc == D88_ENCODE_MFM) ? FDC_OPT_MFM : FDC_OPT_NONE
#define ISDAMDEL(dam)		(dam == D88_DAM_DELETED)

static int verbose = 0;

enum {
	OPT_2D  = 0x00,
	OPT_2DD,
	OPT_2HD,
	OPT_1D,
	OPT_1DD
};
char *const token_media[] = {
	[OPT_2D]  = "2D",
	[OPT_2DD] = "2DD",
	[OPT_2HD] = "2HD",
	[OPT_1D]  = "1D",
	[OPT_1DD] = "1DD",
	NULL
};

enum {
	OPT_OFF  = 0x00,
	OPT_ON
};
char *const token_switch[] = {
	[OPT_OFF] = "off",
	[OPT_ON]  = "on",
	NULL
};

void usage()
{
	printf("fdm v1.0\n");
	printf("Usage: fdimage [dump|restore] <filename> <options>\n");
	printf("  -h              : show usage\n");
	printf("  -v              : enable verbose mode\n");
	printf("  -m<type>        : media type(2D/2DD/2HD/1D/1DD) *default 2HD\n");
	printf("  -w[on|off]      : overwrite write protect flag\n");
	printf("  -C<start>-<end> : overwrite cylinder range\n");
	printf("  -S<side>        : overwrite side select (0/1/2)\n");
	printf("  -M<multiplier>  : overwrite physical drive seek mult multiplier\n");
	printf("  -D<rpm>,<kbps>  : overwrite drive parameter\n");
	printf("  -R<drate>       : overwrite drate register\n");
}

int calcUnformatSizeNum(int trklen, int enc)
{
	int num = 0;
	
	switch (enc) {
		case D88_ENCODE_FM:
			trklen = trklen / 2;
		case D88_ENCODE_MFM:
			while (trklen > NSECSIZE(num)) {
				num++;
			}
			break;
		default:
			num = 8;
			break;
	}
	return num;
}

int calcFormatGapLen(int trklen, int num, int sects, int enc)
{
	int gaplen = 0;
	
	switch (enc) {
		case D88_ENCODE_MFM:
			/* Calculated with GAP4b as 128 bytes */
			gaplen = (trklen - (274 + (62 + NSECSIZE(num)) * sects)) / sects;
			/* GAP3 is small, calculate with GAP4b as 0 */
			if (gaplen < 22) {
				gaplen = (trklen - (146 + (62 + NSECSIZE(num)) * sects)) / sects;
			}
			/* GAP3 is still small, calculate with GAP4a and GAP4b as 0 */
			if (gaplen < 22) {
				gaplen = (trklen - (66 + (62 + NSECSIZE(num)) * sects)) / sects;
			}
			break;
		case D88_ENCODE_FM:
			trklen = trklen / 2;
			/* Calculated with GAP4b as 64 bytes */
			gaplen = (trklen - (137 + (33 + NSECSIZE(num)) * sects)) / sects;
			/* GAP3 is small, calculate with GAP4b as 0 */
			if (gaplen < 11) {
				gaplen = (trklen - (73 + (33 + NSECSIZE(num)) * sects)) / sects;
			}
			/* GAP3 is still small, calculate with GAP4a and GAP4b as 0 */
			if (gaplen < 11) {
				gaplen = (trklen - (33 + (33 + NSECSIZE(num)) * sects)) / sects;
			}
			break;
		default:
			break;
	}
	return gaplen;
}

int convertStatus(struct fdc_res_cmd *res)
{
	int code = 0x00;
	
	/* Control Mark */
	if ((res->st2 & FDC_ST2_CM) != 0) {
		code = D88_STATUS_CM;
	}
	/* End of cylinder */
	if ((res->st1 & FDC_ST1_EN) != 0) {
		code = D88_STATUS_EN;
	}
	/* Equipment Check */
	if ((res->st0 & FDC_ST0_EC) != 0) {
		code = D88_STATUS_EC;
	}
	/* OverRun */
	if ((res->st1 & FDC_ST1_OR) != 0) {
		code = D88_STATUS_OR;
	}
	/* Not Ready */
	if ((res->st0 & FDC_ST0_NR) != 0) {
		code = D88_STATUS_NR;
	}
	/* Not Writable */
	if ((res->st1 & FDC_ST1_NW) != 0) {
		code = D88_STATUS_NW;
	}
	/* DataError */
	if ((res->st1 & FDC_ST1_DE) != 0) {
		if	((res->st2 & FDC_ST2_DD) != 0) {
			/* DataError(Data) */
			code = D88_STATUS_DD;
		} else {
			/* DataError(ID) */
			code = D88_STATUS_DE;
		}
	}
	/* No Data */
	if ((res->st1 & FDC_ST1_ND) != 0) {
		code = D88_STATUS_ND;
	}
	/* Missing Address mark */
	if ((res->st1 & FDC_ST1_MA) != 0) {
		if	((res->st2 & FDC_ST2_MD) != 0) {
			/* Missing Address mark(Data) */
			code = D88_STATUS_MD;
		} else {
			/* Missing Address mark(ID) */
			code = D88_STATUS_MA;
		}
	}
	return code;
}

int checkTrackEncoding(int dev, int head)
{
	int enc = -1;
	struct fdc_res_cmd res;
	
	/* Read ID FM Encode */
	if (fdcReadId(dev, head, FDC_OPT_NONE, &res) == 0) {
		if (convertStatus(&res) == 0) {
			enc = D88_ENCODE_FM;
		}
	}
	/* Read ID MFM Encode */
	if (fdcReadId(dev, head, FDC_OPT_MFM, &res) == 0) {
		if (convertStatus(&res) == 0) {
			enc = D88_ENCODE_MFM;
		}
	}
	return enc;
}

int readSectorSequence(int dev, int head, int enc, struct fdc_sector_id *idbuf)
{
	int cnt = 0;
	struct fdc_sector_id *idptr = idbuf;
	struct fdc_res_cmd res;
	
	/* Positioning first sector */
	fdcReadId(dev, head, (enc == D88_ENCODE_MFM) ? FDC_OPT_NONE : FDC_OPT_MFM, &res);
	
	/* Loop read sector ID */
	do {
		fdcReadId(dev, head, GETENCFDC(enc), &res);
		if (convertStatus(&res) != 0) {
			return 0;
		}
		/* same R ID detected */
		if ((cnt != 0) && (idbuf[0].r == res.r)) {
			break;
		}
		memcpy(idptr++, &res.c, sizeof(struct fdc_sector_id));
	} while (cnt++ < MAXSECNUM);
	return cnt;
}

int dumpFloppyDisk(int start, int end, int mult, int side, int media, int protect, char *filename)
{
	int trk;
	int cyl;
	int head;
	int sects;
	int cnt;
	int offset;
	int enc;
	unsigned char data[MAXTRKLEN];
	
	FILE *fo;
	struct D88_HEADER dsk;
	struct D88_SECTOR sec;
	struct fdc_res_cmd res;
	struct fdc_res_intr intr;
	struct fdc_sector_id *idPtr;
	struct fdc_sector_id idBuf[MAXSECNUM];
	
	printf("Dump Started\n");
	printf("*Cylinder   : %d - %d\n", start, end);
	printf("*Step       : %d\n", mult);
	printf("*Side       : %d\n", side);
	printf("*MediaType  : %.2x\n", media);
	printf("*WiteProtect: %.2x\n", protect);
	printf("*Filename   : %s\n", filename);

	/* Open(write) disk image file */
	if ((fo = fopen(filename, "wb")) == NULL) {
		perror("fopen");
		fclose(fo);
		return -1;
	}
	
	/* Set and write disk image header */
	memset(&dsk, 0, sizeof(dsk));
	dsk.bMediaType = media;
	dsk.bWriteProtect = protect;
	if( fwrite(&dsk, sizeof(dsk), 1, fo) < 0) {
		perror("fwrite");
		return -1;
	}
	offset = sizeof(dsk);
	/* Read tracks from floppy disk */
	trk = (side == 2) ? start * 2: start;
	for (cyl = start; cyl <= end; cyl++) {
		head = (side == 2) ? 0 : side;
		do {
			printf("\nTrack: %d / Offset: 0x%.8x\n", trk, offset);
			printf("[Seek] Cylinder:%d / Step:%d\n", cyl, mult);
			
			sects = 0;
			memset(&idBuf, 0, sizeof(idBuf));
			/* Seek floppy */
			if (fdcSeek(FD_DEVNUM, cyl * mult, &intr) != 0) {
				printf("fdcSeek error\n");
				return -1;
			}
			/* Check track encoding */
			if ((enc = checkTrackEncoding(FD_DEVNUM, head)) != -1) {
				/* Read sector sequence */
				if ((sects = readSectorSequence(FD_DEVNUM, head, enc, idBuf)) != 0) {
					/* Set track image address */
					dsk.adwTrackOffsets[trk] = offset;
				}
			}
			printf("[ReadData] Side:%d / Encode:%.2X / Sectors:%d\n", head, enc, sects);
			if (verbose != 0) {
				printf(" C  H  R  N  : RESULT CODE   : DATA\n");
			}
			idPtr = idBuf;
			for (cnt = 0; cnt < sects; cnt++) {
				/* Initialize buffer */
				memset(&sec, 0, sizeof(sec));
				memset(&data, 0, sizeof(data));
				/* Read sector data from floppy */
				if (fdcReadData(FD_DEVNUM, head, GETENCFDC(enc), idPtr, 0, data, &res) != 0) {
					printf("fdcReadData error\n");
				}
				/* Write sector header to file */
				memcpy(&sec.c, idPtr, sizeof(struct fdc_sector_id));
				sec.wSectors = sects;
				sec.bEncoding = enc;
				sec.wLength = NSECSIZE(idPtr->n) ;
				sec.bStatus = convertStatus(&res);
				sec.bDataAddressMark = (sec.bStatus & D88_STATUS_CM) ? D88_DAM_DELETED : D88_DAM_NORMAL;
				if (verbose != 0) {
					printf(" %.2X %.2X %.2X %.2X : %.2X (%.2X %.2X %.2X) : %.2X\n",
						sec.c, sec.h, sec.r, sec.n, sec.bStatus, res.st0, res.st1, res.st2, data[0]);
				}
				if(fwrite(&sec, sizeof(sec), 1, fo) < 0) {
					perror("fwrite");
					return -1;
				}
				/* Write sector data to file */
				if(fwrite(&data, sec.wLength, 1, fo) < 0) {
					perror("fwrite");
					return -1;
				}
				offset += sizeof(sec) + sec.wLength;
				idPtr++;
			}
			trk++;
			head++;
		} while (head != side);
	}
	dsk.dwDiskSize = offset;
	
	if(fseek(fo, 0, SEEK_SET) != 0) {
		perror("fseek");
		return -1;
	}
	if( fwrite(&dsk, sizeof(dsk), 1, fo) < 0) {
		perror("fwrite");
		return -1;
	}
	fclose(fo);
	printf("Dump Ended\n");
	return 0;
}

int restoreFloppyDisk(int start, int end, int mult, int side, int trklen, char *filename)
{
	int trk;
	int cyl;
	int head;
	int sects;
	int cnt;
	int offset;
	int gap3;
	unsigned char data[MAXTRKLEN];
	unsigned char *dataPtr;
	size_t ret;
	
	FILE *fp;
	struct D88_HEADER dsk;
	struct D88_SECTOR secBuf[MAXSECNUM];
	struct D88_SECTOR *secPtr;
	struct fdc_sector_id idBuf[MAXSECNUM];
	struct fdc_sector_id *idPtr;
	struct fdc_res_cmd res;
	struct fdc_res_intr intr;
	
	printf("Restore Started\n");
	printf("*Cylinder   : %d - %d\n", start, end);
	printf("*Step       : %d\n", mult);
	printf("*Side       : %d\n", side);
	printf("*TrackLength: %d\n", trklen);
	printf("*Filename   : %s\n", filename);
	
	/* Open(read) disk image file */
	if ((fp = fopen(filename, "rb")) == NULL) {
		perror("fopen");
		fclose(fp);
		return -1;
	}
	/* Read disk image header */
	ret = fread(&dsk, sizeof(dsk), 1, fp);
	if( ret < 0) {
		perror("fread");
		return -1;
	}
	printf("*Title      : %s\n", dsk.szTitle);
	printf("*WiteProtect: %.2x\n", dsk.bWriteProtect);
	printf("*MediaType  : %.2x\n", dsk.bMediaType);
	
	/* Read tracks from file */
	trk = (side == 2) ? start * 2: start;
	for (cyl = start; cyl <= end; cyl++) {
		head = (side == 2) ? 0 : side;
		do {
			offset = dsk.adwTrackOffsets[trk];
			printf("\nTrack: %d / Offset: 0x%.8x\n", trk, offset);
			
			memset(&idBuf, 0, sizeof(idBuf));
			memset(&data, 0, sizeof(data));
			memset(&secBuf, 0,sizeof(secBuf));
			secPtr = secBuf;
			idPtr = idBuf;
			dataPtr = data;
			
			/* Set sector image */
			if (offset == 0) {
				/* Set unformat parameter*/
				secBuf[0].bEncoding = D88_ENCODE_MFM;
				secBuf[0].n = calcUnformatSizeNum(trklen, secBuf[0].bEncoding);
				secBuf[0].wSectors = 1;
				idBuf[0].n = secBuf[0].n ;
				gap3 = 0;
			} else {
				/* Seek file */
				if(fseek(fp, offset, SEEK_SET) != 0) {
					perror("fseek");
					return -1;
				}
				cnt = 0;
				do {
					/* Read sector header from file */
					ret = fread(secPtr, sizeof(struct D88_SECTOR), 1, fp);
					if(ret < 0) {
						perror("fread");
						return -1;
					}
					sects = secPtr->wSectors;
					memcpy(idPtr++, &(secPtr->c), sizeof(struct fdc_sector_id));
					/* Read sector data from file */
					ret = fread(dataPtr, secPtr->wLength, 1, fp);
					if(ret < 0) {
						perror("fread");
						return -1;
					}
					dataPtr += secPtr->wLength;
					secPtr++;
					cnt++;
				} while ( cnt < sects);
				/* Calculate GAP3 length */
				gap3 = calcFormatGapLen(trklen, secBuf[0].n, secBuf[0].wSectors, secBuf[0].bEncoding);
			}
			/* Seek floppy */
			printf("[Seek] Cylinder:%d / Step:%d\n", cyl, mult);
			if (fdcSeek(FD_DEVNUM, cyl * mult, &intr) != 0) {
				printf("fdcSeek error\n");
				return -1;
			}
			/* Format floppy */
			secPtr = secBuf;
			printf("[Format] Side:%d / Encode:%.2X / SectorSize:%.2X / Sectors:%d / Gap3:%d\n",
				head, secPtr->bEncoding, secPtr->n, secPtr->wSectors, gap3);
			if (fdcFormat(FD_DEVNUM, head, GETENCFDC(secPtr->bEncoding), secPtr->n, secPtr->wSectors, gap3, 0x00, idBuf, &res) != 0) {
				printf("fdcFormat error\n");
				return -1;
			}
			if (offset != 0) {
				secPtr = secBuf;
				idPtr = idBuf;
				dataPtr = data;
				cnt = 0;
				/* Write Data to floppy */
				printf("[WriteData] Side:%d / Encode:%.2X / Sectors:%d\n", head, secPtr->bEncoding, secPtr->wSectors);
				if (verbose != 0) {
					printf(" C  H  R  N  DAM : RESULT CODE   : DATA\n");
				}
				do {
					if (verbose != 0) {
						printf(" %.2X %.2X %.2X %.2X  %.2X : ", idPtr->c, idPtr->h, idPtr->r, idPtr->n, secPtr->bDataAddressMark);
					}
					if (fdcWriteData(FD_DEVNUM, head, GETENCFDC(secPtr->bEncoding), idPtr++, ISDAMDEL(secPtr->bDataAddressMark), dataPtr, &res) != 0) {
						printf("fdcWriteData error\n");
						return -1;
					}
					if (verbose != 0) {
						printf("%.2X (%.2X %.2X %.2X) : %.2X\n", convertStatus(&res), res.st0, res.st1, res.st2, *dataPtr);
					}
					dataPtr += secPtr->wLength;
					secPtr++;
					cnt++;
				} while (cnt < sects);
			}
			head++;
			trk++;
		} while (head != side);
	}
	fclose(fp);
	printf("Restore Ended\n");
	return 0;
}

int main(int argc, char* argv[])
{
	struct fdc_res_intr intr;
	struct fdc_res_sens sens;
	
	int media   = D88_TYPE_2HD;
	int protect = D88_PROTECT_OFF;
	int start = 0;
	int end	  = 81;
	int mult  = 1;
	int side  = 2;
	int rpm   = 360;
	int kbps  = 500;
	int drate = 0;
	int trklen;
	char *filename;
	
	int opt;
	char *subopts;
	char *value;
	
	fdcInit();
	
	/* Check write protect */
	fdcSenseDrive(FD_DEVNUM, &sens);
	if ((sens.st3 & FDC_ST3_WP) != 0) {
		protect = D88_PROTECT_ON;
	}
	/* Get option parameter */
	while((opt = getopt(argc, argv,"hvm:w:C:S:M:D:R:")) != -1){
		switch(opt){
			case 'h':
				usage();
				exit(0);
				break;
			case 'v':
				verbose = 1;
				break;
			case 'm':
				subopts = optarg;
				switch (getsubopt(&subopts, token_media, &value)) {
					case OPT_2D: 
						media = D88_TYPE_2D;
						start = 0;
						end = 41;
						side = 2;
						rpm = 360;
						kbps = 300;
						drate = 1;
						break;
					case OPT_2DD:
						media = D88_TYPE_2DD;
						start = 0;
						end = 81;
						side = 2;
						rpm = 360;
						kbps = 300;
						drate = 1;
						break;
					case OPT_2HD:
						media = D88_TYPE_2HD;
						start = 0;
						end = 81;
						side = 2;
						rpm = 360;
						kbps = 500;
						drate = 0;
						break;
					case OPT_1D:
						media = D88_TYPE_1D;
						start = 0;
						end = 41;
						side = 0;
						rpm = 360;
						kbps = 300;
						drate = 1;
						break;
					case OPT_1DD:
						media = D88_TYPE_1DD;
						start = 0;
						end = 81;
						side = 0;
						rpm = 360;
						kbps = 300;
						drate = 1;
						break;
					default:
						fprintf(stderr, "No match found for token: %s\n", value);
						break;
				}
				break;
			case 'w':
				subopts = optarg;
				switch (getsubopt(&subopts, token_switch, &value)) {
					case OPT_ON:
						protect = D88_PROTECT_ON;
						break;
					case OPT_OFF:
						protect = D88_PROTECT_OFF;
						break;
					default:
						fprintf(stderr, "No match found for token: %s\n", value);
						break;
				}
				break;
			case 'C':
				sscanf(optarg,"%d-%d",&start, &end);
				break;
			case 'S':
				side = atoi(optarg);
				break;
			case 'M':
				mult= atoi(optarg);
				break;
			case 'D':
				sscanf(optarg,"%d,%d",&rpm, &kbps);
				break;
			case 'R':
				drate = atoi(optarg);
				break;
			default:
				fprintf(stderr, "error: invalid option\n");
				exit(1);
				break;
		}
	}
	
	/* Calculate unformat track length */
	trklen = (60 * kbps * 1000) / (rpm * 8);
	
	/* Set FDC DRATE register */
	fdcSetDataRate(drate);
	
	/* Seek floppy track 0 */
	do {
		if (fdcRecalibrate(FD_DEVNUM, &intr) != 0){
			fprintf(stderr, "fdcRecalibrate error\n");
			exit(1);
		}
	} while((intr.st0 & FDC_ST0_EC) != 0);
	
	/* Get command and filename */
	argc -= optind;
	argv += optind;
	if (argc != 2) {
		usage();
		exit(0);
	}
	filename = argv[1];
	if (strncmp(argv[0], "dump", 4) == 0) {
		dumpFloppyDisk(start, end, mult, side, media, protect, filename);
	} else if (strncmp(argv[0], "restore", 7) == 0) {
		restoreFloppyDisk(start, end, mult, side, trklen, filename);
	} else {
		usage();
		exit(0);
	}
	fdcExit();
}
