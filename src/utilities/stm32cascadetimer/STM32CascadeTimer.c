#include "./STM32CascadeTimer.h"

#if defined(_STM32_DEF_)

STM32CascadeTimer::STM32CascadeTimer(TIM_TypeDef parentTimer, TIM_TypeDef cascadeTimer, DMA_Channel_TypeDef dmaChannel)
{
    TIM_HandleTypeDef cascadeHandle;
    cascadeHandle.Instance = cascadeTimer;

    DMA_Channel_HandleTypeDef dmaHandle;
    dmaHandle.Instance = dmaChannel;

    parent_timer = parentTimer;
    cascade_handle = cascadeHandle;
    dma_handle = dmaHandle;
}

STM32CascadeTimer::init()
{
    initDMA();
    initTimer();

    initalized = true;

    return 0;
}

STM32CascadeTimer::initDMA()
{

    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    dma_handle.Init.Request = DMA_REQUEST_TIM4_TRIG;
    dma_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
    dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    dma_handle.Init.MemInc = DMA_MINC_ENABLE;
    dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    dma_handle.Init.Mode = DMA_CIRCULAR;
    dma_handle.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&dma_handle) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(&cascade_handle,hdma[TIM_DMA_ID_TRIGGER],&dma_handle);
}

STM32CascadeTimer::initTimer()
{
    /**
     * Need to write something to find free timers. It needs to be an advanced or general purpose timer.
     */

    // Set up the timer.
    cascade_handle = (TIM_TypeDef)TIM4;

    cascade_handle.Init.Period = 0xFFFF; // For now treat all timers as 16 bit even if they are 32bit.
    cascade_handle.Init.Prescaler = 0;
    cascade_handle.Init.ClockDivision = 0;
    cascade_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    cascade_handle.Init.RepetitionCounter = 0;
    cascade_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLED;

    if (HAL_TIM_Base_Init(&cascade_handle) != HAL_OK)
    {
        return 0;
    }

    TIM_ClockConfigTypeDef child_clock;

    child_clock.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

    if (HAL_TIM_ConfigClockSource(&cascade_handle, &child_clock) != HAL_OK)
    {
        return 0;
    }

    // Configure input capture mode.
    TIM_IC_InitTypeDef cascade_config;

    if (HAL_TIM_IC_Init(&cascade_handle) != HAL_OK)
    {
        return 0;
    }

    cascade_config.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    cascade_config.ICSelection = TIM_ICSELECTION_INDIRECTTI;
    cascade_config.ICPrescaler = TIM_ICPSC_DIV1;
    cascade_config.Filter = 0;

    if (HAL_TIM_IC_ConfigChannel(&cascade_handle, &cascade_config, TIM_CHANNEL_1) != HAL_OK)
    {
        initialized = false;
        return 0;
    }

    // Configure the link between timers.
    TIM_SlaveConfigTypeDef child_config;

    child_config.SlaveMode = TIM_SLAVEMODE_RESET; // Restart the counter on trigger.
    child_config.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
    child_config.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
    child_config.TriggerFilter = 0; // up to 0xF

    switch (parent_timer)
    {
    case TIM1:
        child_config.InputTrigger = TIM_TS_ITR0; // OC5REF encoder clk -> TRGO TIM1 on parent
        break;

    case TIM2:
        child_config.InputTrigger = TIM_TS_ITR1;
        break;

    case TIM3:
        child_config.InputTrigger = TIM_TS_ITR2;
        break;

    case TIM4:
        child_config.InputTrigger = TIM_TS_ITR3;
        break;

    case TIM5:
        child_config.InputTrigger = TIM_TS_ITR4;
        break;

    case TIM8:
        child_config.InputTrigger = TIM_TS_ITR5;
        break;

    default:
        break;
    }

    if (HAL_TIM_SlaveConfigSynchro(&cascade_handle, &child_config) != HAL_OK)
    {
        initialized = false;
        return 0;
    }

    if (HAL_TIM_IC_Start_DMA(&cascade_handle, TIM_CHANNEL_1, &velocityCounter, 1) != HAL_OK)
    {
        return 0;
    }
}

STM32CascadeTimer::getVelocityValue()
{
}

STM32CascadeTimer::findFreeTimer()
{
}

STM32CascadeTimer::findFreeDMAChannel()
{
}

#endif