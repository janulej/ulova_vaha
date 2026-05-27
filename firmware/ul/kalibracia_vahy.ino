// kalibračný kód

#include <HX711_ADC.h>
#include <EEPROM.h>

const int HX711_dout = 4; 
const int HX711_sck = 14;   

HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Adresy v EEPROM
const int eeprom_calFactor_address = 0; 
const int eeprom_tareOffset_address = 4; 

unsigned long t = 0;

void setup() {
  Serial.begin(115200); 
  delay(1000);
  Serial.println("\n--- Spúšťam KALIBRÁCIU S UKLADANÍM OFFSETU ---");

  EEPROM.begin(512); 

  LoadCell.begin();
  unsigned long stabilizingtime = 2000; 
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);

  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Chyba: Skontroluj káble k HX711!");
    while (1);
  }
  
  LoadCell.setCalFactor(1.0); 
  while (!LoadCell.update());
  calibrate(); 
}

void loop() {
  static boolean newDataReady = 0;
  if (LoadCell.update()) newDataReady = true;

  if (newDataReady) {
    if (millis() > t + 500) {
      float i = LoadCell.getData();
      Serial.print("Aktuálna hmotnosť: ");
      Serial.print(i, 3); 
      Serial.println(" kg");
      newDataReady = 0;
      t = millis();
    }
  }

  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay(); 
    else if (inByte == 'r') calibrate(); 
  }
}

void calibrate() {
  Serial.println("\n***");
  Serial.println("1. Odstráň všetko z váhy (úplne prázdna váha).");
  Serial.println("2. Pošli 't' pre vynulovanie (tare).");

  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 't') LoadCell.tareNoDelay();
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Váha je vynulovaná.");
      _resume = true;
    }
  }

  Serial.println("3. Polož na váhu známe závažie.");
  Serial.println("4. Zadaj hmotnosť do monitora a stlač Enter.");

  float known_mass = 0;
  _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print("Zadaná váha: ");
        Serial.println(known_mass);
        _resume = true;
      }
    }
  }

  LoadCell.refreshDataSet(); 
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass); 
  long newTareOffset = LoadCell.getTareOffset();

  Serial.print("Nová kalibračná hodnota: "); Serial.println(newCalibrationValue);
  Serial.print("Nový uložený Offset (nula): "); Serial.println(newTareOffset);
  Serial.println("Uložiť obidve hodnoty do EEPROM? y/n");

  _resume = false;
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
        
        // Zápis oboch hodnôt do EEPROM pre ESP32
        EEPROM.put(eeprom_calFactor_address, newCalibrationValue);
        EEPROM.put(eeprom_tareOffset_address, newTareOffset);
        EEPROM.commit();

        Serial.println("Všetko úspešne uložené do EEPROM.");
        _resume = true;
      }
      else if (inByte == 'n') {
        Serial.println("Neuložené.");
        _resume = true;
      }
    }
  }
  Serial.println("Kalibrácia ukončená");
}