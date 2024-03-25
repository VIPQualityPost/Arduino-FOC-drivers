#pragma once

#include "Arduino.h"

#if defined(STM32G431xx) || defined(STM32G474xx) || defined(STM32G494xx)
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_tim.h"
#include "stm32g4xx_hal_tim_ex.h"
#include "stm32g4xx_ll_tim.h"


#include "common/foc_utils.h"
#include "utilities/stm32cascadetimer/STM32CascadeTimer.h"

class STM32G4HWStepDirInterface {
    public:
        STM32G4HWStepDirInterface(uint32_t pin_dir, uint32_t pin_step, float step_angle);

        int32_t init();
        bool initialized;

        /**
         * There are two options: counting on each edge, or counting on one polarity.
         */
        int32_t setHighResolution(bool res);

        /**
         * If using 1x resolution (normal step-dir) then set positive or negative edge counting.
         */
        int32_t setFallingEdge(int edge);

        /**
         * Output the direction bit from the timer.
         */
        int32_t getDirection();

        /**
         * get the velocity from the cascade timer
        */
        float getVelocity();

        /**
         * Read the counter and return desired angle.
         */
        float getValue();
        uint32_t getCount();

        float _step_angle;
        
        TIM_HandleTypeDef stepdir_handle;
        PinName _pin_step, _pin_dir;

        STM32CascadeTimer linkedTimer;

};

#endif