//#define DEBUG  // commente cette ligne pour desactiver debug 

#include <MKRWAN.h>
#include <DHT.h>
#include <BH1750.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include "HX711.h"


// ====== Définition des broches =======
#define DHTTYPE DHT22
#define DHT_PIN_INT 7      // DHT22 intérieur
#define DHT_PIN_EXT 6      // DHT22 extérieur
#define ONE_WIRE_BUS 2     //Bus one wire sondes de températures
#define LOADCELL_DOUT_PIN 0  //broche capteur poids HX711
#define LOADCELL_SCK_PIN 1  // Broche horloge HX711
#define VBAT_PIN A0        // Broche de mesure tension batterie
#define DONE_PIN 5  // Broche Arduino reliée au TPL5110 "DONE"


// ====== Paramètres Batterie =======
#define DIVISEUR_RATIO (51.0 / 40.0)  // Ratio du diviseur de tension
#define VBAT_MAX 4.2                  // Tension pleine charge
#define VBAT_MIN 3.2                  // Tension critique

//=============parametre calibration poids ================
#define TARE_FIXE 114685  // Valeur mesurée à vide


// Indicateur de disponibilité des capteurs
bool hx711_detected = true;

// ========= Clés LoRaWAN ===========
String appEui = "1234567891023456";   //id 
String appKey = "F099D03575EE3A38EB4DEC3AA3D7575B";

// =========== Déclaration des capteurs ============
DHT dhtInt(DHT_PIN_INT, DHTTYPE);  
DHT dhtExt(DHT_PIN_EXT, DHTTYPE);
BH1750 lightMeter;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
HX711 scale;
LoRaModem modem;

// ========== Adresses des capteurs OneWire ==========
DeviceAddress addrGauche = {0x28, 0x83, 0x27, 0x54, 0x0A, 0x00, 0x00, 0xB8};   //adresse one wire gauche
DeviceAddress addrDroite = {0x28, 0xAF, 0x29, 0x31, 0x0C, 0x00, 0x00, 0x11};    //adresse one wire droite

// ============ Calibration HX711 ==========
float calibration_factor = 13400;

// ========== Gestion du temps ==========
unsigned long previousMillis = 0;
uint32_t interval = 20000;  // Temps par défaut (20 sec)

// ========== Prototypes ==========
void readDHT(DHT &dht, float &temperature, float &humidite, String lieu);
float readLuminosite();
void readWire(float &tempGauche, float &tempDroite);
float readPoids();
float readBatterieVoltage();
int readBatterieLevel();
void sendLoRaData(float tempInt, float humInt, float tempExt, float humExt, float lux, float tempGauche, float tempDroite, float poids, int batterieLevel);
void connectLoRaWAN();

//============= DEbug =================
#ifdef DEBUG
  #define debugPrint(...) Serial.print(__VA_ARGS__)
  #define debugPrintln(...) Serial.println(__VA_ARGS__)
#else
  #define debugPrint(x)
  #define debugPrintln(x)
#endif


// ========= Connexion LoRaWAN ==========
void connectLoRaWAN() {


  debugPrintln("Connexion à LoRaWAN...");
 

  int attempts = 0;
  while (!modem.joinOTAA(appEui, appKey) && attempts < 5) {
    debugPrintln("Échec, nouvelle tentative...");
    attempts++;
    delay(5000);
  }

  if (attempts >= 5) {
    debugPrintln("Impossible de se connecter, redémarrage...");
    NVIC_SystemReset(); //Redémarrage forcé en cas d'échec
  }

  debugPrintln("Connecté à LoRaWAN !");
}

