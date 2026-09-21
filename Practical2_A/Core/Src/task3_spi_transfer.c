/**
  ******************************************************************************
  * @file    task3_spi_transfer.c
  * @brief   TASK 3 : SEND ONE BYTE AND PROVE THE WAVEFORM
  ******************************************************************************
  */

#include "prac2a.h"

/* ==========================================================================
 * Chip select
 * ========================================================================== */

void eeprom_cs_low(void)
{
    EE_SPI_GPIO->BRR = EE_CS_MASK;          /* RM0091 GPIOx_BRR             */
}

void eeprom_cs_high(void)
{
    /* RM0091, "Procedure for disabling the SPI": wait for FTLVL empty, then
     * for BSY to clear. The manual names this exact case, a master providing
     * the end of the NSS pulse in software. RXNE only says a byte arrived,
     * not that the last bit has left the shift register. */
    while ((EE_SPI->SR & SPI_SR_FTLVL) != 0u)
    {
    }
    while ((EE_SPI->SR & SPI_SR_BSY) != 0u)
    {
    }

    EE_SPI_GPIO->BSRR = EE_CS_MASK;         /* RM0091 GPIOx_BSRR            */
}

/* ==========================================================================
 * TASK 3
 * ========================================================================== */

uint8_t spi_transfer(uint8_t tx)
{
    while ((EE_SPI->SR & SPI_SR_TXE) == 0u)
    {
    }

    /* An 8-bit store. A 32- or 16-bit write to DR packs two frames and clocks
     * 16 bits, which on the scope looks like twice as many pulses per call. */
    *(volatile uint8_t *)&EE_SPI->DR = tx;

    while ((EE_SPI->SR & SPI_SR_RXNE) == 0u)
    {
    }

    /* Read every time. A byte left in the receive FIFO shifts the next
     * transfer's result by one and the driver reads stale data from then on. */
    return *(volatile uint8_t *)&EE_SPI->DR;
}

/* ==========================================================================
 * RUN_TASK 3 - given
 * Sends task3_test_byte every 100 ms with CS held HIGH, so the EEPROM ignores
 * it and your oscilloscope has a repeating burst to trigger on.
 * task3_test_byte can be changed from Live Expressions without a rebuild.
 * ========================================================================== */

#define TASK3_TX_PERIOD_MS  100u

volatile uint8_t  task3_test_byte = (uint8_t)TEST_BYTE_B;
volatile uint8_t  task3_rx_byte   = 0u;
volatile uint32_t task3_tx_count  = 0u;

static uint32_t t3_last = 0u;

void task3_setup(void)
{
    task1_gpio_init();
    eeprom_spi_init();
}

void task3_loop(uint32_t now)
{
    task1_gpio_update(now);

    if ((uint32_t)(now - t3_last) < TASK3_TX_PERIOD_MS)
    {
        return;
    }
    t3_last = now;

    task3_rx_byte = spi_transfer(task3_test_byte);
    task3_tx_count++;
}
