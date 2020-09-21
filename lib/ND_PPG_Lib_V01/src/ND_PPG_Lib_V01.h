#ifndef ND_PPG_Lib_V01_h
#define ND_PPG_Lib_V01_h

#include <Arduino.h>
#include <Wire.h>
#include "algorithm_by_RF.h"
#include "max30102.h"

// define o tipo do modulo
#define PPG_TYPE                     9         // define como sendo modulo do tipo 2 (MPU9255)

#define PPG_MAX301020_ADDRESS               0x57      
#define PPG_PIN_SDA                         21        //
#define PPG_PIN_SCL                         22        //
#define PPG_PIN_INT                         4        //

#define LED_BRIGHTNESS                      30        //Options: 0=Off to 255=50mA
#define LED_MODE                            3         //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
#define DEBUG_SERIAL                        3         //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green

#define SAMPLING_RATE                       100       //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
#define SAMPLE_AVERAGE                      1         //Options: 1, 2, 4, 8, 16, 32
#define PULSE_WIDTH                         400       //Options: 69, 118, 215, 411
#define ADC_RANGE                           8192       //Options: 2048, 4096, 8192, 16384

#define PPG_TASK_DELAY_MS                   10

// SD Card
#define SD_SCK_PIN                          14
#define SD_MISO_PIN                         2
#define SD_MOSI_PIN                         13
#define SD_SS_PIN                           25

#define SD_FILE_NAME_PREFIX                 "data_"
#define SD_FILE_NAME_DEFAULT_BOOT_SEQUENCE  0

typedef struct {
  float spo2;
  int8_t spo2Valid;
  int32_t heartRate;
  int8_t hrValid;
  float temperature;
  uint32_t readingTime;
} PPG_t;

typedef struct {
  uint8_t status;
  uint32_t irBuffer[BUFFER_SIZE];
  uint32_t redBuffer[BUFFER_SIZE];
} PPG_Buffer_t;

class PPG_Lib {
  public:

    PPG_Lib();

    boolean begin(void);
    boolean begin(uint16_t boot_sequence);
    void run(void);

    uint32_t readIR(void);
    uint32_t readRed(void);
    float readTemperature(void);

    uint16_t getHeartRate(void);
    float getSpO2(void);
    float getTemperature(void);
    boolean newDataAvailable(void);
    String getRawDataBuffer(uint8_t index);
    uint32_t getIRRawData(uint8_t index);
    uint32_t getRedRawData(uint8_t index);
    PPG_Buffer_t getRawData(void);

    void setLEDBrightness(uint8_t value);
    void setLEDMode(uint8_t value);
    void setSamplingRate(uint16_t value);
    void setSampleAverage(uint8_t value);
    void setPulseWidth(uint16_t value);
    void setADCFullScaleRange(uint16_t value);

    String get_ir_buffer(void);
    String get_red_buffer(void);
    String get_hr_spo2(void);

    uint32_t tsHRSPO2TaskDelayMS = PPG_TASK_DELAY_MS;

  private:
    void millis_to_hours(uint32_t ms, char* hr_str);
    // bool appendFile(fs::FS &fs, const char * filename, const char * message);
    // boolean appendRegisterFile(fs::FS &fs, const char * filename, const char * message);

    boolean availableData = false;

};

#endif  // ND_PPG_Lib_V01_h