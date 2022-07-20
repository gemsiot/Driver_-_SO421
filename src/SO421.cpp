#include <SO421.h>

SO421::SO421(SDI12Talon& talon_, uint8_t talonPort_, uint8_t sensorPort_, uint8_t version): talon(talon_), presSensor(), rhSensor()
{
	//Only update values if they are in range, otherwise stick with default values
	if(talonPort_ > 0) talonPort = talonPort_ - 1;
	else talonPort = 255; //Reset to null default if not in range
	if(sensorPort_ > 0) sensorPort = sensorPort_ - 1;
	else sensorPort = 255; //Reset to null default if not in range 
	sensorInterface = BusType::SDI12; 
}

String SO421::begin(time_t time, bool &criticalFault, bool &fault)
{
	// Serial.println("SO421 - BEGIN"); //DEBUG!
	// presSensor.begin(Wire, 0x76); //DEBUG!
	// if(rhSensor.begin(0x44) == false) {
	// 	Serial.println("\tSHT31 Init Fail"); //DEBUG!
	// 	throwError(SHT3X_INIT_ERROR | talonPortErrorCode); //Error subtype = I2C error
	// } 
	
	// Wire.beginTransmission(0x76);
	// int error = Wire.endTransmission();
	// if(error != 0) {
	// 	Serial.println("\tDPS368 Init Fail"); //DEBUG!
	// 	throwError(DPS368_INIT_ERROR | (error << 12) | talonPortErrorCode); //Error subtype = I2C error
	// }
	
	// Wire.beginTransmission(0x44);
	// int errorB = Wire.endTransmission();
	// ret = pres.measureTempOnce(temperature, oversampling);
	// Serial.print("INIT: ");
	// if(errorA == 0 || errorB == 0) Serial.println("PASS");
	// else {
	// 	Serial.print("ERR - ");
	// 	if(errorA != 0) {
	// 		Serial.print("A\t");
	// 		throwError(SHT3X_I2C_ERROR | (errorA << 12) | talonPortErrorCode); //Error subtype = I2C error
	// 	}
	// 	if(errorB != 0) Serial.print("B\t");
	// 	Serial.println("");
	// }
	return "{}"; //DEBUG!
}

String SO421::getMetadata()
{
	// Wire.beginTransmission(0x58); //Write to UUID range of EEPROM
	// Wire.write(0x98); //Point to start of UUID
	// int error = Wire.endTransmission();
	// // uint64_t uuid = 0;
	// String uuid = "";

	// if(error != 0) throwError(EEPROM_I2C_ERROR | error);
	// else {
	// 	uint8_t val = 0;
	// 	Wire.requestFrom(0x58, 8); //EEPROM address
	// 	for(int i = 0; i < 8; i++) {
	// 		val = Wire.read();//FIX! Wait for result??
	// 		// uuid = uuid | (val << (8 - i)); //Concatonate into full UUID
	// 		uuid = uuid + String(val, HEX); //Print out each hex byte
	// 		// Serial.print(Val, HEX); //Print each hex byte from left to right
	// 		// if(i < 7) Serial.print('-'); //Print formatting chracter, don't print on last pass
	// 		if(i < 7) uuid = uuid + "-"; //Print formatting chracter, don't print on last pass
	// 	}
	// }
	uint8_t adr = (talon.sendCommand("?!")).toInt(); //Get address of local device 
	String id = talon.command("I", adr);
	Serial.println(id); //DEBUG!
	String sdi12Version;
	String mfg;
	String model;
	String senseVersion;
	String sn;
	if((id.substring(0, 1)).toInt() != adr) { //If address returned is not the same as the address read, throw error
		Serial.println("ADDRESS MISMATCH!"); //DEBUG!
		//Throw error!
		sdi12Version = "null";
		mfg = "null";
		model = "null";
		senseVersion = "null";
		sn = "null";
	}
	else {
		sdi12Version = (id.substring(1,3)).trim(); //Grab SDI-12 version code
		mfg = (id.substring(3, 11)).trim(); //Grab manufacturer
		model = (id.substring(11,17)).trim(); //Grab sensor model name
		senseVersion = (id.substring(17,20)).trim(); //Grab version number
		sn = (id.substring(20,33)).trim(); //Grab the serial number 
	}
	String metadata = "{\"Apogee O2\":{";
	// if(error == 0) metadata = metadata + "\"SN\":\"" + uuid + "\","; //Append UUID only if read correctly, skip otherwise 
	metadata = metadata + "\"Hardware\":\"" + senseVersion + "\","; //Report sensor version pulled from SDI-12 system 
	metadata = metadata + "\"Firmware\":\"" + FIRMWARE_VERSION + "\","; //Static firmware version 
	metadata = metadata + "\"SDI12_Ver\":\"" + sdi12Version.substring(0,1) + "." + sdi12Version.substring(1,2) + "\",";
	metadata = metadata + "\"ADR\":" + String(adr) + ",";
	metadata = metadata + "\"Mfg\":\"" + mfg + "\",";
	metadata = metadata + "\"Model\":\"" + model + "\",";
	metadata = metadata + "\"SN\":\"" + sn + "\",";
	//GET SERIAL NUMBER!!!! //FIX!
	metadata = metadata + "\"Pos\":[" + getTalonPortString() + "," + getSensorPortString() + "]"; //Concatonate position 
	metadata = metadata + "}}"; //CLOSE  
	return metadata; 
}

