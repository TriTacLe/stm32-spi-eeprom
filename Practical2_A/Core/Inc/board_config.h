/**
  ******************************************************************************
  * @file    board_config.h
  * @brief   Board facts and the values looked up for them.
  *
  * Every value here carries the document it came from, because the report
  * has to cite one per field.
  *
  * Two are not in any document we were given: the SPI pin mapping and the
  * EEPROM address format. The handout is wrong about both on this board, so
  * they come from the starter README and still need confirming on the bench.
  ******************************************************************************
  */

#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

#include "stm32f0xx.h"

/* ==========================================================================
 * 1. CLOCK
 * ========================================================================== */

/* SystemClock_Config() selects HSI with no PLL, AHB /1 and APB1 /1. HSI is the
 * internal 8 MHz RC oscillator (RM0091 section 7.2.2), so SYSCLK = HCLK =
 * PCLK1 = 8 MHz. SPI2 is on APB1 (RM0091 Table 1), so this is its kernel
 * clock. */
#define PCLK1_HZ                8000000UL

/* ==========================================================================
 * 2. SPI PINS - GIVEN (see README section 5, and verify by continuity)
 * ========================================================================== */
#define EE_SPI_GPIO             GPIOB
#define EE_PIN_CS               12u         /* EEPROM pin 1 (CS#)           */
#define EE_PIN_SCK              13u         /* EEPROM pin 6 (SCK)           */
#define EE_PIN_MISO             14u         /* EEPROM pin 2 (SO)            */
#define EE_PIN_MOSI             15u         /* EEPROM pin 5 (SI)            */
#define EE_CS_MASK              (1UL << EE_PIN_CS)

/* STM32F051 datasheet Table 15, alternate functions for port B: PB13 is
 * SPI2_SCK, PB14 SPI2_MISO and PB15 SPI2_MOSI, all on AF0. The functions are
 * fixed to those pins, which is why the handout's pin table cannot be right. */
#define EE_SPI                  SPI2
#define EE_SPI_AF               0u

/* ==========================================================================
 * 3. SPI BAUD RATE
 * ========================================================================== */

/* 8 MHz / 250 kHz = 32, and RM0091 section 27.7.1 gives BR = 100 for
 * fPCLK/32. The divide is 2^(BR+1), which is what EE_SCK_HZ_PREDICTED below
 * works out. */
#define EE_SPI_BR               4UL
#define EE_SCK_HZ_PREDICTED     (PCLK1_HZ >> (EE_SPI_BR + 1U))

/* ==========================================================================
 * 4. EEPROM
 * ========================================================================== */

/* GIVEN (see README section 5): the fitted part is addressed differently
 * from the CAT25040 named in the handout. Confirm both values on the bench. */
#define EEPROM_ADDR_BYTES       2u          /* address bytes, MSB first     */
#define EEPROM_SIZE_BYTES       8192u

/* CAT25640 datasheet, instruction set table. The 8192-byte array and the
 * 16-bit address in that datasheet match what the README says is fitted. */
#define EEPROM_CMD_WREN         0x06u
#define EEPROM_CMD_WRDI         0x04u
#define EEPROM_CMD_RDSR         0x05u
#define EEPROM_CMD_WRSR         0x01u
#define EEPROM_CMD_READ         0x03u
#define EEPROM_CMD_WRITE        0x02u

/* CAT25640 status register table: bit 0 RDY, bit 1 WEL. RDY is named for
 * ready but reads the other way round - HIGH means the internal write cycle
 * is still running, LOW means the device will accept commands. */
#define EEPROM_SR_RDY           0x01u       /* set while BUSY writing       */
#define EEPROM_SR_WEL           0x02u

/* Upper bound on waiting for a write. A hang guard only: completion must be
 * decided from the status register, never from elapsed time. */
#define EEPROM_WRITE_TIMEOUT_MS 50u

/* ==========================================================================
 * 5. BOARD I/O - GIVEN (UCT board: Board.md)
 * ========================================================================== */
#define LED_BYTE_GPIO           GPIOB
#define LED_BYTE_MASK           0x00FFu     /* PB0..PB7                     */
#define LED_RED_PIN             10u         /* PB10                         */
#define LED_GREEN_PIN           11u         /* PB11                         */

#define BTN_GPIO                GPIOA
#define BTN_START_PIN           0u          /* PA0 / SW0, active low        */
#define BTN_ABORT_PIN           3u          /* PA3 / SW3, active low        */

#define SCOPE_GPIO              GPIOC
#define SCOPE_PIN               13u         /* PC13, header P1              */

/* ==========================================================================
 * 6. GROUP VALUES
 * ========================================================================== */

/* Last three decimal digits of each student number, without leading zeros
 * since a leading zero would make these octal. B and A are checked against
 * hand-worked values in main.c. */
#define STUDENT_N1              4u          /* LXXTRI004 */
#define STUDENT_N2              4u          /* SHRGIL004 */

/* The formulas from the handout. */
#define TEST_BYTE_B_RAW         ((((STUDENT_N1 ^ STUDENT_N2) + 0x3Du)) % 256u)
#define TEST_BYTE_B             (((TEST_BYTE_B_RAW == 0x00u) ||   \
                                  (TEST_BYTE_B_RAW == 0xFFu))     \
                                 ? (TEST_BYTE_B_RAW ^ 0x5Au)      \
                                 :  TEST_BYTE_B_RAW)
#define EEPROM_ADDR_A           ((STUDENT_N1 + 3u * STUDENT_N2) % 256u)

/* ==========================================================================
 * Helpers for fields that are 2 bits per pin (MODER, OSPEEDR, PUPDR) and
 * 4 bits per pin (AFRH, pins 8..15). They only do the bit positions - you
 * still have to know what value goes in each field.
 * ========================================================================== */
#define MODER2(pin, val)        ((uint32_t)(val) << ((pin) * 2u))
#define MODER2_MASK(pin)        (3UL << ((pin) * 2u))
#define AFRH4(pin, af)          ((uint32_t)(af) << (((pin) - 8u) * 4u))
#define AFRH4_MASK(pin)         (0xFUL << (((pin) - 8u) * 4u))

#endif /* __BOARD_CONFIG_H */
