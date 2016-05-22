/*
   V-REP simulator plugin code for Hackflight

   Copyright (C) Simon D. Levy 2016

   This file is part of Hackflight.

   Hackflight is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   Hackflight is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with Hackflight.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>
#include <unistd.h>

#include "v_repExtHackflight.hpp"
#include "scriptFunctionData.h"
#include "v_repLib.h"

// From firmware
extern void setup(void);
extern void loop(void);

#define JOY_DEV "/dev/input/js0"

#define CONCAT(x,y,z) x y z
#define strConCat(x,y,z)	CONCAT(x,y,z)

#define PLUGIN_NAME "Hackflight"

static LIBRARY vrepLib;

struct sQuadcopter
{
    int handle;
    int prop1handle;
    int prop2handle;
    int prop3handle;
    int prop4handle;
    char * waitUntilZero;
};

static sQuadcopter quadcopter;

// simExtHackflight_create -------------------------------------------------------------------------

#define LUA_CREATE_COMMAND "simExtHackflight_create"

// Five handles: quadcopter + four propellers
static const int inArgs_CREATE[]={
    5,
    sim_script_arg_int32,0,
    sim_script_arg_int32,0,
    sim_script_arg_int32,0,
    sim_script_arg_int32,0,
    sim_script_arg_int32,0,
};

void LUA_CREATE_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(cb->stackID,inArgs_CREATE,inArgs_CREATE[0],LUA_CREATE_COMMAND)) {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        quadcopter.handle      = inData->at(0).int32Data[0];
        quadcopter.prop1handle = inData->at(1).int32Data[0];
        quadcopter.prop2handle = inData->at(2).int32Data[0];
        quadcopter.prop3handle = inData->at(3).int32Data[0];
        quadcopter.prop4handle = inData->at(4).int32Data[0];
        quadcopter.waitUntilZero=NULL;
    }
    D.pushOutData(CScriptFunctionDataItem(true)); // success
    D.writeDataToStack(cb->stackID);
}

// simExtHackflight_destroy --------------------------------------------------------------------

#define LUA_DESTROY_COMMAND "simExtHackflight_destroy"

void LUA_DESTROY_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;
    D.pushOutData(CScriptFunctionDataItem(true)); // success
    D.writeDataToStack(cb->stackID);
}


// simExtHackflight_start ------------------------------------------------------------------------

#define LUA_START_COMMAND "simExtHackflight_start"

static const int inArgs_START[]={
    1,
    sim_script_arg_bool,0,
};

void LUA_START_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;

    // -1 because the last argument is optional
    if (D.readDataFromStack(cb->stackID,inArgs_START,inArgs_START[0]-1,LUA_START_COMMAND)) {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        bool leaveDirectly=false;
        leaveDirectly=inData->at(0).boolData[0];
        if (!leaveDirectly)
    }

    cb->waitUntilZero=1; // the effect of this is that when we leave the callback, the Lua script 

    D.pushOutData(CScriptFunctionDataItem(true)); // success
    D.writeDataToStack(cb->stackID);

    setup();
}

// simExtHackflight_stop --------------------------------------------------------------------------------

#define LUA_STOP_COMMAND "simExtHackflight_stop"

void LUA_STOP_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;
    D.pushOutData(CScriptFunctionDataItem(true)); // success
    D.writeDataToStack(cb->stackID);
}


VREP_DLLEXPORT unsigned char v_repStart(void* reservedPointer,int reservedInt)
{ // This is called just once, at the start of V-REP.
    // Dynamically load and bind V-REP functions:
    char curDirAndFile[1024];
    getcwd(curDirAndFile, sizeof(curDirAndFile));

    std::string currentDirAndPath(curDirAndFile);
    std::string temp(currentDirAndPath);

    temp+="/libv_rep.so";

    vrepLib=loadVrepLibrary(temp.c_str());
    if (vrepLib==NULL)
    {
        std::cout << "Error, could not find or correctly load v_rep.dll. Cannot start 'Hackflight' plugin.\n";
        return(0); // Means error, V-REP will unload this plugin
    }
    if (getVrepProcAddresses(vrepLib)==0)
    {
        std::cout << "Error, could not find all required functions in v_rep.dll. Cannot start 'Hackflight' plugin.\n";
        unloadVrepLibrary(vrepLib);
        return(0); // Means error, V-REP will unload this plugin
    }

    // Check the V-REP version:
    int vrepVer;
    simGetIntegerParameter(sim_intparam_program_version,&vrepVer);
    if (vrepVer<30200) // if V-REP version is smaller than 3.02.00
    {
        std::cout << "Sorry, V-REP 3.2.0 or higher is required. Cannot start 'Hackflight' plugin.\n";
        unloadVrepLibrary(vrepLib);
        return(0); // Means error, V-REP will unload this plugin
    }

    // Register 4 new Lua commands:

    simRegisterScriptCallbackFunction(strConCat(LUA_CREATE_COMMAND,"@",PLUGIN_NAME),
            strConCat("number success=",LUA_CREATE_COMMAND,
                "(number prop1, number prop2, number prop3, number prop4)"),
            LUA_CREATE_CALLBACK);

    simRegisterScriptCallbackFunction(strConCat(LUA_DESTROY_COMMAND,"@",PLUGIN_NAME),
            strConCat("boolean success=",LUA_DESTROY_COMMAND,"()"),
            LUA_DESTROY_CALLBACK);

    simRegisterScriptCallbackFunction(strConCat(LUA_START_COMMAND,"@",PLUGIN_NAME),
            strConCat("boolean success=", LUA_START_COMMAND,"()"),
            LUA_START_CALLBACK);

    simRegisterScriptCallbackFunction(strConCat(LUA_STOP_COMMAND,"@",PLUGIN_NAME),
            strConCat("boolean success=",LUA_STOP_COMMAND,"()"),LUA_STOP_CALLBACK);

    return(8); // return the version number of this plugin (can be queried with simGetModuleName)
}

VREP_DLLEXPORT void v_repEnd()
{ // This is called just once, at the end of V-REP
    unloadVrepLibrary(vrepLib); // release the library
}

VREP_DLLEXPORT void* v_repMessage(int message,int* auxiliaryData,void* customData,int* replyData)
{   
    // This is called quite often. Just watch out for messages/events you want to handle
    // This function should not generate any error messages:

    int errorModeSaved;
    simGetIntegerParameter(sim_intparam_error_report_mode,&errorModeSaved);
    simSetIntegerParameter(sim_intparam_error_report_mode,sim_api_errormessage_ignore);

    void* retVal=NULL;

    float force = 1;
    float torque = 0;
    simAddForceAndTorque(quadcopter.prop1handle, &force, &torque);
    simAddForceAndTorque(quadcopter.prop2handle, &force, &torque);
    simAddForceAndTorque(quadcopter.prop3handle, &force, &torque);
    simAddForceAndTorque(quadcopter.prop4handle, &force, &torque);

    if (message==sim_message_eventcallback_modulehandle)
    {
        // is the command also meant for Hackflight?
        if ( (customData==NULL)||(std::string("Hackflight").compare((char*)customData)==0) ) 
        {
        }
    }

    loop();

    simSetIntegerParameter(sim_intparam_error_report_mode,errorModeSaved); // restore previous settings

    return(retVal);
}

// Board implementation --------------------------------------------------------------

#include "../firmware/pwm.hpp"
#include "../firmware/board.hpp"

// V-REP memory model seems to prevent us from making these instance variables of a Board object
static int pwm[8];
static int joyfd;
static struct timespec start_time;

void Board::imuInit(uint16_t & acc1G, float & gyroScale)
{
    // XXX use MPU6050 settings for now
    acc1G = 4096;
    gyroScale = (4.0f / 16.4f) * (M_PI / 180.0f) * 0.000001f;
}

static void rotate(float x, float y, float theta, float & pitch, float & roll)
{
    pitch =  cos(theta) * x + sin(theta) * y;
    roll  = -sin(theta) * x + cos(theta) * y;
}

void Board::imuRead(int16_t accADC[3], int16_t gyroADC[3])
{
    float eulerAngles[3];

    // Get Euler angles of quadcopter.
    // -1 input means = relative to global cooredinates; -1 return = failure
    if (simGetObjectOrientation(quadcopter.handle, -1, eulerAngles) != -1) {

        // Convert Euler angles to pitch and roll
        float pitch, roll;
        rotate(eulerAngles[0], eulerAngles[1], eulerAngles[2], pitch, roll);
    }
}

void Board::init(void)
{
    // Initialize nanosecond timer
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);

    // Close joystick if open
    if (joyfd > 0)
        close(joyfd);

    // Initialize joystick
    joyfd = open( JOY_DEV , O_RDONLY);
    if(joyfd > 0) 
        fcntl(joyfd, F_SETFL, O_NONBLOCK);

    // Set initial fake PWM values
    for (int k=0; k<CONFIG_RC_CHANS; ++k)  {
        pwm[k] = (CONFIG_PWM_MIN + CONFIG_PWM_MAX) / 2;
    }
}

void Board::checkReboot(bool pendReboot)
{

}

void Board::delayMilliseconds(uint32_t msec)
{
    uint32_t startMicros = this->getMicros();

    while ((this->getMicros() - startMicros)/1000 > msec)
        ;
}

uint32_t Board::getMicros()
{
    struct timespec end_time;
    
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);

    return 1000000 * (end_time.tv_sec - start_time.tv_sec) + 
        (end_time.tv_nsec - start_time.tv_nsec) / 1000;
}

void Board::ledGreenOff(void)
{
    //printf("GREEN OFF\n");
}

void Board::ledGreenOn(void)
{
    //printf("GREEN ON\n");
}

void Board::ledGreenToggle(void)
{
    //printf("GREEN TOGGLE\n");
}

void Board::ledRedOff(void)
{
    //printf("RED OFF\n");
}

void Board::ledRedOn(void)
{
    //printf("RED ON\n");
}

void Board::ledRedToggle(void)
{
    //printf("RED TOGGLE\n");
}

uint16_t Board::readPWM(uint8_t chan)
{
    if (joyfd > 0) {

        struct js_event js;
        read(joyfd, &js, sizeof(struct js_event));

        if (js.type & ~JS_EVENT_INIT) {
            int fakechan = 0;
            switch (js.number) {
                case 0:
                    fakechan = 3;
                    break;
                case 1:
                    fakechan = 1;
                    break;
                case 2:
                    fakechan = 2;
                    break;
                case 3:
                    fakechan = 4;
                    break;
                case 5:
                    fakechan = 5;
                    break;
            }

            if (fakechan > 0)
                pwm[fakechan-1] = 
                        CONFIG_PWM_MIN + (int)((js.value + 32767)/65534. * (CONFIG_PWM_MAX-CONFIG_PWM_MIN));
        }
    }

    return pwm[chan];
}

void Board::reboot(void)
{
}

uint8_t Board::serialAvailableBytes(void)
{
    return 0;
}

uint8_t Board::serialReadByte(void)
{
    return 0;
}

void Board::serialWriteByte(uint8_t c)
{
}

void Board::writeMotor(uint8_t index, uint16_t value)
{
}


