"""Denon/Sharp 15-bit IR protocol for ESPHome.

Frame: 5 address bits + 10 command bits (MSB first, in transmission order) + stop mark.
Every code is sent twice: the normal frame, ~43.6 ms gap, then the frame with the
10 command bits inverted. Codes match Tasmota's DENON protocol with Bits=15.
"""

import inspect

import esphome.codegen as cg
from esphome import automation
from esphome.components import remote_base
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TRIGGER_ID

AUTO_LOAD = ["remote_base"]
MULTI_CONF = True

CONF_TRANSMITTER_ID = "transmitter_id"
CONF_RECEIVER_ID = "receiver_id"
CONF_ON_CODE = "on_code"
CONF_CODE = "code"
CONF_ADDRESS = "address"
CONF_COMMAND = "command"
CONF_REPEAT = "repeat"

denon_ir_ns = cg.esphome_ns.namespace("denon_ir")
DenonIR = denon_ir_ns.class_(
    "DenonIR",
    cg.Component,
    remote_base.remote_base_ns.class_("RemoteReceiverListener"),
)
CodeTrigger = denon_ir_ns.class_("CodeTrigger", automation.Trigger.template(cg.uint16))
TransmitAction = denon_ir_ns.class_("TransmitAction", automation.Action)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(DenonIR),
        cv.Optional(CONF_TRANSMITTER_ID): cv.use_id(remote_base.RemoteTransmitterBase),
        cv.Optional(CONF_RECEIVER_ID): cv.use_id(remote_base.RemoteReceiverBase),
        cv.Optional(CONF_ON_CODE): automation.validate_automation(
            {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(CodeTrigger)}
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


def _add_listener(receiver, listener):
    # Newer ESPHome sizes listener lists at compile time and requires add_listener().
    if hasattr(remote_base, "add_listener"):
        remote_base.add_listener(receiver, listener)
    else:
        cg.add(receiver.register_listener(listener))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_TRANSMITTER_ID in config:
        tx = await cg.get_variable(config[CONF_TRANSMITTER_ID])
        cg.add(var.set_transmitter(tx))

    if CONF_RECEIVER_ID in config:
        rx = await cg.get_variable(config[CONF_RECEIVER_ID])
        _add_listener(rx, var)

    for conf in config.get(CONF_ON_CODE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.uint16, "x")], conf)


def _validate_transmit(conf):
    has_code = CONF_CODE in conf
    has_address = CONF_ADDRESS in conf
    has_command = CONF_COMMAND in conf
    if has_code and (has_address or has_command):
        raise cv.Invalid("Use either 'code' or 'address' + 'command', not both")
    if not has_code and not (has_address and has_command):
        raise cv.Invalid("Either 'code' or both 'address' and 'command' are required")
    return conf


TRANSMIT_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DenonIR),
            cv.Optional(CONF_CODE): cv.templatable(cv.int_range(min=0, max=0x7FFF)),
            cv.Optional(CONF_ADDRESS): cv.templatable(cv.int_range(min=0, max=0x1F)),
            cv.Optional(CONF_COMMAND): cv.templatable(cv.int_range(min=0, max=0x3FF)),
            cv.Optional(CONF_REPEAT, default=1): cv.templatable(
                cv.int_range(min=1, max=20)
            ),
        }
    ),
    _validate_transmit,
)


# Newer ESPHome wants synchronous= on every action; older versions don't know it.
_ACTION_KWARGS = (
    {"synchronous": True}
    if "synchronous" in inspect.signature(automation.register_action).parameters
    else {}
)


@automation.register_action(
    "denon_ir.transmit", TransmitAction, TRANSMIT_SCHEMA, **_ACTION_KWARGS
)
async def transmit_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    if CONF_CODE in config:
        cg.add(var.set_code(await cg.templatable(config[CONF_CODE], args, cg.uint16)))
    else:
        cg.add(
            var.set_address(await cg.templatable(config[CONF_ADDRESS], args, cg.uint8))
        )
        cg.add(
            var.set_command(await cg.templatable(config[CONF_COMMAND], args, cg.uint16))
        )
    cg.add(var.set_repeat(await cg.templatable(config[CONF_REPEAT], args, cg.uint32)))
    return var
