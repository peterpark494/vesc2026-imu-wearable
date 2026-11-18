/* Standalone diagnostic utility for the VESC result-frame wire format. */
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { HEADER_SIZE = 9, RECORD_SIZE = 11, MAX_FRAME = HEADER_SIZE + 255 * RECORD_SIZE };

static uint8_t crc8(const uint8_t *data, size_t size)
{
    uint8_t remainder = 0;

    for (size_t i = 0; i < size; ++i) {
        remainder ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit) {
            unsigned shifted = (unsigned)remainder << 1;
            remainder = (uint8_t)(shifted ^ ((remainder & 0x80) ? 0x07U : 0U));
        }
    }
    return remainder;
}

static uint64_t read_le(const uint8_t *data, unsigned size)
{
    uint64_t value = 0;

    for (unsigned i = 0; i < size; ++i) {
        value |= (uint64_t)data[i] << (8U * i);
    }
    return value;
}

static const char *frame_error(const uint8_t *frame, size_t size)
{
    if (size < HEADER_SIZE || memcmp(frame, "vesc", 4) != 0) {
        return "invalid frame header";
    }
    size_t payload_size = (size_t)read_le(frame + 7, 2);
    if (payload_size != (size_t)frame[6] * RECORD_SIZE ||
        payload_size != size - HEADER_SIZE) {
        return "record count or payload length mismatch";
    }
    if (crc8(frame + HEADER_SIZE, payload_size) != frame[5]) {
        return "payload CRC mismatch";
    }
    return NULL;
}

static int hex_digit(unsigned char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    ch = (unsigned char)tolower(ch);
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return -1;
}

static bool parse_hex(const char *input, uint8_t *frame, size_t *size)
{
    int high = -1;
    *size = 0;
    for (; *input; ++input) {
        unsigned char ch = (unsigned char)*input;
        if (isspace(ch) && high < 0) continue;
        int digit = hex_digit(ch);
        if (digit < 0) return false;
        if (high < 0) {
            high = digit;
        } else {
            if (*size == MAX_FRAME) return false;
            frame[(*size)++] = (uint8_t)(high * 16 + digit);
            high = -1;
        }
    }
    return high < 0;
}

static bool self_test(void)
{
    uint8_t frame[HEADER_SIZE + RECORD_SIZE] = {
        'v', 'e', 's', 'c', 3, 0, 1, 11, 0,
        0x00, 0x68, 0xe5, 0xcf, 0x8b, 0x01, 0, 0, 0x34, 0x12, 8
    };
    if (crc8((const uint8_t *)"123456789", 9) != 0xf4) return false;
    frame[5] = crc8(frame + HEADER_SIZE, RECORD_SIZE);
    if (frame_error(frame, sizeof frame) != NULL ||
        frame_error(frame, sizeof frame - 1) == NULL ||
        read_le(frame + HEADER_SIZE, 8) != UINT64_C(1700000000000) ||
        read_le(frame + HEADER_SIZE + 8, 2) != 0x1234) {
        return false;
    }
    frame[sizeof frame - 1] ^= 1;
    return frame_error(frame, sizeof frame) != NULL;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s HEX_FRAME | --self-test\n", argv[0]);
        return 2;
    }
    if (strcmp(argv[1], "--self-test") == 0) {
        bool passed = self_test();
        puts(passed ? "Self-test passed" : "Self-test failed");
        return passed ? 0 : 1;
    }

    uint8_t frame[MAX_FRAME];
    size_t size;
    if (!parse_hex(argv[1], frame, &size)) {
        fputs("Invalid hexadecimal input\n", stderr);
        return 2;
    }
    const char *error = frame_error(frame, size);
    if (error != NULL) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    printf("flags=0x%02x records=%u\n", (unsigned)frame[4], (unsigned)frame[6]);
    for (unsigned i = 0; i < frame[6]; ++i) {
        const uint8_t *record = frame + HEADER_SIZE + i * RECORD_SIZE;
        printf("utc_ms=%" PRIu64 " sequence=%" PRIu64 " action=%u\n",
               read_le(record, 8), read_le(record + 8, 2), (unsigned)record[10]);
    }
    return 0;
}
