/*
 * This file is part of baseflight
 * Licensed under GPL V3 or modified DCL - see https://github.com/multiwii/baseflight/blob/master/README.md
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "stm32f10x_conf.h"

#include "board/drv_serial.h"
#include "board/drv_gpio.h"
#include "board/drv_pwm.h"

#include "mixer.h"
#include "axes.h"
#include "mw.h"
#include "config.h"
#include "utils.h"

// Custom mixer data per motor
typedef struct motorMixer_t {
    float throttle;
    float roll;
    float pitch;
    float yaw;
} motorMixer_t;

static motorMixer_t currentMixer[4];

static const motorMixer_t mixerQuadX[] = {
    { 1.0f, -1.0f,  1.0f, -1.0f },          // REAR_R
    { 1.0f, -1.0f, -1.0f,  1.0f },          // FRONT_R
    { 1.0f,  1.0f,  1.0f,  1.0f },          // REAR_L
    { 1.0f,  1.0f, -1.0f, -1.0f },          // FRONT_L
};

// =========================================================================

void mixerInit(int16_t motor_disarmed[4])
{
    int i;

    for (i = 0; i < 4; i++) {
        currentMixer[i] = mixerQuadX[i];
        motor_disarmed[i] = CONFIG_MINCOMMAND;
    }
}

void writeMotors(int16_t motor[4])
{
    uint8_t i;

    for (i = 0; i < 4; i++)
        pwmWriteMotor(i, motor[i]);
}


void mixTable(int16_t * rcCommand, bool armed, int16_t motor[4], int16_t motor_disarmed[4])
{
    int16_t maxMotor;
    uint32_t i;

    // prevent "yaw jump" during yaw correction
    axisPID[YAW] = constrain(axisPID[YAW], -100 - abs(rcCommand[YAW]), +100 + abs(rcCommand[YAW]));

    for (i = 0; i < 4; i++)
        motor[i] = rcCommand[THROTTLE] * currentMixer[i].throttle + axisPID[PITCH] * currentMixer[i].pitch + 
            axisPID[ROLL] * currentMixer[i].roll + -CONFIG_YAW_DIRECTION * axisPID[YAW] * currentMixer[i].yaw;

    maxMotor = motor[0];
    for (i = 1; i < 4; i++)
        if (motor[i] > maxMotor)
            maxMotor = motor[i];
    for (i = 0; i < 4; i++) {
        if (maxMotor > CONFIG_MAXTHROTTLE)     
            // this is a way to still have good gyro corrections if at least one motor reaches its max.
            motor[i] -= maxMotor - CONFIG_MAXTHROTTLE;

        motor[i] = constrain(motor[i], CONFIG_MINTHROTTLE, CONFIG_MAXTHROTTLE);
        if ((rcData[THROTTLE]) < CONFIG_MINCHECK) {
            motor[i] = CONFIG_MINTHROTTLE;
        } 
        if (!armed) {
            motor[i] = motor_disarmed[i];
        }
    }
}
