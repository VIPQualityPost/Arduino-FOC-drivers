#pramga once

#include "Arduino.h"

#if defined(STM32G431xx) || defined(STM32G474xx) || defined(STM32G494xx)

#include "common/foc_utils.h"
#include "communication/StepDirListener.h"

class STM32HWStepDirInterface
{
public:
    STM32SpeedDirInput(int pin_dir, int pin_step, float step_angle);

    int init();
    bool initalized = 0;

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

    float getVelocityValue();

    /**
     * Integrate the velocity to get acceleration.
     */
    float getAcceleration();

    float _step_angle;
    uint16_t velocityCounter;

protected:
    /**
     * Configure slave timer (TIM6) that is tracking step pulse frequency for velocity measurements.
     */
    int linkedTimerInit();

    TIM_HandleTypeDef stepdir_handle, velocity_handle;
    PinName _pin_step, _pin_dir;
};

#endif