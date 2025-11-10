
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <SPI.h>
#include <SD.h>
#include "SdFat.h"
#include "SdFs.h"
#include "PiDP8I.h"

int xmain(int argc, char* args[]);
extern void display();
extern int keywait(int state);

int readline(char* buffer, int len) {
	int pos = 0;
	char tm;

	while (1) {
		yield();
		if (Serial.available()) {
			tm = Serial.read();
			Serial.write(tm);
		}
		switch (tm) {
			case 0:
			case '\n':  // Ignore CR
				break;
			case '\r':  // Return on new-line
				buffer[pos] = 0;
				return pos;
			default:
				if (pos < len - 1) {
					buffer[pos++] = tm;
					buffer[pos] = 0;
				}
		}
		tm = 0;
	}
}

void serial_putchar(char c) {
	Serial.write(c);
}

char serial_getchar() {
	return Serial.read();
}

// the setup function runs once when you press reset or power the board
void setup() {
	// initialize
	Serial.begin(115200);
	ttiox.begin(115200);
	while (!Serial)
		yield();

	Serial.println("Initializing SD card...");
	delay(500);
	if (!sd.begin(MySDIO)) {
		Serial.println("SD card initialization failed!");
		return;
	}
	Serial.println("SD Card initialized successfully!");
	//sd.ls(LS_SIZE);   SD Card file listing
	Serial.printf("Startup:\r\nSD Card size (kByte):%lld\r\n", sd.card()->sectorCount() / 2);
	Serial.printf("Attach SDCard as USB drive (y/N):");
	Serial.flush();
	if (Serial_getchar() == 'y') {
		Serial.printf("\r\nPress an key to restart....");
		Serial.flush();
		delay(100);
		usb_msc.setID("PiDP8I", "Mass Storage", "1.0");
		// Set disk size
		usb_msc.setCapacity(sd.card()->sectorCount(), 512);
		// Set callbacks
		usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
		//
		usb_msc.setUnitReady(true);
		usb_msc.begin();
		USBDevice.detach();  // Code required to init MSC device
		delay(100);
		USBDevice.attach();          // Will (re)attach MSC/USB drive and Serial
		Serial_getchar();            // Wait here
		sd.end();                    // Unmount file system
		watchdog_reboot(0, 0, 100);  // Hard reset to disconnect MSC/USB drive
		while (1)
			;
	}
	gpio_init_mask(LED_ROWS | LED_COLS | SW_ROWS);
	delay(500);
	strcpy(buffer, "/");
	Serial.print("\r\nEnter papertape reader filename:");
	readline(bfr, 80);
	ptread = sd.open(bfr, O_RDONLY);
	if (ptread)
		Serial.printf("\r\nFile mounted in papertape reader.\r\n");
	else
		Serial.printf("\r\nFile not found\r\n");
	ptwrite = sd.open("PUNCH.TAPE", O_WRITE | O_CREAT);
	if (ptwrite)
		Serial.printf("File PUNCH.TAPE attached.\r\n");
	else
		Serial.printf("Failed to create PUNCH.TAPE\r\n");
	Serial.printf("Run....\r\nBoot (1:DMS 2:OS/8):");
	PC = 030;
	if (Serial_getchar() == '1')
		PC = 0200;
	rk05 = sd.open("rk05.dsk", O_RDWR | O_BINARY);
	if (!rk05)
		Serial.println("Cannot open RK05.dsk");
	df32 = sd.open("DF32.DSK", O_RDWR | O_BINARY);
	if (!df32)
		Serial.println("Cannot open DF32.DSK");
	ttiox.write("OK\r\n");
	Serial.flush();
	memset(SWDATA, 0, sizeof(SWDATA));
	caf();
	dbg = 0;
	pinMode(19, OUTPUT);
	gpio_set_dir_out_masked64(LED_ROWS | LED_COLS);
	gpio_set_dir_in_masked64(SW_ROWS);
	gpio_put_all(LED_COLS | LED_ROWS);
	Dispgo = 1;
}

// the loop function runs over and over again forever
void loop() {
	xmain(3, NULL);
}

void setup1() {
	while (!Dispgo)
		yield();
	display();
}

void caf() {
	ACC = MQ = tti = tto = ttf = ibus = pto = dinf = doutf = dsPCntr = 0;
	dfr = ifr = dfl = ifl = uflag = dskfl = gtf = eaemd = clkfl = clken = clkcnt = 0;
	pti = -2;  // Flag clear until explicit fetch
}

