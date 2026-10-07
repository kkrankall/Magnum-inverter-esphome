import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, DEVICE_CLASS_TIMESTAMP, ENTITY_CATEGORY_DIAGNOSTIC
from . import MagnumInverter

CONF_MODE = "mode"
CONF_INVERTER_FAULT = "inverter_fault"
CONF_INVERTER_MODEL = "inverter_model"
CONF_INVERTER_STACKMODE = "inverter_stackmode"
CONF_BMK_FAULT = "bmk_fault"
CONF_LAST_FAULT_TEXT = "last_fault_text"
CONF_LAST_FAULT_TIME = "last_fault_time"
CONF_RTR_MODEL = "rtr_model"
CONF_REMOTE_BATTERY_TYPE = "remote_battery_type"
CONF_INVERTER_LED = "inverter_led"
CONF_CHARGER_LED = "charger_led"
CONF_INVERTER_ON = "inverter_on"


def _diagnostic(icon):
    return text_sensor.text_sensor_schema(entity_category=ENTITY_CATEGORY_DIAGNOSTIC, icon=icon)


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.use_id(MagnumInverter),
        cv.Optional(CONF_MODE): text_sensor.text_sensor_schema(icon="mdi:sine-wave"),
        cv.Optional(CONF_INVERTER_FAULT): text_sensor.text_sensor_schema(icon="mdi:alert-circle-outline"),
        cv.Optional(CONF_INVERTER_MODEL): _diagnostic("mdi:information-outline"),
        cv.Optional(CONF_INVERTER_STACKMODE): _diagnostic("mdi:layers-outline"),
        cv.Optional(CONF_BMK_FAULT): text_sensor.text_sensor_schema(icon="mdi:battery-alert-variant-outline"),
        cv.Optional(CONF_LAST_FAULT_TEXT): text_sensor.text_sensor_schema(icon="mdi:alert-circle-outline"),
        # ISO 8601 in UTC, so Home Assistant shows it as a time ("3 hours ago")
        cv.Optional(CONF_LAST_FAULT_TIME): text_sensor.text_sensor_schema(device_class=DEVICE_CLASS_TIMESTAMP),
        cv.Optional(CONF_RTR_MODEL): _diagnostic("mdi:router-network"),
        cv.Optional(CONF_REMOTE_BATTERY_TYPE): _diagnostic("mdi:car-battery"),
        cv.Optional(CONF_INVERTER_LED): text_sensor.text_sensor_schema(icon="mdi:led-outline"),
        cv.Optional(CONF_CHARGER_LED): text_sensor.text_sensor_schema(icon="mdi:led-outline"),
        cv.Optional(CONF_INVERTER_ON): text_sensor.text_sensor_schema(icon="mdi:power"),
    }
)

_TEXT_SENSOR_MAP = {
    CONF_MODE: "set_mode_text_sensor",
    CONF_INVERTER_FAULT: "set_inverter_fault_text_sensor",
    CONF_INVERTER_MODEL: "set_inverter_model_text_sensor",
    CONF_INVERTER_STACKMODE: "set_inverter_stackmode_text_sensor",
    CONF_BMK_FAULT: "set_bmk_fault_text_sensor",
    CONF_LAST_FAULT_TEXT: "set_last_fault_text_sensor",
    CONF_LAST_FAULT_TIME: "set_last_fault_time_text_sensor",
    CONF_RTR_MODEL: "set_rtr_model_text_sensor",
    CONF_REMOTE_BATTERY_TYPE: "set_remote_battery_type_text_sensor",
    CONF_INVERTER_LED: "set_inverter_led_text_sensor",
    CONF_CHARGER_LED: "set_charger_led_text_sensor",
    CONF_INVERTER_ON: "set_inverter_on_text_sensor",
}

async def to_code(config):
    var = await cg.get_variable(config[CONF_ID])
    for conf_key, setter_name in _TEXT_SENSOR_MAP.items():
        if conf_key in config:
            s = await text_sensor.new_text_sensor(config[conf_key])
            cg.add(getattr(var, setter_name)(s))