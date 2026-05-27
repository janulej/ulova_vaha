// esp 2

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>

// NASTAVENIE WI-FI & RASPBERRY PI (THINGSBOARD)
const char* ssid = "***";
const char* password = "***";

// IP ADRESA RASPBERRY PI
const char* raspberry_ip = "***"; 

// prístupový token do ThingsBoard
const char* access_token = "***";

// KONFIGURÁCIA PIN_OUT pre LoRa 
#define SCK_LORA  33
#define MISO_LORA 34
#define MOSI_LORA 25
#define SS_LORA   32
#define RST_LORA  26
#define DIO0_LORA 27
#define LED_PIN   2 

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // 1. Pripojenie k Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Pripájam sa na Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi pripojená!");
  Serial.print("IP adresa tohto ESP: ");
  Serial.println(WiFi.localIP());

  // 2. Inicializácia SPI a LoRa
  SPI.begin(SCK_LORA, MISO_LORA, MOSI_LORA, SS_LORA);
  LoRa.setPins(SS_LORA, RST_LORA, DIO0_LORA);

  Serial.println("Štartujem LoRa prijímač...");
  if (!LoRa.begin(868E6)) {
    Serial.println("Chyba prijímača! Skontroluj zapojenie.");
    while (1);
  }

  // Parametre LoRa
  LoRa.setSyncWord(0x34); 
  LoRa.setSpreadingFactor(12); 
  LoRa.setSignalBandwidth(125E3); 
  LoRa.setCodingRate4(8); 
  LoRa.enableCrc();

  Serial.println("Prijímač pripravený. Čakám na hodinový balík dát z úľa...");
}

void loop() {
  // Skontrolujeme, či prišiel balík dát cez LoRa
  int packetSize = LoRa.parsePacket();
  
  if (packetSize) {
    digitalWrite(LED_PIN, HIGH); // Blikneme LED, že prijímame dáta
    
    String prijataSprava = "";

    // Prečítame obsah balíka
    while (LoRa.available()) {
      prijataSprava += (char)LoRa.read();
    }

    Serial.println("------------------------------------");
    Serial.print("Prijatý reťazec z úľa: ");
    Serial.println(prijataSprava);

    // rozdelenie retazca vaha,teplota,vlhkost,tlak
    int prvaCiarka = prijataSprava.indexOf(',');
    int druhaCiarka = prijataSprava.indexOf(',', prvaCiarka + 1);
    int tretiaCiarka = prijataSprava.indexOf(',', druhaCiarka + 1);

    if (prvaCiarka != -1 && druhaCiarka != -1 && tretiaCiarka != -1) {
      String vaha = prijataSprava.substring(0, prvaCiarka);
      String teplota = prijataSprava.substring(prvaCiarka + 1, druhaCiarka);
      String vlhkost = prijataSprava.substring(druhaCiarka + 1, tretiaCiarka);
      String tlak = prijataSprava.substring(tretiaCiarka + 1);

      // Výpis do sériového monitora pre kontrolu
      Serial.println("Spracované údaje:");
      Serial.print("  Váha:     "); Serial.print(vaha); Serial.println(" kg");
      Serial.print("  Teplota:  "); Serial.print(teplota); Serial.println(" °C");
      Serial.print("  Vlhkosť:  "); Serial.print(vlhkost); Serial.println(" %");
      Serial.print("  Tlak:     "); Serial.print(tlak); Serial.println(" hPa");

      // POSLANIE DÁT NA RASPBERRY PI
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        
        // Vytvorenie cieľovej URL adresy smerom na Raspberry Pi
        String url = String(raspberry_ip) + String(access_token) + "/telemetry";
        http.begin(url);
        
        // Nastavenie HTTP hlavičky pre JSON dáta, ktoré ThingsBoard vyžaduje
        http.addHeader("Content-Type", "application/json");
        
        // Vytvorenie JSON balíka
        String jsonPayload = "{";
        jsonPayload += "\"vaha\":" + vaha + ",";
        jsonPayload += "\"teplota\":" + teplota + ",";
        jsonPayload += "\"vlhkost\":" + vlhkost + ",";
        jsonPayload += "\"tlak\":" + tlak; 
        jsonPayload += "}";
        
        Serial.print("Odosielam JSON na Raspberry Pi: ");
        Serial.println(jsonPayload);
        
        // Odoslanie dát pomocou POST requestu
        int httpResponseCode = http.POST(jsonPayload);
        
        if (httpResponseCode > 0) {
          Serial.print("ThingsBoard potvrdil príjem. Kód odpovede: ");
          Serial.println(httpResponseCode);
        } else {
          Serial.print("Chyba pri odosielaní na Raspberry: ");
          Serial.println(httpResponseCode);
        }
        
        http.end(); // Zatvoríme HTTP spojenie
      } else {
        Serial.println("Chyba: ESP stratilo Wi-Fi pripojenie, nedá sa odoslať na Raspberry.");
      }

    } else {
      Serial.println("Chyba: Neplatný formát dát z LoRa.");
    }

    digitalWrite(LED_PIN, LOW);
  }
}
