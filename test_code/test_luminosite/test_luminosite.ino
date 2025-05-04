#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Wire.begin();  // Initialise la communication I2C

  if (!lightMeter.begin()) {
    Serial.println("Erreur : Capteur SEN0562 non détecté !");
    while (1);
  }

  Serial.println("Capteur SEN0562 prêt !");
}

void loop() {
  float lux = lightMeter.readLightLevel();
  Serial.print("Luminosité : ");
  Serial.print(lux);
  Serial.println(" lux");
  delay(1000);
}
