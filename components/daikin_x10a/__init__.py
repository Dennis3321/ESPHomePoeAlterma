import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, text_sensor
from esphome.const import CONF_ID, UNIT_CELSIUS
from esphome.core import ID

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@local"]

CONF_UART_ID = "uart_id"
CONF_REGISTERS = "registers"

# Convids that produce text values (must match is_text_convid() in register_definitions.h)
TEXT_CONVIDS = {200, 201, 203, 204, 211, 217, 300, 301, 302, 303, 304, 305, 306, 307, 315, 316}

REGISTER_SCHEMA = cv.Schema({
    cv.Required("mode"): cv.int_,
    cv.Required("convid"): cv.int_,
    cv.Required("offset"): cv.int_,
    cv.Required("registryID"): cv.int_,
    cv.Required("dataSize"): cv.int_,
    cv.Required("dataType"): cv.int_,
    cv.Required("label"): cv.string,
    cv.Optional("unit", default=""): cv.string,
    cv.Optional("device_class", default=""): cv.string,
    cv.Optional("accuracy_decimals", default=1): cv.int_,
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

# Add method to register dynamic text sensors
DaikinX10A.add_method(
    "register_dynamic_text_sensor",
    cg.void,
    [cg.std_string, text_sensor.TextSensor.operator("ptr")],
)

# Add method to update dynamic text sensors
DaikinX10A.add_method(
    "update_text_sensor",
    cg.void,
    [cg.std_string, cg.std_string],
)

# Expose last successful read timestamp for diagnostics
DaikinX10A.add_method(
    "last_successful_read_ms",
    cg.uint32,
    [],
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(DaikinX10A),
        cv.Required(CONF_UART_ID): cv.use_id(uart.UARTComponent),
        cv.Optional("mode", default=0): cv.int_,
        cv.Optional(CONF_REGISTERS): cv.ensure_list(REGISTER_SCHEMA),
    }
).extend(cv.COMPONENT_SCHEMA)


def _sanitize_label(label):
    """Sanitize label for use as C++ identifier."""
    return (label
            .lower()
            .replace(" ", "_")
            .replace(".", "")
            .replace("(", "")
            .replace(")", "")
            .replace("/", "_"))


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

            # AUTO-CREATE SENSOR/TEXT_SENSOR for mode=1 registers
            if r["mode"] == 1:
                label_sanitized = _sanitize_label(r["label"])
                is_text = r["convid"] in TEXT_CONVIDS

                if is_text:
                    # Create text_sensor for text-type convids
                    ts_id = ID(f"daikin_{label_sanitized}", is_declaration=True, type=text_sensor.TextSensor)
                    ts = cg.new_Pvariable(ts_id)
                    cg.add(ts.set_name(r["label"]))
                    cg.add(cg.App.register_text_sensor(ts))
                    cg.add(var.register_dynamic_text_sensor(r["label"], ts))
                else:
                    # Create numeric sensor with per-register metadata
                    sensor_id = ID(f"daikin_{label_sanitized}", is_declaration=True, type=sensor.Sensor)
                    sens = cg.new_Pvariable(sensor_id)
                    cg.add(sens.set_name(r["label"]))

                    unit = r.get("unit", "")
                    device_class = r.get("device_class", "")
                    accuracy = r.get("accuracy_decimals", 1)

                    if unit:
                        cg.add(sens.set_unit_of_measurement(unit))
                    if device_class:
                        cg.add(sens.set_device_class(device_class))
                    cg.add(sens.set_state_class(sensor.StateClasses.STATE_CLASS_MEASUREMENT))
                    cg.add(sens.set_accuracy_decimals(accuracy))

                    cg.add(cg.App.register_sensor(sens))
                    cg.add(var.register_dynamic_sensor(r["label"], sens))
