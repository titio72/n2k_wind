# Wind Correction Table — Design

**Date:** 2026-11-12
**Status:** draft for review
**Repos touched:** `n2k_wind` (this repo, the masthead sensor firmware) and `rust_nmea_router` (`~/dev/rust_nmea_router`).

## Why

The masthead wind sensor's apparent wind angle is biased by the boat itself: upwash from main and jib, flow acceleration around the rig, heel, and possibly vane mechanics. The bias is repeatable and shows up as true wind direction (TWD) changing across a tack.

The router project measured it: `rust_nmea_router/docs/wind-calibration-investigation.md`. On 87 tacks and gybes (2025-06 to 2026-11, original hardware), starboard TWD reads on average +14.7° above port TWD (+18° upwind and reaching). The apparent angle reads too far from the bow by roughly 7°, more in light air. The error depends mainly on wind speed. About 8° of per-event scatter stays unexplained by any fixed correction.

We correct it the way NKE does on its Processor HR (see "Prior art"): an empirical **true-wind correction table**, indexed by true wind speed with a few angle anchors, applied to true wind, with apparent wind recalculated from the corrected true wind. The sensor outputs the corrected apparent wind on NMEA 2000 (N2K), so every device on the bus (plotter, autopilot, router) sees corrected values.

Most of the table will come from telemetry analysis in the router, and individual values will be tweaked while sailing. The analysis needs the **raw** (uncorrected) wind. So the sensor publishes its table on N2K, and the router uses it to recover the raw values, which it stores with the telemetry.

## Goals

1. The sensor applies a true-wind correction table to its output and sends the corrected AWA/AWS on PGN 130306.
2. The table can be changed while sailing (BLE commands) and switched on and off. It survives reboots.
3. The sensor broadcasts its table on N2K so the router always knows which table was active.
4. The router recovers raw AWA/AWS from the corrected values plus the active table and stores raw values, and the table in effect, alongside the telemetry.
5. Upgrading the firmware does not lose the existing configuration (vane calibration, offset, N2K source, BLE name).

## Non-goals

