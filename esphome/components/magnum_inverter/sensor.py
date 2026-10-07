import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_FREQUENCY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_HERTZ,
    UNIT_HOUR,
    UNIT_PERCENT,
    UNIT_SECOND,
    UNIT_VOLT,
    UNIT_WATT,
)
from . import MagnumInverter

UNIT_AMPERE_HOURS = "Ah"

# Inverter sensors
CONF_BATTERY_VOLTAGE = "battery_voltage"
CONF_DC_AMPS = "dc_amps"
CONF_AC_OUT_VOLTAGE = "ac_out_voltage"
CONF_AC_IN_VOLTAGE = "ac_in_voltage"
CONF_AC_OUT_AMPS = "ac_out_amps"
CONF_AC_IN_AMPS = "ac_in_amps"
CONF_AC_OUT_WATTS = "ac_out_watts"
CONF_BATTERY_WATTS = "battery_watts"
CONF_FREQUENCY = "frequency"
CONF_BATTERY_TEMP = "battery_temp"
CONF_FET_TEMP = "fet_temp"
CONF_XFMR_TEMP = "xfmr_temp"
CONF_INVERTER_FAULT_CODE = "inverter_fault_code"
CONF_INVERTER_FAULT_ACTIVE = "inverter_fault_active"
CONF_INVERTER_REVISION = "inverter_revision"

# BMK sensors
CONF_BMK_SOC = "bmk_soc"
CONF_BMK_VOLTAGE = "bmk_voltage"
CONF_BMK_AMPS = "bmk_amps"
CONF_BMK_MIN_VOLTAGE = "bmk_min_voltage"
CONF_BMK_MAX_VOLTAGE = "bmk_max_voltage"
CONF_BMK_AH_INOUT = "bmk_ah_inout"
CONF_BMK_AH_TRIP = "bmk_ah_trip"
CONF_BMK_CUMULATIVE_AH = "bmk_cumulative_ah"
CONF_BMK_REVISION = "bmk_revision"
CONF_BMK_WATTS = "bmk_watts"

# Remote/ARTR sensors
CONF_REMOTE_SEARCHWATTS = "remote_searchwatts"
CONF_REMOTE_CHARGERAMPS = "remote_chargeramps"
CONF_REMOTE_ABSORB = "remote_absorb"
CONF_REMOTE_FLOAT = "remote_float"
CONF_REMOTE_EQ = "remote_eq"
CONF_REMOTE_LBCO = "remote_lbco"
CONF_REMOTE_BATTERY_SIZE = "remote_battery_size"
CONF_REMOTE_SHORE_AMPS = "remote_shore_amps"
CONF_REMOTE_VAC_CUTOUT = "remote_vac_cutout"
CONF_REMOTE_ABSORB_TIME = "remote_absorb_time"

# RTR sensors
CONF_RTR_REVISION = "rtr_revision"
CONF_RTR_FRAMES = "rtr_frames"

# Diagnostics
CONF_INV_FRAMES = "inverter_frames"
CONF_BMK_FRAMES = "bmk_frames"
CONF_REMOTE_FRAMES = "remote_frames"
CONF_REJECTED = "rejected_frames"
CONF_LAST_FRAME_AGE = "last_frame_age"

# Fault latch / history
CONF_LAST_FAULT_CODE = "last_fault_code"


def _measure(unit, device_class, decimals, **kwargs):
    return sensor.sensor_schema(
        unit_of_measurement=unit,
        device_class=device_class,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=decimals,
        **kwargs,
    )


def _setting(unit, device_class, decimals, icon=cv.UNDEFINED):
    # Read-only charger settings broadcast by the remote
    return sensor.sensor_schema(
        unit_of_measurement=unit,
        device_class=device_class,
        accuracy_decimals=decimals,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon=icon,
    )


def _diagnostic(decimals, icon, **kwargs):
    return sensor.sensor_schema(
        accuracy_decimals=decimals,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon=icon,
        **kwargs,
    )


