/**
  ******************************************************************************
  * @file    task1_gpio.c
  * @brief   TASK 1 : MEMORY-MAPPED GPIO ACCESS
  *
  * Drive PC13 as a digital output using EXPLICIT volatile register pointers,
  * built from addresses you find in RM0091:
  *
  *     register address = peripheral base address + register offset
  *
  * Do not use HAL_GPIO_Init(), HAL_GPIO_WritePin() or HAL_GPIO_TogglePin().
  * You must be able to show where every base and offset came from.
  ******************************************************************************
  */

#include "prac2a.h"

/* RM0091 Table 1, peripheral register boundary addresses (section 2.2.2). */
#define RCC_BASE_ADDR       0x40021000UL
#define GPIOC_BASE_ADDR     0x48000800UL

/* RM0091 section 7.4.6 (RCC_AHBENR) and sections 9.4.1, 9.4.6, 9.4.7, 9.4.11
 * for the GPIO registers. */
#define RCC_AHBENR_OFFSET   0x14UL
#define GPIO_MODER_OFFSET   0x00UL
#define GPIO_ODR_OFFSET     0x14UL
#define GPIO_BSRR_OFFSET    0x18UL
#define GPIO_BRR_OFFSET     0x28UL

/* RM0091 section 7.4.6: RCC_AHBENR bit 19 is the port C clock. */
#define RCC_AHBENR_IOPCEN   (1UL << 19)

#define PC13                13UL
#define PC13_MASK           (1UL << PC13)

/* MODER holds two bits per pin; 01 is general purpose output (RM0091 9.4.1). */
#define MODER_OUTPUT        1UL

/* One pointer is declared for you to show the pattern. Be ready to explain
 * what `volatile` does here, and what can go wrong without it.
 *
 * TODO 1.3  Declare pointers for the GPIOC registers you use, the same way. */
static volatile uint32_t * const pRCC_AHBENR =
        (volatile uint32_t *)(RCC_BASE_ADDR + RCC_AHBENR_OFFSET);

static volatile uint32_t * const pGPIOC_MODER =
        (volatile uint32_t *)(GPIOC_BASE_ADDR + GPIO_MODER_OFFSET);
static volatile uint32_t * const pGPIOC_ODR =
        (volatile uint32_t *)(GPIOC_BASE_ADDR + GPIO_ODR_OFFSET);
static volatile uint32_t * const pGPIOC_BSRR =
        (volatile uint32_t *)(GPIOC_BASE_ADDR + GPIO_BSRR_OFFSET);
static volatile uint32_t * const pGPIOC_BRR =
        (volatile uint32_t *)(GPIOC_BASE_ADDR + GPIO_BRR_OFFSET);

/* TODO 1.4  Once TODO 1.1 and 1.2 are done, UNCOMMENT these checks. They
 *           compare your addresses with the CMSIS device header, so a wrong
 *           base or offset fails the build and names the culprit. Fix your
 *           address - do not edit the right-hand side.
 *           (They are commented out only so the project builds before you
 *           start, letting you check your toolchain first.) */

_Static_assert(RCC_BASE_ADDR   == RCC_BASE,   "RCC base mismatch");
_Static_assert(GPIOC_BASE_ADDR == GPIOC_BASE, "GPIOC base mismatch");
_Static_assert(RCC_BASE_ADDR + RCC_AHBENR_OFFSET
               == (uint32_t)(uintptr_t)&RCC->AHBENR,  "AHBENR offset");
_Static_assert(GPIOC_BASE_ADDR + GPIO_MODER_OFFSET
               == (uint32_t)(uintptr_t)&GPIOC->MODER, "MODER offset");
_Static_assert(GPIOC_BASE_ADDR + GPIO_ODR_OFFSET
               == (uint32_t)(uintptr_t)&GPIOC->ODR,   "ODR offset");
_Static_assert(GPIOC_BASE_ADDR + GPIO_BSRR_OFFSET
               == (uint32_t)(uintptr_t)&GPIOC->BSRR,  "BSRR offset");
_Static_assert(GPIOC_BASE_ADDR + GPIO_BRR_OFFSET
               == (uint32_t)(uintptr_t)&GPIOC->BRR,   "BRR offset");

volatile uint32_t task1_half_period_ms = 5u;
volatile uint32_t task1_toggle_count   = 0u;

static uint32_t t1_last = 0u;

void task1_gpio_init(void)
{
    uint32_t moder;

    /* Read-modify-write: other clocks are already enabled by the time Tasks 4
     * and 6 call this, and a plain store would switch them off. */
    *pRCC_AHBENR |= RCC_AHBENR_IOPCEN;
    (void)*pRCC_AHBENR;             /* clock needs a cycle before the writes */

    moder = *pGPIOC_MODER;
    moder &= ~MODER2_MASK(PC13);
    moder |= MODER2(PC13, MODER_OUTPUT);
    *pGPIOC_MODER = moder;

    *pGPIOC_BRR = PC13_MASK;        /* start low, so the first edge is a rise */
}

void task1_gpio_update(uint32_t now)
{
    if ((uint32_t)(now - t1_last) < task1_half_period_ms)
    {
        return;
    }
    t1_last = now;

    /* BSRR and BRR touch only PC13. A read-modify-write of ODR would leave a
     * window where an interrupt could write ODR between the read and the
     * store, and lose that write. */
    if ((*pGPIOC_ODR & PC13_MASK) != 0u)
    {
        *pGPIOC_BRR = PC13_MASK;
    }
    else
    {
        *pGPIOC_BSRR = PC13_MASK;
    }

    task1_toggle_count++;
}

/* ==========================================================================
 * RUN_TASK 1 - given
 * ========================================================================== */

void task1_setup(void)
{
    task1_gpio_init();
}

void task1_loop(uint32_t now)
{
    task1_gpio_update(now);
}
