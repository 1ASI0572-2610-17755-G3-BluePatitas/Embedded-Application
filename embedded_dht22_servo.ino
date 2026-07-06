#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "DHTesp.h"
#include <ESP32Servo.h>

// Configuración de tu red Wi-Fi
const char* ssid = "AndroidP5";
const char* password = "mmhg9502";

// CONFIGURACIÓN DE TU API (IP del PC corriendo el Gateway)
const String serverIP = "http://10.111.91.94:18090"; 

const int DHT_PIN = 23;     // Pin P4 para datos del DHT22
const int SERVO_PIN = 18;  // Pin P13 para señal del Servo

DHTesp dhtSensor;
Servo dispensadorServo;

unsigned long ultimoEnvioSensors = 0;
const unsigned long intervaloSensors = 10000; // Envia sensores cada 10 segundos

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== INICIANDO ESP32 DISPENSADOR/SENSOR ===");
  
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  dispensadorServo.attach(SERVO_PIN, 500, 2400);
  dispensadorServo.write(90); // Posición inicial cerrado

  WiFi.begin(ssid, password);
  Serial.print("Conectando a Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n¡Conectado a la red!");
  Serial.print("Dirección IP asignada al ESP32: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    unsigned long tiempoActual = millis();

    // TAREA 1: Enviar datos del DHT22 cada 10 segundos
    if (tiempoActual - ultimoEnvioSensors >= intervaloSensors) {
      ultimoEnvioSensors = tiempoActual;
      enviarDatosSensor();
    }

    // TAREA 2: Revisar si hay orden de comida (Cada 2 segundos)
    revisarOrdenComida();
  } else {
    Serial.println("⚠️ Wi-Fi Desconectado, intentando reconectar...");
    WiFi.begin(ssid, password);
  }
  
  delay(2000); // Sincronización básica del ciclo
}

void enviarDatosSensor() {
  Serial.println("\n--- TAREA: Intentando leer y enviar sensores ---");
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  
  Serial.print("Lectura cruda -> Temp: ");
  Serial.print(data.temperature);
  Serial.print(" °C | Humedad: ");
  Serial.print(data.humidity);
  Serial.println(" %");

  if (isnan(data.temperature) || isnan(data.humidity)) {
    Serial.println("❌ ERROR: Fallo en la lectura del DHT22.");
    Serial.println("   Verifique que el sensor esté bien alimentado (VCC a 3.3V/5V, GND a GND)");
    Serial.println("   y que el pin de datos esté conectado al pin 23.");
    return;
  }

  HTTPClient http;
  String targetURL = serverIP + "/api/telemetria";
  Serial.print("Conectando al Edge Gateway en: ");
  Serial.println(targetURL);
  
  http.begin(targetURL);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["temperatura"] = data.temperature;
  doc["humedad"] = data.humidity;

  String jsonString;
  serializeJson(doc, jsonString);
  Serial.print("Cuerpo del POST JSON: ");
  Serial.println(jsonString);

  int responseCode = http.POST(jsonString);
  if (responseCode > 0) {
    Serial.printf("✅ [POST] Sensores enviados. Código HTTP del Edge: %d\n", responseCode);
    String responseBody = http.getString();
    Serial.print("Respuesta del Edge: ");
    Serial.println(responseBody);
  } else {
    Serial.printf("❌ [POST] Error de conexión HTTP: %s\n", http.errorToString(responseCode).c_str());
  }
  http.end();
  Serial.println("-----------------------------------------------\n");
}

void revisarOrdenComida() {
  HTTPClient http;
  String targetURL = serverIP + "/api/dispensador/status";
  
  http.begin(targetURL);
  int responseCode = http.GET();
  if (responseCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    deserializeJson(doc, payload);
    
    bool activar = doc["activar"];
    if (activar) {
      Serial.println("[GET] ¡Orden de comida detectada! Moviendo servo...");
      
      // Mover servo para dispensar
      dispensadorServo.write(180); 
      delay(1000); 
      dispensadorServo.write(90);
      
      // Notificar al backend que ya se entregó para apagar la alerta
      confirmarEntregaAlimento();
    }
  } else if (responseCode < 0) {
    Serial.printf("[GET] Error de conexión en dispensador: %s\n", http.errorToString(responseCode).c_str());
  }
  http.end();
}

void confirmarEntregaAlimento() {
  HTTPClient http;
  String targetURL = serverIP + "/api/dispensador/confirmar";
  http.begin(targetURL);
  int responseCode = http.POST(""); // POST vacío para confirmar
  if (responseCode > 0) {
    Serial.printf("[POST] Entrega confirmada al servidor. Código HTTP: %d\n", responseCode);
  } else {
    Serial.printf("[POST] Error al confirmar entrega: %s\n", http.errorToString(responseCode).c_str());
  }
  http.end();
}
