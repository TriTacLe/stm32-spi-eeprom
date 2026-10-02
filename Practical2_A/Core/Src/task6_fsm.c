/**
 ******************************************************************************
 * @file    task6_fsm.c
 * @brief   TASK 6 : NON-BLOCKING EEPROM TRANSACTION STATE MACHINE
 *
 * Restructure the Task 4 transaction so the main loop never stops:
 *
 *   request -> write-enable -> write -> wait for EEPROM
 *           -> read-back -> verify -> result
 *
 * Rules (handout, Task 6):
 *   - No HAL_Delay(), and no software busy-wait for the EEPROM's internal
 *     write cycle. You may use HAL_GetTick() to decide when the next status
 *     check is due.
 *   - Each call does a small amount of work, updates the state, and returns.
 *   - Do not put the whole transaction inside one blocking function called
 *     from the loop.
 *   - While a transaction is in progress the loop must still respond to PA3.
 *
 * Controls: PA0 starts, PA3 aborts. PB0..PB7 show the last byte read, PB11
 * (green) a successful verification, PB10 (red) a failed one.
 ******************************************************************************
 */

#include "prac2a.h"

typedef enum {
  EE_IDLE = 0,
  EE_WRITE_ENABLE,
  EE_WRITE,
  EE_BUSY,
  EE_READ,
  EE_VERIFY,
  EE_SUCCEEDED,
  EE_FAILED
} ee_state_t;

/* A 5 ms typical write costs about five status reads at this interval. */
#define EE_POLL_INTERVAL_MS 1u

static uint32_t ee_started_ms = 0u;
static uint32_t ee_next_poll_ms = 0u;

volatile uint8_t ee_state = 0u;
volatile uint8_t ee_last_read = 0u;
volatile uint8_t ee_use_fsm = 1u;

void update_eeprom_state_machine(uint32_t now) {
  /* Abort first, so PA3 is honoured on the same call it arrives. Every
   * transaction below raises CS before the state advances, so there is no
   * point here where abandoning the run can strand CS low. */
  if (btn_abort_edge) {
    btn_abort_edge = 0u;
    if ((ee_state != EE_IDLE) && (ee_state != EE_SUCCEEDED) &&
        (ee_state != EE_FAILED)) {
      ee_state = EE_IDLE;
      return;
    }
  }

  switch ((ee_state_t)ee_state) {
  case EE_IDLE:
  case EE_SUCCEEDED:
  case EE_FAILED:
    if (btn_start_edge) {
      btn_start_edge = 0u;
      eeprom_verify_ok = 0u;
      ee_state = EE_WRITE_ENABLE;
    }
    break;

  case EE_WRITE_ENABLE:
    eeprom_status_before = eeprom_read_status();
    eeprom_write_enable();
    ee_state = EE_WRITE;
    break;

  case EE_WRITE:
    /* One transaction, then out. The internal write cycle starts when CS
     * rises inside this step, so the clock starts here too. */
    eeprom_cs_low();
    (void)spi_transfer(EEPROM_CMD_WRITE);
    for (uint32_t i = EEPROM_ADDR_BYTES; i > 0u; i--) {
      (void)spi_transfer((uint8_t)(eeprom_test_addr >> ((i - 1u) * 8u)));
    }
    (void)spi_transfer(eeprom_test_byte);
    eeprom_cs_high();

    ee_started_ms = now;
    ee_next_poll_ms = now + EE_POLL_INTERVAL_MS;
    ee_state = EE_BUSY;
    break;

  case EE_BUSY:
    /* The non-blocking part. Return immediately unless a poll is due. */
    if ((int32_t)(now - ee_next_poll_ms) < 0) {
      break;
    }
    ee_next_poll_ms = now + EE_POLL_INTERVAL_MS;

    eeprom_status_after = eeprom_read_status();
    if ((eeprom_status_after & EEPROM_SR_RDY) == 0u) {
      eeprom_write_wait_ms = (uint32_t)(now - ee_started_ms);
      ee_state = EE_READ;
    } else if ((uint32_t)(now - ee_started_ms) >= EEPROM_WRITE_TIMEOUT_MS) {
      /* A part that never reports ready. Not a verification failure,
       * so it is counted separately. */
      eeprom_timeout_count++;
      ee_state = EE_FAILED;
    } else {
      /* still writing */
    }
    break;

  case EE_READ:
    eeprom_read_value = eeprom_read_byte(eeprom_test_addr);
    ee_last_read = eeprom_read_value;
    ee_state = EE_VERIFY;
    break;

  case EE_VERIFY:
    eeprom_verify_ok = (eeprom_read_value == eeprom_test_byte) ? 1u : 0u;
    ee_state = eeprom_verify_ok ? EE_SUCCEEDED : EE_FAILED;
    break;

  default:
    ee_state = EE_IDLE;
    break;
  }
}

void update_outputs(void) {
  leds_write_byte(ee_last_read);

  /* Both status LEDs off while a transaction runs and after an abort, so
   * neither colour can be mistaken for the result of the run in progress.
   * Green and red mean a verification actually happened. */
  switch ((ee_state_t)ee_state) {
  case EE_SUCCEEDED:
    status_leds_show(STATUS_PASS);
    break;
  case EE_FAILED:
    status_leds_show(STATUS_FAIL);
    break;
  case EE_IDLE:
    break; /* keep whatever the last run showed */
  default:
    status_leds_show(STATUS_OFF);
    break;
  }
}

/* ==========================================================================
 * RUN_TASK 6 - given
 *
 * The main loop has exactly the shape the handout asks for:
 *     read_inputs();  update_eeprom_state_machine();  update_outputs();
 *
 * Set ee_use_fsm = 0 in Live Expressions to run the Task 4 blocking path on
 * PA0 instead - useful when you explain why yours is non-blocking. PC13 keeps
 * toggling as a heartbeat; watch it on the scope in both modes.
 * ========================================================================== */

void task6_setup(void) {
  task1_gpio_init();
  eeprom_spi_init();
  board_io_init();

  eeprom_read_only_path();
  ee_last_read = eeprom_read_value;

  /* eeprom_read_only_path() has already lit the LEDs from the stored byte.
   * Idle leaves them alone, so the persistence test still shows green after
   * a reset without this state machine writing anything. */
  ee_state = EE_IDLE;
}

void task6_loop(uint32_t now) {
  task1_gpio_update(now);
  read_inputs(now);

  if (ee_use_fsm) {
    update_eeprom_state_machine(now);
    update_outputs();
  } else {
    if (btn_start_edge) {
      btn_start_edge = 0u;
      eeprom_write_verify_path(); /* Task 4: blocks for the write */
      ee_last_read = eeprom_read_value;
    }
    btn_abort_edge = 0u;
  }
}
