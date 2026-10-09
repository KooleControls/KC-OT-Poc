# KC-OT-Poc

Proof of concept for OpenTherm on the KC1245 gateway and the PCB1246 OpenTherm plug-in board, rebuilt from scratch on the Strux framework. Lives at [KooleControls/KC-OT-Poc](https://github.com/KooleControls/KC-OT-Poc) (private).

- `gateway/`: ESP32 firmware for the KC1245 gateway. A copy of [Strux](https://github.com/vanBassum/Strux) at commit `3c2e4ca`.
- `opentherm/`: LPC11U68 firmware for the PCB1246 (bare metal, no RTOS, Strux-style). Laid out like the gateway: the board, the Strux command/settings/system managers, and the OpenTherm modem with its link to the gateway.

Each folder holds its full source; nothing is shared between the two.

Decided so far:
- The LPC runs bare metal: one main loop, interrupts only set flags and fill buffers. No RTOS.
- The gateway and the PCB1246 talk over the UART using the Strux channel protocol instead of KC2.
- On the UART, each channel frame is COBS-framed with a CRC-16: `0x00 | COBS(frame | crc16) | 0x00`. A frame that fails the check is dropped and counted. The code is `lib/protocol/SerialFraming.h`, meant to be the same file on both sides.
- The PCB1246 is a modem and does not look inside a frame. Frames from either line go to the gateway on a stream the PCB1246 opens (`opentherm stream`); the gateway sends with `opentherm send side=<thermostat|boiler> frame=<uint32>`. Everything else is the gateway's.
- What the proof of concept does with OpenTherm is still open.

## Build

Gateway, board `pcb1245`, with ESP-IDF 6.0:

```sh
cd gateway
idf.py -DBOARD=pcb1245 set-target esp32
idf.py -DBOARD=pcb1245 build
```

OpenTherm board, board `pcb1246`, with the Arm GNU Toolchain (`arm-none-eabi-gcc`, on PATH or in its default Windows install folder), CMake 3.20+, Ninja and Python 3:

```sh
cd opentherm
cmake -S . -B build -G Ninja
cmake --build build
```

Output in `opentherm/build/`: `opentherm.elf`, `.hex`, `.bin` and `.map`, with the LPC boot checksum already patched in.
