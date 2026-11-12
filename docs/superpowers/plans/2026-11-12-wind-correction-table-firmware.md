# Wind Correction Table (Sensor Firmware) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** The masthead sensor applies a true-wind correction table to its output, sends the corrected apparent wind on PGN 130306 with a "corrected" flag and table tag, and broadcasts the table on proprietary PGN 130900. The table is editable over BLE and persisted in EEPROM.

**Architecture:**
- **Pure, natively tested modules:** the table math (`WindCorrection`), the wire format (`CorrectionProtocol`), STW freshness (`BoatSpeed`), the per-sample output decision (`WindOutput`) and persistence (`CorrectionPersistence`).
- **Thin glue:** BLE commands extend `CommandHandler`. `N2KWind` and `main.cpp` wire everything onto the bus.
- **Table copies:** the main loop owns an *active* copy of the table. BLE commands edit a separate *edit* copy and raise a flag. On the next iteration the main loop takes the new copy, broadcasts it, and only then starts correcting with it.

**Tech Stack:** PlatformIO, Arduino-ESP32 (ESP32-C3), ttlappalainen NMEA2000 library via `n2k_tools_libs` `N2K` wrapper, NimBLE, Unity native tests (`pio test -e test`).

**Spec:** `docs/superpowers/specs/2026-11-12-wind-correction-table-design.md`

## Global Constraints

- `CONF_SERIAL` stays `12`. Existing configurations must load unchanged after the upgrade.
- Table: at most **10 rows**, **3 anchors** per row. Units: TWS 0.1 kn (`uint16`), anchor angle 1° (`uint8`), `dA` 0.1° (`int16`), `dS` 0.1 kn (`int16`).
- Validation:
  - each row: `0 < a1 < a2 < a3 < 180`, `|dA| ≤ 300` (30.0°), `|dS| ≤ 100` (10.0 kn), `tws ≤ 1000` (100.0 kn);
  - rows strictly increasing by TWS;
  - `enabled ∈ {0,1}`.
- Default table: empty, disabled.
- Sign: positive `dA` widens |TWA| (moves the wind away from the bow).
- Boat speed: STW from PGN 128259 only, fresh if received within **3000 ms**. Otherwise the output is uncorrected.
- PGN 130306 byte 5: bits 0–2 = reference (2, apparent); bit 3 = corrected; bits 4–7 = `table_id & 0xF` when corrected, else 0. Uncorrected output byte is exactly `0x02`, as today.
- PGN 130900:
  - proprietary fast-packet, priority 7, manufacturer code `N2K_MANIFACTURER_CODE` (2046), industry code 4;
  - payload ≤ 223 bytes, layout per spec;
  - CRC-32 (IEEE, reflected, poly 0xEDB88320) over bytes 8..end.
  - Sent 5 s after boot, immediately after each table change, and every 60 s.
- BLE commands: `C` (0/1), `U` (10 comma-separated fields), `Z` (TWS), `Y` (clear). `X` (factory reset) also clears the table.

## Review Focus

1. **STW stops arriving mid-session.** Output must revert to uncorrected with byte 5 = `0x02` within 3 s, and resume correcting when STW returns. Tested in Task 4 and Task 7.
2. **Wind crossing the bow or stern** (e.g. AWA 359.9° → 0.1°, 179.9° → 180.1°). The corrected output must not jump. Tested in Task 1.
3. **No wind signal (NaN AWA/AWS).** No correction, values pass through unchanged so the sender skips them as today. Tested in Task 1 and Task 7.
4. **Malformed or over-limit BLE row commands** (too few or too many fields, non-numbers, out-of-range anchor, 11th row). Rejected with the table unchanged and nothing persisted. Tested in Task 1 and Task 5.
5. **Boat stopped or drifting** (STW = 0): correction still applies with true wind = apparent wind. Tested in Task 1.

## File Structure

