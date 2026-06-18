#include <WiFi.h>
#include <HTTPClient.h>

// Datos del hotspot
const char* ssid = "AndroidP5";
const char* password = "mmhg9502";

// Edge Gateway
const char* serverUrl = "http://10.138.178.94:18090/location";

unsigned long lastSent = 0;
// Coordenadas de prueba estáticas (ejemplo: Lima, Perú)
double mockLat = -12.046374;
double mockLng = -77.042793;

void setup() {
  Serial.begin(115200);
  Serial.println("=== Iniciando Simulador GPS ===");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n¡WiFi conectado!");
}

void loop() {
  // Enviar datos cada 2 segundos
  if (millis() - lastSent >= 2000) {
    // ELIMINADO: Ya no variamos mockLat ni mockLng para que la ubicación sea estática

    Serial.println();
    Serial.println("===== ENVIANDO UBICACIÓN SIMULADA ESTÁTICA =====");
    Serial.print("Latitud: ");
    Serial.println(mockLat, 6);
    Serial.print("Longitud: ");
    Serial.println(mockLng, 6);

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");

      String json = "{";
      json += "\"latitude\":";
      json += String(mockLat, 6);
      json += ",";
      json += "\"longitude\":";
      json += String(mockLng, 6);
      json += "}";

      int httpResponseCode = http.POST(json);
      Serial.print("Código HTTP del Edge: ");
      Serial.println(httpResponseCode);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Respuesta del Edge: " + response);
      } else {
        Serial.print("Error al enviar: ");
        Serial.println(http.errorToString(httpResponseCode));
      }
      http.end();
    } else {
      Serial.println("WiFi desconectado.");
    }
    lastSent = millis();
  }
}
