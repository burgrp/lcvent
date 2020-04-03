
require("@device.farm/appglue")({require, file: __dirname + "/config.json"}).main(async config => {
    //await config.mcu.flash();
    await config.webglue.start();
});