| File | Responsibility |
|---|---|
| `src/WindCorrection.h/.cpp` (new) | Table types, validation, edit operations, row parsing, lookup/interpolation, the correction chain |
| `src/CorrectionProtocol.h/.cpp` (new) | CRC-32, PGN 130900 payload encoding, `table_id`, 130306 reference byte |
| `src/CorrectionPersistence.h/.cpp` (new) | EEPROM block for the table, after `configuration` |
| `src/BoatSpeed.h/.cpp` (new) | STW freshness (3 s) and source stickiness |
| `src/WindOutput.h/.cpp` (new) | Per-sample decision: corrected or pass-through, plus the reference byte |
| `src/DataAndConf.h/.cpp` | Shared `EEPROM_TOTAL_SIZE`; `correction_status` in `all_data` |
| `src/CommandHandler.h/.cpp` | `C`/`U`/`Z`/`Y` commands; `X` clears the table |
| `src/BLESerializer.cpp` | Appended correction fields |
| `src/N2kWind.hpp` | Receive 128259; reference byte on 130306; send 130900 |
| `src/main.cpp` | Wiring: table copies, flag, broadcast timing, correction per loop |
| `src/WindDirectionMCP320x.h/.cpp` | Excluded from native builds (Task 0) |
| `test/test_windcorrection/`, `test/test_correctionprotocol/`, `test/test_correctionpersistence/`, `test/test_boatspeed/`, `test/test_windoutput/` (new) | Native tests |
| `test/vectors/wind_correction_v1.json` (new) | Shared vectors; the router plan consumes this file |

### Test table T1 (used throughout)

T1 is enabled and has 2 rows:

| TWS | Anchor 1 (angle, dA, dS) | Anchor 2 | Anchor 3 |
|---|---|---|---|
| 6.0 kn | 45°, −6.0, 0.0 | 95°, −4.0, 0.0 | 150°, −2.0, 0.0 |
| 10.0 kn | 40°, −3.5, 0.0 | 95°, −1.0, 0.0 | 150°, +2.0, +0.5 |

As BLE `U` commands: `U6.0,45,-6.0,0.0,95,-4.0,0.0,150,-2.0,0.0` and `U10.0,40,-3.5,0.0,95,-1.0,0.0,150,2.0,0.5`. Each test file that needs T1 builds it with a local `make_t1()` helper, by direct struct initialisation using those integer units.

---

### Task 0: Restore the native test baseline

Two problems predate this work:

- PlatformIO's cached `n2k_tools_libs` was stale, so `SpeedSensorInterrupt::loop_micros` was missing. This is already fixed by a reinstall during planning.
- `WindDirectionMCP320x` pulls an Arduino-only library into the native build.

After those, `test_configuration.cpp:test_save_and_read` still fails because the `MockEEPROM` in `n2k_tools_libs`:

- doesn't count `put()` as a write;
- deletes its buffer on `end()`, so data doesn't survive a `ConfPersistence::write()` / `read()` cycle.

Task 3 needs a mock that keeps data, so this task fixes the mock in the library repo.

**Files:**
- Modify: `src/WindDirectionMCP320x.h`, `src/WindDirectionMCP320x.cpp` (wrap in `#ifndef NATIVE` … `#endif`; the `.cpp` has no trailing newline, so add one before `#endif`)
- Modify: `../n2k_tools_libs/src/MockEEPROM.h`
- Modify: `platformio.ini` (`[env:test]` only)

- [ ] **Step 1: Guard the MCP320x files** with `#ifndef NATIVE` around their whole contents.
- [ ] **Step 2: Run the tests.** Run `pio test -e test`. Expected: builds; `168 test cases: 1 failed`; the failure is `test_save_and_read: Expected 1 Was 0`.
- [ ] **Step 3: Fix `MockEEPROM` in `../n2k_tools_libs/src/MockEEPROM.h`.**
  - `put()` increments `write_count`.
  - `end()` sets `initialized = false` but keeps `data_buffer`.
  - `begin()` on an existing buffer doesn't reset it. Only the first allocation fills it with 0xFF.
  - `reset()` stays the explicit way for tests to wipe it.
