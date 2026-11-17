import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv

from . import CONF_CODE, DenonIR, denon_ir_ns

DEPENDENCIES = ["denon_ir"]

CONF_DENON_IR_ID = "denon_ir_id"

DenonIRBinarySensor = denon_ir_ns.class_(
    "DenonIRBinarySensor", binary_sensor.BinarySensor
)

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(DenonIRBinarySensor).extend(
    {
        cv.GenerateID(CONF_DENON_IR_ID): cv.use_id(DenonIR),
        cv.Required(CONF_CODE): cv.int_range(min=0, max=0x7FFF),
    }
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    cg.add(var.set_code(config[CONF_CODE]))
    parent = await cg.get_variable(config[CONF_DENON_IR_ID])
    cg.add(parent.register_binary_sensor(var))
