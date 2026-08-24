#include "stm32f4xx.h"
#include "nrf.h"
#include "logger.h"
#include "command_queue.h"

extern SPI_HandleTypeDef hspi1;

constexpr int RF_Channel = 76;
static const uint8_t Pipe_Addresses[8][5] = {
    { 0xE7, 0xE7, 0xE7, 0xE7, 0xE7 }, // pipe 0
    { 0xC2, 0xC2, 0xC2, 0xC2, 0xC2 }, // pipe 1
    { 0xC3, 0xC2, 0xC2, 0xC2, 0xC2 }, // pipe 2
    { 0xC4, 0xC2, 0xC2, 0xC2, 0xC2 }, // pipe 3
    { 0xC5, 0xC2, 0xC2, 0xC2, 0xC2 }, // pipe 4
    { 0xC6, 0xC2, 0xC2, 0xC2, 0xC2 }, // pipe 5
    { 0xC7, 0xC2, 0xC2, 0xC2, 0xC2 }, // pipe 6
    { 0xC8, 0xC2, 0xC2, 0xC2, 0xC2 }, // pipe 7
};

static inline void CSN_Low(void) {
    HAL_GPIO_WritePin(nRF_ChipSelect_GPIO_Port, nRF_ChipSelect_Pin, GPIO_PIN_RESET);
}

static inline void CSN_High(void) {
    HAL_GPIO_WritePin(nRF_ChipSelect_GPIO_Port, nRF_ChipSelect_Pin, GPIO_PIN_SET);
}

static inline void CE_Low(void)  {
    HAL_GPIO_WritePin(nRF_ClockEnable_GPIO_Port, nRF_ClockEnable_Pin, GPIO_PIN_RESET);
}

static inline void CE_High(void) {
    HAL_GPIO_WritePin(nRF_ClockEnable_GPIO_Port, nRF_ClockEnable_Pin, GPIO_PIN_SET);
}