- [ ] **Step 4: Point the test env at the local library.** In `[env:test]`, add `lib_deps = ${env.lib_deps}` with the `n2k_tools_libs` git URL replaced by `symlink://../n2k_tools_libs`. The test build then uses the fixed mock without a push. The ESP32 envs keep the git URL.
- [ ] **Step 5: Run the tests again.** Run `pio test -e test`. Expected: `168 test cases: 168 succeeded`.
- [ ] **Step 6: Commit both repos.**
  - In `../n2k_tools_libs`: `git commit -am "MockEEPROM: keep data across end(), count put() as a write"`.
  - In `n2k_wind`: `git add src/WindDirectionMCP320x.* platformio.ini && git commit -m "Restore native test build"`.
  - **Do not push `n2k_tools_libs` without asking the user.**

---

### Task 1: Correction table core (`WindCorrection`)

**Files:**
- Create: `src/WindCorrection.h`, `src/WindCorrection.cpp`
- Test: `test/test_windcorrection/test_windcorrection.cpp`

**Interfaces:**
- Produces:
  ```cpp
  #define CORRECTION_MAX_ROWS 10
  #define CORRECTION_ANCHORS 3
  struct correction_anchor { uint8_t angle; int16_t d_angle; int16_t d_speed; };  // deg, 0.1 deg, 0.1 kn
  struct correction_row { uint16_t tws; correction_anchor anchors[CORRECTION_ANCHORS]; }; // tws 0.1 kn
  struct correction_table { uint8_t enabled; uint8_t n_rows; correction_row rows[CORRECTION_MAX_ROWS]; };

  void correction_clear(correction_table &t);                       // enabled=0, n_rows=0, rows zeroed
  bool correction_row_valid(const correction_row &r);
  bool correction_table_valid(const correction_table &t);
  bool correction_upsert(correction_table &t, const correction_row &r); // false: invalid row or table full (unchanged)
  bool correction_delete(correction_table &t, uint16_t tws);          // false: not found
  bool parse_correction_row(const char *s, correction_row &r);        // "tws,a1,dA1,dS1,a2,dA2,dS2,a3,dA3,dS3"
  void correction_lookup(const correction_table &t, double tws_kn, double abs_twa_deg,
                         double &d_angle_deg, double &d_speed_kn);
  bool correct_wind(const correction_table &t, double awa_deg, double aws_kn, double stw_kn,
                    double &out_awa_deg, double &out_aws_kn);          // out_awa in [0,360)
  ```

- [ ] **Step 1: Write the failing tests**, using T1 and tolerance `1e-3` for doubles.

  `correction_lookup` with T1:
  - `(10, 40)` → −3.5, 0.0
  - `(10, 20)` → −1.75 (taper toward the bow)
  - `(10, 0)` → 0, 0
  - `(10, 180)` → 0, 0
  - `(10, 165)` → 1.0, 0.25 (taper toward the stern)
  - `(10, 67.5)` → −2.25
  - `(8, 95)` → −2.5 (between rows)
  - `(3, 95)` → −4.0 (clamped below)
  - `(20, 95)` → −1.0 (clamped above)

  `correct_wind(T1, awa, aws, stw)` → returns true with:

  | awa | aws | stw | out_awa | out_aws |
  |---|---|---|---|---|
  | 40 | 15 | 6 | 38.2193 | 15.1701 |
  | 320 | 15 | 6 | 321.7807 | 15.1701 (mirror) |
  | 95 | 12 | 6.5 | 96.0454 | 12.1659 |
  | 150 | 20 | 7.5 | 152.0800 | 20.2697 |
  | 40 | 4 | 2 | 36.8700 | 4.1106 |
  | 40 | 10 | 0 | 36.5 | 10.0 (STW 0: true wind = apparent) |

  Other `correct_wind` cases:
  - Continuity: outputs for awa 0.1 vs 359.9, and 179.9 vs 180.1 (aws 15, stw 6) differ by < 0.5° circularly.
  - Disabled T1, enabled-but-empty, or a NaN awa/aws/stw → returns false and outputs equal the inputs.

  Edit operations:
  - `correction_upsert`: inserting 10.0 then 6.0 gives rows ordered [60, 100]. Upserting 10.0 again replaces it, so `n_rows` stays 2.
  - An 11th distinct row → false, table unchanged.
  - Rows with `a1=0`, with `a2<=a1`, or with `d_angle=301` → false, table unchanged.
  - `correction_delete`: an existing TWS → true and the row is removed; a missing one → false.

  `parse_correction_row`:
  - `"10.0,40,-3.5,0.0,95,-1.0,0.0,150,2.0,0.5"` → tws 100, anchors {40,−35,0}, {95,−10,0}, {150,20,5}.
  - Rejects `"10.0,40,-3.5"` (too few fields), 11 fields, `"10.0,40,x,0,95,-1,0,150,2,0.5"` (not a number) and `""`.

