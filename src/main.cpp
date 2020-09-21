#include <Arduino.h>
#include "esp_task.h"
#include "esp_task_wdt.h"
                                        // https://github.com/alexandresvifpb/Node_SD_PPG_TTGO-TBeam_V02.git
                                        // D:\Users\alexa\OneDrive\doutorado\2020\prototipos\firmware\node\Node_SD_PPG_TTGO-TBeam_V02          // localizacao do projeto
#define MAIN_MESSAGE_INITIAL            ("D:\\Users\\alexa\\OneDrive\\doutorado\\2020\\prototipos\\firmware\\tests\\Node_SD_PPG_TTGO-TBeam_V02.00")
#define MAIN_DEBUG                      (true)          // Variable that enables (1) or disables (0) the sending of data by serial to debug the program
#define MAIN_CORE_0                     (0)             // Defined the core to be used
#define MAIN_CORE_1                     (1)             
#define MAIN_WATCHDOC_TIME_OUT          (600)           // Watchdog wait time to restart the process in seconds

TaskHandle_t TaskIdSDCard;
TaskHandle_t TaskIdSerial;
TaskHandle_t TaskIdPPG;

void TaskSDCard( void * pvParameters );
void TaskSerial( void * pvParameters );
void TaskPPG( void * pvParameters );

//===========================================
//  ESP32 Util
#include "ESP32UtilLibV01.h"

ESP32UtilLib module;
String device_id = module.get_MAC();
String gateway_id = "FFFFFF";
uint8_t device_type = NODE;
uint16_t boot_sequence = -1;
behavior_t behavior_paramenters;
uint8_t behavior = 0;

//===========================================
//  SD Card
#include "ND_SDCard_Lib_V01.h"

SDCardLib sdCard;
bool sdCard_enable = false;

//===========================================
//  PPG (MAX30102)
#include "ND_PPG_Lib_V01.h"

PPG_Lib ppg_sensor;
boolean ppg_enable = false;
// HRSpO2_Buffer_t ppg_sensor_Buffer;
boolean readRawDataBuffer = false;
PPG_t ppg_data;

String filename_ir = "/IR_B8_";
String filename_red = "/RD_B8_";
String filename_hr = "/HR_B8_";

void setup() {
  // Serial Initialization
  Serial.begin(115200);
  Serial.println();
  Serial.println(MAIN_MESSAGE_INITIAL);
  Serial.println();

  // incrementa o contador de boot
  EEPROM.begin(2);
  boot_sequence = module.get_boot_sequence();
  Serial.print("Boot Sequence: ");
  Serial.println(boot_sequence);

  filename_ir += String(boot_sequence) + ".csv";
  filename_red += String(boot_sequence) + ".csv";
  filename_hr += String(boot_sequence) + ".csv";

  // Serial.println(__LINE__);

  // Creates a Task that will execute the TaskLoRa () function, with priority 1 and running in the nucleus 1
  xTaskCreatePinnedToCore(
                  TaskSDCard,     // Function with the code that implements the Task
                  "TaskSDCard",   // Task name
                  4096,           // Stack size to be allocated when creating the Task
                  NULL,           // Task input parameters
                  1,              // Task priority
                  &TaskIdSDCard,  // Reference to accompany the Task
                  MAIN_CORE_1);   // Core on which the Task will run (0 or 1 for ESP32)
  delay(100);                     // Delay for next command

  // Serial.println(__LINE__);

  // Creates a Task that will execute the TaskLoRa () function, with priority 1 and running in the nucleus 1
  xTaskCreatePinnedToCore(
                  TaskSerial,        // Function with the code that implements the Task
                  "TaskSerial",      // Task name
                  2048,           // Stack size to be allocated when creating the Task
                  NULL,           // Task input parameters
                  1,              // Task priority
                  &TaskIdSerial,     // Reference to accompany the Task
                  MAIN_CORE_1);   // Core on which the Task will run (0 or 1 for ESP32)
  delay(100);                     // Delay for next command

  // Serial.println(__LINE__);

  // Creates a Task that will execute the TaskLoRa () function, with priority 1 and running in the nucleus 1
  xTaskCreatePinnedToCore(
                  TaskPPG,        // Function with the code that implements the Task
                  "TaskPPG",      // Task name
                  2048,           // Stack size to be allocated when creating the Task
                  NULL,           // Task input parameters
                  1,              // Task priority
                  &TaskIdPPG,     // Reference to accompany the Task
                  MAIN_CORE_0);   // Core on which the Task will run (0 or 1 for ESP32)
  delay(100);                     // Delay for next command

  // Serial.println(__LINE__);

  // Enables the watchdog with a 15-second timeout
  esp_task_wdt_init(MAIN_WATCHDOC_TIME_OUT, true);
}