String SO421::getData(time_t time)
{
	float temperatureDPS368;
	float pressure;
	uint8_t oversampling = 7;
	int16_t ret;

	uint8_t adr = (talon.sendCommand("?!")).toInt(); //Get address of local device 
	String stat = talon.command("M", adr);
	Serial.print("STAT: "); //DEBUG!
	Serial.println(stat);

	delay(1000); //Wait 1 second to get data back
	String data = talon.command("D0", adr);
	Serial.print("DATA: "); //DEBUG!
	Serial.println(data);

	String output = "{\"Apogee O2\":{"; //OPEN JSON BLOB

	float sensorData[3] = {0.0}; //Store the 3 vals from the sensor in float form
	if((data.substring(0, data.indexOf("+"))).toInt() != adr) { //If address returned is not the same as the address read, throw error
		Serial.println("ADDRESS MISMATCH!"); //DEBUG!
		//Throw error!
	}
	data.remove(0, 2); //Delete address from start of string
	for(int i = 0; i < 3; i++) { //Parse string into floats -- do this to run tests on the numbers themselves and make sure formatting is clean
		if(data.indexOf("+") > 0) {
			sensorData[i] = (data.substring(0, data.indexOf("+"))).toFloat();
			Serial.println(data.substring(0, data.indexOf("+"))); //DEBUG!
			data.remove(0, data.indexOf("+") + 1); //Delete leading entry
		}
		else {
			data.trim(); //Trim off trailing characters
			sensorData[i] = data.toFloat();
		}
	}
	output = output + "\"Oxygen_%\":" + String(sensorData[0]) + ",\"Oxygen_mV\":" + String(sensorData[1]) + ",\"Temperature\":" + String(sensorData[2]); //Concatonate data
	// String dps368Data = "\"DPS368\":{\"Temperature\":"; //Open dps368 substring
	// String sht3xData = "\"SHT31\":{\"Temperature\":"; //Open SHT31 substring //FIX! How to deal with SHT31 vs SHT35?? Do we deal with it at all

	// //lets the Dps368 perform a Single temperature measurement with the last (or standard) configuration
	// //The result will be written to the paramerter temperature
	// //ret = Dps368PressureSensor.measureTempOnce(temperature);
	// //the commented line below does exactly the same as the one above, but you can also config the precision
	// //oversampling can be a value from 0 to 7
	// //the Dps 368 will perform 2^oversampling internal temperature measurements and combine them to one result with higher precision
	// //measurements with higher precision take more time, consult datasheet for more information
	// // Wire.beginTransmission(0x77);
	// // int errorA = Wire.endTransmission();
	// // Wire.beginTransmission(0x76);
	// // int errorB = Wire.endTransmission();
	// bool dummy1;
	// bool dummy2;
	// begin(0, dummy1, dummy2); //DEBUG!
	// ret = presSensor.measureTempOnce(temperatureDPS368, oversampling); //Measure temp
	// if(ret == 0) { //If no error in read
	// 	dps368Data = dps368Data + String(temperatureDPS368,2) + ","; //Append temp with 2 decimal points since resolution is 0.01°C, add comma
	// 	// dps368Data = dps368Data + "Pressure" + String()
	// }
	// else {
	// 	dps368Data = dps368Data + "null,"; //Append null as non-report 
	// 	throwError(DPS368_READ_ERROR | 0x1000 | talonPortErrorCode | sensorPortErrorCode); //Error subtype = temp
	// 	//FIX! Throw error
	// }
	// ret = presSensor.measurePressureOnce(pressure, oversampling); //Measure pressure 
	// if(ret == 0) { //If no error in read
	// 	dps368Data = dps368Data + "\"Pressure\":" + String(pressure/100.0,3); //Append pressure (divided from Pa to hPa, which is equal to mBar) with 3 decimal points since resolution is 0.002hPa
	// }
	// else {
	// 	dps368Data = dps368Data + "\"Pressure\":null"; //Append null as non-report
	// 	throwError(DPS368_READ_ERROR | 0x2000 | talonPortErrorCode | sensorPortErrorCode); //Error subtype = pres
	// 	//FIX! Throw error
	// }
	// dps368Data = dps368Data + "}"; //Close DPS368 substring

	// float temperatureSHT3x = rhSensor.readTemperature();
  	// float humidity = rhSensor.readHumidity();

	// if(!isnan(temperatureSHT3x)) { //If no error in read
	// 	sht3xData = sht3xData + String(temperatureSHT3x,2) + ","; //Append temp with 2 decimal points since resolution is 0.01°C, add comma
	// 	// dps368Data = dps368Data + "Pressure" + String()
	// }
	// else {
	// 	sht3xData = sht3xData + "null,"; //Append null as non-report 
	// 	Wire.beginTransmission(0x76);
	// 	int error = Wire.endTransmission();
	// 	if(error == 0) throwError(SHT3X_NAN_ERROR | 0x1000 | talonPortErrorCode | sensorPortErrorCode); //Error subtype = temp
	// 	else throwError(SHT3X_I2C_ERROR | (error << 12) | talonPortErrorCode | sensorPortErrorCode); //Error subtype = I2C error
		
	// 	//FIX! Throw error
	// }

	// if(!isnan(humidity)) { //If no error in read
	// 	sht3xData = sht3xData + "\"Humidity\":" + String(humidity,2); //Append humidity with 2 decimal points since resolution is 0.01%
	// }
	// else {
	// 	sht3xData = sht3xData + "\"Humidity\":null"; //Append null as non-report
	// 	Wire.beginTransmission(0x76);
	// 	int error = Wire.endTransmission();
	// 	if(error == 0) throwError(SHT3X_NAN_ERROR | 0x2000 | talonPortErrorCode | sensorPortErrorCode); //Error subtype = RH
	// 	else throwError(SHT3X_I2C_ERROR | (error << 12) | talonPortErrorCode | sensorPortErrorCode); //Error subtype = I2C error
	// }
	// sht3xData = sht3xData + "}"; //Close SHT3x substring

	// // Serial.print("TEMP: ");
	// // if(errorA == 0 || errorB == 0) Serial.println(temperature); 
	// // else {
	// // 	Serial.print("ERR - ");
	// // 	if(errorA != 0) Serial.print("A\t");
	// // 	if(errorB != 0) Serial.print("B\t");
	// // 	Serial.println("");
	// // }
	
	// output = output + dps368Data + "," + sht3xData + ",";
	output = output + ",\"Pos\":[" + getTalonPortString() + "," + getSensorPortString() + "]"; //Concatonate position 
	output = output + "}}"; //CLOSE JSON BLOB
	Serial.println(output); //DEBUG!
	return output;
}