- [ ] **Step 2: Run the tests to see them fail.** Run `pio test -e test -f test_windcorrection`. Expected: build failure, because `WindCorrection.h` doesn't exist yet.
- [ ] **Step 3: Implement `WindCorrection.cpp`.**
  - Interpolation exactly as the spec says:
    - within a row: linear between anchors;
    - x ≤ a1: `v1·x/a1`; x ≥ a3: `v3·(180−x)/(180−a3)`;
    - between rows: linear in TWS, clamping to the end rows.
  - `correct_wind` follows spec steps 2–4: `TWS' = max(0, TWS+dS)`, `TWA' = sign(TWA)·(|TWA|+dA)` with `sign(0)=+1`.
  - Parse with `strtod`, checking the end pointer. Round to integer units with `lround(x*10)` for tws/dA/dS and `lround(x)` for angles, then validate with `correction_row_valid`.
- [ ] **Step 4: Run the tests to see them pass.** Run `pio test -e test -f test_windcorrection`. Expected: all pass.
- [ ] **Step 5: Commit.** `git add src/WindCorrection.* test/test_windcorrection && git commit -m "Add wind correction table core"`

---

### Task 2: Wire format (`CorrectionProtocol`) and shared vectors

**Files:**
- Create: `src/CorrectionProtocol.h`, `src/CorrectionProtocol.cpp`, `test/vectors/wind_correction_v1.json`
- Test: `test/test_correctionprotocol/test_correctionprotocol.cpp`

**Interfaces:**
- Consumes: `correction_table` (Task 1).
- Produces:
  ```cpp
  #define CORRECTION_PGN 130900UL
  #define CORRECTION_MSG_TYPE_TABLE 1
  #define CORRECTION_PAYLOAD_VERSION 1
  #define CORRECTION_MAX_PAYLOAD 223
  uint32_t crc32_ieee(const uint8_t *data, size_t len);
  size_t encode_correction_table(const correction_table &t, uint16_t manufacturer_code,
                                 uint8_t *buf, size_t buf_len);   // bytes written; 0 if buf_len too small
  uint32_t correction_table_id(const correction_table &t);       // CRC-32 of payload bytes 8..end
  uint8_t wind_reference_byte(bool corrected, uint32_t table_id);
  ```

- [ ] **Step 1: Write the failing tests.**
  - `crc32_ieee("123456789", 9) == 0xCBF43926`.
  - `encode_correction_table(T1, 2046, buf, 223)` returns 44 and the bytes equal:
    `FE 9F 01 01 DE A1 30 53 01 02 3C 00 2D C4 FF 00 00 5F D8 FF 00 00 96 EC FF 00 00 64 00 28 DD FF 00 00 5F F6 FF 00 00 96 14 00 05 00`.
  - Table ids:
    - `correction_table_id(T1) == 0x5330A1DE`;
    - T1 with `enabled=0` → `0xC640754B`;
    - empty disabled table → `0x41D912FF`.
  - A full 10-row table encodes to 180 bytes. `buf_len` 43 for T1 → returns 0.
  - Reference byte:
    - `wind_reference_byte(true, 0x5330A1DE) == 0xEA`;
    - `wind_reference_byte(false, 0x5330A1DE) == 0x02`.
