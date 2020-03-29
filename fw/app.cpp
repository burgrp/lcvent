int LED_PIN = 23;

class ToggleTimer : public genericTimer::Timer {

	void onTimer() {
		target::PORT.OUTTGL.setOUTTGL(1 << LED_PIN);
		start(10);
	}

};

ToggleTimer timer;

void initApplication() {
	target::PM.APBBMASK.setPORT(1);
	target::PORT.DIR.setDIR(1 << LED_PIN);
	timer.start(10);
}
