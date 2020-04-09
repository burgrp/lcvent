let pa2cmh2o = pa => Math.round(pa * 0.0101972 * 10) / 10;

wg.pages.home = {
    async render(container) {

        function update(registers) {
            $(".pressurePa>.value").text(pa2cmh2o(registers.pressurePa));
            $(".mode>.value").text(registers.mode.toUpperCase());
            $(".json.registers").text(JSON.stringify(registers, null, 2))
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
                    DIV("value mode")
                ]),
                DIV("json registers")
            ]).onCockpitRegistersUpdate(({ registers }) => {
                update(registers);
            })
        )

        update(await wg.cockpit.getRegisters());
    }
}