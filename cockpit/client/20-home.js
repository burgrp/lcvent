let pa2cmh2o = pa => Math.round(pa * 0.0101972 * 10) / 10;

wg.pages.home = {
    async render(container) {

        let registers;

        function update(r) {
            registers = r;
            $(".json.registers").text(JSON.stringify(registers, null, 2));
            $(".pressurePa>.value").text(pa2cmh2o(registers.pressurePa));
            $(".mode>.value").text((registers.mode || "ERROR").toUpperCase());
            $(".fan>.value").text(registers.fanPwmPerc);
        }

        async function changeFanPwm(change) {
            try {
                let v = registers.fanPwmPerc + change;
                if (v < 0) {
                    v = 0;
                }
                if (v > 100) {
                    v = 100;
                }
                await wg.cockpit.setFanPwmPerc({ value: v });
            } catch (e) {
                console.error(e);
            }

        }

        container.append(
            DIV("home", [
                DIV("pressurePa", [
                    DIV("label").text("Pressure"),
                    DIV("value"),
                    DIV("unit").html("cm H<sub>2</sub>O")
                ]),
                DIV("mode", [
                    DIV().text("Mode"),
                    DIV("value")
                ]),
                DIV("fan", [
                    DIV().text("Fan"),
                    DIV("value"),
                    DIV("unit").html("%"),
                    DIV("buttons", [
                        BUTTON().text("0").click(e => changeFanPwm(-100)),
                        BUTTON().text("<<").click(e => changeFanPwm(-20)),
                        BUTTON().text("<").click(e => changeFanPwm(-1)),
                        BUTTON().text(">").click(e => changeFanPwm(+1)),
                        BUTTON().text(">>").click(e => changeFanPwm(+20)),
                        BUTTON().text("100").click(e => changeFanPwm(+100))
                    ])
                ]),
                DIV("json registers")
            ]).onCockpitRegistersUpdate(({ registers }) => {
                update(registers);
            })
        )

        update(await wg.cockpit.getRegisters());
    }
}