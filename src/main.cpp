#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>

// Definir as credenciais do Wi-Fi
#define WIFI_SSID "UPE-Estudantes(UnL)"
#define WIFI_PASSWORD "OrgulhodeserUPE"

// Definir as credenciais do Firebase
#define DATABASE_URL "https://lixeirainteligente-d0a1f-default-rtdb.firebaseio.com/" // <databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app
#define DATABASE_SECRET "gkXg8YMeqfiW2Cmgg8c2mshFK5C563pbLp2ZQ5q9"

// Definir pinos e variáveis globais
#define LED_PIN_RED 5
#define LED_PIN_YELLOW 18
#define LED_PIN_GREEN 19
#define TRIG_PIN 2
#define ECHO_PIN 4
#define BUTTON_PIN 12
#define POT_PIN 35

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long dataMillis = 0;
bool alertActive = false;
unsigned long alertStartMillis = 0;

void setup() {
    // Iniciar a comunicação serial
    Serial.begin(115200);

    // Configurar os pinos dos LEDs como saída
    pinMode(LED_PIN_RED, OUTPUT);
    pinMode(LED_PIN_YELLOW, OUTPUT);
    pinMode(LED_PIN_GREEN, OUTPUT);

    // Configurar os pinos do sensor ultrassônico
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // Configurar o pino do botão como entrada com pull-up interno
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Configurar o pino do potenciômetro como entrada
    pinMode(POT_PIN, INPUT);

    // Conectar ao Wi-Fi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    // Configurar o Firebase
    config.database_url = DATABASE_URL;
    config.signer.tokens.legacy_token = DATABASE_SECRET;

    Firebase.reconnectNetwork(true);
    fbdo.setBSSLBufferSize(4096, 4096); // Atualizado para usar dois argumentos
    Firebase.begin(&config, &auth);
}

long getDistance() {
    // Enviar pulso de 10us ao pino TRIG
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Ler o tempo do pulso ECHO em microsegundos
    long duration = pulseIn(ECHO_PIN, HIGH);

    // Calcular a distância em centímetros
    long distance = duration * 0.034 / 2;

    return distance;
}

void loop() {
    // Verificar se o botão foi pressionado
    if (digitalRead(BUTTON_PIN) == HIGH && !alertActive) {
        alertActive = true;
        alertStartMillis = millis();
        Serial.println("Botão pressionado. Alerta de coleta ativado.");

        // Acender o LED vermelho e enviar estado ao Firebase
        digitalWrite(LED_PIN_RED, HIGH);
        digitalWrite(LED_PIN_YELLOW, LOW);
        digitalWrite(LED_PIN_GREEN, LOW);
        if (Firebase.setString(fbdo, "/luz", "Vermelho aceso (alerta de coleta)")) {
            Serial.println("Estado de alerta enviado para o Firebase com sucesso");
        } else {
            Serial.printf("Falha ao enviar estado de alerta: %s\n", fbdo.errorReason().c_str());
        }
    }

    // Verificar se o alerta está ativo e o tempo de 10 segundos passou
    if (alertActive) {
        if (millis() - alertStartMillis < 10000) {
            return; // Manter o alerta ativo
        } else {
            alertActive = false;
            Serial.println("Alerta de coleta desativado.");
        }
    }

    // Verificar se já se passaram 2 segundos
    if (millis() - dataMillis > 2000) {
        dataMillis = millis();

        // Ler o valor do potenciômetro e mapear para a faixa de distância
        int potValue = analogRead(POT_PIN);
        long maxDistance = map(potValue, 0, 4095, 20, 100); // Mapeando de 20 cm a 100 cm

        // Obter a distância medida pelo sensor ultrassônico
        long distance = getDistance();
        Serial.print("Distância: ");
        Serial.print(distance);
        Serial.print(" cm, MaxDistância: ");
        Serial.print(maxDistance);
        Serial.println(" cm");

        // Determinar qual LED acender com base na distância
        if (distance <= maxDistance * 0.3) {
            digitalWrite(LED_PIN_RED, HIGH);
            digitalWrite(LED_PIN_YELLOW, LOW);
            digitalWrite(LED_PIN_GREEN, LOW);
            Serial.println("LED Vermelho aceso");
            if (Firebase.setString(fbdo, "/luz", "Vermelho aceso")) {
                Serial.println("Estado enviado para o Firebase com sucesso");
            } else {
                Serial.printf("Falha ao enviar estado: %s\n", fbdo.errorReason().c_str());
            }
        } else if (distance <= maxDistance * 0.7) {
            digitalWrite(LED_PIN_RED, LOW);
            digitalWrite(LED_PIN_YELLOW, HIGH);
            digitalWrite(LED_PIN_GREEN, LOW);
            Serial.println("LED Amarelo aceso");
            if (Firebase.setString(fbdo, "/luz", "Amarelo aceso")) {
                Serial.println("Estado enviado para o Firebase com sucesso");
            } else {
                Serial.printf("Falha ao enviar estado: %s\n", fbdo.errorReason().c_str());
            }
        } else {
            digitalWrite(LED_PIN_RED, LOW);
            digitalWrite(LED_PIN_YELLOW, LOW);
            digitalWrite(LED_PIN_GREEN, HIGH);
            Serial.println("LED Verde aceso");
            if (Firebase.setString(fbdo, "/luz", "Verde aceso")) {
                Serial.println("Estado enviado para o Firebase com sucesso");
            } else {
                Serial.printf("Falha ao enviar estado: %s\n", fbdo.errorReason().c_str());
            }
        }
    }
}