void group2() {
	int state;

	state = 0;
	if (ACC & 04000)
		state |= 0100;
	if ((ACC & 07777) == 0)
		state |= 040;
	if (ACC & 010000)
		state |= 020;
	if ((inst & 0160) == 0)
		state = 0;
	if (inst & 010) {
		if ((~state & inst) == inst)
			PC++;
	} else if (state & inst)
		PC++;
	if (inst & 0200)
		ACC &= 010000;
	if (inst & 4)
		ACC |= SWsr;  //OSR
}

void group1() {

	if (inst & 0200)
		ACC &= 010000;
	if (inst & 0100)
		ACC &= 07777;
	if (inst & 040)
		ACC ^= 07777;
	if (inst & 020)
		ACC ^= 010000;
	if (inst & 1)
		ACC++;
	ACC &= 017777;
	switch (inst & 016) {
		case 2:
			tmp = (ACC << 6) | ((ACC >> 6) & 077);  // BSW .. v untidy!
			tmp &= 07777;
			if (ACC & 010000) tmp |= 010000;
			ACC = tmp;
			break;
		case 06:
			ACC = ACC << 1;
			if (ACC & 020000)
				ACC++;
		case 04:
			ACC = ACC << 1;
			if (ACC & 020000)
				ACC++;
			break;
		case 012:
			if (ACC & 1)
				ACC |= 020000;
			ACC = ACC >> 1;
		case 010:
			if (ACC & 1)
				ACC |= 020000;
			ACC = ACC >> 1;
			break;
	}
	ACC &= 017777;
}

//
//	This is the primary simulator loop
//

bool cycl(void) {
	//
	// Interrupt handler
	//
	if (intf && ibus) {
		mem[0] = PC & 07777;
		PC = 1;
		intf = intinh = 0;
		svr = (ifl >> 9) + (dfl >> 12);
		if (uflag == 3)
			svr |= 0100;
		dfr = ifr = dfl = ifl = uflag = 0;
		RUN &= ~ION;
	}
	//
	digitalWriteFast(19, (cycf = ~cycf));             // Toggel GPIO19 for cycle time check
	ibus = ttf || (tto == TTWAIT) || clkfl || dskfl;  // Set INTBUS from device flags
	ibus |= (doutf == TTWAIT);
	//
	// FETCH
	//
	MB = inst = mem[PC + ifl];  // Fetch instruction
	instr = MB >> 9;
	if (instr < IOT_INSTR)
		MA = ((inst & 0177) | ((inst & 0200) ? (PC & 07600) : 0)) + ifl;
	else
		MA = PC;
	PC = (PC + 1) & 07777;
	MSTATE = FETCH;
	EMA = (ACC & 010000) ? LINK : 0;
	EMA |= (dfl >> 3) | (ifl >> 6);
	MSTATE |= insttbl[instr];  // Display instruction
	RUN &= ~PAUSE;             // Clear PAUSE
	if (instr == IOT_INSTR)    // Instruction is IOT set PAUSE
		RUN |= PAUSE;
	if (keywait(0))						 // Check panel switched and take a FETCH snapshot
		return true;
	//
	//	DEFER
	//
	if (inst & 0400 && instr < IOT_INSTR) {  // Enter DEFER cycle if required
		MSTATE = insttbl[instr] | DEFER;
		if ((MA & 07770) == 010)
			mem[MA]++;
		mem[MA] &= 07777;
		if (inst & 04000)
			MB = MA = mem[MA] + ifl;
		else
			MB = MA = mem[MA] + dfl;
		keywait(1);															// Take a DEFER snapshot
	}
	//
	//	EXEC
	//
	if (instr < MRI_INSTR) {           // Instruction is MRI	// Enter EXECUTE cycle for all MRIs except JMP
		MSTATE = insttbl[instr] | EXEC;  // EXEC state
		MB = mem[MA];
	}
	//
	if (dbg) {
		sprintf(bfr, "PC:%04o Inst:%04o MA:%04o Mem:%04o Acc:%05o\r\n", PC, inst, MA, mem[MA], ACC);
		Serial.print(bfr);
		Serial.flush();
		delay(100);
	}
	//
	//	This section is used to sample input devices and set flags
	//
	if (kcnt++ > 1000) {
		if (Serial.available() && !ttf) {
			Serial.readBytes((char*)&tti, 1);
			ttf = 1;
		}
		if (ttiox.available() && !dinf) {
			ttiox.readBytes((char*)&dti, 1);
			dinf = 1;
		}
		if (ptread)
			if (pti == -1 && ptread.available()) {
				pti = 0;
				ptread.read((uint8_t*)&pti, 1);
			}
		kcnt = 0;
		if (tti == 1 || tti == 5)  // Halt on keypress ^a or ^e
			return false;
		if (tti == 4) {
			dbg++;
			ttf = 0;
		}
	}
	//
	// This section throttles the output devces
	//
	if (tto && (tto < TTWAIT))
		tto++;
	if (doutf && (doutf < TTWAIT))
		doutf++;
	if (pto && (pto < TTWAIT))
		pto++;
	if (clken)
		if (++clkcnt >= 20000) {
			clkfl = 1;
			clkcnt = 0;
		}
	//
	//	Do instruction
	//
	switch (inst & 07000) {
		case 0:  //AND
			ACC &= mem[MA] | 010000;
			break;
		case 01000:  //TAD
			ACC += mem[MA] & 07777;
			break;
		case 02000:
			if (!(mem[MA] = (mem[MA] + 1) & 07777))
				PC++;
			break;
		case 03000:  //DCA
			mem[MA] = ACC & 07777;
			ACC &= 010000;
			break;
		case 04000:  //JMS
			ifl = ifr << 9;
			MA = (MA & 07777) + ifl;
			mem[MA] = PC;
			PC = (MA + 1) & 07777;
			intinh &= 1;
			uflag |= 2;
			break;
		case 05000:  //JMP
			ifl = ifr << 9;
			PC = (MA & 07777);
			intinh &= 1;
			uflag |= 2;
			break;
		case 06000:  //IOT
			iot();
			break;
		case 07000:  //OPR
			if (inst & 0400) {
				if (inst & 1) {
					if (group3(inst))
						PC++;
					break;
				}
				if (inst & 2) {
					Serial.printf("HALT:%04o\r\nPress ^a to exit or any key to continue....\r\n", PC);
					while (!Serial.available())
						yield();
					int inchar = Serial.read();
					return inchar == 1 ? false : true;
				}
				group2();
			} else
				group1();
			break;
	}
	//
	ACC &= 017777;  // Make sure ACC is valid
	                //
	                // Interrupt delay
	if (intinh == 1 && inst != 06001)
		intf = 1;
	//
	RUN = (RUN & 07603) | (EAESC << 2);  // Add in EAE stepcounter data
	keywait(2);													 // End of FETCH/EXECUTE snapshot
	return true;
}

