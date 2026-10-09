#pragma once

#include "interfaces/SerialPort.h"
#include "interfaces/OpenThermLine.h"
#include "interfaces/DigitalOutput.h"

// ──────────────────────────────────────────────────────────────
// The board layer's provider: the ROLES every board owes, in application
// vocabulary. `interfaces/` holds the individual roles; this states which of
// them a board must bind.
//
// The bottom layer's half of the same context/provider pair the two layers
// above use — BoardContext owns the driver instances and answers this.
//
// Deliberately roles ONLY. A concrete driver accessor (the escape hatch, for
// when the application needs a driver's full API) stays off this interface and
// on BoardContext itself, where it is checked at compile time. That is what
// keeps this list from becoming the union of every board's peripherals.
//
// Multi-instance roles get a semantic enum parameter.
// ──────────────────────────────────────────────────────────────

/// Which side of the board an OpenTherm line faces.
enum class OpenThermSide
{
    Thermostat,   // the room thermostat: this board plays the boiler towards it
    Boiler,       // the boiler: this board plays the thermostat towards it
};

class BoardProvider
{
public:
    virtual ~BoardProvider() = default;

    /// The serial link to the PCB1245 gateway.
    virtual SerialPort& GetGatewaySerial() = 0;

    virtual OpenThermLine& GetOpenThermLine(OpenThermSide side) = 0;

    /// Switches the power the board feeds to the room thermostat.
    virtual DigitalOutput& GetThermostatPower() = 0;
};
