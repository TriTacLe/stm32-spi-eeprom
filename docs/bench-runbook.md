# Practical 2 bench runbook

Order of operations for the lab sessions. Everything in the repo that does not
need hardware is already done, so a session that follows this list end to end
produces every missing number and all six figures the report asks for.

The report is finished when no red text remains in it. That is the only
completion test worth using.

## Before power

1. **P2 loopback jumpers out.** PB13 to PB15 reach both the EEPROM and the P2
   header. A looped-back pin configured as an output drives against another
   output.
2. **Scope probes on 10x, and the channel menu set to 10x as well.** If the two
   disagree a 3.3 V signal reads as about 33 V.
3. **Scope ground to board ground only.**
4. Confirm a pin before probing it. **PB12 is on no header** and only reaches
   pin 1 of the EEPROM, so the CS capture in step 5 needs a probe on the chip
   itself.

## 1. Confirm the wiring (issue #2)

Board unpowered, jumpers out, meter on continuity.

Anchor first: EEPROM VCC should beep to 3V3 and VSS to GND. If those two are
not where expected, the pins are being counted from the wrong end and
everything after it is wrong.

Then confirm, and write down what was measured:

| EEPROM pin | Expected | STM32 |
|---|---|---|
| 1 | CS# | PB12 |
| 6 | SCK | PB13 |
| 2 | SO | PB14 |
| 5 | SI | PB15 |

Goes into report section 2, replacing the continuity `\bench{}`.

## 2. Task 1, PC13 (issue #3, code already merged)

`RUN_TASK 1`. Probe PC13 on header P1.

Expect a square wave at roughly 100 Hz, the default `task1_half_period_ms` of
5 ms each way. Change that variable in Live Expressions to prove the loop is
live without rebuilding.

**Capture as `report/figures/task1-pc13.png`.**

Nothing to measure precisely, the frequency is not assessed. If the pin is
dead, the clock enable is the first suspect.

## 3. Task 2, registers (issue #5, code already merged)

`RUN_TASK 2`. No capture needed, this is a debugger step.

In Live Expressions check `dbg_spi_cr1`, `dbg_spi_cr2`, `dbg_spi_sr` and
`dbg_sck_hz_predicted`. `dbg_sck_hz_predicted` must read 250000.

In the SFRs view confirm on SPI2 and GPIOB that MSTR is set, BR is 100, CPOL
and CPHA are both 0, LSBFIRST is 0, SSM and SSI are both set, DS is 0111,
FRXTH is 1, and PB13 to PB15 read AF0 in AFRH with mode 10 in MODER.

If CR1 reads back with MSTR clear, the peripheral has dropped out of master
mode on its own and SSI is the thing to look at.

## 4. Task 3, the waveform (issue #7)

`RUN_TASK 3`. CH1 on SCK, CH2 on MOSI. The byte goes out every 100 ms with CS
held high, so trigger on the SCK burst.

**Capture as `report/figures/task3-sck-mosi.png`.** The capture has to be good
enough to read all of: SCK period, clock idle state, eight clock cycles, the
bit sequence, the bit order, and the value.

Then, with cursors placed by hand:

- SCK period, and the frequency from it
- percentage error against 250 kHz
- decode the eight bits off the screen and check they give 0x3D

**Decode from the waveform, not from the source.** The waveform is the
evidence, and the demonstrator may hand over a different byte and ask for the
MOSI pattern to be predicted before the program runs. `task3_test_byte` is
writable in Live Expressions for exactly that.

**If there are 16 clock pulses per call instead of 8**, the write to DR went
out wider than 8 bits. The code already writes through an 8-bit pointer, so
this should not appear, but it is the first thing to count.

## 5. Task 4, the EEPROM (issues #8, #9, #10)

`RUN_TASK 4`. Press PA0 for one write, read and verify.

Record from Live Expressions:

- `eeprom_status_before` and `eeprom_status_after`
- `eeprom_write_wait_ms`, the measured write duration
- `eeprom_read_value` and `eeprom_verify_ok`

Expect green, the byte 0x3D on PB0 to PB7, and a write of a few milliseconds
against the datasheet maximum of 5 ms.

**If verification fails, read `eeprom_status_after` before touching anything.**
WEL still set there means the write was discarded, and the address length is
the suspect rather than the wiring. That is the README's stated tell for the
CAT25040 versus the fitted part, and confirming it is itself a report item.

For the capture, set `eeprom_read_loop_enable` to 1 in Live Expressions. That
repeats a **read-only** transaction every 50 ms, giving a steady trace that
never writes. CH1 on CS, CH2 on SCK.

**Capture as `report/figures/task4-cs-sck.png`.** It must show the clock pulse
count while the device is selected. Count it and check it against opcode plus
two address bytes plus one data byte.

**Persistence test:** write with PA0, then reset or power-cycle, then let the
board come up. Startup runs the read-only path, so the stored byte should
appear on the LEDs with green, without anything being written. Record what was
written, what the reset was, and what came back.

Never leave a write running in a loop. Write endurance is finite, and only PA0
writes.

## 6. Task 5, the fault (issue #11)

`RUN_TASK 5` is the Task 4 program with one configuration value changed.
`RUN_TASK 4` is the same program without it.

**Check `run_task` in Live Expressions before trusting any measurement here.**
It says which mode is actually on the board.

Work in this order and write each step down as it happens, because the marks
are the chain and not the answer:

1. What waveform or behaviour was expected.
2. What the board actually produced.
3. The first meaningful difference between the two.
4. The register and the specific field responsible.
5. The correction.
6. The repeated measurement, and why it now satisfies the device.

**Capture the same measurement in both modes** so the two images compare
directly: `report/figures/task5-faulty.png` and
`report/figures/task5-corrected.png`.

Do not hunt by changing SPI fields until the EEPROM responds. The diagnosis is
what is being marked.

## 7. Task 6, the state machine (issue #13, code already merged)

`RUN_TASK 6`. PA0 starts, PA3 aborts.

- Watch `ee_state` change in the debugger through a full run.
- Confirm the byte appears on PB0 to PB7 and green lights on success.
- Abort mid-transaction with PA3 and confirm the machine returns to idle and
  the next PA0 still works.
- Ask for a different address or byte: `eeprom_test_addr` and
  `eeprom_test_byte` are writable, and the demonstrator may well do this.

**To show non-blocking:** probe PC13 and set `ee_use_fsm` to 0, which runs the
Task 4 blocking path on PA0 instead. The heartbeat stops during the write cycle
in the blocking path and keeps running under the state machine. Same build,
one variable.

Export the state diagram from `docs/task6-state-machine.md` as
`report/figures/task6-fsm.png`.

## Closing out

Rebuild the report and check that no red text and no red boxes remain. Then
push the code, upload the PDF to Gradescope **before** the demo, and add the
other group member to that same submission.

Do not resubmit after the demo. It resets the demo mark to zero.
