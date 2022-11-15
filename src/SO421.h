#ifndef SO421_h
#define SO421_h

// #include "DPS368-Library-Arduino/src/Dps368.h"
// #include "Adafruit_SHT31/src/Adafruit_SHT31.h"
#include <Sensor.h>
#include <SDI12Talon.h>

class SO421: public Sensor
{
	constexpr static int DEAFULT_PORT = 2; ///<Use port 2 by default
	constexpr static int DEFAULT_SENSOR_PORT = 0; ///<Use port 0 by default
  	constexpr static int DEFAULT_VERSION = 0x00; ///<Use hardware version unknown by default
	const String FIRMWARE_VERSION = "0.0.0"; //FIX! Read from system??
  	// constexpr static int MAX_NUM_ERRORS = 10; ///<Maximum number of errors to log before overwriting previous errors in buffer

	// const uint32_t SENSOR_PORT_RANGE_ERROR = 0x90010100; //FIX! 
	// const uint32_t TALON_PORT_RANGE_ERROR = 0x90010200; //FIX! 
	const uint32_t DPS368_READ_ERROR = 0x80010000; //FIX! Error subtype = 1 for temp read, subtype = 2 for pres
	const uint32_t SHT3X_NAN_ERROR = 0x10040000; //FIX! Error subtype = 1 for temp read, subtype = 2 for RH
	const uint32_t DPS368_INIT_ERROR = 0x10010000; //FIX! Error subtype = I2C error code
	const uint32_t SHT3X_INIT_ERROR = 0x10030000; //FIX! 
	const uint32_t SHT3X_I2C_ERROR = 0x10020000; //FIX! Error subtype = I2C error code

	public:
		SO421(SDI12Talon& talon_, uint8_t talonPort_ = DEAFULT_PORT, uint8_t sensorPort_ = DEFAULT_SENSOR_PORT, uint8_t version = DEFAULT_VERSION);
		String begin(time_t time, bool &criticalFault, bool &fault);
		String getData(time_t time);
		String getMetadata();
		String getErrors();
		bool isPresent();
		// uint8_t getTalonPort() {
		// 	if(talonPort < 255) return talonPort + 1;
		// 	else return 0;
		// }
		// uint8_t getSensorPort() {
		// 	if(sensorPort < 255) return sensorPort + 1;
		// 	else return 0;
		// }
		// uint8_t totalErrors() {
		// 	return numErrors;
		// }
		// void setTalonPort(uint8_t port);
		// void setSensorPort(uint8_t port);
		// String getSensorPortString();
		// String getTalonPortString();

		// const uint8_t sensorInterface = BusType::I2C; 
	private:
		// Dps368 presSensor = Dps368();
		// Adafruit_SHT31 rhSensor = Adafruit_SHT31();
		SDI12Talon& talon;
		int indexOfSep(String input);
		// uint8_t port = 0;
		// int throwError(uint32_t error);

		bool initDone = false; //Used to keep track if the initaliztion has run - used by hasReset() 
		

		// uint32_t errors[MAX_NUM_ERRORS] = {0};
		// uint8_t numErrors = 0; //Used to track the index of errors array
		// bool errorOverwrite = false; //Used to track if errors have been overwritten in time since last report
		// bool timeBaseGood = false; //Used to keep track of the valitity of the current timebase
		// uint8_t talonPort = 255; //Used to keep track of which port the Talon is connected to on Kestrel
		// uint8_t sensorPort = 255; //Used to keep track of which port the sensor is connected to on associated Talon
		// uint32_t talonPortErrorCode = 0; //Used to easily OR with error codes to add the Talon port
		// uint32_t sensorPortErrorCode = 0; //Used to easily OR with error codes to add the Sensor port
		uint8_t version = 0; //FIX! This should be read from EEPROM in future 
};

#endif