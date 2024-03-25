#pragma once

#include "Arduino.h"

#if defined(_STM32_DEF_)

class STM32CascadeTimer {
    public:
        STM32CascadeTimer(TIM_HandleTypeDef parentTimer, TIM_TypeDef *cascadeTimer, DMA_Channel_TypeDef *dmaChannel);
        STM32CascadeTimer();
        ~STM32CascadeTimer();

        int link(TIM_HandleTypeDef parentTimer, TIM_TypeDef *cascadeTimer, DMA_Channel_TypeDef *dmaChannel);

        int init();
        int initTimer();
        int initDMA();
        bool initialized;

        /**
         * The DMA takes care of pulling the counter value out of the timer,
         * but this still needs to be turned into real units (rad/s). Requires checking
         * what the timer clocking is. 
        */
        uint32_t velocityCounts;
        float getVelocityValue();

        // The timer that is doing velocity calculations.
        TIM_HandleTypeDef parent_timer;
        TIM_TypeDef *cascade_timer;

    protected:
        /**
         * It would be nice if we don't have to provide a timer and instead the firmware
         * can determine a free timer to use, as long as it meets the criteria.
        */
        TIM_TypeDef findFreeTimer();

        /**
         * It would be also nice if we don't need to specify a DMA channel and instead
         * the firmware can just pick a channel. 
        */
        DMA_Channel_TypeDef findFreeDMAChannel();

        DMA_HandleTypeDef dma_handle;
        TIM_HandleTypeDef cascade_handle;
};

#endif