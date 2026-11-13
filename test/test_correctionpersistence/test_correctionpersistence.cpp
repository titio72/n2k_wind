#include <unity.h>
#include <string.h>
#include <MockEEPROM.h>
#include "../src/DataAndConf.h"
#include "../src/WindCorrection.h"
#include "../src/CorrectionPersistence.h"

static correction_table make_t1()
{
    correction_table t;
    correction_clear(t);
    t.enabled = 1;
    t.n_rows = 2;
    t.rows[0] = {60, {{45, -60, 0}, {95, -40, 0}, {150, -20, 0}}};
    t.rows[1] = {100, {{40, -35, 0}, {95, -10, 0}, {150, 20, 5}}};
    return t;
}

static correction_table filled()
{
    // non-empty garbage so "read cleared the table" is observable
    correction_table t = make_t1();
    t.enabled = 1;
    return t;
}

void setUp(void)
{
    mockEEPROM.begin(EEPROM_TOTAL_SIZE);
    mockEEPROM.reset();
}

void tearDown(void)
{
    mockEEPROM.end();
}

void test_roundtrip(void)
{
    CorrectionPersistence p;
    correction_table t1 = make_t1();
    TEST_ASSERT_TRUE(p.write(t1));
    correction_table back;
    correction_clear(back);
    TEST_ASSERT_TRUE(p.read(back));
    TEST_ASSERT_EQUAL_MEMORY(&t1, &back, sizeof(t1));
}

void test_fresh_eeprom(void)
{
    CorrectionPersistence p;
    correction_table t = filled();
    TEST_ASSERT_FALSE(p.read(t));
    TEST_ASSERT_EQUAL_UINT8(0, t.enabled);
    TEST_ASSERT_EQUAL_UINT8(0, t.n_rows);
}

void test_old_image_keeps_configuration(void)
{
    ConfPersistence cp;
    configuration conf;
    conf.offset = 45;
    conf.n2k_source = 42;
    conf.set_ble_name("TestDevice");
    TEST_ASSERT_TRUE(cp.write(conf));

    configuration back;
    TEST_ASSERT_TRUE(cp.read(back));
    TEST_ASSERT_EQUAL_INT16(45, back.offset);
    TEST_ASSERT_EQUAL_UINT8(42, back.n2k_source);
    TEST_ASSERT_EQUAL_STRING("TestDevice", back.ble_name);

    CorrectionPersistence p;
    correction_table t = filled();
    TEST_ASSERT_FALSE(p.read(t));
    TEST_ASSERT_EQUAL_UINT8(0, t.enabled);
    TEST_ASSERT_EQUAL_UINT8(0, t.n_rows);
}

void test_config_write_does_not_clobber_table(void)
{
    CorrectionPersistence p;
    correction_table t1 = make_t1();
    TEST_ASSERT_TRUE(p.write(t1));

    ConfPersistence cp;
    configuration conf;
    conf.offset = 12;
    TEST_ASSERT_TRUE(cp.write(conf));
    // On ESP32 commit() writes exactly begin(size) bytes: a smaller size
    // would cut off the table block.
    TEST_ASSERT_EQUAL_UINT32(EEPROM_TOTAL_SIZE, mockEEPROM.size());

    correction_table back;
    correction_clear(back);
    TEST_ASSERT_TRUE(p.read(back));
    TEST_ASSERT_EQUAL_MEMORY(&t1, &back, sizeof(t1));

    configuration cback;
    TEST_ASSERT_TRUE(cp.read(cback));
    TEST_ASSERT_EQUAL_INT16(12, cback.offset);
}

void test_corrupt_block(void)
{
    correction_table bad = make_t1();
    bad.rows[1].tws = 50; // rows not sorted
    uint8_t hdr[2] = {CORRECTION_EEPROM_MAGIC, CORRECTION_EEPROM_VERSION};
    mockEEPROM.set_data(CORRECTION_EEPROM_OFFSET, hdr, 2);
    mockEEPROM.set_data(CORRECTION_EEPROM_OFFSET + 2, (const uint8_t *)&bad, sizeof(bad));

    CorrectionPersistence p;
    correction_table t = filled();
    TEST_ASSERT_FALSE(p.read(t));
    TEST_ASSERT_EQUAL_UINT8(0, t.enabled);
    TEST_ASSERT_EQUAL_UINT8(0, t.n_rows);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_roundtrip);
    RUN_TEST(test_fresh_eeprom);
    RUN_TEST(test_old_image_keeps_configuration);
    RUN_TEST(test_config_write_does_not_clobber_table);
    RUN_TEST(test_corrupt_block);
    return UNITY_END();
}
