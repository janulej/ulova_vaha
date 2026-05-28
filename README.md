# Úľová váha
Zariadenie ktoré zbiera údaje o hmotnosti včelieho úľa každých 15 min. Monitorovanie hmotnosti včelstva je pre včelára veľmi nápomocná. Včelár má prehľad o tom čo sa deje v úli bez toho aby fyzicky zasahoval do včelstva. Súčasťou zariadenia sú aj senzory na meranie teploty, vlhkosti a atmosférického tlaku pri včelnici.

## Architektúra systému
```text
                                                                                                        
                                                                                                              CENTRÁLNY SERVER (Raspberry Pi 4B)
  ESP 32 (Úľová váha)                         ESP 32 (Gateway)                                                    
   SEN-10245 50 kg       LoRa (868 MHz)        - Príjem dát z LoRa         MQTT (Port 1883)        Mosquitto Broker   ----->               ThingsBoard
   BME 280              --------------->       - Pripojenie na WiFi        --------------->           (MQTT server)               (PostgreSQL db + Web Dashboard)
  
