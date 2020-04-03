const int SAFEBOOT_PIN = 22;
const int LED_PIN = 23;
const int PRESSURE_SENSOR_PIN = 2;
const int PRESSURE_SENSOR_AIN = 0;

// internal routing:
// TC1 TC2 clocked from GEN0
// ADC clocked from GEN0
// PRESSURE_SENSOR_PIN MUXed to AIN0

enum Mode { OFF, TEST };

volatile struct {
  Mode mode = Mode::TEST;
  unsigned int pressurePa;
} registers;

void interruptHandlerADC() {
  if (target::ADC.INTFLAG.getRESRDY()) {
    registers.pressurePa = target::ADC.RESULT;
    target::ADC.INTFLAG.setRESRDY(true);
  }
}

void setPortMux(int pin, target::port::PMUX::PMUXE pmux) {
  if (pin & 1) {
    target::PORT.PMUX[pin >> 1].setPMUXO((target::port::PMUX::PMUXO)pmux);
  } else {
    target::PORT.PMUX[pin >> 1].setPMUXE(pmux);
  }
  target::PORT.PINCFG[pin].setPMUXEN(true);
}

void initApplication() {
  atsamd::safeboot::init(SAFEBOOT_PIN, false, LED_PIN);

  // Power Manager
  {
    target::PM.APBBMASK.setPORT(true);
    target::PM.APBCMASK.setADC(true);
  }

  // Clock Generators
  {
    // target::gclk::GENDIV::Register workGendiv;
    // target::gclk::GENCTRL::Register workGenctrl;
    target::gclk::CLKCTRL::Register workClkctrl;

    target::GCLK.CLKCTRL = workClkctrl.zero()
                               .setID(target::gclk::CLKCTRL::ID::TC1_TC2)
                               .setGEN(target::gclk::CLKCTRL::GEN::GCLK0)
                               .setCLKEN(true);
    while (target::GCLK.STATUS.getSYNCBUSY())
      ;

    target::GCLK.CLKCTRL = workClkctrl.zero()
                               .setID(target::gclk::CLKCTRL::ID::ADC)
                               .setGEN(target::gclk::CLKCTRL::GEN::GCLK0)
                               .setCLKEN(true);
    while (target::GCLK.STATUS.getSYNCBUSY())
      ;
  }

  // LED
  {
    //
    target::PORT.DIR.setDIR(1 << LED_PIN);
  }

  // ADC
  {
    setPortMux(PRESSURE_SENSOR_PIN, target::port::PMUX::PMUXE::B);

    target::ADC.INPUTCTRL.setMUXPOS(
        (target::adc::INPUTCTRL::MUXPOS)PRESSURE_SENSOR_AIN);
    target::ADC.INPUTCTRL.setMUXNEG(target::adc::INPUTCTRL::MUXNEG::GND);
    target::ADC.INTENSET.setRESRDY(true);
    target::ADC.CTRLB.setPRESCALER(target::adc::CTRLB::PRESCALER::DIV512);
    target::ADC.AVGCTRL.setSAMPLENUM(
        target::adc::AVGCTRL::SAMPLENUM::_16_SAMPLES);
    target::ADC.AVGCTRL.setADJRES(4);
    target::ADC.CTRLA.setENABLE(true);
    target::ADC.CTRLB.setFREERUN(true);

    target::NVIC.ISER.setSETENA(1 << target::interrupts::External::ADC);
  }
}
