#pragma once

// ──────────────────────────────────────────────────────────────
// What this product is, and how it is meant to be driven — in the product's own
// words, in the application layer, beside the managers the words are about.
//
// It lives here and not on whatever talks to the device for one reason: whatever
// answers "how do I use this device" has to be the firmware that is actually running.
// A copy kept elsewhere is a copy that describes last month's build, and the one
// caller who cannot notice is the one most likely to act on it.
//
// Registered into the framework from AppContext::Init(), like every other thing the
// application tells Strux about itself (see SystemManager::SetDocumentation). Strux
// never reaches up for it.
//
// Two strings, on purpose:
//
//   DESCRIPTION   one line. Sentence, not paragraph.
//
//   INSTRUCTIONS  free-form, as long as it needs to be, served only when asked for
//                 (`system describe`). Deliberately prose and NOT a schema: every
//                 command already declares its own name, arguments, types and
//                 required/optional state, and this is for the part that never fits
//                 in a declaration — which commands belong together, what order they
//                 go in, what the units are, what will not work and why.
//
// Fill both in as the OpenTherm feature lands.
// ──────────────────────────────────────────────────────────────
namespace DeviceDoc {

inline constexpr const char* DESCRIPTION =
    "PCB1246 OpenTherm plug-in board - an OpenTherm modem for the PCB1245 gateway.";

inline constexpr const char* INSTRUCTIONS =
    "This is the PCB1246 OpenTherm proof-of-concept firmware. It is a modem: it "
    "puts OpenTherm frames on two lines and takes them off, and does not look "
    "inside them. Parity, message types, data ids and what to answer are all the "
    "gateway's.\n"
    "\n"
    "The two lines\n"
    "  'thermostat' faces the room thermostat - this board plays the boiler "
    "towards it. 'boiler' faces the boiler - this board plays the thermostat. A "
    "frame is the 32 bits between the start and stop bits, parity bit included, "
    "as one uint32.\n"
    "\n"
    "Receiving\n"
    "  Every frame either line receives is pushed to the gateway as it arrives, on "
    "a stream this board opens itself, named {\"type\":\"opentherm stream\"}. One "
    "JSON line per frame: {\"side\":\"boiler\",\"frame\":3221291008,\"ms\":123456}. "
    "'ms' is this board's millisecond clock when the frame was collected, about "
    "12 ms after its stop bit; differences between two of them are exact to the "
    "millisecond. Frames that arrive while the link is down are dropped, not "
    "queued. RESET the stream to stop it; the next frame opens it again.\n"
    "\n"
    "Sending\n"
    "  `opentherm send side=boiler frame=0x80000000` starts one frame and answers "
    "once it has started. A frame takes 34 ms on the line; a send while the "
    "previous one is still going out answers ok=false, error=busy, and is not "
    "queued.\n"
    "\n"
    "Diagnostics\n"
    "  `opentherm status` has per-line counters - received, forwarded, sent, "
    "refused as busy, decode errors, overruns. `link status` has the serial "
    "link's: handshake phase and frame counts. A rising 'corrupt' count there "
    "means noise on the UART or a baud-rate mismatch.\n"
    "\n"
    "Discovering what it can do\n"
    "  Every command declares itself. `help` returns every command this firmware "
    "has, with its description and each of its arguments - name, type, whether it "
    "is required, and what it means - in one reply.\n"
    "\n"
    "Settings\n"
    "  `settings list` enumerates every setting on the device with its type and "
    "current value; `settings set` changes one in RAM. Keys are dotted "
    "(`device.name`) and are at most 15 characters. This board has no persistent "
    "storage yet, so `settings save` reports failure and every setting returns to "
    "its default at reboot.\n"
    "\n"
    "Things worth knowing before driving this device\n"
    "  * `system reboot` answers first and then restarts about half a second later, "
    "so anything in flight is lost.\n"
    "  * The clock is not set until something sets it; there is no network here to "
    "fetch the time from.\n"
    "  * Numbers in arguments are unsigned 32-bit. There are no floating-point "
    "arguments anywhere in the protocol.";

} // namespace DeviceDoc
