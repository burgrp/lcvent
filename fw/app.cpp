int SAFEBOOT_PIN = 22;
int LED_PIN = 23;

Stepper stepper;

void interruptHandlerTC1() {
  stepper.handleTimeInterrupt();
}

void initApplication() {
  target::PM.APBBMASK.setPORT(true);
  atsamd::safeboot::init(SAFEBOOT_PIN, false, LED_PIN);

  target::gclk::GENDIV::Register workGendiv;
  target::gclk::GENCTRL::Register workGenctrl;
  target::gclk::CLKCTRL::Register workClkctrl;

  target::GCLK.CLKCTRL = workClkctrl.zero()
                             .setID(target::gclk::CLKCTRL::ID::TC1_TC2)
                             .setGEN(target::gclk::CLKCTRL::GEN::GCLK0)
                             .setCLKEN(true);
  while (target::GCLK.STATUS.getSYNCBUSY())
    ;

  target::PM.APBCMASK.setTC(1, true);
  target::NVIC.ISER.setSETENA(1 << target::interrupts::External::TC1);
  stepper.init(&target::TC1, 2, 3, 4, 5);
}