int xmain(int argc, char* args[]) {
	//
	// Bootstraps
	//
	char bfr[128];
	short dms[] = {
		// DF32 4K DMS Bootstrap
		06603,
		06622,
		05201,
		05604,
		07600,
	};
	short os8[] = {
		// DF32 OS/8 bootstrap
		07600,
		06603,
		06622,
		05352,
		05752,
	};

	short test[] = { 06031, 05200, 06036, 06046, 07200, 05200 };

	memset(mem, 0, sizeof(mem));
	mem[07750] = 07576;
	mem[07751] = 07576;
	memcpy(&mem[0200], dms, sizeof(dms));
	//memcpy(&mem[07750], os8, sizeof(os8));
	mem[030] = 06743;  // RK05 OS/8 bootstrap
	mem[031] = 05031;
	//memcpy(&mem[0200], test, sizeof(test));
	caf();

	//PC = 07750;		// SA for OS/8 on DF32
	//PC = 0200;		// SA for DMS on DF32
	//PC = 030;			// SA for OS/8 on RK05

	Serial.printf("\r\nRun from: %04o\r\n", PC);
	RUN = LRUN;
	while (1) {
		while (cycl()) {
		}
		RUN &= ~LRUN;
		//
		//	Close and re-open file in PTR:
		//
		ptread.close();
		Serial.printf("\r\nEnter papertape reader filename:");
		readline(bfr, 80);
		ptread = sd.open(bfr, O_RDONLY);
		if (ptread)
			Serial.printf("\r\nFile mounted in papertape reader.\r\n");
		else
			Serial.printf("\r\nFile not found\r\n");
		//
		Serial.print("Enter new PC (octal) (0 for hard reset):");
		readline(bfr, 80);
		kcnt = 0;
		sscanf(bfr, "%o", &kcnt);  // Use kcnt as temp int
		if (!kcnt) {
			ptwrite.close();             // Close PUNCH.TAPE
			watchdog_reboot(0, 0, 100);  // Hard reset to disconnect MSC/USB drive
			while (1)
				;
		}
		// User has type a non zero start address. PUNCH.TAPE file remains open.
		// NB This restart is the same as LOAD ADDRESS and START
		while (Serial.read() != -1)
			;
		caf();
		RUN |= LRUN;
		PC = kcnt;
	}
	return 0;
}
