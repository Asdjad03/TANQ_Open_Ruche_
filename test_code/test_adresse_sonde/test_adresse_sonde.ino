#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2  // Broche de connexion des capteurs
const long interval = 30000;  
unsigned long previousMillis = 0;  

OneWire oneWire(ONE_WIRE_BUS);  
DallasTemperature sensors(&oneWire);  
DeviceAddress deviceAddress;  // Pour stocker l'adresse d'un capteur

// Fonction pour afficher l'adresse d'un capteur
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");  // Ajoute un zéro si < 16 pour un format uniforme
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) Serial.print(":");  // Ajoute les deux-points entre les octets
  }
}

// Fonction pour lire et afficher les températures
void readSensors() {
  Serial.println("\nLecture des capteurs...");

  int deviceCount = sensors.getDeviceCount();
  Serial.print("Nombre de capteurs trouvés : ");
  Serial.println(deviceCount);

  for (int i = 0; i < deviceCount; i++) {
    if (sensors.getAddress(deviceAddress, i)) {
      Serial.print("Capteur "); Serial.print(i); Serial.print(" - Adresse : ");
      printAddress(deviceAddress);

      sensors.requestTemperatures();  // Demande la température
      float tempC = sensors.getTempC(deviceAddress);  // Lit la température

      if (tempC != DEVICE_DISCONNECTED_C) {
        Serial.print(" |  Température : "); Serial.print(tempC); Serial.println(" °C");
      } else {
        Serial.println(" | Erreur de lecture !");
      }
    } else {
      Serial.print(" Impossible de trouver l'adresse du capteur "); Serial.println(i);
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);  // Attend l'ouverture du port série

  sensors.begin();  // Initialisation des capteurs
  Serial.println("Système prêt !");
  readSensors();  // Première lecture au démarrage
}

void loop() {
  unsigned long currentMillis = millis();

  // Si 30 secondes se sont écoulées, relance la lecture
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Met à jour le temps
    readSensors();  // Effectue la lecture des capteurs
  }
}
