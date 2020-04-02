const int SAFEBOOT_PIN = 22;
const int LED_PIN = 23;

volatile int led = 10;

class ToggleTimer : public genericTimer::Timer {

  void onTimer() {
    target::PORT.OUTTGL.setOUTTGL(1 << LED_PIN);
    start(led);
  }
};

ToggleTimer timer;

Stepper stepper;

void interruptHandlerTC1() { stepper.handleTimeInterrupt(); }

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

  target::PORT.DIR.setDIR(1 << LED_PIN);
  timer.start(led);
}
