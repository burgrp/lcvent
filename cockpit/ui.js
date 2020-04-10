const deepEqual = require("fast-deep-equal");

module.exports = async config => {

    let events = {
        cockpit: {
            registersUpdate: undefined
        }
    }

    let registers = {};
    let previousRegisters = {};

    let registerConverters = {
        mode: (raw, offset) => ["off", "test"][raw.readInt32LE(offset)],
        pressurePa: (raw, offset) => raw.readInt32LE(offset),
        flowMl: (raw, offset) => raw.readInt32LE(offset),
        fanPwmPerc: (raw, offset) => raw.readInt32LE(offset)
    }

    let registerNames = Object.keys(registerConverters);

    setInterval(async () => {

        try {
            let raw = await config.mcu.read("registers", registerNames.length * 4, 80);
            registerNames.forEach((name, index) => registers[name] = raw ? registerConverters[name](raw, index * 4) : undefined);            
        } catch (e) {
            console.error(e);
            registers = {
                errors: {
                    ERROR_READING_REGISTERS: true
                }
            }
        }

        if (!deepEqual(registers, previousRegisters)) {
            previousRegisters = {...registers};
            console.info("Registers changed");
            events.cockpit.registersUpdate({ registers });
        }

    }, 500);

    return {
        client: __dirname + "/client",
        api: {
            cockpit: {
                async getRegisters() {
                    return registers;
                },
                async setFanPwmPerc({ value }) {
                    await config.mcu.write32("setFanPwmPerc", value);
                }
            }
        },
        events
    }
}