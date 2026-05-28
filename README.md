# Úľová váha
Zariadenie ktoré zbiera údaje o hmotnosti včelieho úľa každých 15 min. Monitorovanie hmotnosti včelstva je pre včelára veľmi nápomocná. Včelár má prehľad o tom čo sa deje v úli bez toho aby fyzicky zasahoval do včelstva. Súčasťou zariadenia sú aj senzory na meranie teploty, vlhkosti a atmosférického tlaku pri včelnici.

## Architektúra systému
```text
                                                                                                              CENTRÁLNY SERVER (Raspberry Pi 4B)
                                                                                                                           ThingsBoard
  ESP 32 (Úľová váha)                         ESP 32 (Gateway)                                         Dátová vrstva                  Prezentacná vrstva
   SEN-10245 50 kg       LoRa (868 MHz)        - Príjem dát z LoRa       HTTP POST (Port 8080)      Integrovaná databáza   ----->        Web Dashboard
   BME 280              --------------->       - Pripojenie na WiFi      -------------------->         (PostgreSQL)                          Grafy
                                               - Konverzia na JSON
```
## Použitý hardvér a senzory

|Komponent|Popis|
|-------|--------|
|ESP 32-WROOM-32D|hlavná jednotka na zber údajov zo senzorov|
|ESP 32-WROOM-32D|vnútorná prijímacia jednotka, gateway|
|Raspberry Pi 4 Model B - 4GB RAM|zariadenie kde beží databáza a ThingsBoard|
|BME 280|senzor na meranie teploty, vlhkosti a atmosférického tlaku|
|SEN-10245 50 kg|4x hmotnostný deformačný senzor|
|AD Prevodník HX711|prevedenie analógového signálu zo senzorov hmotnosti na digitálny signál| 
|SX1276 Lora 868 MHz|komunikačný modul na bezdrôtové posielanie údajov zo senzora

## Inštalácia Raspberry Pi
### 1. Inštalácia OS
Nainštaluj si na SD kartu operačný systém Raspberry Pi OS Lite (64-bit) pomocou programu Raspberry Pi Imager. Po nainštalovaní vlož SD kartuž do Raspberry Pi a spusti ho. Následne sa pripoj na Raspberry Pi cez SSH.
```
# aktualizuj systém
sudo apt update && sudo apt upgrade -y

```
### 2. Inštalácia platformy ThingsBoard 
```
# inštalacia Java 21
sudo apt install openjdk-21-jdk -y

# stiahni si inštalačný balík
wget https://github.com/thingsboard/thingsboard/releases/download/v4.3.1.1/thingsboard-4.3.1.1.deb

# nainštaluj ThingsBoard
sudo dpkg -i thingsboard-4.3.1.1.deb
```
### 3. Inštalácia databázy
```
# nainštaluj PostgreSQL
sudo apt install postgresql postgresql-contrib -y

# spusť databázu a nastav, aby sa zapla sama po každom štarte
sudo systemctl start postgresql
sudo systemctl enable postgresql

# vytvor databázu
sudo -u postgres psql
CREATE DATABASE thingsboard;
\q
# prepoj ThingsBoard s databázou
sudo nano /etc/thingsboard/conf/thingsboard.conf
# na koniec nakopíruj tieto riadky a ulož
export SPRING_DATASOURCE_URL=jdbc:postgresql://localhost:5432/thingsboard
export SPRING_DATASOURCE_USERNAME=postgres

# spusti finálnu inštaláciu
sudo /usr/share/thingsboard/bin/install/install.sh --loadDemo

# spustenie ThingsBoard a nastavenie automatického štartu
sudo systemctl start thingsboard
sudo systemctl enable thingsboard
```

## Prístup na ThingsBoard
Prístup je cez webový prehliadač (na lokálnej sieti) na adrese
```
http://<IP Raspberry Pi>:8080
```

## Nahranie firmvéru do ESP jednotiek
### Poziadavky
- Arduino IDE
- Knižnice: `HX711_ADC`,`LoRa`,`Adafruit_BME280`

### ESP 1 úľ
Ako prvé je potrebné nahrať kalibračný kód do ESP a postupovať podľa požiadaviek v Serial monitore. `firmware/ul/kalibracia_vahy.ino`
Po nakalibrovaný môžeme nahrať do ESP finálny program. `firmware/ul/ul_esp1.ino`

### ESP 2 gateway
Do druhého ESP ktoré je umiestnené v dome nahráme kód `firmware/vnutorne_esp/ulovavaha_esp2.ino`a spustíme ho.

## Formát správ
Formát správy poslanej z ESP 1 (úľ) do ESP 2 (gateway) je (LoRa)
```
42.53,21.45,58.20,1013.25
```
Správa je čo najmenšia aby nedochádzalo k starte dát. ESP 2 vie, že hodnoty prišli v poradí váha, teplota, vlhkosť, tlak. Správa sa posiela každých 15 min.

Formát správy odoslanej z ESP 2 (gateway) do Raspberry Pi je (JSON)
```
{
  "vaha": 42.53,
  "teplota": 21.45,
  "vlhkost": 58.20,
  "tlak": 1013.25
}
```
Správa sa posiela hneď po príde z ESP 1 (úľ).
Jenotky k jednotlivým veličinám sú nastavené staticky v ThingsBoard pri tvorbe grafu.
