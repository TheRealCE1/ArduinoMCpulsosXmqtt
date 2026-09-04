#include <Arduino_PortentaMachineControl.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi
const char* ssid = "Atotech";
const char* password = "smartfactory";

// Broker MQTT
// No agregues :1883 aquí
const char* mqtt_server = "192.168.6.3";
const int mqtt_port = 1883;

// MQTT
const char* mqtt_topic = "planta/maquina1/contador";
const char* mqtt_client_id = "PortentaMachineControl";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

bool estadoAnterior = false;
unsigned long contador = 0;

void conectarWiFi() {
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  unsigned long inicio = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - inicio < 30000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi conectado");

    Serial.print("IP del Arduino: ");
    Serial.println(WiFi.localIP());

    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
  } else {
    Serial.print("No se pudo conectar al WiFi. Estado: ");
    Serial.println(WiFi.status());
  }
}

void conectarMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando MQTT a ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.print(mqtt_port);
    Serial.println("...");

    if (mqttClient.connect(mqtt_client_id)) {
      Serial.println("MQTT conectado");
    } else {
      Serial.print("Error MQTT. Codigo: ");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  unsigned long inicioSerial = millis();

  while (!Serial && millis() - inicioSerial < 5000) {
    delay(10);
  }

  Serial.println();
  Serial.println("Iniciando Portenta Machine Control...");

  Wire.begin();

  if (!MachineControl_DigitalInputs.begin()) {
    Serial.println("ERROR: no se pudo inicializar Digital Input");

    while (true) {
      delay(1000);
    }
  }

  Serial.println("Entradas digitales inicializadas");

  conectarWiFi();

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Esperando conexion WiFi...");
    delay(5000);
    conectarWiFi();
  }

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setKeepAlive(60);

  conectarMQTT();

  Serial.println("ARDUINO_LISTO");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado");
    conectarWiFi();
    return;
  }

  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();

  bool estadoActual =
      MachineControl_DigitalInputs.read(DIN_READ_CH_PIN_00);

  if (estadoActual && !estadoAnterior) {
    contador++;

    char mensaje[20];
    snprintf(mensaje, sizeof(mensaje), "%lu", contador);

    bool publicado = mqttClient.publish(
      mqtt_topic,
      mensaje,
      true
    );

    if (publicado) {
      Serial.print("Pulso publicado: ");
      Serial.println(mensaje);
    } else {
      Serial.println("Error publicando el pulso MQTT");
    }
  }

  estadoAnterior = estadoActual;

  delay(5);
}