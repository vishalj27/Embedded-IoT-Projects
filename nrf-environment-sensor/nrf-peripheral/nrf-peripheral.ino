/*
  custom datapacket
  -------------------------------------------------
  | id_L | id_H | temp_L | temp_H | hum_L | hum_H |
  -------------------------------------------------
  id - company name can be ignored
*/

#include <bluefruit.h>
#include <Wire.h>
#include <Adafruit_SPIFlash.h>
#include <SparkFunHTU21D.h>

#define DEBUG 1
#define SENSOR_DATA 0  // selects between sensor data and random data

#define DEVICE_NAME "E001"

Adafruit_FlashTransport_QSPI flashTransport;
HTU21D myHumidity;

SemaphoreHandle_t stopSemaphore = NULL;

// Variables to store sensor data
int16_t id = 0, temp_scaled, hum_scaled;

void stopCallBack() {
    #if DEBUG == 1
      Serial.print("Advertising done!!!");
    #endif
   xSemaphoreGive(stopSemaphore);
}

void QSPIF_sleep(void) {
  flashTransport.begin();
  flashTransport.runCommand(0xB9);
  delayMicroseconds(5);
  flashTransport.end();
}

void bleAdvInit(void) {
  Bluefruit.Advertising.setStopCallback(stopCallBack);
  Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED);
  Bluefruit.Advertising.restartOnDisconnect(false);
  Bluefruit.Advertising.setInterval(160, 160);  // Advertising interval (0.625 ms units)
  Bluefruit.Advertising.setFastTimeout(2);     // Fast advertising timeout
}


void start_adv(void) {
  
  #if DEBUG==1
    Serial.println("Adevertising!!!");
  #endif

  Bluefruit.Advertising.clearData();


  int16_t customData[] = {
    id,  // Header byte
    temp_scaled,
    hum_scaled,
  };

  // Add custom data to advertising packet
  Bluefruit.Advertising.addData(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, (uint8_t*)customData, sizeof(customData));


  // Add additional advertising data
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();

  if(!Bluefruit.Advertising.start(2)){
    #if DEBUG == 1
      Serial.print("adverteiment !!!!!!");
    #endif
    delay(1000);
  }
}

// Setup function
void setup() {

  #if DEBUG == 1
    Serial.begin(115200);
  #endif

  stopSemaphore = xSemaphoreCreateBinary();
  if (stopSemaphore == NULL) {
    #if DEBUG == 1
      Serial.println("Failed to create semaphore");
    #endif
    while (1) {
      
      #if DEBUG == 1
        Serial.println(".");
      #endif
    }
  }

  myHumidity.begin();

  if (!Bluefruit.begin()) {
    Serial.println("Bluefruit initialization failed!");
    while (1); // Halt execution
  }

  Bluefruit.autoConnLed(false);
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  Bluefruit.setTxPower(4);
  Bluefruit.setName(DEVICE_NAME);

  // setupCustomService();
  bleAdvInit();

  #if DEBUG == 1
    Serial.println("********** Setup Complete **********");
    delay(1000);
  #endif
}

// Loop function
void loop() {

  #if SENSOR_DATA == 1
    float temp = myHumidity.readTemperature();
    float hum = myHumidity.readHumidity();

    temp_scaled = temp * 100;
    hum_scaled = hum * 100;
  #else
    temp_scaled = random(-20, 20) * 100;
    hum_scaled = random(0, 80) * 100;
  #endif

  #if DEBUG == 1
    Serial.printf("temp: %d, hum: %d", temp_scaled, hum_scaled);
  #endif

  start_adv();


  if (xSemaphoreTake(stopSemaphore, portMAX_DELAY) == pdTRUE) {
    // Bluefruit.Advertising.stop(); // if advertisment is indefinte
  }
  
  delay(5000);
}