static uint8_t spi_xfer(uint8_t tx) {
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

static uint8_t nrf_cmd(uint8_t c) {
    CSN_Low();
    uint8_t s = spi_xfer(c);
    CSN_High();
    return s;
}

static uint8_t nrf_read_reg(uint8_t reg) {
    CSN_Low();
    spi_xfer(0x00 | (reg & 0x1F));
    uint8_t v = spi_xfer(0xFF);
    CSN_High();
    return v;
}

static void nrf_write_reg(uint8_t reg, uint8_t val) {
    CSN_Low();
    spi_xfer(0x20 | (reg & 0x1F));
    spi_xfer(val);
    CSN_High();
}

static void nrf_write_reg_multi(uint8_t reg, const uint8_t *buf, uint8_t len) {
    CSN_Low();
    spi_xfer(0x20 | (reg & 0x1F));
    for (uint8_t i = 0; i < len; i++) {
        spi_xfer(buf[i]);
    }
    CSN_High();
}

enum NRF_CMD {
    NRF_CMD_R_RX_PAYLOAD = 0x61,
    NRF_CMD_W_TX_PAYLOAD = 0xA0,
    NRF_CMD_FLUSH_TX = 0xE1,
    NRF_CMD_FLUSH_RX = 0xE2,
    NRF_CMD_NOP      = 0xFF
};

enum NRF_REGISTERS {
    NRF_REG_CONFIG      = 0x00,
    NRF_REG_EN_AA       = 0x01,
    NRF_REG_EN_RXADDR   = 0x02,
    NRF_REG_SETUP_AW    = 0x03,
    NRF_REG_SETUP_RETR  = 0x04,
    NRF_REG_RF_CH       = 0x05,
    NRF_REG_RF_SETUP    = 0x06,
    NRF_REG_STATUS      = 0x07,
    NRF_REG_OBSERVE_TX  = 0x08,
    NRF_REG_RPD         = 0x09,
    NRF_REG_RX_ADDR_P0  = 0x0A,
    NRF_REG_RX_ADDR_P1  = 0x0B,
    NRF_REG_RX_ADDR_P2  = 0x0C,
    NRF_REG_RX_ADDR_P3  = 0x0D,
    NRF_REG_RX_ADDR_P4  = 0x0E,
    NRF_REG_RX_ADDR_P5  = 0x0F,
    NRF_REG_TX_ADDR     = 0x10,
    NRF_REG_RX_PW_P0    = 0x11,
    NRF_REG_RX_PW_P1    = 0x12,
    NRF_REG_RX_PW_P2    = 0x13,
    NRF_REG_RX_PW_P3    = 0x14,
    NRF_REG_RX_PW_P4    = 0x15,
    NRF_REG_RX_PW_P5    = 0x16,
    NRF_REG_FIFO_STATUS = 0x17,
    NRF_REG_DYNPD       = 0x1C,
    NRF_REG_FEATURE     = 0x1D
};

void nrf_prx_init(int address)
{
    CE_Low();

    // flush everything
    nrf_cmd(NRF_CMD_FLUSH_TX);
    nrf_cmd(NRF_CMD_FLUSH_RX);
    // clear IRQ status
    nrf_write_reg(NRF_REG_STATUS, 0x70);

    // enable pipe0
    nrf_write_reg(NRF_REG_EN_RXADDR, 0x01);
    // enable auto-ack on pipe0
    nrf_write_reg(NRF_REG_EN_AA, 0x01);
    // set address width to 5 bytes
    nrf_write_reg(NRF_REG_SETUP_AW, 0x03);
    // set address for pipe0
    const uint8_t dongle_pipe = ((uint8_t)address) & 0x07;
    nrf_write_reg_multi(NRF_REG_RX_ADDR_P0, Pipe_Addresses[dongle_pipe], 5);

    // set RF channel
    nrf_write_reg(NRF_REG_RF_CH, RF_Channel);
    // radio setup: 2Mbps, 0dBm
    nrf_write_reg(NRF_REG_RF_SETUP, 0x0F);

    // disable additional features
    nrf_write_reg(NRF_REG_FEATURE, 0x00);
    // disable dynamic payload length (we use fixed 4-byte length)
    nrf_write_reg(NRF_REG_DYNPD, 0x00);
    // set payload length on pipe0 to 4 bytes
    nrf_write_reg(NRF_REG_RX_PW_P0, 4);

    // power up, primary RCX, enable CRC (1 byte)
    nrf_write_reg(NRF_REG_CONFIG, 0x0B | (1 << 4) | (1 << 5));

    // wait for the device to power up (~1.5ms) before asserting CE
    HAL_Delay(2);

    CE_High();
}

/* ---------- Poll & read frames ---------- */
bool nrf_poll_rx(void) {
    uint8_t pl[4];

    // write NOP, read STATUS
    const uint8_t status = nrf_cmd(NRF_CMD_NOP);
    //if (status != 0 && status != 0x0E) {
    //    u2_printf("nRF STATUS: 0x%02X\r\n", status);
    //}

    // RX data ready
    if (status & 0x40) {
        while (1) {
            // read FIFO status
            const uint8_t fifo = nrf_read_reg(NRF_REG_FIFO_STATUS);
            //u2_printf("FIFO status: 0x%02X\r\n", fifo);

            // RX FIFO empty? break - we've read all data
            if (fifo & 0x01) {
                break;
            }

            // read payload
            CSN_Low();

            // trigger RX payload read
            spi_xfer(NRF_CMD_R_RX_PAYLOAD);
            // send dummy NOPs to read the payload
            for (uint8_t i = 0; i < 4; i++) {
                pl[i] = spi_xfer(NRF_CMD_NOP);
            }

            CSN_High();

            const uint8_t opcode = pl[0];
            switch (opcode) {
                case 0x00: u2_printf(">>>>>>>>>>>>>>>>> RX: Initialize mode=%u\r\n", pl[1]); break;
                case 0x01: u2_printf(">>>>>>>>>>>>>>>>> RX: Set speed=%u\r\n", pl[1]); break;
                case 0x02: u2_printf(">>>>>>>>>>>>>>>>> RX: Move distance=%u\r\n", (int16_t)((uint16_t)pl[1] | ((uint16_t)pl[2] << 8))); break;
                case 0x03: u2_printf(">>>>>>>>>>>>>>>>> RX: Turn angle=%d\r\n", (int16_t)((uint16_t)pl[1] | ((uint16_t)pl[2] << 8))); break;
                case 0x04: u2_printf(">>>>>>>>>>>>>>>>> RX: Stop\r\n"); break;
                //case 0x05: u2_printf(">>>>>>>>>>>>>>>>> RX: Immediate L=%u R=%u T=%u\r\n", pl[1], pl[2], pl[3]); break;
            }

            if (opcode == 0x04) {
                command_queue_clear();
            }

            command_queue_push(pl);

            pl[0] = 0xFF; // mark as processed
        }

        // clear RX data ready flag
        nrf_write_reg(NRF_REG_STATUS, 0x40);
        return true;
    }

    return false;
}