- Heel or mast-motion compensation (NKE's "3D" step). There is no motion sensor at the masthead. A heel-based geometric correction would widen AWA, which makes this boat's bias worse. Revisit later if needed.
- Leeway. Treated as zero.
- The router transmitting on the bus (no ISO request, no address claim). The router stays receive-only.
- Android app UI for the table. The firmware exposes BLE commands. A UI is a separate follow-up in the marine-kit apps.
- Learning or fitting the table automatically, in the sensor or the router. Fitting stays an offline analysis.
- Reworking the fitter (`scripts/wind_split.py --fit-correction`) to produce table rows from the new raw telemetry. That is the next project, once raw data with STW has accumulated.
- Fixing the router's `calculate_true_wind()` leeway defect, or the MCP3202 ADC board work. Both are tracked separately.

## Anti-goals (failure even if it "works")

- Losing the vane calibration on firmware upgrade.
- A jump in the output angle when the wind crosses the bow or the stern.
- Corrected data in the router's telemetry that cannot be turned back into raw data, or raw values derived from the wrong table.
- Flooding the bus. The table broadcast must stay a few frames per minute.

## Prior art: NKE Processor HR

From the Processor HR user manual (§7.3, §9.7–9.9) and the Multidisplay manual (§2.8.2–2.8.4):

- Order: raw wind → (3D motion compensation) → true wind from boat speed and leeway → **TWA table (`Adjwa.d`) and TWS table (`Adjvt.d`) applied to true wind** → constant wind-shear offset → apparent wind recalculated from corrected true wind.
- Table layout: rows by TWS (0, 5, 10, 15, 20, 25, 30, 35, 50 kn). Each row has three (correction, angle) pairs: upwind around 36–44°, reaching around 93–100°, downwind around 141–158°. One table for both tacks.
- Calibration order: compass, heel, boat speed, AWA offset (with tables disabled, using raw data), then the tables, tack to tack: "TWD port > TWD starboard: add half of the difference."
- Tables can be disabled (`TrueWindTable = Y/N`).

## Design: sensor firmware (`n2k_wind`)

### Correction chain

Runs each time the sensor sends PGN 130306 (every 500 ms, `WIND_N2K_DATA_FREQ`). Inputs: the existing output angle `a` (smoothed angle + `conf.offset`, signed to (−180°, 180°], positive = starboard) and the smoothed AWS `s` in knots.

1. Get the boat speed `b` (see "Boat speed"). If none is available, or the table is disabled, send `a`, `s` unchanged with the "corrected" flag clear.
2. True wind, with leeway treated as zero:
   `tw_x = s·cos(a) − b`, `tw_y = s·sin(a)`; `TWS = hypot`, `TWA = atan2(tw_y, tw_x)` (signed).
3. Look up the corrections at (`TWS`, `|TWA|`): `dA` (degrees) and `dS` (knots).
   `TWA' = sign(TWA)·(|TWA| + dA)`, `TWS' = max(0, TWS + dS)`.
   **Sign convention:** positive `dA` moves the wind away from the bow (widens |TWA|). Given the investigation, this boat's values will mostly be negative.
4. Apparent wind back from corrected true wind:
   `aw_x = TWS'·cos(TWA') + b`, `aw_y = TWS'·sin(TWA')`; `AWA' = atan2`, `AWS' = hypot`.
5. Send `AWA'`, `AWS'` with the "corrected" flag set.

The mounting offset (`conf.offset`) stays separate and is applied before the table, as in NKE's chain. It is calibrated with the table off.

### Table

- Up to **10 rows**. Each row has a TWS breakpoint (kn) and **3 anchors**. Each anchor holds an angle (deg, 0–180), an angle correction `dA` (deg) and a speed correction `dS` (kn).
- *Builder's call / my call:* angle and speed corrections share the same anchor angles. NKE uses two separate tables with their own anchors; sharing keeps the table and the protocol small and matches NKE's example tables, where the anchors are identical.
- Rows sorted by TWS, anchors sorted by angle. Validation rejects anything else.
- Resolution: TWS 0.1 kn, angle 1°, `dA` 0.1°, `dS` 0.1 kn. Limits: |dA| ≤ 30°, |dS| ≤ 10 kn.
- Slope limit (added after review): every correction slope is at most 0.5, i.e. |ΔdA| ≤ 0.5·Δangle between anchors and on the tapers to 0° and 180°, |dS| ≤ 0.5·TWS within a row, and |ΔdS| ≤ 0.5·ΔTWS between rows (same anchor index). Steeper tables fold the output across itself and break the router's fixed-point inversion.
- No correction when AWS < 0.1 kn (cups stopped): there is no direction to correct, and a zero vector can't be inverted.
- Default: empty (no rows) and **disabled**.

**Interpolation (my call):**
- Within a row: linear between anchors. Below the first anchor, taper linearly from that anchor's value to **0 at 0°**. Above the last anchor, taper to **0 at 180°**. Corrections must be zero on the bow and stern lines, otherwise mirroring would make the output jump when the wind crosses the bow or the stern.
- Between rows: linear in TWS. Below the first row and above the last, use the end row.

### Boat speed

- Listen for PGN 128259 (speed through water, STW) only. SOG is not used: the boat already has a system that puts a simulated STW (derived from SOG) on the bus when the paddlewheel is offline, so 128259 is always the right input.
- Use STW if a message arrived in the last **3 s**. Otherwise no correction.
- On this boat one device (a custom unit that serves the paddlewheel and integrates the GPS) sends 128259 from a single bus address, both for measured and for simulated STW. If several sources ever appear, use the first one seen and stick with it until it goes stale.

### Persistence

- The table and its enabled flag are stored in their **own EEPROM block** after the `configuration` struct, with their own magic/version byte.
- `CONF_SERIAL` stays unchanged, so existing configurations survive the upgrade. A missing or invalid table block means an empty, disabled table.
- Factory reset (`X`) also clears the table.

### Table identity

`table_id` = CRC-32 of the serialized table contents plus the enabled flag, using the same bytes as the N2K payload (below). It is content-based, so two sensors or a reset-and-reload give the same id for the same table, with no counter to keep in sync.

### BLE commands

New entries in `CommandHandler`. The letters are free in the current table; the builder can pick others if they clash.

| Cmd | Payload | Effect |
|---|---|---|
| `C` | `0` or `1` | Disable or enable the table |
| `U` | `tws,a1,dA1,dS1,a2,dA2,dS2,a3,dA3,dS3` | Upsert the row for `tws`: replace the row with the same TWS, otherwise insert it in TWS order. Rejected if the table already has 10 rows. Values in display units with decimals (e.g. `10.0,40,-3.5,0.0,95,-1.0,0.0,150,2.0,0.0`) |
| `Z` | `tws` | Delete the row with that TWS |
| `Y` | — | Clear the table |

Each accepted change is validated, persisted, gets a new `table_id`, and triggers an immediate table broadcast. A rejected change leaves the table as it was and returns `INVALID_FORMAT`.

BLE data: append `table_id`, the enabled flag, the corrected flag, and the corrected AWA/AWS to the end of the BLE data message (`BLESerializer.cpp`), after the variable-length Wind360 part. If the app can't take fields after a variable-length part, the builder may instead add a separate characteristic. The current layout stays byte-compatible for existing app versions.

### PGN 130306: "corrected" flag

Byte 5 (wind reference) stays exactly `0x02` (apparent), so standard equipment sees a normal 130306. (Revised: the flag first lived in byte 5's reserved bits, but equipment that compares the whole byte to 2 would have rejected the message.)

The last two bytes are reserved in the standard. This sensor has always used them, non-standard, for the vane ellipse (a 2-byte value, 0.01 resolution), and the equipment on board accepts that. They are now split:

- **Byte 6:** ellipse × 100 as one byte: 0–254 (0.00–2.54), 255 = not available. An ellipse above 1.5 already means the calibration is off, so the cap loses nothing.
- **Byte 7:** correction status. Bit 3: 1 when the sample was corrected. Bits 4–7: when bit 3 is set, the low 4 bits of the `table_id` that corrected the sample; 0 otherwise. Bits 0–2: 0.

Backward compatibility: older firmware's 2-byte ellipse has, for any ellipse below 2.55, the same byte 6 and a byte 7 of 0, which reads as "uncorrected". So a router reading this layout can't misread older firmware.

The router uses bit 3 to know which samples to invert, and the 4-bit tag to check every sample against the table it thinks is active. That way a lost or late 130900 broadcast can't make it invert with the wrong table unnoticed. (My call: the review raised it; it costs nothing and protects the "wrong table" anti-goal.)

On a table change, the sensor sends the new 130900 broadcast **before** it starts correcting with the new table.

### Table broadcast (proprietary PGN)

- **PGN 130900**: proprietary, fast-packet, global (130816–131071 range). Manufacturer code 2046, the existing `N2K_MANIFACTURER_CODE`, with industry code 4 (marine).
- Sent at startup (once the address is claimed), immediately after every table change, and every **60 s**. Priority 7.
- Payload, little-endian, at most 223 bytes (about 26 frames for a full table):

| Offset | Size | Field |
|---|---|---|
| 0 | 2 | Manufacturer code (11 bits), reserved (2 bits, 1s), industry code (3 bits) = 4 |
| 2 | 1 | Message type = 1 (correction table) |
| 3 | 1 | Payload format version = 1 |
| 4 | 4 | `table_id` (CRC-32) |
| 8 | 1 | Enabled (0/1) |
| 9 | 1 | Row count N (0–10) |
| 10 | 17·N | Rows: TWS (uint16, 0.1 kn), then 3 × [angle (uint8, deg), dA (int16, 0.1°), dS (int16, 0.1 kn)] |

The CRC-32 (IEEE 802.3 polynomial) is computed over bytes 8 to the end, so it covers enabled, row count and rows.

## Design: router (`rust_nmea_router`)

This is the contract the router must meet. Its own detailed plan comes later.

1. **Receive the table.** Register PGN 130900 as fast-packet in `nmea2k/src/stream_reader.rs`, decode the payload above, check manufacturer code 2046, and check the CRC. A bad CRC is dropped and logged.
2. **Keep table history.** Persist every distinct table (`table_id`, content, first seen) in a new DB table. Track the current `table_id` and the time it was last confirmed by a broadcast. If no broadcast has been seen for 3 minutes, the active table is "unknown".
3. **Recover raw values.** For each 130306 sample with the corrected bit set, and a known active table:
   - Corrected AWA/AWS + boat speed → corrected TWA/TWS.
   - Solve for raw TWA/TWS such that `raw + table(raw) = corrected`. A fixed-point iteration starting from `raw = corrected` converges because the corrections vary slowly. Stop at 0.01°, or after 10 iterations and flag the sample.
   - Raw TWA/TWS → raw AWA/AWS with the same boat speed.
   - Use the same boat-speed rule as the sensor (STW from 128259, 3 s freshness). The router's value can differ slightly from the sensor's in timing and smoothing; with corrections of a few degrees, the effect on the recovered raw angle is well below the noise.
   - Samples with the flag clear are already raw.
   - Corrected samples with an unknown table, or whose 4-bit tag doesn't match the active `table_id`: store the corrected value and mark the raw value as missing. Never guess.
   - The router also routes PGN 130900 by the source address that sends 130306, so a second device can't be mistaken for the wind sensor.
4. **Store.** Telemetry keeps the corrected values as today, plus **raw AWA/AWS** (apparent, so they don't depend on boat speed), the **STW used** to derive them, and the `table_id` in effect. This goes wherever wind is persisted today (`vessel_status`, `environmental_data`); the exact schema is the router plan's call. Existing rows are raw by definition, since they come from before any table existed.
   - **Why the STW is stored:** the sensor computes true wind from STW, but the router's own true wind today uses SOG (`calculate_true_wind`). A table fitted from SOG-based true wind would be indexed differently from the one the sensor uses. Storing raw apparent wind and the STW lets the fit rebuild true wind exactly as the sensor does. (My call.)
5. **Analysis.** `scripts/wind_split.py` and the investigation fits must run on raw values, so they need an API or field that exposes them.

## Initial table

The first table comes from what the investigation already has: `ε = 5.0° + 1.9°·(10/AWS)²`, a narrowing of the apparent angle, fitted on SOG-based data from the original hardware.

The table works on true wind, so ε has to be converted:

- Pick TWS rows, for example 4, 6, 8, 10, 14, 20, 30 kn, dense at low speed where ε is steep.
- For each row, pick three anchors at typical upwind, reaching and downwind true wind angles (about 45°, 95°, 150°).
- Take typical boat speeds for each anchor, from the boat's polars (`dufour40.csv` in the router repo). Compute the apparent wind, apply −ε to AWA, and recompute true wind. The change in TWA is `dA`.
- Set `dS = 0` everywhere.

The deliverable is a short one-off script (in the router's `scripts/`) that prints the rows as BLE `U` commands. The values are a starting point to be refined; they are not a calibration.

## Testing

- **Firmware, native unit tests (`env:test`):**
  - interpolation, the bow/stern taper, mirroring between tacks
  - row validation, the BLE command parsing
  - the full correction chain on known vectors
  - round-trip: correct, then invert, gives back the original within 0.05°
  - the PGN 130900 encoder against golden bytes
  - persistence: an old EEPROM image with no table block loads the configuration intact, with an empty, disabled table
- **Shared test vectors:** a small set (table bytes plus sample in/out values) committed to both repos, so the sensor encoder and correction, and the router decoder and inversion, are checked against the same numbers.
- **On the bus:** candump shows 130900 at startup, on change and every 60 s, and byte 5 of 130306 staying `02`, and the byte 7 corrected bit following the enable switch and boat-speed availability.

## Left to the builder

- Code structure: classes, where the table lives in memory, N2K receive handler wiring through `N2K::get_instance` and `add_rx_pgn`.
- Exact EEPROM layout of the table block.
- Smoothing of boat speed, if any, as long as it is light (≤ 2 s).
- Log/trace messages.
