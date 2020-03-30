class Stepper {
  volatile target::tc::Peripheral *tc;
  unsigned int mask;
  unsigned int bits[7];
  int phase = 0;
  bool dir = true;

public:
  void init(volatile target::tc::Peripheral *tc, int pinA1, int pinA2,
            int pinB1, int pinB2) {

    this->tc = tc;

    mask = (1 << pinA1) | (1 << pinA2) | (1 << pinB1) | (1 << pinB2);

    // bits[0] = 1 << pinA1;
    // bits[1] = 1 << pinA1 | 1 << pinB1;
    // bits[2] = 1 << pinB1;
    // bits[3] = 1 << pinB1 | 1 << pinA2;
    // bits[4] = 1 << pinA2;
    // bits[5] = 1 << pinA2 | 1 << pinB2;
    // bits[6] = 1 << pinB2;
    // bits[7] = 1 << pinB2 | 1 << pinA1;

    bits[0] = 1 << pinA1;
    bits[1] = 1 << pinB1;
    bits[2] = 1 << pinA2;
    bits[3] = 1 << pinB2;

    // bits[0] = 1 << pinA1 | 1 << pinB1;
    // bits[1] = 1 << pinA2 | 1 << pinB1;
    // bits[2] = 1 << pinA2 | 1 << pinB2;
    // bits[3] = 1 << pinA1 | 1 << pinB2;

    target::PORT.DIRSET.setDIRSET(mask);

    tc->COUNT16.CTRLA.setMODE(target::tc::COUNT16::CTRLA::MODE::COUNT16)
        .setPRESCALER(target::tc::COUNT16::CTRLA::PRESCALER::DIV1)
        .setWAVEGEN(target::tc::COUNT16::CTRLA::WAVEGEN::MFRQ);

    tc->COUNT16.INTENSET = 1 << 4;

    setSpeed(30);
  }

  void handleTimeInterrupt() {

    target::PORT.OUTCLR.setOUTCLR(mask);
    target::PORT.OUTSET.setOUTSET(bits[phase ^ dir]);
    phase = (phase + 1) & 0x03;

    tc->COUNT16.INTFLAG = 1 << 4;
  }

  void setSpeed(int speed) {
    bool newDir = speed > 0;
    if (!newDir) {
      speed = -speed;
    } 
    
    if (dir != newDir || !speed) {
        tc->COUNT16.CTRLA.setENABLE(false);
        target::PORT.OUTCLR.setOUTCLR(mask);
        dir = newDir;
    }

    if (speed) {
        tc->COUNT16.CC[0].setCC(0xFFFF / speed);
        tc->COUNT16.CTRLA.setENABLE(true);
    }
  }
};
