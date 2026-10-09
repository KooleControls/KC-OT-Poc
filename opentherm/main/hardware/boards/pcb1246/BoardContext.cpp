#include "BoardContext.h"

void BoardContext::Init()
{
    lpc::EnableClock(lpc::CLK_GPIO | lpc::CLK_IOCON);

    thermostatPower_.Init();
    gatewaySerial_.Init(BoardConfig::GATEWAY_BAUD);
    thermostat_.Init();
    boiler_.Init();

    // Last: from here on the main loop has to keep feeding it.
    watchdog_.Init(BoardConfig::WATCHDOG_TIMEOUT_S);
}

OpenThermLine &BoardContext::GetOpenThermLine(OpenThermSide side)
{
    switch (side)
    {
    case OpenThermSide::Thermostat: return thermostat_;
    case OpenThermSide::Boiler:     return boiler_;
    }
    return thermostat_;
}
