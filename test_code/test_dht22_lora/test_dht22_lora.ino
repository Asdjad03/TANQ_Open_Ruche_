#include <MKRWAN.h>
#include <DHT.h>

// Définition du capteur DHT22
#define DHTPIN 7        // Broche où est connecté le capteur
#define DHTTYPE DHT22   // Type du capteur

DHT dht(DHTPIN, DHTTYPE);
LoRaModem modem;

// Identifiants LoRa (remplace avec tes vraies clés)
String appEui = "1234567891023456";  
String appKey = "F099D03575EE3A38EB4DEC3AA3D7575B";  

void connectLoRaWAN() {
  Serial.println(" Tentative de connexion à LoRaWAN...");
  while (!modem.joinOTAA(appEui, appKey)) {
    Serial.println(" Échec de connexion, nouvelle tentative dans 10 sec...");
    delay(10000);  // Attente avant une nouvelle tentative
  }
  Serial.println(" Connecté à LoRaWAN !");
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Initialisation du capteur DHT...");
  dht.begin(); 

  Serial.println("Démarrage du module LoRa...");
  if (!modem.begin(EU868)) {  
    Serial.println(" Échec de démarrage du module LoRa !");
    while (1);
  }
  Serial.println(" Module LoRa détecté !");
  
  // Connexion LoRaWAN avec re-tentatives
  connectLoRaWAN();
  
  // Définition du temps minimal entre envois 
  modem.minPollInterval(30);
}

void loop() {
  Serial.println("Lecture des données du DHT22...");

  float temperature = dht.readTemperature();
  float humidite = dht.readHumidity();

  if (isnan(temperature) || isnan(humidite)) {
    Serial.println(" Erreur de lecture du capteur DHT !");
    return;
  }

  Serial.print(" Température : ");
  Serial.print(temperature);
  Serial.print(" °C,  Humidité : ");
  Serial.print(humidite);
  Serial.println(" %");

  // Conversion des valeurs en entiers (ex: 23.45°C → 2345)
  int temp = (int)(temperature * 100);
  int hum = (int)(humidite * 100);

  // Construction du message sous format binaire (4 octets)
  uint8_t payload[4];
  payload[0] = highByte(temp);
  payload[1] = lowByte(temp);
  payload[2] = highByte(hum);
  payload[3] = lowByte(hum);

  Serial.println("Envoi des données LoRa...");
  modem.beginPacket();
  modem.write(payload, sizeof(payload));
  int status = modem.endPacket(true);

  if (status > 0) {
    Serial.println("Données envoyées avec succès !");
  } else {
    Serial.println("Échec d'envoi, tentative de reconnexion...");
    connectLoRaWAN();  // Reconnexion automatique en cas d'échec
  }

  delay(30000);  // Envoi toutes les 30 secondes
}