bool SO421::isPresent() 
{ //FIX!
	// Wire.beginTransmission(0x77);
	// int errorA = Wire.endTransmission();
	// Wire.beginTransmission(0x76);
	// int errorB = Wire.endTransmission();
	// Serial.print("SO421 TEST: "); //DEBUG!
	// Serial.print(errorA);
	// Serial.print("\t");
	// Serial.println(errorB);
	uint8_t adr = (talon.sendCommand("?!")).toInt();
	
	String id = talon.command("I", adr);
	id.remove(0, 1); //Trim address character from start
	Serial.print("SDI12 Address: "); //DEBUG!
	Serial.print(adr);
	Serial.print(",");
	Serial.println(id);
	if(id.indexOf("SO-4") > 0) return true; //FIX! Check version here!
	// if(errorA == 0 || errorB == 0) return true;
	else return false;
}

// void SO421::setTalonPort(uint8_t port)
// {
// 	// if(port_ > numPorts || port_ == 0) throwError(PORT_RANGE_ERROR | portErrorCode); //If commanded value is out of range, throw error 
// 	if(port > 4 || port == 0) throwError(TALON_PORT_RANGE_ERROR | talonPortErrorCode | sensorPortErrorCode); //If commanded value is out of range, throw error //FIX! How to deal with magic number? This is the number of ports on KESTREL, how do we know that??
// 	else { //If in range, update the port values
// 		talonPort = port - 1; //Set global port value in index counting
// 		talonPortErrorCode = (talonPort + 1) << 4; //Set port error code in rational counting 
// 	}
// }

