#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>

// Reemplaza con el nombre y la contraseña de tu red Wi-Fi
const char* ssid = "TINAJAS";
const char* password = "clarin2870";

// Definiciones de la pantalla
#define OLED_RESET -1
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);

  // Inicializa la pantalla
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Error al inicializar la pantalla OLED."));
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Conectando a WiFi...");
  display.display();

  // Conecta a la red Wi-Fi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Muestra la IP en la pantalla OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Conectado!");
  display.println("");
  display.println("IP:");
  display.println(WiFi.localIP());
  display.display();
}

void loop() {
  // Nada en el loop, la IP se muestra una vez en el setup
}