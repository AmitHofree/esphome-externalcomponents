import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_ID

AUTO_LOAD = ["climate_ir"]

kelvinator_ns = cg.esphome_ns.namespace("kelvinator")
KelvinatorClimate = kelvinator_ns.class_("KelvinatorClimate", climate_ir.ClimateIR)

CONFIG_SCHEMA = cv.All(
    climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(KelvinatorClimate),
        }
    ),
    cv.only_with_arduino
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await climate_ir.register_climate_ir(var, config)
    cg.add_library("crankyoldgit/IRremoteESP8266", "2.8.6")
    