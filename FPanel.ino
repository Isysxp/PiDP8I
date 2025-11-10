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

void __not_in_flash_func(tdelay)(volatile int count) {
  int cntr = 0;
  while (cntr < count)
    cntr++;
}

void __not_in_flash_func(ldelay)(volatile int count) {
  int cntr = 0;
  while (cntr < count)
    cntr++;
}

void __not_in_flash_func(snapshot)(int sdelay) {        // Take a snapshot of the CPU state for display

  if (snapdelay++ > sdelay) {
    snapdelay = 0;
    for (int i = 0; i < 8; i++)
      snap[i] = *cpu[i];
  }
}

int __not_in_flash_func(keywait)(int state) {

  snapshot(0271);

  if (!SWflag)          // Combined flag of STOP/SINGINST/SINGSTEP
    return 0;
  else
    RUN &= ~LRUN;

  if (state && (SWctrl & SWSINST))
    return 0;
  if (state == 2 && instr >= MRI_INSTR)
    return 0;

  if (single) {
    while (SWctrl & SWCONT)
      ;
    single = 0;
  }

  while (!(RUN & LRUN)) {               // Stay in this loop with CPU halted until exit
    ldelay(10000);
    snapshot(0);
    if (SWctrl & SWCONT) {
      if (SWctrl & (SWSINST | SWSSTEP))
        single = 1;
      else
        RUN |= LRUN;
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
      while (SWctrl & SWEXAM)
        ;
    }
    if (SWctrl & SWDEP) {
      mem[PC] = SWsr;
      MB = mem[PC + ifl];
      MA = PC;
      PC = (PC + 1) & 07777;
      while (SWctrl & SWDEP)
        ;
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
  int swcntr = 0;

  // Drive all of the LED groups at 39KHz and scan the switches at 390 Hz.

  while (1) {
    gpio_set_dir_out_masked(LED_ROWS | LED_COLS);  // This is an interesting exercise in logic to drive the Front Panel!
    for (int k = 0, ledrow = LED1; k < 8; k++) {
      tbit = 1;
      patrn = LED_COLS;
      for (int i = 0; i < 12; i++, tbit = tbit << 1)
        if ((snap[k] & tbit))
          patrn = patrn ^ (1 << shift[i]);
      gpio_put_all(patrn | ledrow);
      ledrow = ledrow << 1;
      tdelay(0700);             // Display the register pattern from snap[k]
      gpio_put_all(LED_COLS);
      tdelay(300);              // Set all LED cols high and XLED low ... all LEDS off .. prevent any background glow
    }

    if (swcntr++ < 100)         // Scan switches every 100 display cycles ... 390 Hz
      continue;
    swcntr = 0;
    memset(SWDATA, 0, sizeof(SWDATA));
    for (int i = 0; i < 12; i++) {
      gpio_set_dir_out_masked(LED_ROWS | LED_COLS | SW_ROWS);  // This code is required to prevent latchup of the SW_ROW inputs
      gpio_put_all(1 << shift[i]);                             // A known problem with the RP2350
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
    SWflag = SWctrl & (SWSTOP | SWSINST | SWSSTEP);
  }
}
