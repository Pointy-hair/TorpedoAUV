///
/// @mainpage	SMC
///
/// @details	Description of the project
/// @n
/// @n
/// @n @a		Developed with [embedXcode+](http://embedXcode.weebly.com)
///
/// @author		Alex Bondarenko
/// @author		Alex Bondarenko
/// @date		2018-02-28 5:47 PM
/// @version	<#version#>
///
/// @copyright	(c) Alex Bondarenko, 2018
/// @copyright	GNU General Public Licence
///
/// @see		ReadMe.txt for references
///


///
/// @file		SMC.ino
/// @brief		Main sketch
///
/// @details	<#details#>
/// @n @a		Developed with [embedXcode+](http://embedXcode.weebly.com)
///
/// @author		Alex Bondarenko
/// @author		Alex Bondarenko
/// @date		2018-02-28 5:47 PM
/// @version	<#version#>
///
/// @copyright	(c) Alex Bondarenko, 2018
/// @copyright	GNU General Public Licence
///
/// @see		ReadMe.txt for references
/// @n
///


// Core library for code-sense - IDE-based
// !!! Help: http://bit.ly/2AdU7cu
#if defined(WIRING) // Wiring specific
#include "Wiring.h"
#elif defined(MAPLE_IDE) // Maple specific
#include "WProgram.h"
#elif defined(ROBOTIS) // Robotis specific
#include "libpandora_types.h"
#include "pandora.h"
#elif defined(MPIDE) // chipKIT specific
#include "WProgram.h"
#elif defined(DIGISPARK) // Digispark specific
#include "Arduino.h"
#elif defined(ENERGIA) // LaunchPad specific
#include "Energia.h"
#elif defined(LITTLEROBOTFRIENDS) // LittleRobotFriends specific
#include "LRF.h"
#elif defined(MICRODUINO) // Microduino specific
#include "Arduino.h"
#elif defined(TEENSYDUINO) // Teensy specific
#include "Arduino.h"
#elif defined(REDBEARLAB) // RedBearLab specific
#include "Arduino.h"
#elif defined(RFDUINO) // RFduino specific
#include "Arduino.h"
#elif defined(SPARK) || defined(PARTICLE) // Particle / Spark specific
#include "application.h"
#elif defined(ESP8266) // ESP8266 specific
#include "Arduino.h"
#elif defined(ARDUINO) // Arduino 1.0 and 1.5 specific
#include "Arduino.h"
#else // error
#error Platform not defined
#endif // end IDE

// Set parameters


// Include application, user and local libraries
#include "ESC.h"
#include "SMC_SERIAL.h"
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BNO055.h"
#include <utility/imumaths.h>
#include "BQ34110.h"
#include "debounce.h"
#include "MS5837.h"



extern ESC_Struct ESC[];

// Define structures and classes

#define BAUD_RATE 115200

#define MIN_SPEED 500

#define TIMEOUT (1000)

//#define MANUAL_CONTROL

#define BNO055_SAMPLERATE_DELAY_MS (100)

//#define MATLAB_MODE

// Battery
#define BATTERY_VOLTAGE_MIN (9200)
#define BATTERY_VOLTAGE_MAX (12700)


int16_t depthOffset = 0;


Adafruit_BNO055 IMU = Adafruit_BNO055();
BQ34110 gasGauge = BQ34110();
MS5837 depthSensor = MS5837();


// Define variables and constants
int currChar = 0;



// Prototypes
// !!! Help: http://bit.ly/2l0ZhTa

#define PIN_REED_SW_FRONT   9
#define PIN_REED_SW_CENTER  10
#define PIN_REED_SW_REAR    5





// Utilities
debounce swFront;
debounce swCenter;
debounce swRear;

SystemRunState smc_curent_status;

//Host Timeout
uint32_t timeLastHostContact;

// Add loop code

int loopCount = 0;

int16_t thrusterSpeed[6];
unsigned long lastTime = 0;
// Functions

void reedSwitchInit()
{
    swFront.initButton(PIN_REED_SW_FRONT,INPUT_PULLUP);
    swCenter.initButton(PIN_REED_SW_CENTER,INPUT_PULLUP);
    swRear.initButton(PIN_REED_SW_REAR,INPUT_PULLUP);
}
bool IMUError()
{
    uint8_t system_status;
    uint8_t self_test_result;
    uint8_t systemError;
    IMU.getSystemStatus(&system_status, &self_test_result, &systemError);
    if(1
       &&(system_status ==5)
       &&(self_test_result==0x0F)
       &&(systemError ==0)
       )
    {
        return false;
    }
    return true;
}

// Add setup code

void setup()
{
    // ESC Init
    ESC_init_all();
    smc_curent_status = System_Idle;
    // IMU Init
    IMU.begin();
    IMU.setAxisRemap(0x21);  //flip X and Y
    IMU.setAxisSign(0x02); // invert Z
    IMU.setExtCrystalUse(true);
    // Gas Guage init
    gasGauge.begin();
    
    //TODO: figure out why IMU self test doesnt work
//
//    if(IMUError())
//    {
//        smc_curent_status = System_Fault_IMU;
//    }
//
    // serial init
    Serial.begin(BAUD_RATE);
    //Serial.println("Hello!");
    // reed swich Init
    reedSwitchInit();
    depthSensor.init();
    depthSensor.read();

}

void loop()
{
#ifdef MANUAL_CONTROL
    printStatusStruct(ESC_Fast_COMM(&ESC[2]));
    delay(20);
    
#else
#endif
    
#ifdef MATLAB_MODE
    depthSensor.readAsync();
    ESC_update_all();
    delay(5);
#else
#endif


////    printESCState(ESCGetStatus(&ESC[0]));
    
    

//    Serial.print("ver: ");
//    Serial.println(gasGauge.testDataWriteToFlash());

//    int8_t temp = bno.getTemp();
//    Serial.print("Current Temperature: ");
//    Serial.print(temp);
//    Serial.println(" C");
//    Serial.println("");
//
//   imu::Quaternion quat = bno.getQuat();
////
//    Serial.print("quat Size: ");
//    Serial.println(sizeof(quat));
//
//    Serial.print("qW: ");
//    Serial.print(quat.w(), 4);
//    Serial.print(" qX: ");
//    Serial.print(quat.y(), 4);
//    Serial.print(" qY: ");
//    Serial.print(quat.x(), 4);
//    Serial.print(" qZ: ");
//    Serial.print(quat.z(), 4);
//    Serial.print("\n");
//

    if(Serial.available())
    {
        readSerialCommand();
    }

    loopCount++;
    loopCount = loopCount%1000;

}
            
