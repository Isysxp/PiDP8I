//
// PiDP8I front panel driver
//

#define LED_ROWS 0xff << 20
#define SW_ROWS 0x7 << 16
#define LED_COLS 0xfff0
#define LED1 1 << 20
#define GPIO_OUT_OFFSET 0x10
#define GPIO_OUT (SIO_BASE + GPIO_OUT_OFFSET)

uint32_t ledrow, ledval, mdelay = 1000, swrow, rsl;
uint32_t tbit, patrn, sw5;
int shift[] = { 13, 9, 10, 11, 12, 8, 7, 6, 15, 14, 5, 4 };

void __not_in_flash_func(snapshot)(int flag) {
  if (CYCL++ < snapdelay && flag)
    return;
  RUN = (RUN & 07603) | (EAESC << 2);
  for (int i = 0; i < 8; i++)
    snap[i] = *cpu[i];
  CYCL = 0;
}

void __not_in_flash_func(tdelay)(volatile int count) {
  int cntr = 0;
  while (cntr < count)
    cntr++;
}

int __not_in_flash_func(keywait)(int state) {

  if (single) {
    //		Serial.printf("STOP:%d\r\n",single);
    while (SWctrl & SWCONT)
      yield();
    single = 0;
  }

  if (SWctrl & (SWSTOP | SWSINST | SWSSTEP)) {
    RUN &= ~LRUN;
    //		Serial.printf("Stop:%d ",SWctrl & (SWSINST | SWSSTEP));
  }

  if (state && (SWctrl & SWSINST))
    return 0;

  while (!(RUN & LRUN)) {
    snapshot(0);
    tdelay(10000);
    if (SWctrl & SWCONT) {
      RUN |= LRUN;
      if (SWctrl & (SWSINST | SWSSTEP))
        single = 1;
      //			Serial.printf("%d\r\n",single);
      return 0;
    }
    if (SWctrl & SWLOAD) {
      PC = SWsr;
      EMA = (ACC & 010000) ? LINK : 0;
      EMA |= SWdfif;
      ifl = (SWdfif & 0700) << 6;
      dfl = (SWdfif & 7000) << 3;
    }
    if (SWctrl & SWEXAM) {
      MSTATE = 0;
      MB = mem[PC + ifl];
      MA = PC;
      PC = (PC + 1) & 07777;
      snapshot(0);
      while (SWctrl & SWEXAM)
        yield();
    }
    if (SWctrl & SWDEP) {
      mem[PC] = SWsr;
      MB = mem[PC + ifl];
      MA = PC;
      PC = (PC + 1) & 07777;
      snapshot(0);
      while (SWctrl & SWDEP)
        yield();
    }
    if (SWctrl & SWSTART) {
      caf();
      PC = SWsr;
      ifl = (SWdfif & 0700) << 6;
      dfl = (SWdfif & 7000) << 3;
      RUN |= LRUN;
      return 1;
    }
  }
  return 0;
}


void __not_in_flash_func(display)() {

  while (1) {
    gpio_set_dir_out_masked(LED_ROWS | LED_COLS);
    for (int k = 0, ledrow = LED1; k < 8; k++) {
      tbit = 1;
      patrn = LED_COLS;
      for (int i = 0; i < 12; i++, tbit = tbit << 1)
        if ((snap[k] & tbit))
          patrn = patrn ^ (1 << shift[i]);
      gpio_put_all(patrn | ledrow);
      ledrow = ledrow << 1;
      tdelay(4000);
      gpio_put_all(LED_COLS);
      tdelay(500);
    }

    memset(SWDATA, 0, sizeof(SWDATA));
    for (int i = 0; i < 12; i++) {
      gpio_set_dir_out_masked(LED_ROWS | LED_COLS | SW_ROWS);
      gpio_put_all(1 << shift[i]);
      gpio_set_dir_in_masked(SW_ROWS);
      gpio_put_all(1 << shift[i]);
      tdelay(200);
      rsl = (gpio_get_all() & SW_ROWS) >> 16;
      SWDATA[2] >>= 1;
      SWDATA[1] >>= 1;
      SWDATA[0] >>= 1;
      SWDATA[2] |= (rsl & 4) ? 04000 : 0;
      SWDATA[1] |= (rsl & 2) ? 04000 : 0;
      SWDATA[0] |= (rsl & 1) ? 04000 : 0;
    }
    SWctrl = SWDATA[2];
    SWdfif = SWDATA[1];
    SWsr = SWDATA[0];
    snapdelay = 0300;                   // This is the display fiddle factor
    //snapdelay = SWsr;                 // Uncomment this line and adjust the SR to what looks best for you!!!
  }
}
