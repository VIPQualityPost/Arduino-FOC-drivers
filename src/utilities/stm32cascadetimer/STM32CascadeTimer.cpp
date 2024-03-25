#include "./STM32CascadeTimer.h"

#if defined(_STM32_DEF_)

STM32CascadeTimer::STM32CascadeTimer(){};
STM32CascadeTimer::~STM32CascadeTimer(){};

STM32CascadeTimer::STM32CascadeTimer(TIM_HandleTypeDef parentTimer, TIM_TypeDef *cascadeTimer, DMA_Channel_TypeDef *dmaChannel)
{
    link(parentTimer, cascadeTimer, dmaChannel);
}

int STM32CascadeTimer::link(TIM_HandleTypeDef parentTimer, TIM_TypeDef *cascadeTimer, DMA_Channel_TypeDef *dmaChannel)
{

    cascade_handle.Instance = cascadeTimer;

    dma_handle.Instance = dmaChannel;

    parent_timer = parentTimer;
    cascade_timer = cascadeTimer;
    return 0;
}

int STM32CascadeTimer::init()
{
    int ret = initDMA();
    ret += initTimer();

    initialized = true;

    return ret;
}

int STM32CascadeTimer::initDMA()
{

    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    // TODO use chosen DMA Request, not TIM4
    dma_handle.Init.Request = DMA_REQUEST_TIM4_UP;
    dma_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
    dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    dma_handle.Init.MemInc = DMA_MINC_ENABLE;
    dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    dma_handle.Init.Mode = DMA_CIRCULAR;
    dma_handle.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&dma_handle) != HAL_OK)
    {
        return -10;
    }

    __HAL_LINKDMA(&cascade_handle, hdma[TIM_DMA_ID_CC1], dma_handle);

    // if (HAL_DMA_Start(&dma_handle, (uint32_t) & (cascade_handle.Instance->CCR1), (uint32_t)&velocityCounts, 3) != HAL_OK) {
    //     return -10;
    // }
    return 0;
}

int STM32CascadeTimer::initTimer()
{
    /**
     * Need to write something to find free timers. It needs to be an advanced or general purpose timer.
     */

    // Set up the timer.
    cascade_handle.Instance = TIM4;

    cascade_handle.Init.Period = 0xFFFF; // For now treat all timers as 16 bit even if they are 32bit.
    cascade_handle.Init.Prescaler = 0;
    cascade_handle.Init.ClockDivision = 0;
    cascade_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    cascade_handle.Init.RepetitionCounter = 0;
    cascade_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&cascade_handle) != HAL_OK)
    {
        return -1;
    }

    TIM_ClockConfigTypeDef child_clock;

    child_clock.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

    if (HAL_TIM_ConfigClockSource(&cascade_handle, &child_clock) != HAL_OK)
    {
        return -2;
    }

    // Configure input capture mode.
    TIM_IC_InitTypeDef cascade_config;

    if (HAL_TIM_IC_Init(&cascade_handle) != HAL_OK)
    {
        return -3;
    }

    cascade_config.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    cascade_config.ICSelection = TIM_ICSELECTION_TRC;
    cascade_config.ICPrescaler = TIM_ICPSC_DIV1;
    cascade_config.ICFilter = 0;

    if (HAL_TIM_IC_ConfigChannel(&cascade_handle, &cascade_config, TIM_CHANNEL_1) != HAL_OK)
    {
        initialized = false;
        return -4;
    }

    // Configure the link between timers.
    TIM_SlaveConfigTypeDef child_config;

    child_config.SlaveMode = TIM_SLAVEMODE_COMBINED_RESETTRIGGER;
    child_config.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
    child_config.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
    child_config.TriggerFilter = 0; // up to 0xF

    // int _getInternalSourceTrigger(HardwareTimer* master, HardwareTimer* slave) { // put master and slave in temp variables to avoid arrows
    TIM_TypeDef *parentTimer = parent_timer.Instance;
#if defined(TIM1) && defined(LL_TIM_TS_ITR0)
    if (parentTimer == TIM1)
        child_config.InputTrigger = LL_TIM_TS_ITR0;
#endif
#if defined(TIM2) && defined(LL_TIM_TS_ITR1)
    else if (parentTimer == TIM2)
        child_config.InputTrigger = LL_TIM_TS_ITR1;
#endif
#if defined(TIM3) && defined(LL_TIM_TS_ITR2)
    else if (parentTimer == TIM3)
        child_config.InputTrigger = LL_TIM_TS_ITR2;
#endif
#if defined(TIM4) && defined(LL_TIM_TS_ITR3)
    else if (parentTimer == TIM4)
        child_config.InputTrigger = LL_TIM_TS_ITR3;
#endif
#if defined(TIM5) && defined(LL_TIM_TS_ITR4)
    else if (parentTimer == TIM5)
        child_config.InputTrigger = LL_TIM_TS_ITR4;
#endif
#if defined(TIM8) && defined(LL_TIM_TS_ITR5)
    else if (parentTimer == TIM8)
        child_config.InputTrigger = LL_TIM_TS_ITR5;
#endif

    if (HAL_TIM_SlaveConfigSynchro(&cascade_handle, &child_config) != HAL_OK)
    {
        return -5;
    }

    int ret = HAL_TIM_IC_Start_DMA(&cascade_handle, TIM_CHANNEL_1, velocityCounts, 2);
    if ( ret != HAL_OK)
    {
        Serial.printf("error in cascade timer start: %d\n", ret);
        return -6;
    }
    return 0;
}

float STM32CascadeTimer::getVelocityValue()
{
    return velocityCounts[0];
}

TIM_TypeDef STM32CascadeTimer::findFreeTimer()
{
    return TIM_TypeDef{0};
}

DMA_Channel_TypeDef STM32CascadeTimer::findFreeDMAChannel()
{
    return DMA_Channel_TypeDef{0};
}

#endif