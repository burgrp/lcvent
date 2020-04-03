module.exports = async config => {

    let events = {
        cockpit: {
            registersUpdate: undefined
        }
    }

    let registers = {
    }

    let registerConverters = {
        mode: (raw, offset) => ["off", "test"][raw.readUInt32LE(offset)],
        pressurePa: (raw, offset) => raw.readUInt32LE(offset)
    }

    let registerNames = Object.keys(registerConverters);

    let previousRaw;

    setInterval(async () => {

        let raw;

        try {
            raw = await config.mcu.read("registers", registerNames.length * 4, 80);
        } catch (e) {
            console.error("Error reading registares:", e);
        }


        if (((!raw || !previousRaw) && raw !== previousRaw) || Buffer.compare(raw, previousRaw) !== 0) {
            previousRaw = raw;
            console.info("Registers changed");

            registerNames.forEach((name, index) => registers[name] = raw ? registerConverters[name](raw, index * 4) : undefined);

            events.cockpit.registersUpdate({ registers });
        }


    }, 500);

    return {
        client: __dirname + "/client",
        api: {
            cockpit: {
                async getRegisters() {
                    return registers;
                }
            }
        },
        events
    }
}