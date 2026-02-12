import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, switch
from esphome.const import CONF_ID

from . import daikin_x10a_ns, DaikinX10A

CONF_DAIKIN_X10A_ID = "daikin_x10a_id"
CONF_CURRENT_TEMP_SOURCE = "current_temperature_source"
CONF_HEAT_RELAY = "heat_relay"
CONF_COOL_RELAY = "cool_relay"

DaikinX10AClimate = daikin_x10a_ns.class_(
    "DaikinX10AClimate", climate.Climate, cg.Component
)

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(DaikinX10AClimate).extend(
        {
            cv.Required(CONF_DAIKIN_X10A_ID): cv.use_id(DaikinX10A),
            cv.Optional(
                CONF_CURRENT_TEMP_SOURCE,
                default="Leaving water temp. before BUH (R1T)",
            ): cv.string,
            cv.Required(CONF_HEAT_RELAY): cv.use_id(switch.Switch),
            cv.Required(CONF_COOL_RELAY): cv.use_id(switch.Switch),
        }
    ).extend(cv.COMPONENT_SCHEMA),
)


async def to_code(config):
    var = await climate.new_climate(config)
    await cg.register_component(var, config)

    parent = await cg.get_variable(config[CONF_DAIKIN_X10A_ID])
    cg.add(var.set_parent(parent))

    cg.add(var.set_current_temperature_source(config[CONF_CURRENT_TEMP_SOURCE]))

    heat_relay = await cg.get_variable(config[CONF_HEAT_RELAY])
    cg.add(var.set_heat_relay(heat_relay))

    cool_relay = await cg.get_variable(config[CONF_COOL_RELAY])
    cg.add(var.set_cool_relay(cool_relay))