//================= SETUP =====================
void setup() {
  Serial.begin(115200);
  #ifdef DEBUG
    while (!Serial);  // Attendre l'ouverture du moniteur série
  #endif

 // delay(500);  // Évite un crash si pas branché à un PC

  dhtInt.begin();
  dhtExt.begin();
  sensors.begin();
  Wire.begin();

  // vérification capteur de luminosité BH1750
  if (!lightMeter.begin()) {
    debugPrintln("Capteur BH1750 non détecté !");
  } else debugPrintln("BH1750 prêt !");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  delay(1000); // Attendre 5 secondes avant tare pour éviter les valeurs négatives
  
  if (scale.is_ready()) {  // Vérifier si le capteur HX711 est détecté
    //scale.tare();  // Remettre à zéro
    scale.set_scale(calibration_factor); // Appliquer la calibration
    scale.set_offset(TARE_FIXE); //on applique la tare fixe
    hx711_detected = true;  // Marquer comme détecté
    debugPrintln("HX711 prêt !");
  } else {
    hx711_detected = false;  // Marquer comme absent
    debugPrintln("Erreur : Capteur HX711 non détecté !");
  }

  //tpl5110
  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, LOW); // Assure que le timer fonctionne au démarrage
  

  // initialisation module LoRa
  delay(5000);

  if (!modem.begin(EU868)) {
    debugPrintln("Module LoRa non détecté !");
    while (1);
  }
  connectLoRaWAN();
  modem.minPollInterval(20);
}

// ========== Lecture DHT22 ==========
void readDHT(DHT &dht, float &temperature, float &humidite, String lieu) {
  temperature = dht.readTemperature();
  humidite = dht.readHumidity();

  if (isnan(temperature) || isnan(humidite)) {
    debugPrint("Erreur DHT ("); debugPrint(lieu); debugPrintln(")");
    temperature = -999;   //valeur d'erreur temperature
    humidite = -999;      //valeur d'erreur humidite
  } else {
    debugPrint(lieu); debugPrint(" | Température : "); debugPrint(temperature);
    debugPrint(" °C | Humidité : "); debugPrint(humidite); debugPrintln(" %");
  }
}

// ========== Lecture Luminosité ==========
float readLuminosite() {
  float lux = lightMeter.readLightLevel();

  if (lux < 0) {
    debugPrintln("Erreur BH1750 !");
    return -999;      //valeur d'erreur luminosite
  }

  debugPrint("Luminosité : "); debugPrint(lux); debugPrintln(" lux");

  return lux;
}

// ========== Lecture Températures DS18B20 ==========
void readWire(float &tempGauche, float &tempDroite) {
  sensors.requestTemperatures();  // demande  mise à jour des températures
  
  //Lecture des capteurs identifiés par leur adresse unique
  tempGauche = sensors.getTempC(addrGauche);
  tempDroite = sensors.getTempC(addrDroite);

  if (tempGauche == DEVICE_DISCONNECTED_C) {
    debugPrintln("Erreur : Capteur DS18B20 Gauche non détecté !");
    tempGauche = -999;  // Valeur d'erreur
  }

  if (tempDroite == DEVICE_DISCONNECTED_C) {
    debugPrintln("Erreur : Capteur DS18B20 Droite non détecté !"); 
    tempDroite = -999;  // Valeur d'erreur
  }

   //affichage  valeurs lues
  debugPrint("Température Gauche : "); debugPrint(tempGauche); debugPrintln(" °C");
  debugPrint("Température Droite : "); debugPrint(tempDroite); debugPrintln(" °C");
}

// ========== Lecture Poids ==========
float readPoids() {

  //if (!scale.is_ready()) {
  if (!hx711_detected) {
    debugPrintln("Erreur capteur de poids HX711 !");
    return 0;
  }

  if (!scale.is_ready()) {  // Vérifie si le capteur est prêt avant de lire
    debugPrintln("Erreur : Capteur HX711 non prêt !");
    return -999;  // Valeur d'erreur
  }

  float poids =  scale.get_units() / 2.2046;  //conversion poids Ibs -> kg

  // Affichage du poids mesuré
  debugPrint("Poids : "); debugPrint(poids); debugPrintln(" kg");
  return (poids < 0) ? 0 : poids;   //si valeur negative mise a 0
  
}

// ========== Lecture Batterie ==========
float readBatterieVoltage() {
  analogReadResolution(12);  // Définit une résolution de 12 bits 
  int raw = analogRead(VBAT_PIN);     //lecture tension brute (ADC (0 - 4095))
  float vBat = ((raw / 4095.0) * 3.3) * DIVISEUR_RATIO;   //conversion en Volt
  
  // vérification des valeurs anormales
  if (vBat < 3.1 || vBat > 4.3) {
    debugPrintln("Erreur : Valeur de tension batterie incohérente !");

    return -999;  // Valeur d'erreur
  } else {
  #ifdef Debug  
    debugPrint("Tension Batterie : "); debugPrint(vBat, 2); debugPrintln(" V");
  #endif
  return vBat;
  }
}


