#include <OneWire.h>
#include <DallasTemperature.h>


#define ONE_WIRE_BUS 2 // Utilisation de la broche D2 (PA10)

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  while (!Serial);
  sensors.begin();


  }

void sonde(int port) {
  // Demande la lecture des températures sur tous les capteurs
  sensors.requestTemperatures();
  
  // Récupère le nombre de capteurs détectés
  int deviceCount = sensors.getDeviceCount();
  
  // Parcourt chaque capteur et affiche sa température
  for (int i = 0; i < deviceCount; i++) {
    float tempC = sensors.getTempCByIndex(i); // Lecture de la température du capteur i
    Serial.print("Capteur ");
    Serial.print(i);
    Serial.print(" - Température: ");
    Serial.print(tempC);
    Serial.println(" °C");
  }
}


void loop() { 
  sonde(ONE_WIRE_BUS);
}
