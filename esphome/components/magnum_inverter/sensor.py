import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID
from . import MagnumInverter

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

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.use_id(MagnumInverter),

        # Inverter
        cv.Optional(CONF_BATTERY_VOLTAGE): sensor.sensor_schema(accuracy_decimals=1),
        cv.Optional(CONF_DC_AMPS): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_AC_OUT_VOLTAGE): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_AC_IN_VOLTAGE): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_AC_OUT_AMPS): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_AC_IN_AMPS): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_AC_OUT_WATTS): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_BATTERY_WATTS): sensor.sensor_schema(accuracy_decimals=1),
        cv.Optional(CONF_FREQUENCY): sensor.sensor_schema(accuracy_decimals=1),
        cv.Optional(CONF_BATTERY_TEMP): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_FET_TEMP): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_XFMR_TEMP): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_INVERTER_FAULT_CODE): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_INVERTER_FAULT_ACTIVE): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_INVERTER_REVISION): sensor.sensor_schema(accuracy_decimals=1),

        # BMK
        cv.Optional(CONF_BMK_SOC): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_BMK_VOLTAGE): sensor.sensor_schema(accuracy_decimals=2),
        cv.Optional(CONF_BMK_AMPS): sensor.sensor_schema(accuracy_decimals=1),
        cv.Optional(CONF_BMK_MIN_VOLTAGE): sensor.sensor_schema(accuracy_decimals=2),
        cv.Optional(CONF_BMK_MAX_VOLTAGE): sensor.sensor_schema(accuracy_decimals=2),
        cv.Optional(CONF_BMK_AH_INOUT): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_BMK_AH_TRIP): sensor.sensor_schema(accuracy_decimals=1),
        cv.Optional(CONF_BMK_CUMULATIVE_AH): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_BMK_REVISION): sensor.sensor_schema(accuracy_decimals=1),
        cv.Optional(CONF_BMK_WATTS): sensor.sensor_schema(accuracy_decimals=1),

        # Remote/ARTR
        cv.Optional(CONF_REMOTE_SEARCHWATTS): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_REMOTE_CHARGERAMPS): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_REMOTE_ABSORB): sensor.sensor_schema(accuracy_decimals=1),
        cv.Optional(CONF_REMOTE_FLOAT): sensor.sensor_schema(accuracy_decimals=1),
        cv.Optional(CONF_REMOTE_EQ): sensor.sensor_schema(accuracy_decimals=1),
        cv.Optional(CONF_REMOTE_LBCO): sensor.sensor_schema(accuracy_decimals=1),
        cv.Optional(CONF_REMOTE_BATTERY_SIZE): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_REMOTE_SHORE_AMPS): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_REMOTE_VAC_CUTOUT): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_REMOTE_ABSORB_TIME): sensor.sensor_schema(accuracy_decimals=1),

        # RTR
        cv.Optional(CONF_RTR_REVISION): sensor.sensor_schema(accuracy_decimals=1),
        cv.Optional(CONF_RTR_FRAMES): sensor.sensor_schema(accuracy_decimals=0),

        # Diagnostics
        cv.Optional(CONF_INV_FRAMES): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_BMK_FRAMES): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_REMOTE_FRAMES): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_REJECTED): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_LAST_FRAME_AGE): sensor.sensor_schema(accuracy_decimals=1),

        # Fault history
        cv.Optional(CONF_LAST_FAULT_CODE): sensor.sensor_schema(accuracy_decimals=0),
    }
)

_SENSOR_MAP = {
    CONF_BATTERY_VOLTAGE: "set_battery_voltage_sensor",
    CONF_DC_AMPS: "set_dc_amps_sensor",
    CONF_AC_OUT_VOLTAGE: "set_ac_out_voltage_sensor",
    CONF_AC_IN_VOLTAGE: "set_ac_in_voltage_sensor",
    CONF_AC_OUT_AMPS: "set_ac_out_amps_sensor",
    CONF_AC_IN_AMPS: "set_ac_in_amps_sensor",
    CONF_AC_OUT_WATTS: "set_ac_out_watts_sensor",
    CONF_BATTERY_WATTS: "set_battery_watts_sensor",
    CONF_FREQUENCY: "set_frequency_sensor",
    CONF_BATTERY_TEMP: "set_battery_temp_sensor",
    CONF_FET_TEMP: "set_fet_temp_sensor",
    CONF_XFMR_TEMP: "set_xfmr_temp_sensor",
    CONF_INVERTER_FAULT_CODE: "set_inverter_fault_code_sensor",
    CONF_INVERTER_FAULT_ACTIVE: "set_inverter_fault_active_sensor",
    CONF_INVERTER_REVISION: "set_inverter_revision_sensor",
    CONF_BMK_SOC: "set_bmk_soc_sensor",
    CONF_BMK_VOLTAGE: "set_bmk_voltage_sensor",
    CONF_BMK_AMPS: "set_bmk_amps_sensor",
    CONF_BMK_MIN_VOLTAGE: "set_bmk_min_voltage_sensor",
    CONF_BMK_MAX_VOLTAGE: "set_bmk_max_voltage_sensor",
    CONF_BMK_AH_INOUT: "set_bmk_ah_inout_sensor",
    CONF_BMK_AH_TRIP: "set_bmk_ah_trip_sensor",
    CONF_BMK_CUMULATIVE_AH: "set_bmk_cumulative_ah_sensor",
    CONF_BMK_REVISION: "set_bmk_revision_sensor",
    CONF_BMK_WATTS: "set_bmk_watts_sensor",
    CONF_REMOTE_SEARCHWATTS: "set_remote_searchwatts_sensor",
    CONF_REMOTE_CHARGERAMPS: "set_remote_chargeramps_sensor",
    CONF_REMOTE_ABSORB: "set_remote_absorb_sensor",
    CONF_REMOTE_FLOAT: "set_remote_float_sensor",
    CONF_REMOTE_EQ: "set_remote_eq_sensor",
    CONF_REMOTE_LBCO: "set_remote_lbco_sensor",
    CONF_REMOTE_BATTERY_SIZE: "set_remote_battery_size_sensor",
    CONF_REMOTE_SHORE_AMPS: "set_remote_shore_amps_sensor",
    CONF_REMOTE_VAC_CUTOUT: "set_remote_vac_cutout_sensor",
    CONF_REMOTE_ABSORB_TIME: "set_remote_absorb_time_sensor",
    CONF_RTR_REVISION: "set_rtr_revision_sensor",
    CONF_RTR_FRAMES: "set_rtr_frames_sensor",
    CONF_INV_FRAMES: "set_inverter_frames_sensor",
    CONF_BMK_FRAMES: "set_bmk_frames_sensor",
    CONF_REMOTE_FRAMES: "set_remote_frames_sensor",
    CONF_REJECTED: "set_rejected_frames_sensor",
    CONF_LAST_FRAME_AGE: "set_last_frame_age_sensor",
    CONF_LAST_FAULT_CODE: "set_last_fault_code_sensor",
}

async def to_code(config):
    var = await cg.get_variable(config[CONF_ID])
    for conf_key, setter_name in _SENSOR_MAP.items():
        if conf_key in config:
            s = await sensor.new_sensor(config[conf_key])
            cg.add(getattr(var, setter_name)(s))