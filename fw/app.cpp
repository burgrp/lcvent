const int SAFEBOOT_PIN = 22;
const int LED_PIN = 23;
const int PRESSURE_SENSOR_PIN = 2;
const int PRESSURE_SENSOR_AIN = 0;
const int FLOW_SENSOR_PIN = 16;
const int FLOW_SENSOR_EXTINT = 0;
const int FAN_PWM_PIN = 14;
const int FAN_PWM_WO = 0;

const int TC1_CLK_HZ = 1E6;
const int TC1_OVF_HZ = 100;
const int TC1_PER = TC1_CLK_HZ / 64 / TC1_OVF_HZ - 1;
const int FAN_PWM_CC_LOW = TC1_PER / 10 + 1;
const int FAN_PWM_CC_HI = FAN_PWM_CC_LOW * 2;

// internal routing:
// TC1, TC2, ADC, EIC, TCC0 clocked from GEN0

enum Mode { OFF, TEST };

volatile struct {
  Mode mode = Mode::TEST;
  unsigned int pressurePa;
  unsigned int flowLitSec = 0;
  unsigned int fanPwmPerc = 0;
} registers;

volatile unsigned int setFanPwmPerc = registers.fanPwmPerc;

void interruptHandlerADC() {
  if (target::ADC.INTFLAG.getRESRDY()) {
    registers.pressurePa = target::ADC.RESULT;
    // target::ADC.RESULT;//
    target::ADC.INTFLAG.setRESRDY(true);
  }
}

void interruptHandlerEIC() {
  if (target::EIC.INTFLAG.getEXTINT(FLOW_SENSOR_EXTINT)) {
    registers.flowLitSec++;
    target::EIC.INTFLAG.setEXTINT(FLOW_SENSOR_EXTINT, true);
    target::PORT.OUTTGL.setOUTTGL(1 << LED_PIN);
  }
}

void fanPwmPercUpdated() {
  target::TC1.COUNT8.CC[FAN_PWM_WO].setCC(16 + (registers.fanPwmPerc * (FAN_PWM_CC_HI - FAN_PWM_CC_LOW)) / 100);
}

void interruptHandlerTC1() {
  if (target::TC1.COUNT8.INTFLAG.getOVF()) {
    target::TC1.COUNT8.INTFLAG.setOVF(true);

    if (registers.fanPwmPerc != setFanPwmPerc) {
      registers.fanPwmPerc = setFanPwmPerc;
      fanPwmPercUpdated();
    }
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
    target::PM.APBCMASK.setTC(1, true);
    target::PM.APBCMASK.setTC(2, true);
    target::PM.APBCMASK.setTCC0(true);
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

    target::GCLK.CLKCTRL = workClkctrl.zero()
                               .setID(target::gclk::CLKCTRL::ID::EIC)
                               .setGEN(target::gclk::CLKCTRL::GEN::GCLK0)
                               .setCLKEN(true);
    while (target::GCLK.STATUS.getSYNCBUSY())
      ;

    target::GCLK.CLKCTRL = workClkctrl.zero()
                               .setID(target::gclk::CLKCTRL::ID::TCC0)
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
    target::ADC.REFCTRL.setREFSEL(target::adc::REFCTRL::REFSEL::INTVCC0);
    target::ADC.REFCTRL.setREFCOMP(true);
    target::ADC.INTENSET.setRESRDY(true);
    target::ADC.CTRLB.setPRESCALER(target::adc::CTRLB::PRESCALER::DIV512);
    target::ADC.AVGCTRL.setSAMPLENUM(
        target::adc::AVGCTRL::SAMPLENUM::_16_SAMPLES);
    target::ADC.AVGCTRL.setADJRES(4);
    target::ADC.CTRLA.setENABLE(true);
    target::ADC.CTRLB.setFREERUN(true);

    target::NVIC.ISER.setSETENA(1 << target::interrupts::External::ADC);
  }

  // External interrupts
  target::PORT.PINCFG[FLOW_SENSOR_PIN].setINEN(true);
  target::PORT.OUTCLR.setOUTCLR(1 << FLOW_SENSOR_PIN);
  target::PORT.PINCFG[FLOW_SENSOR_PIN].setPULLEN(true);
  setPortMux(FLOW_SENSOR_PIN, target::port::PMUX::PMUXE::A);

  target::EIC.EVCTRL.setEXTINTEO(FLOW_SENSOR_EXTINT, true);
  target::EIC.INTENSET.setEXTINT(FLOW_SENSOR_EXTINT, true);
  target::EIC.CONFIG.setSENSE(FLOW_SENSOR_EXTINT,
                              target::eic::CONFIG::SENSE::RISE);

  target::EIC.CTRL.setENABLE(true);
  target::NVIC.ISER.setSETENA(1 << target::interrupts::External::EIC);

  // Fan PWM
  setPortMux(FAN_PWM_PIN, target::port::PMUX::PMUXE::E);

  target::TC1.COUNT8.CTRLA.setPRESCALER(
      target::tc::COUNT8::CTRLA::PRESCALER::DIV64);
  target::TC1.COUNT8.CTRLA.setMODE(target::tc::COUNT8::CTRLA::MODE::COUNT8);
  target::TC1.COUNT8.CTRLA.setWAVEGEN(target::tc::COUNT8::CTRLA::WAVEGEN::NPWM);
  target::TC1.COUNT8.PER.setPER(TC1_PER);
  fanPwmPercUpdated();
  target::TC1.COUNT8.INTENSET.setOVF(true);
  target::TC1.COUNT8.CTRLA.setENABLE(true);
  target::NVIC.ISER.setSETENA(1 << target::interrupts::External::TC1);
}
