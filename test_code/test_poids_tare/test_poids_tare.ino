#include "HX711.h"

#define LOADCELL_DOUT_PIN 0
#define LOADCELL_SCK_PIN 1

HX711 scale;

void setup() {
  Serial.begin(115200);
  while (!Serial);  

  Serial.println("Démarrage...");
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  delay(3000);  // Augmenter le délai pour la stabilité

  if (scale.is_ready()) {
    Serial.println("Capteur HX711 prêt !");
    scale.set_scale(13400);
    scale.tare();
    
    Serial.println("Mesure de la tare en cours...");
    delay(5000);  // Augmenter le temps de stabilisation

    long tareValue = scale.get_offset();
    Serial.print("Valeur de tare mesurée à vide : ");
    Serial.println(tareValue);
  } else {
    Serial.println("Erreur : Capteur HX711 non détecté !");
  }
}

void loop() {
  // Pas besoin de loop(), la tare est affichée une seule fois
}


//tare a vide = 114685
