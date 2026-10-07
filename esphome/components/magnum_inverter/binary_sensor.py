import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_BATTERY_CHARGING,
    DEVICE_CLASS_CONNECTIVITY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
from . import MagnumInverter

CONF_CONNECTED = "connected"
CONF_INVERTER_LED = "inverter_led"
CONF_CHARGER_LED = "charger_led"
CONF_INVERTER_FAULT = "inverter_fault"

# config key -> (setter, schema)
_BINARY_SENSORS = {
    # On while frames are arriving from the Magnum network
    CONF_CONNECTED: (
        "set_connected_binary_sensor",
        binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_CONNECTIVITY, entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),
    ),
    CONF_INVERTER_LED: (
        "set_inverter_led_binary_sensor",
        binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_POWER),
    ),
    CONF_CHARGER_LED: (
        "set_charger_led_binary_sensor",
        binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_BATTERY_CHARGING),
    ),
    CONF_INVERTER_FAULT: (
        "set_inverter_fault_binary_sensor",
        binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_PROBLEM),
    ),
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.use_id(MagnumInverter),
        **{cv.Optional(key): schema for key, (_, schema) in _BINARY_SENSORS.items()},
    }
)


async def to_code(config):
    var = await cg.get_variable(config[CONF_ID])
    for conf_key, (setter_name, _) in _BINARY_SENSORS.items():
        if conf_key in config:
            bs = await binary_sensor.new_binary_sensor(config[conf_key])
            cg.add(getattr(var, setter_name)(bs))