// void SO421::setSensorPort(uint8_t port)
// {
// 	// if(port_ > numPorts || port_ == 0) throwError(PORT_RANGE_ERROR | portErrorCode); //If commanded value is out of range, throw error 
// 	if(port > 4 || port == 0) throwError(SENSOR_PORT_RANGE_ERROR | talonPortErrorCode | sensorPortErrorCode); //If commanded value is out of range, throw error //FIX! How to deal with magic number? This is the number of ports on KESTREL, how do we know that??
// 	else { //If in range, update the port values
// 		sensorPort = port - 1; //Set global port value in index counting
// 		sensorPortErrorCode = (sensorPort + 1); //Set port error code in rational counting 
// 	}
// }

// String SO421::getSensorPortString()
// {
// 	if(sensorPort >= 0 && sensorPort < 255) return String(sensorPort + 1); //If sensor port has been set //FIX max value
// 	else return "null";
// }

// String SO421::getTalonPortString()
// {
// 	if(talonPort >= 0 && talonPort < 255) return String(talonPort + 1); //If sensor port has been set //FIX max value
// 	else return "null";
// }

// int SO421::throwError(uint32_t error)
// {
// 	errors[(numErrors++) % MAX_NUM_ERRORS] = error; //Write error to the specified location in the error array
// 	if(numErrors > MAX_NUM_ERRORS) errorOverwrite = true; //Set flag if looping over previous errors 
// 	return numErrors;
// }

String SO421::getErrors()
{
	// if(numErrors > length && numErrors < MAX_NUM_ERRORS) { //Not overwritten, but array provided still too small
	// 	for(int i = 0; i < length; i++) { //Write as many as we can back
	// 		errorOutput[i] = error[i];
	// 	}
	// 	return -1; //Throw error for insufficnet array length
	// }
	// if(numErrors < length && numErrors < MAX_NUM_ERRORS) { //Not overwritten, provided array of good size (DESIRED)
	// 	for(int i = 0; i < numErrors; i++) { //Write all back into array 
	// 		errorOutput[i] = error[i];
	// 	}
	// 	return 0; //Return success indication
	// }
	String output = "{\"Apogee O2\":{"; // OPEN JSON BLOB
	output = output + "\"CODES\":["; //Open codes pair

	for(int i = 0; i < min(MAX_NUM_ERRORS, numErrors); i++) { //Interate over used element of array without exceeding bounds
		output = output + "\"0x" + String(errors[i], HEX) + "\","; //Add each error code
		errors[i] = 0; //Clear errors as they are read
	}
	if(output.substring(output.length() - 1).equals(",")) {
		output = output.substring(0, output.length() - 1); //Trim trailing ','
	}
	output = output + "],"; //close codes pair
	output =  output + "\"OW\":"; //Open state pair
	if(numErrors > MAX_NUM_ERRORS) output = output + "1,"; //If overwritten, indicate the overwrite is true
	else output = output + "0,"; //Otherwise set it as clear
	output = output + "\"NUM\":" + String(numErrors) + ","; //Append number of errors
	output = output + "\"Pos\":[" + getTalonPortString() + "," + getSensorPortString() + "]"; //Concatonate position 
	output = output + "}}"; //CLOSE JSON BLOB
	numErrors = 0; //Clear error count
	return output;

	// return -1; //Return fault if unknown cause 
}