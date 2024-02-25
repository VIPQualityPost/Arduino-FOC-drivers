#include "./STM32HWStepDirInterface.h"

#if defined(STM32G431xx) || defined(STM32G474xx) || defined(STM32G494xx)

STM32HWStepDirInterface::STM32HWStepDirInterface(int pin_dir, int pin_step, float step_angle)
{
    _pin_dir = digitalPinToPinName(pin_dir);
    _pin_step = digitalPinToPinName(pin_step);
    _step_angle = step_angle;
}

int STM32HWStepDirInterface::init()
{
    initialized = false;

    pinMode(_pin_dir, INPUT);
    pinMode(_pin_step, INPUT);

    TIM_TypeDef *InstanceA = (TIM_TypeDef *)pinmap_peripheral(_pin_dir, PinMap_TIM);
    TIM_TypeDef *InstanceB = (TIM_TypeDef *)pinmap_peripheral(_pin_step, PinMap_TIM);

    if (!IS_TIM_ENCODER_INTERFACE_INSTANCE(InstanceA) || !IS_TIM_ENCODER_INTERFACE(InstanceB))
    {
        return 0;
    }

    pinmap_pinout(_pin_dir, PinMap_TIM);
    pinmap_pinout(_pin_step, PinMap_TIM);

    // Timer configuration
    stepdir_handle.Instance = InstanceA;

    stepdir_handle.Init.Period = 0xFFFF; // need to check width of timer?
    stepdir_handle.Init.Prescaler = 0;
    stepdir_handle.Init.ClockDivision = 0;
    stepdir_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    stepdir_handle.Init.RepetitionCounter = 0;
    stepdir_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLED;

    enableTimerClock(&stepdir_handle);

    // Encoder configuration
    TIM_Encoder_InitTypeDef stepdir_config;

    stepdir_config.EncoderMode = TIM_ENCODERMODE_CLOCKPLUSDIRECTION_X1;

    stepdir_config.IC1Polarity = TIM_ICPOLARITY_RISING;
    stepdir_config.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    stepdir_config.IC1Prescaler = TIM_ICPSC_DIV1;
    stepdir_config.IC1Filter = 0;

    stepdir_config.IC2Polarity = TIM_ICPOLARITY_RISING;
    stepdir_config.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    stepdir_config.IC2Prescaler = TIM_ICPSC_DIV1;
    stepdir_config.IC2Filter = 0;

    if (HAL_TIM_Encoder_Init(&stepdir_handle, &stepdir_config) != HAL_OK)
    {
        return 0;
    }

    // Setup the output trigger in case we are going to use a cascaded timer.
    TIM_MasterConfigTypeDef parent_config;

    parent_config.MasterOutputTrigger = TIM_TRGO_ENCODER_CLK;
    parent_config.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    parent_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

    if(HAL_TIM_MasterConfigSynchronization(&stepdir_handle, &parent_config) != HAL_OK){
        initalized = false;
        return 0;
    }

    // Set edge polarity to rising (CC2P = 0).
    stepdir_handle.Instance->CCER &= ~TIM_CCER_CC2P;

    // Setup trgo (oc5ref) for edge period timing for velocity measurements.
    stepdir_handle.Instance->CR2 &= ~TIM_CR2_MMS;
    stepdir_handle.Instance->CR2 |= TIM_TRGO_ENCODER_CLK;

    linkedTimerInit();

    if (HAL_TIM_Encoder_Start(&stepdir_handle, TIM_CHANNEL_ALL) != HAL_OK)
    {
        return 0;
    }

    initialized = true;
}