- [ ] **Step 2: Run the tests to see them fail.** Run `pio test -e test -f test_correctionprotocol`. Expected: build failure.
- [ ] **Step 3: Implement `CorrectionProtocol.cpp`.**
  - Header word: `mfr | (3 << 11) | (4 << 13)`, little-endian.
  - `correction_table_id` encodes into a local 223-byte buffer and runs CRC over `buf + 8`.
- [ ] **Step 4: Run the tests to see them pass.** Run `pio test -e test -f test_correctionprotocol`. Expected: all pass.
- [ ] **Step 5: Write `test/vectors/wind_correction_v1.json`** for the router plan:
  - `version: 1`;
  - `manufacturer_code: 2046`;
  - `table`: T1 as `{enabled, rows:[{tws, anchors:[[angle,dA,dS],…]}]}`, in display units;
  - `payload_hex`: the 44 bytes above;
  - `table_id: "0x5330A1DE"`;
  - `samples`: the six `correct_wind` rows from Task 1, plus `{awa:170, aws:8, stw:5, out_awa:170.7089, out_aws:8.0964}` and `{awa:40, aws:30, stw:8, out_awa:37.6622, out_aws:30.2580}`. Each sample has `reference: 234` (0xEA).
- [ ] **Step 6: Commit.** `git add src/CorrectionProtocol.* test/test_correctionprotocol test/vectors && git commit -m "Add PGN 130900 encoding and shared test vectors"`

---

### Task 3: Persistence (`CorrectionPersistence`)

On ESP32, `EEPROM` is one NVS blob whose size is set by `begin(size)`, and `commit()` writes exactly that many bytes. If `ConfPersistence` kept calling `begin(sizeof(configuration))`, every configuration write would cut off the table block. So both classes must use the same total size.

**Files:**
- Create: `src/CorrectionPersistence.h`, `src/CorrectionPersistence.cpp`
- Modify: `src/DataAndConf.h` (add `EEPROM_TOTAL_SIZE`), `src/DataAndConf.cpp` (`ConfPersistence::write/read` use `EEE.begin(EEPROM_TOTAL_SIZE)`)
- Test: `test/test_correctionpersistence/test_correctionpersistence.cpp`

**Interfaces:**
- Consumes: `correction_table`, `correction_table_valid`, `correction_clear` (Task 1).
- Produces:
  ```cpp
  #define CORRECTION_EEPROM_MAGIC 0xC7
  #define CORRECTION_EEPROM_VERSION 1
  #define CORRECTION_EEPROM_OFFSET (sizeof(configuration))
  #define EEPROM_TOTAL_SIZE (sizeof(configuration) + 2 + sizeof(correction_table))
  class CorrectionPersistence {
  public:
    virtual ~CorrectionPersistence() {}
    virtual bool write(const correction_table &t);
    virtual bool read(correction_table &t);   // false + cleared table if magic/version/validation fails
  };
  ```

- [ ] **Step 1: Write the failing tests.** `setUp` calls `mockEEPROM.begin(EEPROM_TOTAL_SIZE); mockEEPROM.reset();`.
  - **Round-trip:** writing T1 then reading gives a byte-identical struct and true.
  - **Fresh EEPROM** (all 0xFF): read → false, `enabled==0`, `n_rows==0`.
  - **Old image:** `ConfPersistence::write` of a config with `offset=45`, `n2k_source=42`, name "TestDevice". Then `ConfPersistence::read` returns the same values, and `CorrectionPersistence::read` → false with an empty table.
  - **Writes don't clobber each other:** write T1, then write a config; reading the table back still gives T1.
  - **Corrupt block:** valid magic/version but rows unsorted → read false and the table is empty.
