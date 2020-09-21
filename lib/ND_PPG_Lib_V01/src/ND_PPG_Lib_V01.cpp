#include "ND_PPG_Lib_V01.h"

uint32_t elapsedTime,timeStart;

uint32_t aun_ir_buffer[BUFFER_SIZE]; //infrared LED sensor data
uint32_t aun_red_buffer[BUFFER_SIZE];  //red LED sensor data
uint8_t uch_dummy;
uint8_t temp_resp, temp_int, temp_frac;

PPG_Buffer_t hrspo2Buffer;

int32_t i = 0;
boolean newSample = false;
PPG_t sample;

int32_t n_heart_rate; //heart rate value
int8_t  ch_hr_valid;  //indicator to show if the heart rate calculation is valid
float n_spo2,ratio,correl;  //SPO2 value
int8_t ch_spo2_valid;  //indicator to show if the SPO2 calculation is valid

String ir_buffer = ";";
String red_buffer = ";";
String hr_spo2_string = "";

// Interrupt pin
const int8_t pinInterrupt = PPG_PIN_INT;      // pin connected to MAX30102 INT

// Construntor
PPG_Lib::PPG_Lib() {}

//
boolean PPG_Lib::begin(void) {
  uint16_t default_boot_sequence = SD_FILE_NAME_DEFAULT_BOOT_SEQUENCE;
  return begin(default_boot_sequence);
}

// 
boolean PPG_Lib::begin(uint16_t boot_sequence) {

  pinMode(pinInterrupt, INPUT);  //pin GPIO23 connects to the interrupt output pin of the MAX30102
  // attachInterrupt(digitalPinToInterrupt(pinInterrupt), sensorInterrupt, CHANGE);  // FALLING = alto (HIGH) -> baixo (LOW); RISING = baixo (LOW) -> alto (HIGH);

  // Initialize sensor
  if (!Wire.begin()) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    return false;
  }

  //Setup to sense a nice looking saw tooth on the plotter
  // byte ledBrightness = LED_BRIGHTNESS;  //Options: 0=Off to 255=50mA
  // byte ledMode = LED_MODE;              //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  // int sampleRate = SAMPLING_RATE;       //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  // byte sampleAverage = SAMPLE_AVERAGE;  //Options: 1, 2, 4, 8, 16, 32
  // int pulseWidth = PULSE_WIDTH;         //Options: 69, 118, 215, 411
  // int adcRange = ADC_RANGE;             //Options: 2048, 4096, 8192, 16384

  maxim_max30102_reset(); //resets the MAX30102
  delay(1000);

  maxim_max30102_read_reg(REG_INTR_STATUS_1,&uch_dummy);  //Reads/clears the interrupt status register
  maxim_max30102_init();  //initialize the MAX30102

  timeStart=millis();

  return true;
}

//
void PPG_Lib::run(void) {

  // String ir_buffer_string = "";
  // String red_buffer_string = "";

  if (!newSample) {


    while(digitalRead(pinInterrupt)==1);  //wait until the interrupt pin asserts
  
    maxim_max30102_read_fifo((aun_ir_buffer+i), (aun_red_buffer+i));  //read from MAX30102 FIFO

    hrspo2Buffer.irBuffer[i] = aun_ir_buffer[i];
    hrspo2Buffer.redBuffer[i] = aun_red_buffer[i];
    hrspo2Buffer.status = 0;

    // ir_buffer_string += String(aun_ir_buffer[i]);
    // red_buffer_string += String(aun_red_buffer[i]);

    i++;

    if (i>=BUFFER_SIZE) {
      newSample = true;
      i = 0;
    } else {
      // ir_buffer_string += ";";
      // red_buffer_string += ";";
    }
    
  }

  if (newSample) {
     char hr_str[10];
    //calculate heart rate and SpO2 after BUFFER_SIZE samples (ST seconds of samples) using Robert's method
    rf_heart_rate_and_oxygen_saturation(aun_ir_buffer, BUFFER_SIZE, aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid, &ratio, &correl); 
    elapsedTime=millis()-timeStart;
    millis_to_hours(elapsedTime, hr_str); // Time in hh:mm:ss format
    elapsedTime/=1000; // Time in seconds

    sample.spo2 = n_spo2;
    sample.spo2Valid = ch_spo2_valid;
    sample.heartRate = n_heart_rate;
    sample.hrValid = n_heart_rate;
    sample.temperature = readTemperature();
    sample.readingTime = elapsedTime;

    newSample = false;
    availableData = true;
    hrspo2Buffer.status = 1;

    hr_spo2_string = String(sample.heartRate, DEC);
    hr_spo2_string += ";";
    hr_spo2_string += String(sample.hrValid);
    hr_spo2_string += ";";
    hr_spo2_string += String(sample.spo2);
    hr_spo2_string += ";";
    hr_spo2_string += String(sample.spo2Valid);
    hr_spo2_string += ";";
    hr_spo2_string += String(sample.temperature);
    hr_spo2_string += ";";
    hr_spo2_string += String(sample.readingTime);

    // ir_buffer = ir_buffer_string;
    // ir_buffer_string = "";

    // red_buffer = red_buffer_string;
    // red_buffer_string = "";

/*
    String _ir_buffer = "";
    String _red_buffer = "";

    for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
      _ir_buffer += String(aun_ir_buffer[i]);
      _ir_buffer += ";";
      _red_buffer += String(aun_red_buffer[i]);
      _red_buffer += ";";
    }
    
    ir_buffer = _ir_buffer;
    red_buffer = _red_buffer;
*/
    ir_buffer = "";
    red_buffer = "";

    for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
      ir_buffer += String(aun_ir_buffer[i]);
      red_buffer += String(aun_red_buffer[i]);
      if ( i < (BUFFER_SIZE - 1) ) {
        ir_buffer += ";";
        red_buffer += ";";
      }
    }

    // Serial.println(ir_buffer);

  }
}

