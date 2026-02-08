import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, text_sensor
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@local"]

CONF_UART_ID = "uart_id"
CONF_REGISTERS = "registers"

REGISTER_SCHEMA = cv.Schema({
    cv.Required("mode"): cv.int_,
    cv.Required("convid"): cv.hex_int,
    cv.Required("offset"): cv.int_,
    cv.Required("registryID"): cv.int_,
    cv.Required("dataSize"): cv.int_,
    cv.Required("dataType"): cv.int_,
    cv.Required("label"): cv.string,
})

daikin_x10a_ns = cg.esphome_ns.namespace("daikin_x10a")
DaikinX10A = daikin_x10a_ns.class_("DaikinX10A", cg.Component, uart.UARTDevice)

# Expose the method so template sensors can use it
DaikinX10A.add_method(
    "get_register_value",
    cg.std_string,
    [cg.std_string],
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(DaikinX10A),
        cv.Required(CONF_UART_ID): cv.use_id(uart.UARTComponent),
        cv.Required("mode"): cv.int_,
        cv.Optional(CONF_REGISTERS): cv.ensure_list(REGISTER_SCHEMA),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    uart_comp = await cg.get_variable(config[CONF_UART_ID])
    var = cg.new_Pvariable(config[CONF_ID], uart_comp)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_REGISTERS in config:
        for r in config[CONF_REGISTERS]:
            cg.add(
                var.add_register(
                    r["mode"],
                    r["convid"],
                    r["offset"],
                    r["registryID"],
                    r["dataSize"],
                    r["dataType"],
                    r["label"],
                )
            )
            
            # Auto-create text sensor for mode==1 (readable) registers
            if r["mode"] == 1:
                # Create text sensor ID safely
                sens_id = cg.declare_id(text_sensor.TextSensor)
                sens = cg.new_Pvariable(sens_id)
                cg.add(sens.set_name(r["label"]))
                # Register with component
                cg.add(var.set_text_sensor_for_register(r["label"], sens))
