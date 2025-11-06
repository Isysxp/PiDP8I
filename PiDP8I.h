#pragma once
#
//#include "SdFat.h"
//#include "SdFs.h"

const int SD_MISO_PIN = 40;  // GPIO for MISO
const int SD_MOSI_PIN = 31;  // GPIO for MOSI
const int SD_SCK_PIN = 30;   // GPIO for SCK
const int SD_CS_PIN = 43;    // GPIO for Chip Select (CS)
int ctr = 0;
char bfr[256];
uint8_t* p;
short rkdn, rkcmd, rkca, rkwc;
int rkda;
int bcnt;
FsFile rk05, df32;
char buffer[256];
Adafruit_USBD_MSC usb_msc;
Adafruit_USBD_CDC ttiox;
FsFile ptread, ptwrite;
int ptrfile = 0, ptpfile = 0;
volatile int dispen = 0, dispgo = 0;
// **** Define SPI_DRIVER_SELECT 0 in SdFatConfig.h as well
SdioConfig MySDIO(SD_SCK_PIN, SD_MOSI_PIN, SD_MISO_PIN, 2.0);
SdFs sd;

// Individual LED shift numbers NB these in reverse of the col order in the scheMAtic
#define AND 1 << 11  // This is actually COL 1
#define TAD 1 << 10
#define ISZ 1 << 9
#define DCA 1 << 8
#define JMS 1 << 7
#define JMP 1 << 6
#define IOT 1 << 5
#define OPR 1 << 4
#define FETCH 1 << 3
#define EXEC 1 << 2
#define DEFER 1 << 1
#define WCNT 1
#define LRUN (1 << 7)
#define ION (1 << 9)
#define PAUSE (1 << 8)
#define LINK 1 << 5
#define AXT 2  // Memory ACCess time in arbitrary units
#define IOT_INSTR 6
#define MRI_INSTR 5
//
//      Panel control switches
//
#define SWSINST 16
#define SWSSTEP 32
#define SWSTOP 64
#define SWCONT 128
#define SWEXAM 256
#define SWDEP 512
#define SWLOAD 1024
#define SWSTART 2048
//
#define LED_ROWS 0xff << 20
#define SW_ROWS 0x7 << 16
#define LED_COLS 0xfff0
#define LED1 1 << 20
#define GPIO_OUT_OFFSET 0x10
#define GPIO_OUT        (SIO_BASE + GPIO_OUT_OFFSET)

int insttbl[] = { AND, TAD, ISZ, DCA, JMS, JMP, IOT, OPR };

int SWctrl, SWlast = 1, single = 0, SWdfif, SWsr;
int snapdelay = 0;
int SWDATA[3];
int snap[8], swdat[3];
int swCNT[] = { 12, 6, 8 };
int EA, MD, CYCL, IRQ, memdata;


// Nano8 declarations
//

#define TTWAIT 5000

#define MEMSIZE 4096 * 8

short mem[MEMSIZE];
int tti, ttf, tto, intf, inst, ibus, pti, pto, mb, instr;
int dinf, doutf, dti;
int ifl, dfl, ifr, dfr, svr, uflag, usint, intinh, eaemd, gtf;
int kcnt = 0;
int clken, clkfl, clkcnt, dsPCntr;
unsigned int dskrg, dskmem, dskfl, tm, i, tmp, dbg;
unsigned int dskad;
int dtr;
// Shared state variables
int ACC, PC, MA, MB, MQ, MSTATE, RUN, EMA, EAESC;
int* cpu[] = { &PC, &MA, &MB, &ACC, &MQ, &MSTATE, &RUN, &EMA };

// Copy disk's data to buffer (up to bufsize) and
// return number of copied bytes (must be multiple of block size)
int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize) {
  return (sd.card()->readSectors(lba, (uint8_t*)buffer, bufsize / 512)) ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
  return (sd.card()->writeSectors(lba, (uint8_t*)buffer, bufsize / 512)) ? bufsize : -1;
}


// Callback invoked when WRITE10 command is completed (status received and ACCepted by host).
// used to flush any pending cache.
void msc_flush_cb(void) {
  // Sync hardware cache
}

void software_reset()
{
    watchdog_reboot(0,0,100);
}

int Serial_getchar() {
	while (!Serial.available())
		yield();
	int schar = Serial.read();
	Serial.write(schar);
	return schar;
}
