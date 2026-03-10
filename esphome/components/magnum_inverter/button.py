import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID
from . import MagnumInverter, magnum_ns

RecordPacketsButton = magnum_ns.class_("RecordPacketsButton", button.Button)
ToggleInverterButton = magnum_ns.class_("ToggleInverterButton", button.Button)

CONF_RECORD_PACKETS = "record_packets"
CONF_TOGGLE_INVERTER = "toggle_inverter"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.use_id(MagnumInverter),
        cv.Optional(CONF_RECORD_PACKETS): button.button_schema(RecordPacketsButton),
        cv.Optional(CONF_TOGGLE_INVERTER): button.button_schema(ToggleInverterButton),
    }
)


async def to_code(config):
    var = await cg.get_variable(config[CONF_ID])
    if CONF_RECORD_PACKETS in config:
        btn = await button.new_button(config[CONF_RECORD_PACKETS])
        cg.add(btn.set_parent(var))
    if CONF_TOGGLE_INVERTER in config:
        btn = await button.new_button(config[CONF_TOGGLE_INVERTER])
        cg.add(btn.set_parent(var))