int STM32HWStepDirInterface::linkedTimerInit()
{
    /**
     * Need to write something to find free timers. It needs to be an advanced or general purpose timer.
     */ 

    // Set up the timer.
    velocity_handle = (TIM_TypeDef)TIM4;

    velocity_handle.Init.Period = 0xFFFF;
    velocity_handle.Init.Prescaler = 0;
    velocity_handle.Init.ClockDivision = 0;
    velocity_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    velocity_handle.Init.RepetitionCounter = 0;
    velocity_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLED;

    if(HAL_TIM_Base_Init(&velocity_handle) != HAL_OK){
        return 0;
    }

    TIM_ClockConfigTypeDef child_clock;

    child_clock.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

    if(HAL_TIM_ConfigClockSource(&velocity_handle, &child_clock) != HAL_OK){
        return 0;
    }

    
    // Configure input capture mode.
    TIM_IC_InitTypeDef velocity_config;

    if(HAL_TIM_IC_Init(&velocity_handle) != HAL_OK){
        return 0;
    }

    velocity_config.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    velocity_config.ICSelection = TIM_ICSELECTION_INDIRECTTI;
    velocity_config.ICPrescaler = TIM_ICPSC_DIV1;
    velocity_config.Filter = 0;

    if(HAL_TIM_IC_ConfigChannel(&velocity_handle, &velocity_config, TIM_CHANNEL_1) != HAL_OK){
        initialized = false;
        return 0;
    }

    // Do we actually need to use both channels? 
    // We don't want to measure pulse length but instead just between common edge of single pulses.
    // velocity_config.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    // velocity_config.ICSelection = TIM_ICSELECTION_DIRECTTI;

    // if(HAL_TIM_IC_ConfigChannel(&velocity_handle, &velocity_config, TIM_CHANNEL_2) != HAL_OK){
    //     initialized = false;
    //     return 0;
    // }

    // Configure the link between timers. 
    TIM_SlaveConfigTypeDef child_config;

    child_config.SlaveMode = TIM_SLAVEMODE_RESET;
    child_config.InputTrigger = TIM_TS_ITR0; // OC5REF encoder clk -> TRGO TIM1 on parent
    child_config.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
    child_config.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
    child_config.TriggerFilter = 0; // up to 0xF

    if(HAL_TIM_SlaveConfigSynchro(&velocity_handle, &child_config) != HAL_OK){
        initialized = false;
        return 0;
    }

    if(HAL_TIM_IC_Start_DMA(&velocity_handle, TIM_CHANNEL_1, &velocityCounter, 1) != HAL_OK){
        return 0;
    }
}

int STM32HWStepDirInterface::setHighResolution(bool res)
{
    /**
     * This sets whether the input capture for the step edge is done on just one edge or on both edges of the step.
     * Standard step-dir output seems to be X1 resolution so this is default although both are availble.
     */
    stepdir_handle.Instance->SCMR &= ~TIM_SCMR_SMS;
    if (res)
    {
        stepdir_handle.Instance->SCMR |= TIM_ENCODERMODE_CLOCKPLUSDIRECTION_X2;
    }
    else
    {
        stepdir_handle.Instance->SCMR |= TIM_ENCODERMODE_CLOCKPLUSDIRECTION_X1;
    }
}

int STM32HWStepDirInterface::setFallingEdge(bool edge)
{
    /**
     * When configured in input capture mode this bit controls the polarity of the capture.
     * When in high resolution mode (X2) this is not used as both edges of step are clocked.
     */
    if (edge)
    {
        stepdir_handle.Instance->CCER |= TIM_CCER_CC2P;
    }
    else
    {
        stepdir_handle.Instance->CCER &= ~TIM_CCER_CC2P;
    }

    return 1;
}

int STM32HWStepDirInterface::getDirection(void){
    // 0 is upcounter, 1 is downcounter
    return ((encoder_handle.Instance->CR1 & TIM_CR1_DIR) == TIM_CR1_DIR)
}

long STM32HWStepDirInterface::getCount(void)
{
    return (long)encoder_angle.Instance->CNT;
}

float STM32HWStepDirInterface::getValue(void)
{
    return (float)(getCount() * _step_angle);
}

float STM32HWStepDirInterface::getVelocityValue(void)
{
    // TODO
    return 0.0f;
}

float STM32HWStepDirInterface::getAcceleration()
{
    // TODO
    return 0.0f;
}

#endif
