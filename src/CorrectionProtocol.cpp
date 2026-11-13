#include "CorrectionProtocol.h"

#define CORRECTION_HEADER_SIZE 8
#define CORRECTION_ROW_SIZE (2 + 5 * CORRECTION_ANCHORS)
#define WIND_REFERENCE_APPARENT 2

uint32_t crc32_ieee(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
    }
    return ~crc;
}

static void put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)(v >> 8);
}

static void put_u32(uint8_t *p, uint32_t v)
{
    for (int i = 0; i < 4; i++)
        p[i] = (uint8_t)(v >> (8 * i));
}

size_t encode_correction_table(const correction_table &t, uint16_t manufacturer_code,
                               uint8_t *buf, size_t buf_len)
{
    uint8_t n = t.n_rows > CORRECTION_MAX_ROWS ? CORRECTION_MAX_ROWS : t.n_rows;
    size_t len = CORRECTION_HEADER_SIZE + 2 + (size_t)n * CORRECTION_ROW_SIZE;
    if (buf == nullptr || buf_len < len)
        return 0;

    put_u16(buf, (uint16_t)((manufacturer_code & 0x7FF) | (3 << 11) | (CORRECTION_INDUSTRY_CODE << 13)));
    buf[2] = CORRECTION_MSG_TYPE_TABLE;
    buf[3] = CORRECTION_PAYLOAD_VERSION;
    buf[8] = t.enabled;
    buf[9] = n;
    uint8_t *p = buf + 10;
    for (int i = 0; i < n; i++)
    {
        const correction_row &r = t.rows[i];
        put_u16(p, r.tws);
        p += 2;
        for (int j = 0; j < CORRECTION_ANCHORS; j++)
        {
            *p++ = r.anchors[j].angle;
            put_u16(p, (uint16_t)r.anchors[j].d_angle);
            p += 2;
            put_u16(p, (uint16_t)r.anchors[j].d_speed);
            p += 2;
        }
    }
    put_u32(buf + 4, crc32_ieee(buf + CORRECTION_HEADER_SIZE, len - CORRECTION_HEADER_SIZE));
    return len;
}

uint32_t correction_table_id(const correction_table &t)
{
    uint8_t buf[CORRECTION_MAX_PAYLOAD];
    size_t len = encode_correction_table(t, 0, buf, sizeof(buf));
    return crc32_ieee(buf + CORRECTION_HEADER_SIZE, len - CORRECTION_HEADER_SIZE);
}

uint8_t wind_reference_byte(bool corrected, uint32_t table_id)
{
    if (!corrected)
        return WIND_REFERENCE_APPARENT;
    return (uint8_t)(WIND_REFERENCE_APPARENT | 0x08 | ((table_id & 0x0F) << 4));
}