- [ ] **Step 2: Run the tests to see them fail.** Run `pio test -e test -f test_correctionpersistence`. Expected: build failure.
- [ ] **Step 3: Implement.** Block layout at `CORRECTION_EEPROM_OFFSET`: magic, version, then the raw `correction_table`. Use `EEE.begin(EEPROM_TOTAL_SIZE)`, `put`, `commit` and `end`, the same pattern as `ConfPersistence`.
- [ ] **Step 4: Run the full suite.** Run `pio test -e test`. Expected: all pass, including `test_configuration`.
- [ ] **Step 5: Commit.** `git add src/CorrectionPersistence.* src/DataAndConf.* test/test_correctionpersistence && git commit -m "Persist correction table in its own EEPROM block"`

---

### Task 4: STW freshness (`BoatSpeed`)

**Files:**
- Create: `src/BoatSpeed.h`, `src/BoatSpeed.cpp`
- Test: `test/test_boatspeed/test_boatspeed.cpp`

**Interfaces:**
- Produces:
  ```cpp
  #define STW_FRESHNESS_MS 3000UL
  class BoatSpeed {
  public:
    void on_stw(double stw_kn, uint8_t source, unsigned long now_ms); // NaN ignored
    bool get(unsigned long now_ms, double &stw_kn) const;             // false if never received or stale
  };
  ```
  The first source seen is kept while it is fresh. Messages from other sources are ignored until it goes stale, after which the next message from any source is adopted.

- [ ] **Step 1: Write the failing tests.**
  - **Before any message:** `get` → false.
  - **Freshness:** `on_stw(5.2, 35, 1000)`, then `get(3999)` → true with 5.2, and `get(4001)` → false.
  - **Source stickiness:** a message from source 40 at 2000 is ignored while 35 is fresh. After 35 goes stale, `on_stw(4.0, 40, 5000)` is adopted.
  - **NaN:** a NaN STW doesn't refresh.
  - **Clock wrap:** `on_stw(3.0, 35, ULONG_MAX - 500)` then `get(1000)` → true.
- [ ] **Step 2: Run the tests to see them fail.** Run `pio test -e test -f test_boatspeed`. Expected: build failure.
- [ ] **Step 3: Implement**, comparing with unsigned subtraction `now_ms - last_ms`.
- [ ] **Step 4: Run the tests to see them pass.** Run `pio test -e test -f test_boatspeed`. Expected: all pass.
- [ ] **Step 5: Commit.** `git add src/BoatSpeed.* test/test_boatspeed && git commit -m "Add STW freshness tracking"`

---

### Task 5: BLE commands

**Files:**
- Modify: `src/CommandHandler.h`, `src/CommandHandler.cpp`, `src/main.cpp:37` (constructor call)
- Test: `test/test_commandhandler/test_commandhandler.cpp` (update the `TEST_CONTEXT` macro, add tests)

**Interfaces:**
- Consumes: Task 1 edit operations, `CorrectionPersistence` (Task 3).
- Produces:
  ```cpp
  typedef void (*table_changed_callback)();
  CommandHandler(configuration &conf, ConfPersistence &confPersistence, Calibration &cal, IBLEWind &ble,
                 correction_table &table, CorrectionPersistence &tablePersistence,
                 table_changed_callback on_table_changed);   // callback may be nullptr
  ```
  `CommandContext` gains `table`, `tablePersistence` and `on_table_changed`.

  Commands:
  - `C`: payload `0`/`1`, anything else `INVALID_FORMAT`.
  - `U`: `parse_correction_row` fails → `INVALID_FORMAT`; table full → `OUT_OF_RANGE`.
  - `Z`: TWS as decimal kn → 0.1-kn units; not found → `OUT_OF_RANGE`.
  - `Y`: clear.
  - `X`: also clears the table.
  - Each successful change: persist (`WRITE_ERROR` on failure), then call the callback, then `SUCCESS`. A rejected command changes nothing and doesn't persist.

