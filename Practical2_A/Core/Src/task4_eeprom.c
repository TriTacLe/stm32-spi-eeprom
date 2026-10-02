/**
 ******************************************************************************
 * @file    task4_eeprom.c
 * @brief   TASK 4 : EEPROM DRIVER
 *
 * Build the driver on spi_transfer() - no pre-written EEPROM library.
 * Command values, transaction order and status bits come from the EEPROM
 * datasheet; the address format for this board is in board_config.h.
 *
 * Write completion must be decided from the EEPROM's status register. A fixed
 * delay is not acceptable as proof that a write finished. Blocking while you
 * poll is allowed in Task 4.
 ******************************************************************************
 */

#include "prac2a.h"

volatile uint16_t eeprom_test_addr = (uint16_t)EEPROM_ADDR_A;
volatile uint8_t eeprom_test_byte = (uint8_t)TEST_BYTE_B;

volatile uint8_t eeprom_status_before = 0u;
volatile uint8_t eeprom_status_after = 0u;
volatile uint8_t eeprom_read_value = 0u;
volatile uint8_t eeprom_verify_ok = 0u;
volatile uint32_t eeprom_write_wait_ms = 0u;
volatile uint32_t eeprom_timeout_count = 0u;

/* ==========================================================================
 * Driver
 * ========================================================================== */

uint8_t eeprom_read_status(void) {
  uint8_t status;

  /* SPI has no independent read. The master owns the clock, so the only way
   * to give the EEPROM 8 edges to shift its status out on is to shift a
   * byte of our own out at the same time. The value sent is ignored. */
  eeprom_cs_low();
  (void)spi_transfer(EEPROM_CMD_RDSR);
  status = spi_transfer(0xFFu);
  eeprom_cs_high();

  return status;
}

void eeprom_write_enable(void) {
  /* Its own transaction, and it has to be. The datasheet sets the latch on
   * the RISING edge of CS after the WREN opcode, so folding WREN into the
   * front of the write transaction would never arm it. */
  eeprom_cs_low();
  (void)spi_transfer(EEPROM_CMD_WREN); // 0x06
  eeprom_cs_high();
}

void eeprom_write_byte(uint16_t address, uint8_t value) {
  uint32_t start;

  // Chip holds 8192 bytes
  if (address >= EEPROM_SIZE_BYTES) {
    return;
  }

  // ask for permissions
  eeprom_write_enable();

  /* Address MSB first. Two bytes on this part, not the one the handout's
   * CAT25040 would take. The internal write cycle starts on the rising edge
   * of CS, not on the last data bit, so nothing is happening until
   * eeprom_cs_high() returns. */
  eeprom_cs_low();
  (void)spi_transfer(EEPROM_CMD_WRITE);
  for (uint32_t i = EEPROM_ADDR_BYTES; i > 0u; i--) {
    (void)spi_transfer((uint8_t)(address >> ((i - 1u) * 8u)));
  }
  (void)spi_transfer(value);
  eeprom_cs_high();

  /* RDY reads HIGH while the write cycle runs. The timeout is a hang guard
   * only: completion is decided by the bit, never by the clock. */
  start = HAL_GetTick();
  while ((eeprom_read_status() & EEPROM_SR_RDY) != 0u) {
    if ((uint32_t)(HAL_GetTick() - start) >= EEPROM_WRITE_TIMEOUT_MS) {
      eeprom_timeout_count++;
      break;
    }
  }

  eeprom_write_wait_ms = (uint32_t)(HAL_GetTick() - start);
}

uint8_t eeprom_read_byte(uint16_t address) {
  if (address >= EEPROM_SIZE_BYTES) {
    return 0u;
  }

  uint8_t value;

  eeprom_cs_low();
  (void)spi_transfer(EEPROM_CMD_READ);
  for (uint32_t i = EEPROM_ADDR_BYTES; i > 0u; i--) {
    (void)spi_transfer((uint8_t)(address >> ((i - 1u) * 8u)));
  }
  value = spi_transfer(0xFFu);
  eeprom_cs_high();

  return value;
}

/* ==========================================================================
 * LEDs
 * ========================================================================== */

void leds_write_byte(uint8_t v) {
  /* BSRR sets in its low half and resets in its high half, so one write
   * places all eight bits and touches nothing else on the port. PB10..PB15
   * carry the status LEDs, CS and the SPI pins; a read-modify-write of ODR
   * would put CS at risk mid-transaction. */
  LED_BYTE_GPIO->BSRR =
      ((uint32_t)v & LED_BYTE_MASK) | ((uint32_t)(~v & LED_BYTE_MASK) << 16);
}

/* ==========================================================================
 * High-level paths
 * ========================================================================== */

void eeprom_read_only_path(void) {
  /* Given. Reads, never writes - this is what makes the persistence test
   * meaningful, and it protects the EEPROM's finite write endurance. */
  eeprom_status_before = eeprom_read_status();
  eeprom_read_value = eeprom_read_byte(eeprom_test_addr);
  eeprom_verify_ok = (eeprom_read_value == eeprom_test_byte) ? 1u : 0u;
  leds_write_byte(eeprom_read_value);
  status_leds_show(eeprom_verify_ok ? STATUS_PASS : STATUS_FAIL);
}

void eeprom_write_verify_path(void) {
  eeprom_status_before = eeprom_read_status();
  eeprom_write_byte(eeprom_test_addr, eeprom_test_byte);

  /* Read after the write for the report: WEL should have cleared itself,
   * and a WEL still set here is the README's symptom of a wrong address
   * length rather than a wiring fault. */
  eeprom_status_after = eeprom_read_status();

  eeprom_read_value = eeprom_read_byte(eeprom_test_addr);
  eeprom_verify_ok = (eeprom_read_value == eeprom_test_byte) ? 1u : 0u;

  leds_write_byte(eeprom_read_value);
  status_leds_show(eeprom_verify_ok ? STATUS_PASS : STATUS_FAIL);
}

/* ==========================================================================
 * RUN_TASK 4 (and 5) - given
 *
 *   reset / power-up : read-only path - shows the stored byte, writes nothing,
 *                      so a power-cycle proves the byte persisted
 *   PA0              : one complete eeprom_write_verify_path()
 *   eeprom_read_loop_enable = 1 (Live Expressions): repeat a READ-ONLY
 *                      transaction every 50 ms for a steady scope trace.
 *                      It never writes.
 * ========================================================================== */

#define EEPROM_READ_LOOP_MS 50u

volatile uint8_t eeprom_read_loop_enable = 0u;

static uint32_t rd_last = 0u;

void task4_setup(void) {
  task1_gpio_init();
  eeprom_spi_init();
  board_io_init();
  eeprom_read_only_path();
}

void task4_loop(uint32_t now) {
  task1_gpio_update(now);
  read_inputs(now);

  if (btn_start_edge) {
    btn_start_edge = 0u;
    eeprom_write_verify_path(); /* one press, one write */
  }
  btn_abort_edge = 0u; /* PA3 is not used in Task 4 */

  if (eeprom_read_loop_enable &&
      ((uint32_t)(now - rd_last) >= EEPROM_READ_LOOP_MS)) {
    rd_last = now;
    eeprom_read_only_path();
  }
}
