// Copyright 2021 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.
#pragma once

#include <stdint.h>

// Disable formatting to make it easier to compare with other config files.
// clang-format off

#define configUSE_16_BIT_TICKS                  0
#define configUSE_CO_ROUTINES                   0
#define configUSE_IDLE_HOOK                     0
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_MUTEXES                       1
#define configUSE_PREEMPTION                    1
#define configUSE_TICK_HOOK                     0
#define configUSE_TIMERS                        1
#define configUSE_TRACE_FACILITY                1

#define configGENERATE_RUN_TIME_STATS           0

#define configCHECK_FOR_STACK_OVERFLOW          0
#define configCPU_CLOCK_HZ                      (SystemCoreClock)
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configMAX_CO_ROUTINE_PRIORITIES         (2)
#define configMAX_PRIORITIES                    (5)
#define configMAX_TASK_NAME_LEN                 (20)
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t
#define configMINIMAL_STACK_SIZE                ((uint16_t)(10 * 1024))
#define configQUEUE_REGISTRY_SIZE               8
#define configRECORD_STACK_HIGH_ADDRESS         1
#define configTICK_RATE_HZ                      ((TickType_t)200)
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_TASK_STACK_DEPTH            (configMINIMAL_STACK_SIZE * 2)

/* Memory allocation related definitions. */
#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   ((size_t)(1 * 1024))
#define configAPPLICATION_ALLOCATED_HEAP        0

/* __NVIC_PRIO_BITS in CMSIS */
#if defined(__NVIC_PRIO_BITS)
#define configPRIO_BITS                         __NVIC_PRIO_BITS
#else
#define configPRIO_BITS                         (3)
#endif // defined(__NVIC_PRIO_BITS)

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      ((1U << (configPRIO_BITS)) - 1)
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 2
#define configKERNEL_INTERRUPT_PRIORITY \
  (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
  (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define configUSE_TICKLESS_IDLE                 0

#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_ALTERNATIVE_API               0 /* Deprecated! */

#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  0
#define configUSE_NEWLIB_REENTRANT              0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5
#define configFRTOS_MEMORY_SCHEME               4
#define configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H 0
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0
#define configASSERT(x) if((x) == 0) {taskDISABLE_INTERRUPTS(); for (;;);}

#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_uxTaskGetStackHighWaterMark     0

#if !defined(configENABLE_FPU)
  #define configENABLE_FPU                      1
#endif
#if !defined(configENABLE_MPU)
  #define configENABLE_MPU                      0
#endif
#if !defined(configENABLE_TRUSTZONE)
  #define configENABLE_TRUSTZONE                0
#endif
#if !defined(configRUN_FREERTOS_SECURE_ONLY)
  #define configRUN_FREERTOS_SECURE_ONLY        1
#endif

#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   0
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  0
#define INCLUDE_xTaskResumeFromISR              1

#if defined(__ICCARM__)||defined(__CC_ARM)||defined(__GNUC__)
  #include <stdint.h>
  extern uint32_t SystemCoreClock;
#endif

#undef configUSE_MUTEXES
#define configUSE_MUTEXES                       1

#define vPortSVCHandler     SVC_Handler
#define vPortPendSVHandler  PendSV_Handler
#define vPortSysTickHandler SysTick_Handler