- [ ] **Step 1: Write the failing tests.** Add a `TestCorrectionPersistence` that counts writes, and a static callback counter.
  - `U10.0,40,-3.5,0.0,95,-1.0,0.0,150,2.0,0.5` → `SUCCESS`, `n_rows==1`, 1 write, 1 callback.
  - `C1` → `enabled==1`; `C2` → `INVALID_FORMAT`.
  - `Z10.0` → removes the row; `Z12.0` → `OUT_OF_RANGE`.
  - `Y` → empty.
  - An 11th `U` row → `OUT_OF_RANGE`; `U10.0,0,…` (a1=0) → `INVALID_FORMAT`. In both cases the table is unchanged, with no write and no callback.
  - `X` → table cleared and persisted.
- [ ] **Step 2: Run the tests to see them fail.** Run `pio test -e test -f test_commandhandler`. Expected: build failure on the new constructor.
- [ ] **Step 3: Implement the commands**, and register them in the `commands[]` table next to the existing entries.
- [ ] **Step 4: Run the full suite.** Run `pio test -e test`. Expected: all pass.
- [ ] **Step 5: Commit.** `git add src/CommandHandler.* src/main.cpp test/test_commandhandler && git commit -m "Add BLE commands to edit the correction table"`

---

### Task 6: BLE data fields

**Files:**
- Modify: `src/DataAndConf.h` (add `correction_status` to `all_data`), `src/BLESerializer.cpp`
- Test: `test/test_bleserializer/test_bleserializer.cpp`

**Interfaces:**
- Produces:
  ```cpp
  struct correction_status { uint32_t table_id = 0; uint8_t enabled = 0; uint8_t corrected = 0;
                             double awa = NAN; double aws = NAN; };
  // in all_data:
  correction_status correction;
  ```
  `make_message` appends, after the Wind360 part:
  - `table_id` (uint32);
  - `enabled` (uint8);
  - `corrected` (uint8);
  - `awa` (uint16, 0.1°, 0–3599, NaN → 0xFFFF);
  - `aws` (uint16, 0.1 kn, NaN → 0xFFFF).

  Existing offsets 0–42 are unchanged.

- [ ] **Step 1: Write the failing tests.** Compute the offset as `43 + 1 + calib.get_wind360().buffer_size() + 1`.
  - `{0x5330A1DE, 1, 1, 38.2193, 15.1701}` → reads back `0x5330A1DE, 1, 1, 382, 152`.
  - NaN awa/aws → 0xFFFF, 0xFFFF.
  - Existing tests unchanged.
- [ ] **Step 2: Run the tests to see them fail.** Run `pio test -e test -f test_bleserializer`. Expected: FAIL on the new tests.
- [ ] **Step 3: Implement**, using the same rounding as the existing angle fields (`+0.5`, then `% 3600`).
- [ ] **Step 4: Run the full suite.** Run `pio test -e test`. Expected: all pass. `BLEWind`'s 128-byte buffer holds the message (about 67 bytes).
- [ ] **Step 5: Commit.** `git add src/DataAndConf.h src/BLESerializer.cpp test/test_bleserializer && git commit -m "Expose correction status over BLE"`

---

### Task 7: Per-sample output decision (`WindOutput`)

**Files:**
- Create: `src/WindOutput.h`, `src/WindOutput.cpp`
- Test: `test/test_windoutput/test_windoutput.cpp`

**Interfaces:**
- Consumes: `correct_wind` (Task 1), `wind_reference_byte` (Task 2).
- Produces:
  ```cpp
  struct wind_output { double awa; double aws; bool corrected; uint8_t reference; };
  wind_output compute_wind_output(const correction_table &t, uint32_t table_id,
                                  double awa_deg, double aws_kn, bool stw_valid, double stw_kn);
  ```

- [ ] **Step 1: Write the failing tests.**
  - **Corrected sample:** T1, id 0x5330A1DE, (40, 15, valid, 6) → `{38.2193, 15.1701, true, 0xEA}`.
  - **No STW:** the same with `stw_valid=false` → `{40, 15, false, 0x02}`.
  - **NaN wind:** NaN awa → `corrected=false`, `reference=0x02`, `awa` still NaN.
  - **Table unusable:** disabled T1, or an enabled empty table → uncorrected with `0x02`.
