import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor
from esphome.const import CONF_ID, UNIT_CELSIUS
from esphome.core import ID

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

# Add method to register dynamic sensors
DaikinX10A.add_method(
    "register_dynamic_sensor",
    cg.void,
    [cg.std_string, sensor.Sensor.operator("ptr")],
)

# Add method to update dynamic sensors
DaikinX10A.add_method(
    "update_sensor",
    cg.void,
    [cg.std_string, cg.float_],
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
        for idx, r in enumerate(config[CONF_REGISTERS]):
            # Add register to component
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

            # AUTO-CREATE SENSOR for mode=1 registers
            if r["mode"] == 1:
                # Sanitize label for C++ identifier
                label_sanitized = (r["label"]
                                  .lower()
                                  .replace(" ", "_")
                                  .replace(".", "")
                                  .replace("(", "")
                                  .replace(")", "")
                                  .replace("/", "_"))

                # Create unique ID for this sensor
                sensor_id = ID(f"daikin_{label_sanitized}", is_declaration=True, type=sensor.Sensor)

                # Create sensor directly (bypass full validation pipeline)
                sens = cg.new_Pvariable(sensor_id)

                # Configure sensor properties
                cg.add(sens.set_name(r["label"]))
                cg.add(sens.set_unit_of_measurement(UNIT_CELSIUS))
                cg.add(sens.set_device_class("temperature"))
                cg.add(sens.set_state_class("measurement"))
                cg.add(sens.set_accuracy_decimals(1))

                # Register with App
                cg.add(cg.App.register_sensor(sens))

                # Link sensor to component so it can publish updates
                cg.add(var.register_dynamic_sensor(r["label"], sens))
