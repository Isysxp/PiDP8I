//
// IO subsystem
//
#include "src/ff.h"

void iot() {

	if (uflag == 3) {
		usint = 1;
		return;
	}
	switch (inst & 0770) {
		case 0:
			switch (inst & 0777) {
				case 000:
					if (intf)
						PC++;
					intf = intinh = 0;
					RUN &= ~ION;
					break;
				case 001:
					intinh |= 1;
					RUN |= ION;
					break;
				case 002:
					intf = intinh = 0;
					RUN &= ~ION;
					break;
				case 003:
					if (ibus)
						PC++;
					break;
				case 004:
					ACC = (ACC & 010000) ? 014000 : 0;
					if (intinh & 1)
						ACC |= 0200;
					if (intf)
						ACC |= 01000;
					if (gtf)
						ACC |= 02000;
					ACC |= svr;
					break;
				case 005:
					intinh = 3;
					ACC &= 07777;
					if (ACC & 04000)
						ACC |= 010000;
					svr = ACC & 0177;
					dfr = (svr & 07) << 3;
					dfl = dfr << 9;
					ifr = (svr & 070);
					if (svr & 0100)
						uflag = 1;
					gtf = ACC & 02000;
					break;
				case 006:
					if (gtf)
						PC++;
					break;
				case 007:
					caf();
					break;
			}
			break;
		case 0200:
		case 0210:
		case 0220:
		case 0230:
		case 0240:
		case 0250:
		case 0260:
		case 0270:
			switch (inst & 0777) {
				case 0204:
					usint = 0;
					break;
				case 0254:
					if (usint)
						PC++;
					break;
				case 0264:
					uflag = 0;
					break;
				case 0274:
					uflag = 1;
					break;
				case 0214:
					ACC |= dfr;
					break;
				case 0224:
					ACC |= ifr;
					break;
				case 0234:
					ACC |= svr;
					break;
				case 0244:
					dfr = (svr & 07) << 3;
					dfl = dfr << 9;
					ifr = (svr & 070);
					if (svr & 0100)
						uflag = 1;
					break;
			}
			switch (inst & 0707) {
				case 0201:
					dfr = inst & 070;
					dfl = dfr << 9;
					break;
				case 0202:
					ifr = inst & 070;
					intinh |= 2;
					break;
				case 0203:
					dfr = inst & 070;
					ifr = inst & 070;
					dfl = dfr << 9;
					intinh |= 2;
					break;
			}
			break;

		case 010:
			switch (inst & 07) {
				case 01:
					if (pti > -1)
						PC++;
					break;
				case 02:
					ACC |= pti & 0377;
					pti = -2;
					break;
				case 04:
					pti = -1;
					break;
				case 06:
					ACC |= pti & 0377;
					pti = -1;
					break;
			}
			break;
		case 020:
			switch (inst & 07) {
				case 1:
					if (pto >= TTWAIT)
						PC++;
					break;
				case 2:
					pto = 0;
					break;
				case 6:
				case 4:
					pto = ACC & 0177;
					dispen = 1;
					while (dispen != 2)
						;
					if (f_write(&ptwrite,&pto,1,&bcnt) != FR_OK)
						Serial.printf("Punch error...\r\n");
					dispen = 0;
					pto = 1;
					break;
			}
			break;
		case 030:
			switch (inst & 07) {
				case 1:
					if (ttf)
						PC++;
					break;
				case 2:
					ttf = 0;
					ACC &= 010000;
					break;
				case 4:
					ACC |= tti | 0200;
					break;
				case 6:
					ACC &= 010000;
					ACC |= tti | 0200;
					ttf = 0;
			}
			break;
		case 040:
			switch (inst & 07) {
				case 1:
					if (tto >= TTWAIT)
						PC++;
					break;
				case 2:
					tto = 0;
					break;
				case 6:
				case 4:
					Serial.write(ACC & 0177);
					Serial.flush();
					tto = 1;
					break;
			}
			break;
		case 0400:  // Remote TTY via socket
		case 0410:
			switch (inst & 017) {
				case 000: dinf = 0; break;
				case 001:
					if (dinf) PC++;
					break;
				case 002:
					dinf = 0;
					ACC &= 010000;
					break;
				case 004: ACC = (ACC & 010000) | dti; break;
				case 006:
					dinf = 0;
					ACC = (ACC & 010000) | dti;
					break;
				case 010: doutf = 1; break;
				case 011:
					if (doutf > TTWAIT / 10) PC++;
					break;
				case 012: doutf = 0; break;
				case 016:
				case 014:
					ttiox.write(ACC & 0377);
					doutf = 1;
					break;
			}
			break;
		case 0600:
		case 0610:
		case 0620:
			switch (inst & 0777) {
				case 0601:
					dskad = dskfl = 0;
					break;
				case 0605:
				case 0603:
					i = (dskrg & 070) << 9;
					dskmem = ((mem[07751] + 1) & 07777) | i; /* mem */
					tm = (dskrg & 03700) << 6;
					dskad = (ACC & 07777) + tm; /* dsk */
					i = 010000 - mem[07750];
					p = (uint8_t*)&mem[dskmem];
					dispen = 1;
					while (dispen != 2)
						;
					if (f_lseek(&df32, dskad * 2) != FR_OK)
						Serial.printf("DF32 seek fail\r\n");
					if (inst & 2) {
						/* read */
						if (f_read(&df32, p, i * 2, &bcnt) != FR_OK)
							Serial.printf("DF32 read fail\r\n");
					//	Serial.printf("Read:%o>%o:%o,%d Len:%d\r\n", dskad, dskmem, dskrg, i, bcnt);
					} else {
						/* write */
						if (f_write(&df32, p, i * 2, &bcnt) != FR_OK)
							Serial.printf("DF32 write fail\r\n");
					//	Serial.printf("Write:%o>%o:%o,%d Len:%d\r\n", dskad, dskmem, dskrg, i, bcnt);
					}
					dispen = 0;
					dskfl = 1;
					mem[07751] = 0;
					mem[07750] = 0;
					ACC = ACC & 010000;
					break;
				case 0611:
					dskrg = 0;
					break;
				case 0615:
					dskrg = (ACC & 07777); /* register */
					break;
				case 0616:
					ACC = (ACC & 010000) | dskrg;
					break;
				case 0626:
					ACC = (ACC & 010000) + (dskad & 07777);
					break;
				case 0622:
					if (dskfl) PC++;
					break;
				case 0612: ACC = ACC & 010000;
				case 0621:
					PC++; /* No error */
					break;
			}
			break;
		case 0740:
			switch (inst & 7) {
				case 0:
					break;
				case 1:
					if (rkdn) {
						PC++;
						rkdn = 0;
					}
					break;
				case 2:
					ACC &= 010000;
					rkdn = 0;
					break;
				case 3:
					rkda = ACC & 07777;
					//
					// OS/8 Scheme. 2 virtual drives per physical drive
					// Regions start at 0 and 6260 (octal).
					//
					ACC &= 010000;
					if (rkcmd & 6) {
						printf("Unit error\n");
						return;
					}
					switch (rkcmd & 07000) {
						case 0:
						case 01000:
							rkca |= (rkcmd & 070) << 9;
							rkwc = (rkcmd & 0100) ? 128 : 256;
							rkda |= (rkcmd & 1) ? 4096 : 0;
							dispen = 1;
							while (dispen != 2)
								;
							if (f_lseek(&rk05, rkda * 512) != FR_OK)
								Serial.printf("RK05 seek fail\r\n");
							p = (uint8_t*)&mem[rkca];
							if (f_read(&rk05, p, rkwc * 2, &bcnt) != FR_OK)
								Serial.printf("RK05 read fail\r\n");
							//Serial.printf("Read Mem:%04o Dsk:%04o Len:%d\r\n", rkca, rkda, bcnt);
							dispen = 0;
							rkca = (rkca + rkwc) & 07777;
							rkdn++;
							break;
						case 04000:
						case 05000:
							rkca |= (rkcmd & 070) << 9;
							rkwc = (rkcmd & 0100) ? 128 : 256;
							rkda |= (rkcmd & 1) ? 4096 : 0;
							dispen = 1;
							while (dispen != 2)
								;
							f_lseek(&rk05, rkda * 512);
							p = (uint8_t*)&mem[rkca];
							f_write(&rk05, p, rkwc * 2, &bcnt);
							f_sync(&rk05);
							dispen = 0;
							//Serial.printf("Write Mem:%04o Dsk:%04o Len:%d\r\n", rkca, rkda, bcnt);
							rkca = (rkca + rkwc) & 07777;
							rkdn++;
							break;
						case 02000:
							break;
						case 03000:
							if (rkcmd & 0200) rkdn++;
							break;
					}
					break;
				case 4:
					rkca = ACC & 07777;
					ACC &= 010000;
					break;
				case 5:
					ACC = (ACC & 010000) | 04000;
					break;
				case 6:
					rkcmd = ACC & 07777;
					ACC &= 010000;
					break;
				case 7:
					printf("Not allowed...RK8E\n");
					break;
			}
		case 0130:
			switch (inst & 0777) {
				case 0131:
					clken = 1;
					clkcnt = 0;
					break;
				case 0132: clken = 0; break;
				case 0133:
					if (clkfl) {
						clkfl = 0;
						PC++;
						break;
					}
			}
	}
}