- [ ] **Step 2: Run the tests to see them fail.** Run `pio test -e test -f test_windoutput`. Expected: build failure.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run the tests to see them pass.** Run `pio test -e test -f test_windoutput`. Expected: all pass.
- [ ] **Step 5: Commit.** `git add src/WindOutput.* test/test_windoutput && git commit -m "Add per-sample corrected output decision"`

---

### Task 8: Bus wiring (`N2KWind`, `main.cpp`)

This task needs hardware, so it is verified by build and a bench check rather than native tests.

**Files:**
- Modify: `src/N2kWind.hpp`, `src/main.cpp`

**Interfaces:**
- Consumes: everything above.
- Produces in `N2KWind`:
  ```cpp
  N2KWind(n2k_msg_handler on_msg, n2k_source_change_handler on_src); // passes on_msg to N2K::get_instance
  void send_N2K(double awa_deg, double aws_kn, double ellipse, uint8_t reference);
  void send_correction_table(const correction_table &t);   // PGN 130900, priority 7
  ```
  In `setup()`: `add_pgn(130306L)`, `add_pgn(CORRECTION_PGN)`, `add_rx_pgn(128259L)`.

- [ ] **Step 1: Update `N2KWind`.**
  - `send_N2K` writes `reference` where it currently writes `N2kWind_Apparent`.
  - `send_correction_table` encodes with `encode_correction_table(t, N2K_MANIFACTURER_CODE, …)` and adds each byte to a `tN2kMsg` with `SetPGN(CORRECTION_PGN)`. The library sends PGNs 130816–131071 as fast-packet automatically.
- [ ] **Step 2: Wire `main.cpp`.**
  - **Globals:** `correction_table table_edit, table_active; uint32_t table_id; CorrectionPersistence correctionPersistence; BoatSpeed boat_speed; volatile bool table_changed;`.
  - **Message handler:** `on_n2k_message` handles 128259 with `ParseN2kBoatSpeed`. It converts `WaterReferenced` from m/s with `msToKnots` and calls `boat_speed.on_stw(stw, msg.Source, millis())`.
  - **`_setup()`:**
    - read `table_edit` with `correctionPersistence.read`;
    - copy it to `table_active`;
    - compute `table_id`;
    - pass `table_edit`, `correctionPersistence` and `on_table_changed` (which sets `table_changed = true`) to `CommandHandler`.
  - **In `_loop()`, before the N2K send:**
    1. If `table_changed`: clear the flag, copy `table_edit` → `table_active`, recompute `table_id`, call `send_correction_table`, reset the 60 s timer.
    2. Otherwise, if 5 s after boot the startup broadcast hasn't gone yet, or 60 s have passed since the last one: call `send_correction_table`.
    3. Get the STW with `boat_speed.get(t_ms, stw)`, then call `compute_wind_output(table_active, table_id, current_data.get_out_angle(conf), current_data.wind.speed, ok, stw)`.
    4. Fill `current_data.correction` from the result, then call `send_N2K(out.awa, out.aws, ellipse, out.reference)` at the existing 500 ms rate.
- [ ] **Step 3: Build every firmware env.** Run `pio run -e esp32-c3-seeed_2_0 -e esp32-c3-seeed_1_0 -e esp32-c3-waveshare`. Expected: `SUCCESS` for all three.
- [ ] **Step 4: Run the full native suite.** Run `pio test -e test`. Expected: all pass.
- [ ] **Step 5: Commit.** `git add src/N2kWind.hpp src/main.cpp && git commit -m "Wire correction table onto the N2K bus"`
- [ ] **Step 6: Bench check with the user (manual, on the boat or the bench bus).**
  - **Startup:** `candump can0` shows PGN 130900 (CAN id `…1FF54…`) about 5 s after boot, then every 60 s.
  - **On a table change:** after `C1` and a `U` row over BLE, a 130900 broadcast goes out immediately. Byte 5 of 130306 changes from `02` to `X A` (X = `table_id` low nibble) while STW is on the bus.
  - **STW loss:** unplugging the STW source returns byte 5 to `02` within 3 s.
