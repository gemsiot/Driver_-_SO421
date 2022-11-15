#include <SO421.h>

SO421::SO421(SDI12Talon& talon_, uint8_t talonPort_, uint8_t sensorPort_, uint8_t version): talon(talon_)
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
	return ""; //DEBUG!
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
	String metadata = "\"Apogee O2\":{";
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
	metadata = metadata + "}"; //CLOSE  
	return metadata; 
}

String SO421::getData(time_t time)
{
	String output = "\"Apogee O2\":{"; //OPEN JSON BLOB
	bool readDone = false;
	if(getSensorPort() != 0) { //Check both for detection 
		for(int i = 0; i < talon.retryCount; i++) {
			if(!isPresent()) continue; //If presence check fails, try again
	// uint8_t adr = (talon.sendCommand("?!")).toInt(); //Get address of local device 
	// String stat = talon.command("M", adr);
	// Serial.print("STAT: "); //DEBUG!
	// Serial.println(stat);
			int adr = talon.getAddress();
			if(adr < 0) {
				Serial.print("SO421 ADR = "); //DEBUG!
				Serial.println(adr);
				continue; //If address is out of range, try again
			}
			int waitTime = talon.startMeasurmentCRC(adr);
			if(waitTime <= 0) {
				Serial.print("SO421 Wait Time = "); //DEBUG!
				Serial.println(waitTime);
				continue; //If wait time out of range, try again
			}
			delay(waitTime*1000 + 500); //Wait for number of seconds requested, plus half a second to make sure
			String data = talon.command("D0", adr);
			Serial.print("DATA: "); //DEBUG!
			Serial.println(data);

			if(!talon.testCRC(data)) continue; //If CRC is bad, try again

			float sensorData[3] = {0.0}; //Store the 5 vals from the sensor in float form
			if((data.substring(0, data.indexOf("+"))).toInt() != adr) { //If address returned is not the same as the address read, throw error
				Serial.println("ADDRESS MISMATCH!"); //DEBUG!
				throwError(talon.SDI12_SENSOR_MISMATCH | 0x100 | talonPortErrorCode | sensorPortErrorCode); //Throw error on address change, this is a weird place for this error to happen, but could
				continue; //Try again
				//Throw error!
			}
			data.remove(0, 2); //Delete address from start of string
			for(int i = 0; i < 3; i++) { //Parse string into floats -- do this to run tests on the numbers themselves and make sure formatting is clean
				if(indexOfSep(data) > 0) {
					sensorData[i] = (data.substring(0, indexOfSep(data))).toFloat();
					Serial.println(data.substring(0, indexOfSep(data))); //DEBUG!
					data.remove(0, indexOfSep(data) + 1); //Delete leading entry
				}
				else {
					data.trim(); //Trim off trailing characters
					sensorData[i] = data.toFloat();
				}
			}
			output = output + "\"Oxygen_%\":" + String(sensorData[0]) + ",\"Oxygen_mV\":" + String(sensorData[1]) + ",\"Temperature\":" + String(sensorData[2]); //Concatonate data
			readDone = true; //Set flag 
			break;
		}
		if(readDone == false) throwError(talon.SDI12_READ_FAIL); //Only throw read error if sensor SHOULD be detected 
	} 
	if(getSensorPort() == 0 || readDone == false) output = output + "\"Oxygen_%\":null,\"Oxygen_mV\":null,\"Temperature\":null"; //Append nulls if no sensor port found, or read did not work
	

	

	// float sensorData[3] = {0.0}; //Store the 3 vals from the sensor in float form
	// if((data.substring(0, data.indexOf("+"))).toInt() != adr) { //If address returned is not the same as the address read, throw error
	// 	Serial.println("ADDRESS MISMATCH!"); //DEBUG!
	// 	//Throw error!
	// }
	// data.remove(0, 2); //Delete address from start of string
	// for(int i = 0; i < 3; i++) { //Parse string into floats -- do this to run tests on the numbers themselves and make sure formatting is clean
	// 	if(data.indexOf("+") > 0) {
	// 		sensorData[i] = (data.substring(0, data.indexOf("+"))).toFloat();
	// 		Serial.println(data.substring(0, data.indexOf("+"))); //DEBUG!
	// 		data.remove(0, data.indexOf("+") + 1); //Delete leading entry
	// 	}
	// 	else {
	// 		data.trim(); //Trim off trailing characters
	// 		sensorData[i] = data.toFloat();
	// 	}
	// }
	
	output = output + ",\"Pos\":[" + getTalonPortString() + "," + getSensorPortString() + "]"; //Concatonate position 
	output = output + "}"; //CLOSE JSON BLOB
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

int SO421::indexOfSep(String input)
{
	int pos1 = input.indexOf('+');
	int pos2 = input.indexOf('-');
	if(pos1 >= 0 && pos2 >= 0) return min(pos1, pos2); //If both are positive, just return the straight min
	else return max(pos1, pos2); //If one of them is -1, then return the other one. If both are -1, then you should return -1 anyway
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
	String output = "\"Apogee O2\":{"; // OPEN JSON BLOB
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
	output = output + "}"; //CLOSE JSON BLOB
	numErrors = 0; //Clear error count
	return output;

	// return -1; //Return fault if unknown cause 
}