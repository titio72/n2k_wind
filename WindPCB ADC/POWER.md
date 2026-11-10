# WindPCB ADC — Power Supply Specification

Scope: how the board is powered from NMEA 2000 and/or the XIAO's USB port,
what each rail feeds, and the required behaviour in each power mode.
Source of truth for the as-built wiring is `WindPCB.kicad_sch` / `.kicad_pcb`;
this document describes the intended behaviour of that design.

## 1. Requirements

| ID | Requirement |
|----|-------------|
| R1 | In normal operation the board is powered **only** from NMEA 2000 (12 V, via JP4). |
| R2 | In normal operation the XIAO's onboard 3.3 V regulator must stay idle (it heats up). The XIAO is fed through its 3V3 pin from the TSR converter. |
| R3 | USB must be usable on its own (flashing, serial console, bench work) with NMEA 2000 disconnected. |
| R4 | USB and NMEA 2000 may be connected **at the same time** (debugging on the boat or bench). This must be safe, with no manual switching. |
| R5 | Neither source may back-feed the other: no current into USB VBUS, into the 7805 output, or into the TSR output. |
| R6 | Connecting or disconnecting either source while the other is live must not reset or damage the board. |

## 2. Power architecture

```
 NMEA 2000 12V --JP4--+--> 78M05 (U5) --> U10 --+
 (J6 pin 3)           |                         +--> +5V   --> MCP2562 VDD (U2), C10
                      |      XIAO 5V pin (14) --> U11 --+
                      |
                      +--> TSR 1-2433 (U8) --> U9 --+
                      |                             +--> +3.3V --> XIAO 3V3 pin (12), MCP3202 (U7),
          XIAO onboard LDO (from VBUS) -------------+              74AHC1G14 (U1), MCP2562 VIO, R9
                      |
                      +--> 78M08 (U6) --> F1 (250 mA) --> +8V --> sensor supply (J_VCC1), R5 speed pull-up
```

U9, U10 and U11 are LM66100 ideal diodes. Each is wired VIN = source,
VOUT = rail, CE (pin 3) = VOUT, ST (pin 5) = GND.

- CE tied to VOUT is the connection the datasheet documents for reverse-current
  protection: when VOUT > VIN the device switches off and blocks.
- ST is open-drain and unused, so grounding it is the datasheet's stated option.
- When the device is disabled, the body diode still conducts VIN → VOUT. Start-up
  into a dead rail therefore works.

## 3. Rails

| Rail | Source(s) | Loads |
|------|-----------|-------|
| `+12V` | NMEA 2000 pin via JP4, bulk cap C6 | U5, U6, U8 |
| `+5V` | max(7805 via U10, XIAO 5V pin via U11) | MCP2562 VDD, C10 |
| `+3.3V` | max(TSR via U9, XIAO onboard LDO) | XIAO (3V3 pin), MCP3202 VDD/Vref, 74AHC1G14, MCP2562 VIO, CS pull-up R9 |
| `+8V` | 78M08 via fuse F1 (250 mA) | Wind sensor supply, SPEED pull-up R5 |

`+8V` exists only when NMEA 2000 12 V is present. In USB-only mode the wind
sensor is unpowered.

## 4. Required behaviour per power mode

### 4.1 NMEA 2000 only (normal operation)
- `+5V` is supplied by the 7805 through U10. U11 blocks, so the XIAO 5V pin is not driven by the board.
- `+3.3V` is supplied by the TSR through U9 and drives the XIAO 3V3 pin. The XIAO's onboard LDO has no input and stays idle (R2).
- All functions are available: CAN, ADC, 8 V sensor supply.

### 4.2 USB only (bench / flashing)
- VBUS feeds the XIAO LDO, which supplies `+3.3V`. U9 blocks, so nothing flows into the TSR output.
- VBUS also appears on the XIAO 5V pin and supplies `+5V` through U11. U10 blocks, so nothing flows into the 7805 output.
- Available: XIAO, MCP3202, MCP2562 (no bus attached).
- **Not available: `+8V`.** Wind-sensor inputs are meaningless unless the sensor or a test source is supplied separately.

### 4.3 Both connected (debug)
- `+5V` is the higher of the 7805 output (about 4.8–5.2 V) and the XIAO 5V pin (about 4.7–5.2 V). The lower source's LM66100 blocks. Either may win, and the winner can change with load and temperature. This is harmless.
- `+3.3V` is the higher of the TSR output and the XIAO LDO. U9 blocks the LDO from back-feeding the TSR.
- The XIAO LDO is **awake** because it is fed straight from VBUS. If it sits above the TSR it carries the whole 3.3 V load. The load is small, but it is the one case where the LDO is not idle and may run warm.
- No current flows into VBUS, the 7805 output, or the TSR output (R5).
- Either source can be unplugged while the other keeps the board running (R6). Expect only a brief dip at the handover.

## 5. Debug-session cautions (both connected)

- **Ground loop:** USB ground and the NMEA 2000 ground are the same net. A mains-powered laptop adds a ground path between the boat's DC negative and the laptop/mains earth. Prefer a battery-powered laptop or a USB isolator, especially when watching ADC readings (SIN/COS/SPEED), which are noise-sensitive.
- **Self-heating:** if the XIAO is warm in this mode, the LDO is carrying the 3.3 V load. This does not happen in normal operation.
- **Power-up order:** any order is acceptable by design.

## 6. Open points to verify on the first board

| # | Item | How to verify |
|---|------|---------------|
| V1 | XIAO onboard LDO is not back-driven harmfully in NMEA-only mode (TSR drives its output while its input is dead). | Measure XIAO 5V-pin voltage and board current with NMEA only. |
| V2 | Whether the XIAO 5V pad has its own diode from VBUS. This only shifts VBUS-side voltages. | Check the XIAO schematic, or measure the 5V pin with USB only. |
| V3 | LM66100 reverse leakage in the blocking state (not confirmed from the datasheet text read so far). | Measure the 5V pin and the 7805 output with USB only. Neither should rise. |
| V4 | Which source wins `+5V` and `+3.3V` in the "both" case. | Probe each source's side of U9, U10 and U11 with both connected. |

## 7. Bring-up checklist

1. NMEA only: `+5V` ≈ 5.0 V, `+3.3V` ≈ 3.3 V, `+8V` ≈ 8 V. XIAO 5V pin is near 0 V and the XIAO is cool.
2. USB only: `+3.3V` ≈ 3.3 V, `+5V` ≈ VBUS, `+8V` = 0 V. The 7805 output and TSR output stay at 0 V.
3. Both: all rails stable. Measure which source wins each rail, and check there is no current into VBUS.
4. Hot-plug USB while NMEA is running, then unplug it. Then hot-plug NMEA while USB is running. No resets in either direction.
5. Check the ADC readings (SIN/COS) with USB attached versus detached, to see how much noise the ground path adds.