//
uint16_t PPG_Lib::getHeartRate(void) {
  return n_heart_rate;
}

//
float PPG_Lib::getSpO2(void) {
  return n_spo2;
}

//
float PPG_Lib::getTemperature(void) {
  return sample.temperature;
}

//
boolean PPG_Lib::newDataAvailable(void) {
  boolean ret = availableData;
  availableData = false;
  return ret;
}

//
float PPG_Lib::readTemperature(void) {
  // Step 1: sete o registro de configuracao da temperatura (REG_TEMP_CONFIG) em 1 para inicial uma nova leitura 
  maxim_max30102_write_reg(REG_TEMP_CONFIG, 0x01);

  // Aguarde que o registro de configuracao da temperatura volte para 0 (zero) indicado que o CI concluiu
  // a aquisicao da temperatura
  // Timeout em 100ms
  unsigned long startTime = millis();
  while (millis() - startTime < 100) {

    if (maxim_max30102_read_reg(REG_TEMP_CONFIG, &temp_resp))
      if (temp_resp == 0)
        break;
    delay(1); //Let's not over burden the I2C bus  

  }

  maxim_max30102_read_reg(REG_TEMP_INTR, &temp_int);
  maxim_max30102_read_reg(REG_TEMP_FRAC, &temp_frac);

  return (float)temp_int + ((float)temp_frac * 0.0625);
}

//
void PPG_Lib::setLEDBrightness(uint8_t value) {

  maxim_max30102_write_reg(REG_LED1_PA,value);       //Choose value for ~ 7mA for LED1
  maxim_max30102_write_reg(REG_LED2_PA,value);       // Choose value for ~ 7mA for LED2
  maxim_max30102_write_reg(REG_PILOT_PA,value);      // Choose value for ~ 25mA for Pilot LED

}

String PPG_Lib::get_ir_buffer(void) {
  return ir_buffer;
}

String PPG_Lib::get_red_buffer(void) {
  return red_buffer;
}

String PPG_Lib::get_hr_spo2(void) {
  return hr_spo2_string;
}

//
void PPG_Lib::millis_to_hours(uint32_t ms, char* hr_str) {
  char istr[6];
  uint32_t secs,mins,hrs;
  secs=ms/1000; // time in seconds
  mins=secs/60; // time in minutes
  secs-=60*mins; // leftover seconds
  hrs=mins/60; // time in hours
  mins-=60*hrs; // leftover minutes
  itoa(hrs,hr_str,10);
  strcat(hr_str,":");
  itoa(mins,istr,10);
  strcat(hr_str,istr);
  strcat(hr_str,":");
  itoa(secs,istr,10);
  strcat(hr_str,istr);
}

/*
boolean PPG_Lib::appendRegisterFile(fs::FS &fs, const char * filename, const char * message) {

  // File file = SD.open(filename, FILE_APPEND);
  File file = fs.open(filename, FILE_APPEND);

  // Serial.println(file);

  if (file) {
    if (file.println(message)) {
      return true;
    } else {
      return false;
    }
    file.close();
  }
}
*/

PPG_Buffer_t PPG_Lib::getRawData(void) {
  return hrspo2Buffer;
}

String PPG_Lib::getRawDataBuffer(uint8_t index) {
  String strRawData = "";       // { valor_Sensor_Red ; Valor_Sensor_IR }

  strRawData += aun_ir_buffer[index];
  strRawData += ";";
  strRawData += aun_red_buffer[index];

  return strRawData;
}

uint32_t PPG_Lib::getIRRawData(uint8_t index) {
  return aun_red_buffer[index];
}

uint32_t PPG_Lib::getRedRawData(uint8_t index) {
  return aun_ir_buffer[index];
}