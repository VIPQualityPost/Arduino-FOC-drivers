#pramga once

#include "Arduino.h"

#if defined(STM32G431xx) || defined(STM32G474xx) || defined(STM32G494xx)

#include "common/foc_utils.h"
#include "utilities/stm32cascadetimer/STM32CascadeTimer.h"

class STM32G4HWStepDirInterface {
    public:
        STM32G4SpeedDirInput(int pin_dir, int pin_step, float step_angle);

        int init();
        bool initalized;

        /**
         * There are two options: counting on each edge, or counting on one polarity.
         */
        int setHighResolution(bool res);

        /**
         * If using 1x resolution (normal step-dir) then set positive or negative edge counting.
         */
        int setFallingEdge(int edge);

        /**
         * Output the direction bit from the timer.
         */
        int getDirection();

        /**
         * Read the counter and return desired angle.
         */
        float getValue();
        long getCount();

        float _step_angle;
        
        TIM_HandleTypeDef stepdir_handle;
        PinName _pin_step, _pin_dir;
};

#endif