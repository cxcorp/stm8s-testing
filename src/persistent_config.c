//FLASH_ProgramWord
#include "persistent_config.h"

#define FLASH_DATA_ADDR 0x4000

#define CONFIG_VALUE_SET 0xFF

#define REFLEX_THRESHOLD_OFFSET 0
#define REFLEX_THRESHOLD_DEFAULT 400

static void flash_data_read(uint16_t address, uint8_t *data, uint16_t len);
static void flash_data_write(uint16_t address, uint8_t *data, uint16_t len);

uint16_t config_read_reflex_threshold()
{
    uint8_t buf[3] = {0};
    flash_data_read(REFLEX_THRESHOLD_OFFSET, buf, sizeof(buf) / sizeof(buf[0]));

    if (buf[0] != CONFIG_VALUE_SET)
    {
        return REFLEX_THRESHOLD_DEFAULT;
    }

    return buf[1] | (buf[2] << 8);
}

void config_write_reflex_threshold(uint16_t value)
{
    uint8_t buf[3];
    buf[0] = CONFIG_VALUE_SET;
    buf[1] = value & 0xFF;
    buf[2] = (value & 0xFF00) >> 8;
    flash_data_write(REFLEX_THRESHOLD_OFFSET, buf, sizeof(buf) / sizeof(buf[0]));
}

static void flash_data_read(uint16_t address, uint8_t *data, uint16_t len)
{
    uint8_t *pointer = (uint8_t *)FLASH_DATA_ADDR + address;
    FLASH_Unlock(FLASH_MEMTYPE_DATA);

    for (; len > 0; len--)
    {
        *data++ = *pointer++;
    }
    FLASH_Lock(FLASH_MEMTYPE_DATA);
}

static void flash_data_write(uint16_t address, uint8_t *data, uint16_t len)
{
    uint8_t *pointer = (uint8_t *)FLASH_DATA_ADDR + address;
    FLASH_Unlock(FLASH_MEMTYPE_DATA);

    for (; len > 0; len--)
    {
        *pointer++ = *data++;
        // wait until End Of Programming gets set
        while (FLASH->IAPSR & FLASH_FLAG_EOP != FLASH_FLAG_EOP)
            ;
    }
    FLASH_Lock(FLASH_MEMTYPE_DATA);
}