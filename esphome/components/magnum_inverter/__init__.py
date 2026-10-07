import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import time, uart
from esphome.const import CONF_ID, CONF_TIME_ID

CODEOWNERS = ["@kkrankall"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "text_sensor", "binary_sensor", "button"]

CONF_STALE_TIMEOUT = "stale_timeout"

magnum_ns = cg.esphome_ns.namespace("magnum_inverter")
MagnumInverter = magnum_ns.class_("MagnumInverter", cg.Component, uart.UARTDevice)


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MagnumInverter),
            cv.Optional(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
            # Sensors go unknown when no frames arrive for this long.
            cv.Optional(
                CONF_STALE_TIMEOUT, default="10s"
            ): cv.positive_time_period_milliseconds,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "magnum_inverter",
    baud_rate=19200,
    require_rx=True,
    data_bits=8,
    parity="NONE",
    stop_bits=1,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_stale_timeout(config[CONF_STALE_TIMEOUT]))

    if CONF_TIME_ID in config:
        t = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time(t))
