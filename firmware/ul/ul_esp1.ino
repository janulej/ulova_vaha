// esp 1

#include <HX711_ADC.h>
#include <EEPROM.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// konfiguracia HX711
const int HX711_dout = 4; 
const int HX711_sck = 14;   
HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Adresy v EEPROM
const int eeprom_calFactor_address = 0; 
const int eeprom_tareOffset_address = 4; 


// konfiguracia LoRa
#define SCK_LORA  33
#define MISO_LORA 34
#define MOSI_LORA 25
#define SS_LORA   32
#define RST_LORA  26
#define DIO0_LORA 27
#define LED_PIN   2 

// BME280 nastavenie
Adafruit_BME280 bme; 

//  nastavenie casu spanku
#define TIME_TO_SLEEP  900         // 15 min = 900 s
#define uS_TO_S_FACTOR 1000000ULL  // Konverzný faktor na mikrosekundy

void setup() {
  Serial.begin(115200);
  delay(500); 
  Serial.println("\n--- ESP32 sa zobudilo, štartujem cyklus merania ---");

  // Nastavenie prebúdzania časovačom
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  pinMode(LED_PIN, OUTPUT);

  // Inicializácia BME280
  if (!bme.begin(0x76)) { 
    Serial.println("Chyba: BME280 nenájdený! Skontroluj zapojenie.");
    chodDoSpanku(); 
  }

  // Inicializácia EEPROM a načítanie hodnôt
  EEPROM.begin(512);
  float calibrationValue;
  long tareOffsetValue;
  
  EEPROM.get(eeprom_calFactor_address, calibrationValue);
  EEPROM.get(eeprom_tareOffset_address, tareOffsetValue);

  // vypisanie kalibračného faktora
  Serial.println(calibrationValue);
  Serial.print("Načítaný Offset (Nula) z EEPROM: "); Serial.println(tareOffsetValue);

  // Inicializácia HX711
  LoadCell.begin();
  unsigned long stabilizingtime = 2000; 
  boolean _tare = false;
  LoadCell.start(stabilizingtime, _tare);

  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Chyba: Nenájdený senzor HX711. Odchádzam spať.");
    chodDoSpanku();
  }

  // aplikovanie hodnôt z EEPROM
  LoadCell.setCalFactor(calibrationValue); 
  LoadCell.setTareOffset(tareOffsetValue);
  
  // Čakáme na ustálenie napätia a načítanie kĺzavého priemeru
  Serial.println("Čakám na ustálenie napätia na tenzometri...");
  unsigned long ustlenieStart = millis();
  while (millis() - ustlenieStart < 1000) { 
    LoadCell.update();
  }

  int pokusy = 0;
  while (!LoadCell.update() && pokusy < 100) {
    delay(10);
    pokusy++;
  }

  // Zber všetkých údajov zo senzorov
  float vahaUla = LoadCell.getData();
  float teplota = bme.readTemperature();
  float vlhkost = bme.readHumidity();
  float tlak = bme.readPressure() / 100.0F;

  // Výpis nameraných hodnôt do Serial Monitora
  Serial.println("\n--- NAMERANÉ ÚDAJE ---");
  Serial.print("Váha: "); Serial.print(vahaUla, 3); Serial.println(" kg");
  Serial.print("Teplota: "); Serial.print(teplota); Serial.println(" °C");
  Serial.print("Vlhkosť: "); Serial.print(vlhkost); Serial.println(" %");
  Serial.print("Tlak: "); Serial.print(tlak); Serial.println(" hPa");

  // Inicializácia SPI a LoRa vysielača
  SPI.begin(SCK_LORA, MISO_LORA, MOSI_LORA, SS_LORA);
  LoRa.setPins(SS_LORA, RST_LORA, DIO0_LORA);

  Serial.println("Štartujem LoRa...");
  int loraTimeout = 0;
  while (!LoRa.begin(868E6) && loraTimeout < 10) {
    Serial.print(".");
    delay(100); 
    loraTimeout++;
  }

  if (loraTimeout >= 10) {
    Serial.println("Chyba: LoRa sa nespustila! Idem spať.");
    chodDoSpanku();
  }

  LoRa.setSyncWord(0x34); 
  LoRa.setSpreadingFactor(12); 
  LoRa.setSignalBandwidth(125E3); 
  LoRa.setCodingRate4(8); 
  LoRa.enableCrc();

  // Odoslanie dát cez LoRa
  digitalWrite(LED_PIN, HIGH);
  Serial.println("Odosielam paket cez LoRa...");

  LoRa.beginPacket();
  LoRa.print(vahaUla, 2); 
  LoRa.print(",");
  LoRa.print(teplota);
  LoRa.print(",");
  LoRa.print(vlhkost);
  LoRa.print(",");
  LoRa.print(tlak);
  LoRa.endPacket();
  
  digitalWrite(LED_PIN, LOW);
  Serial.println("Dáta úspešne odoslané.");
  Serial.flush(); 
  
  chodDoSpanku();
}

void loop() {
  // Prázdne
}

void chodDoSpanku() {
  LoadCell.powerDown(); 
  LoRa.end(); 
  esp_deep_sleep_start();
}