// Task for the SDCard module
void TaskSDCard( void * pvParameters ) {
  esp_task_wdt_add(NULL);

  // Serial.println(__LINE__);

  // Initializes the SD Card module
  sdCard_enable = sdCard.begin();
  ( !sdCard_enable ) ? Serial.println("SD Card module initialization error") : Serial.println("SD Card Module OK!");

  // Mandatory infinite loop to keep the Task running
  while(true) {
    if ( sdCard_enable ) {

      sdCard.run();
    }

    esp_task_wdt_reset();                                       // Reset watchdog counter
    vTaskDelay(pdMS_TO_TICKS(SD_TASK_DELAY_MS));      // Pause Tesk and release the nucleus for the next Tesk in the priority queue
  }
}

// Task for the SDCard module
void TaskSerial( void * pvParameters ) {
  esp_task_wdt_add(NULL);

  // Serial.println(__LINE__);
  // Mandatory infinite loop to keep the Task running
  while(true) {

    for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
      // Serial.println(ppg_sensor.getIRRawData(i));
      delay(20);
    }

    if ( Serial.available() ) {
      int brigh = Serial.read();

      Serial.println(brigh);
    }

    esp_task_wdt_reset();                                       // Reset watchdog counter
    vTaskDelay(pdMS_TO_TICKS(100));      // Pause Tesk and release the nucleus for the next Tesk in the priority queue
  }
}

// Task for the GPS module
void TaskPPG( void * pvParameters ) {
  esp_task_wdt_add(NULL);

  // Serial.println(__LINE__);

  // Initializes the MPU9250 module
  ppg_enable = ppg_sensor.begin();
  ( !ppg_enable ) ? Serial.println("Error initializing the module sensor HR & SpO2 (MAX30105)") : Serial.println("HR & SpO2 module (Max30105) OK!");

  // long last_time = 0;
  // lora_send_t message_REPLAY;

  uint8_t LEDBrightness = 35;
  ppg_sensor.setLEDBrightness(LEDBrightness);      // 265:60   266:90    267:40    268:20   269:100   270:200   271:30   277:30    278:50   280:250   281:35

  // Mandatory infinite loop to keep the Task running
  while(true) {

    if ( ppg_enable ) {
      // Serial.println(__LINE__);
      ppg_sensor.run();
      
      if (ppg_sensor.newDataAvailable()) {

        ppg_data.heartRate = ppg_sensor.getHeartRate();
        ppg_data.spo2 = ppg_sensor.getSpO2();
        ppg_data.temperature = ppg_sensor.getTemperature();

        // Serial.println(__LINE__);

        if (false) {
        Serial.print(ppg_data.heartRate, DEC);
        Serial.print(";");
        Serial.print(ppg_data.spo2);
        Serial.print(";");
        Serial.println(ppg_data.temperature);
        }

        if (sdCard.is_SDCard_present()) {

          // Serial.println(__LINE__);
  
          SDCard_record_t new_recorde_test;
          new_recorde_test.bootSequence = boot_sequence;
          long millis_now = millis();

          new_recorde_test.type = TYPE_PPG_IR;
          new_recorde_test.payload = String((unsigned)millis_now);
          new_recorde_test.payload += ";";
          new_recorde_test.payload += ppg_sensor.get_ir_buffer();
          sdCard.add_record(new_recorde_test);

          new_recorde_test.type = TYPE_PPG_RED;
          new_recorde_test.payload = String((unsigned)millis_now);
          new_recorde_test.payload += ";";
          new_recorde_test.payload += ppg_sensor.get_red_buffer();
          sdCard.add_record(new_recorde_test);

          new_recorde_test.type = TYPE_PPG_HR_SPO2;
          new_recorde_test.payload = String((unsigned)millis_now);
          new_recorde_test.payload += ";";
          new_recorde_test.payload += LEDBrightness;
          new_recorde_test.payload += ";";
          new_recorde_test.payload += ppg_sensor.get_hr_spo2();
          sdCard.add_record(new_recorde_test);
          // Serial.println(new_recorde_test.payload);

        }

        // for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
        //   Serial.println(ppg_sensor.getIRRawData(i));
        // }
        
        /*
        if ( millis() > ( last_time + 1000 ) ) {

          lora_send_t newRecordLoRa;

          newRecordLoRa.sourcer_device_id = device_id;
          newRecordLoRa.target_device_id = gateway_id;
        
          newRecordLoRa.message_type = PPG_TYPE;

          newRecordLoRa.payload = String((unsigned long)millis());
          newRecordLoRa.payload += ',';
          newRecordLoRa.payload += boot_sequence;

          newRecordLoRa.payload += ',';
          newRecordLoRa.payload += ppg_data.heartRate;

          newRecordLoRa.payload += ',';
          newRecordLoRa.payload += ppg_data.spo2;

          newRecordLoRa.payload += ',';
          newRecordLoRa.payload += ppg_data.temperature;

          lora.add_send_message(newRecordLoRa);

        }
        */

      }

    }

    esp_task_wdt_reset();                                       // Reset watchdog counter
    vTaskDelay(pdMS_TO_TICKS(PPG_TASK_DELAY_MS));      // Pause Tesk and release the nucleus for the next Tesk in the priority queue
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}