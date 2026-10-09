# third_party

Unmodified upstream code. Update by replacing files, never by editing them.

- `CMSIS/`: CMSIS-Core headers from [ARM-software/CMSIS_5](https://github.com/ARM-software/CMSIS_5) tag `5.6.0` (Apache-2.0, see `CMSIS/LICENSE.txt`).
- `LPC11U6x/`: `LPC11U6x.h` and `system_LPC11U6x.h` from the Keil `LPC1100_DFP` pack 1.4.1. The matching `system_LPC11U6x.c` holds the clock setup and so lives with the board, in `main/hardware/boards/pcb1246/`.
