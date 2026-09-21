/**
  ******************************************************************************
  * @file    task2_spi_config.c
  * @brief   TASK 2 : CONFIGURE THE HARDWARE SPI PERIPHERAL
  *
  * Configure the SPI peripheral entirely through its registers. Not allowed:
  * HAL_SPI_*, LL SPI transfer functions, CubeMX-generated SPI configuration,
  * GPIO bit-banging. Every register write you add should cite the RM0091
  * register it comes from.
  ******************************************************************************
  */

#include "prac2a.h"

volatile uint32_t dbg_gpiob_moder      = 0u;
volatile uint32_t dbg_gpiob_afrh       = 0u;
volatile uint32_t dbg_spi_cr1          = 0u;
volatile uint32_t dbg_spi_cr2          = 0u;
volatile uint32_t dbg_spi_sr           = 0u;
volatile uint32_t dbg_sck_hz_predicted = 0u;

void eeprom_spi_init(void)
{
    uint32_t reg;

    /* RM0091 section 7.4.6: GPIOB clock is AHBENR bit 18. SPI2 sits on APB1,
     * so its enable is APB1ENR bit 14 (section 7.4.8). */
    RCC->AHBENR  |= RCC_AHBENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    (void)RCC->APB1ENR;

    /* CS high BEFORE it becomes an output, so the pin never presents a low to
     * the EEPROM. MODER last means the first level the device ever sees is
     * the deselected one. */
    EE_SPI_GPIO->BSRR = EE_CS_MASK;
    reg  = EE_SPI_GPIO->MODER;
    reg &= ~MODER2_MASK(EE_PIN_CS);
    reg |= MODER2(EE_PIN_CS, 1u);          /* general purpose output */
    EE_SPI_GPIO->MODER = reg;

    /* SCK, MISO and MOSI to alternate function (MODER = 10), then AF0 in the
     * high alternate-function register since all three are pins 8..15.
     * STM32F051 datasheet Table 15. */
    reg  = EE_SPI_GPIO->AFR[1];
    reg &= ~(AFRH4_MASK(EE_PIN_SCK) | AFRH4_MASK(EE_PIN_MISO) | AFRH4_MASK(EE_PIN_MOSI));
    reg |= AFRH4(EE_PIN_SCK,  EE_SPI_AF)
         | AFRH4(EE_PIN_MISO, EE_SPI_AF)
         | AFRH4(EE_PIN_MOSI, EE_SPI_AF);
    EE_SPI_GPIO->AFR[1] = reg;

    reg  = EE_SPI_GPIO->MODER;
    reg &= ~(MODER2_MASK(EE_PIN_SCK) | MODER2_MASK(EE_PIN_MISO) | MODER2_MASK(EE_PIN_MOSI));
    reg |= MODER2(EE_PIN_SCK,  2u)
         | MODER2(EE_PIN_MISO, 2u)
         | MODER2(EE_PIN_MOSI, 2u);
    EE_SPI_GPIO->MODER = reg;

    /* High speed on the two pins we drive, so the edges are square at the
     * EEPROM rather than slewed. */
    reg  = EE_SPI_GPIO->OSPEEDR;
    reg |= MODER2(EE_PIN_SCK, 3u) | MODER2(EE_PIN_MOSI, 3u);
    EE_SPI_GPIO->OSPEEDR = reg;

    /* The EEPROM releases its output pin whenever CS is high, so MISO would
     * otherwise float and the input buffer would sit at an undefined level. */
    reg  = EE_SPI_GPIO->PUPDR;
    reg &= ~MODER2_MASK(EE_PIN_MISO);
    reg |= MODER2(EE_PIN_MISO, 1u);        /* pull-up */
    EE_SPI_GPIO->PUPDR = reg;

    /* CR2 before the peripheral is enabled. DS = 0111 for 8-bit frames, and
     * FRXTH = 1 so RXNE fires on a quarter-full FIFO, i.e. after 8 bits. The
     * reset value of FRXTH is 0, which waits for 16 bits and would hang a
     * single-byte transfer (RM0091 section 27.7.2). */
    EE_SPI->CR2 = SPI_CR2_FRXTH
                | SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2;

    /* CR1: master, BR from board_config, CPOL = CPHA = 0 for the EEPROM's
     * mode (0,0), MSB first.
     * SSM with SSI held high is what keeps the peripheral in master mode: we
     * drive CS on a GPIO, so nothing drives the NSS input, and a master that
     * sees NSS low clears MSTR and stops producing a clock entirely
     * (RM0091 section 27.7.1). */
    EE_SPI->CR1 = SPI_CR1_MSTR
                | ((uint32_t)EE_SPI_BR << SPI_CR1_BR_Pos)
                | SPI_CR1_SSM | SPI_CR1_SSI;

    /* Task 5 fault case. Leave this call exactly here: after your CR1 and CR2
     * configuration, before the peripheral is enabled. It does nothing unless
     * RUN_TASK is 5. */
    task5_fault_hook();

    EE_SPI->CR1 |= SPI_CR1_SPE;

    dbg_gpiob_moder      = EE_SPI_GPIO->MODER;
    dbg_gpiob_afrh       = EE_SPI_GPIO->AFR[1];
    dbg_spi_cr1          = EE_SPI->CR1;
    dbg_spi_cr2          = EE_SPI->CR2;
    dbg_spi_sr           = EE_SPI->SR;
    dbg_sck_hz_predicted = EE_SCK_HZ_PREDICTED;
}

/* ==========================================================================
 * RUN_TASK 2 - given
 * Configures SPI and then does nothing else, so you can inspect the selected
 * SPI peripheral and GPIOB in the SFR view (or the dbg_* variables) while the
 * program runs. PC13 keeps toggling so you can see the program has not hung.
 * ========================================================================== */

void task2_setup(void)
{
    task1_gpio_init();
    eeprom_spi_init();
}

void task2_loop(uint32_t now)
{
    task1_gpio_update(now);
}
