#include <ESP32httpUpdate.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

#define DHTPIN 23
#define DHTTYPE DHTesp::DHT22

const char* ssid = "Filipe's S24 Ultra";
const char* password = "kratos1035";

// URL do servidor de atualização de firmware
const char* firmwareUpdateURL = "https://github.com/Filipe1034/Filipe-s-Projects.git";

const char* mqtt_server = "45.165.232.38"; // Endereço IP do broker MQTT
const int mqtt_port = 2883;   // Porta do broker MQTT
const char* mqtt_username = "meteop"; // Nome de usuário MQTT
const char* mqtt_password = "@Temp0#1o1";  // Senha MQTT

WiFiClient espClient;
Adafruit_MQTT_Client mqtt(&espClient, mqtt_server, mqtt_port, mqtt_username, mqtt_password);

DHTesp dht;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando-se a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void connect_mqtt() {
  Serial.print("Conectando ao broker MQTT...");
  int8_t ret;
  while ((ret = mqtt.connect()) != 0) { // Conecta ao broker MQTT
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Tentando novamente em 5 segundos...");
    mqtt.disconnect();
    delay(5000);  // Aguarda 5 segundos antes de tentar reconectar
  }
  Serial.println("Conectado ao broker MQTT!");
}

Adafruit_MQTT_Publish temperatura = Adafruit_MQTT_Publish(&mqtt, "casa/sensor/temperatura");
Adafruit_MQTT_Publish umidade = Adafruit_MQTT_Publish(&mqtt, "casa/sensor/umidade");

void checkForFirmwareUpdate() {
  t_httpUpdate_return ret = ESPhttpUpdate.update(firmwareUpdateURL);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("Atualização de firmware falhou (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("Nenhuma atualização disponível");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("Atualização de firmware bem-sucedida");
      break;
  }
}

void setup() {
  Serial.begin(9600);
  dht.setup(DHTPIN, DHTTYPE);
  setup_wifi();
}

void loop() {
  if (!mqtt.connected()) {
    connect_mqtt();
  }
  
  mqtt.processPackets(10000); // Processa os pacotes MQTT recebidos, no máximo, a cada 10 segundos

  delay(2000);
  
  float temperature = dht.getTemperature();
  float humidity = dht.getHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Falha ao ler o sensor DHT!");
    return;
  }

  Serial.println("Enviando dados via MQTT...");

  temperatura.publish(temperature - 0.09); // Publica a temperatura
  umidade.publish(humidity + 5.7);        // Publica a umidade
  
  Serial.println("Dados enviados:");
  Serial.print("Temperatura: ");
  Serial.print(temperature - 0.09);
  Serial.println(" °C");
  Serial.print("Umidade: ");
  Serial.print(humidity + 5.7);
  Serial.println(" %");
  
  // Verifica atualizações de firmware a cada 24 horas (86400000 milissegundos)
  static unsigned long lastFirmwareCheckTime = 0;
  if (millis() - lastFirmwareCheckTime >= 86400000) {
    checkForFirmwareUpdate();
    lastFirmwareCheckTime = millis();
  }
  
  delay(5000);
}