//Calcul niveau batterie (%)
int readBatterieLevel() {
  float vBat = readBatterieVoltage();
  if (vBat == -999) return -999;  // Gestion des erreurs

  // Niveau batterie : 100% à 4.2V, 0% à 3.2V
  int niveau = (int)((vBat - 3.2) / (4.2 - 3.2) * 100.0);

  //Forcer la valeur entre 0 et 100%
  niveau = constrain(niveau, 0, 100);

  debugPrint("Niveau Batterie : ");
  if (niveau == -999) debugPrintln("Erreur de lecture !");
  else {
    debugPrint(niveau); debugPrintln(" %");
  }

  return niveau;
}

// ========== Envoi des données LoRa ==========
void sendLoRaData(float tempInt, float humInt, float tempExt, float humExt, float lux, float tempGauche, float tempDroite, float poids, int batterieLevel) {
  uint8_t payload[17];    // tableau pour stocker les données à envoyer


  // Fonction d'encodage des donnéesa en entiers signés (int16_t)
  auto encodeData = [](float value) -> int16_t {
    return (value == -999) ? 0x7FFF : (int16_t)(value * 100);  // 0x7FFF indique une erreur
  };

  //modif ajouté
  uint16_t luxEncoded = (lux > 65535) ? 65535 : (uint16_t)lux;  // sécurité en cas de dépassement


  //stockage des données dans le payload
  payload[0]  = highByte(encodeData(tempInt)); payload[1]  = lowByte(encodeData(tempInt));
  payload[2]  = highByte(encodeData(humInt)); payload[3]  = lowByte(encodeData(humInt));
  payload[4]  = highByte(encodeData(tempExt)); payload[5]  = lowByte(encodeData(tempExt));
  payload[6]  = highByte(encodeData(humExt)); payload[7]  = lowByte(encodeData(humExt));
  payload[8]  = highByte(luxEncoded); payload[9]  = lowByte(luxEncoded); //rectification
  payload[10] = highByte(encodeData(tempGauche)); payload[11] = lowByte(encodeData(tempGauche));
  payload[12] = highByte(encodeData(tempDroite)); payload[13] = lowByte(encodeData(tempDroite));
  payload[14] = highByte((int)(poids * 100));      payload[15] = lowByte((int)(poids * 100));
  payload[16] = (batterieLevel == -999) ? 255 : batterieLevel;  //niveau batterie (255 = erreur)

  debugPrintln("Envoi des données LoRa...");
  modem.beginPacket();
  modem.write(payload, sizeof(payload));
  int status = modem.endPacket(true);

  if (status > 0) debugPrintln(" Données envoyées !");
  else {
    debugPrintln("Échec d'envoi, tentative de reconnexion...");
    connectLoRaWAN();  
  }
}


// ========== Boucle principale ==========
void loop() {

  if (millis() - previousMillis >= interval) {
    previousMillis = millis();

    //déclaration des variables de mesure
    float tempInt, humInt, tempExt, humExt, lux, tempG, tempD, poids;
    int batterieLevel;

    //lecture de capteurs 
    readDHT(dhtInt, tempInt, humInt, "Intérieur");  // Température & humidité intérieure
    readDHT(dhtExt, tempExt, humExt, "Extérieur");  // Température & humidité extérieure
    lux = readLuminosite();   // Luminosité ambiante
    readWire(tempG, tempD);   // Températures des sondes OneWire (gauche & droite)
    poids = readPoids();      // Poids de la ruche
    batterieLevel = readBatterieLevel();     // Niveau de batterie (%)

    // Envoi des données via LoRaWAN
    sendLoRaData(tempInt, humInt, tempExt, humExt, lux, tempG, tempD, poids, batterieLevel);

    delay(15000); // Attendre
    digitalWrite(DONE_PIN, HIGH); // Envoie un signal DONE pour couper la LED
    delay(100); // Laisse le temps au TPL5110 de traiter le signal
    digitalWrite(DONE_PIN, LOW); // Remet le signal à l'état bas
  }
}