# config key -> (setter, schema)
_SENSORS = {
    # Inverter
    CONF_BATTERY_VOLTAGE: ("set_battery_voltage_sensor", _measure(UNIT_VOLT, DEVICE_CLASS_VOLTAGE, 1)),
    CONF_DC_AMPS: ("set_dc_amps_sensor", _measure(UNIT_AMPERE, DEVICE_CLASS_CURRENT, 0)),
    CONF_AC_OUT_VOLTAGE: ("set_ac_out_voltage_sensor", _measure(UNIT_VOLT, DEVICE_CLASS_VOLTAGE, 0)),
    CONF_AC_IN_VOLTAGE: ("set_ac_in_voltage_sensor", _measure(UNIT_VOLT, DEVICE_CLASS_VOLTAGE, 0)),
    CONF_AC_OUT_AMPS: ("set_ac_out_amps_sensor", _measure(UNIT_AMPERE, DEVICE_CLASS_CURRENT, 0)),
    CONF_AC_IN_AMPS: ("set_ac_in_amps_sensor", _measure(UNIT_AMPERE, DEVICE_CLASS_CURRENT, 0)),
    CONF_AC_OUT_WATTS: ("set_ac_out_watts_sensor", _measure(UNIT_WATT, DEVICE_CLASS_POWER, 0)),
    CONF_BATTERY_WATTS: ("set_battery_watts_sensor", _measure(UNIT_WATT, DEVICE_CLASS_POWER, 1)),
    CONF_FREQUENCY: ("set_frequency_sensor", _measure(UNIT_HERTZ, DEVICE_CLASS_FREQUENCY, 1)),
    CONF_BATTERY_TEMP: ("set_battery_temp_sensor", _measure(UNIT_CELSIUS, DEVICE_CLASS_TEMPERATURE, 0)),
    CONF_FET_TEMP: ("set_fet_temp_sensor", _measure(UNIT_CELSIUS, DEVICE_CLASS_TEMPERATURE, 0)),
    CONF_XFMR_TEMP: ("set_xfmr_temp_sensor", _measure(UNIT_CELSIUS, DEVICE_CLASS_TEMPERATURE, 0)),
    CONF_INVERTER_FAULT_CODE: ("set_inverter_fault_code_sensor", _diagnostic(0, "mdi:alert-circle-outline")),
    CONF_INVERTER_FAULT_ACTIVE: ("set_inverter_fault_active_sensor", sensor.sensor_schema(accuracy_decimals=0)),
    CONF_INVERTER_REVISION: ("set_inverter_revision_sensor", _diagnostic(1, "mdi:chip")),
    # BMK
    CONF_BMK_SOC: ("set_bmk_soc_sensor", _measure(UNIT_PERCENT, DEVICE_CLASS_BATTERY, 0)),
    CONF_BMK_VOLTAGE: ("set_bmk_voltage_sensor", _measure(UNIT_VOLT, DEVICE_CLASS_VOLTAGE, 2)),
    CONF_BMK_AMPS: ("set_bmk_amps_sensor", _measure(UNIT_AMPERE, DEVICE_CLASS_CURRENT, 1)),
    CONF_BMK_MIN_VOLTAGE: ("set_bmk_min_voltage_sensor", _measure(UNIT_VOLT, DEVICE_CLASS_VOLTAGE, 2)),
    CONF_BMK_MAX_VOLTAGE: ("set_bmk_max_voltage_sensor", _measure(UNIT_VOLT, DEVICE_CLASS_VOLTAGE, 2)),
    CONF_BMK_AH_INOUT: (
        "set_bmk_ah_inout_sensor",
        _measure(UNIT_AMPERE_HOURS, cv.UNDEFINED, 0, icon="mdi:battery-sync"),
    ),
    CONF_BMK_AH_TRIP: (
        "set_bmk_ah_trip_sensor",
        _measure(UNIT_AMPERE_HOURS, cv.UNDEFINED, 1, icon="mdi:battery-clock"),
    ),
    CONF_BMK_CUMULATIVE_AH: (
        "set_bmk_cumulative_ah_sensor",
        sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE_HOURS,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            accuracy_decimals=0,
            icon="mdi:battery-arrow-down",
        ),
    ),
    CONF_BMK_REVISION: ("set_bmk_revision_sensor", _diagnostic(1, "mdi:chip")),
    CONF_BMK_WATTS: ("set_bmk_watts_sensor", _measure(UNIT_WATT, DEVICE_CLASS_POWER, 1)),
    # Remote/ARTR
    CONF_REMOTE_SEARCHWATTS: ("set_remote_searchwatts_sensor", _setting(UNIT_WATT, DEVICE_CLASS_POWER, 0)),
    CONF_REMOTE_CHARGERAMPS: (
        "set_remote_chargeramps_sensor",
        _setting(UNIT_PERCENT, cv.UNDEFINED, 0, "mdi:battery-charging"),
    ),
    CONF_REMOTE_ABSORB: ("set_remote_absorb_sensor", _setting(UNIT_VOLT, DEVICE_CLASS_VOLTAGE, 1)),
    CONF_REMOTE_FLOAT: ("set_remote_float_sensor", _setting(UNIT_VOLT, DEVICE_CLASS_VOLTAGE, 1)),
    CONF_REMOTE_EQ: ("set_remote_eq_sensor", _setting(UNIT_VOLT, DEVICE_CLASS_VOLTAGE, 1)),
    CONF_REMOTE_LBCO: ("set_remote_lbco_sensor", _setting(UNIT_VOLT, DEVICE_CLASS_VOLTAGE, 1)),
    CONF_REMOTE_BATTERY_SIZE: (
        "set_remote_battery_size_sensor",
        _setting(UNIT_AMPERE_HOURS, cv.UNDEFINED, 0, "mdi:car-battery"),
    ),
    CONF_REMOTE_SHORE_AMPS: ("set_remote_shore_amps_sensor", _setting(UNIT_AMPERE, DEVICE_CLASS_CURRENT, 0)),
    CONF_REMOTE_VAC_CUTOUT: ("set_remote_vac_cutout_sensor", _setting(cv.UNDEFINED, cv.UNDEFINED, 0, "mdi:sine-wave")),
    CONF_REMOTE_ABSORB_TIME: ("set_remote_absorb_time_sensor", _setting(UNIT_HOUR, DEVICE_CLASS_DURATION, 1)),
    # RTR
    CONF_RTR_REVISION: ("set_rtr_revision_sensor", _diagnostic(1, "mdi:chip")),
    CONF_RTR_FRAMES: ("set_rtr_frames_sensor", _diagnostic(0, "mdi:counter")),
    # Diagnostics
    CONF_INV_FRAMES: ("set_inverter_frames_sensor", _diagnostic(0, "mdi:counter")),
    CONF_BMK_FRAMES: ("set_bmk_frames_sensor", _diagnostic(0, "mdi:counter")),
    CONF_REMOTE_FRAMES: ("set_remote_frames_sensor", _diagnostic(0, "mdi:counter")),
    CONF_REJECTED: ("set_rejected_frames_sensor", _diagnostic(0, "mdi:counter")),
    CONF_LAST_FRAME_AGE: (
        "set_last_frame_age_sensor",
        _diagnostic(1, "mdi:timer-outline", unit_of_measurement=UNIT_SECOND, device_class=DEVICE_CLASS_DURATION),
    ),
    # Fault history
    CONF_LAST_FAULT_CODE: ("set_last_fault_code_sensor", sensor.sensor_schema(accuracy_decimals=0)),
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.use_id(MagnumInverter),
        **{cv.Optional(key): schema for key, (_, schema) in _SENSORS.items()},
    }
)


async def to_code(config):
    var = await cg.get_variable(config[CONF_ID])
    for conf_key, (setter_name, _) in _SENSORS.items():
        if conf_key in config:
            s = await sensor.new_sensor(config[conf_key])
            cg.add(getattr(var, setter_name)(s))
