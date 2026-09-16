import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

cerveaux_poll_ns = cg.esphome_ns.namespace("cerveaux_poll")
CerveauxPollComponent = cerveaux_poll_ns.class_("CerveauxPollComponent", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(CerveauxPollComponent),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
