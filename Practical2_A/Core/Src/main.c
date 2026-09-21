/**
  ******************************************************************************
  * EEE3096S 2026 - Practical 2A
  * Memory-Mapped I/O and Hardware SPI - building an EEPROM driver
  *
  * Student 1 : <name>  <student number>
  * Student 2 : <name>  <student number>
  * Date      : <date>
  *
  * Board : UCT Development Board, STM32F051C8
  *
  * HOW THIS PROJECT WORKS
  *   Each task has its own file (see prac2a.h). Set RUN_TASK below to the task
  *   you are working on, build, and debug. ONLY that task's program runs, so a
  *   task you have not finished yet cannot stop the others working.
  *
  *   RUN_TASK 1   PC13 square wave using memory-mapped GPIO
  *   RUN_TASK 2   SPI configured, CS held high - inspect the registers
  *   RUN_TASK 3   your group byte sent every 100 ms, CS held high
  *   RUN_TASK 4   EEPROM driver: PA0 = write/read/verify, reset = read only
  *   RUN_TASK 5   the Task 4 program with the fault case applied
  *   RUN_TASK 6   your non-blocking state machine: PA0 start, PA3 abort
  *
  * Board facts and the looked-up register values live in board_config.h.
  ******************************************************************************
  */

#include "prac2a.h"

#define RUN_TASK    1

volatile uint8_t run_task = RUN_TASK;

/* Hand-worked values on the left, so the build breaks if the formulas in
 * board_config.h and our arithmetic ever disagree. */
_Static_assert(TEST_BYTE_B   == 0x3Du, "group test byte B mismatch");
_Static_assert(EEPROM_ADDR_A == 0x10u, "group address A mismatch");

_Static_assert(EE_SCK_HZ_PREDICTED == 250000UL,
               "SPI divider does not give 250 kHz from this PCLK1");

_Static_assert(EEPROM_ADDR_A < EEPROM_SIZE_BYTES, "address A is outside the EEPROM");
_Static_assert((RUN_TASK >= 1) && (RUN_TASK <= 6), "RUN_TASK must be 1 to 6");

void SystemClock_Config(void);

int main(void)
{
    /* HAL_Init() starts SysTick as a 1 ms time source (HAL_GetTick). It does
     * not touch GPIO or SPI. */
    HAL_Init();
    SystemClock_Config();

    switch (RUN_TASK)
    {
    case 1:  task1_setup(); break;
    case 2:  task2_setup(); break;
    case 3:  task3_setup(); break;
    case 4:
    case 5:  task4_setup(); break;
    default: task6_setup(); break;
    }

    while (1)
    {
        uint32_t now = HAL_GetTick();

        switch (RUN_TASK)
        {
        case 1:  task1_loop(now); break;
        case 2:  task2_loop(now); break;
        case 3:  task3_loop(now); break;
        case 4:
        case 5:  task4_loop(now); break;
        default: task6_loop(now); break;
        }
    }
}

/**
  * @brief System clock: HSI, no PLL, AHB /1, APB1 /1.
  *        This is where PCLK1_HZ in board_config.h comes from.
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif
