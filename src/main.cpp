#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "esp_sleep.h"

// =====================================================
// 1-4. CONFIGURACIÓN (Wi-Fi, HiveMQ, certificado, ThingSpeak)
// =====================================================
// Los valores no secretos están en include/config.h y las
// credenciales en include/secrets.h (copiar de secrets.example.h).
#include "config.h"
#include "secrets.h"
#include "certificado.h"
#include "logica.h"


// =====================================================
// 5. PINES DEL SISTEMA  (ver include/pines.h)
// =====================================================
#include "pines.h"

// =====================================================
// 6. OBJETOS
// =====================================================

DHT dht(DHT_PIN, DHT_TYPE);

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

// Cliente seguro para MQTT/TLS
WiFiClientSecure espClient;

// Cliente MQTT
PubSubClient client(espClient);

// =====================================================
// 7. VARIABLES DE CONTROL
// =====================================================

unsigned long lastThingSpeak = 0;

// El intervalo de ThingSpeak (thingspeakInterval) está en include/config.h

// =====================================================
// 8. CONEXIÓN Wi-Fi
// =====================================================

void setup_wifi() {

    Serial.println();
    Serial.println("=================================");
    Serial.println("       CONEXIÓN Wi-Fi");
    Serial.println("=================================");

    Serial.print("SSID: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {

        delay(500);

        Serial.print(".");
    }

    Serial.println();
    Serial.println("Wi-Fi conectado correctamente.");

    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
}

// =====================================================
// 9. CONEXIÓN MQTT A HIVEMQ CLOUD
// =====================================================

void reconnectMQTT() {

    while (!client.connected()) {

        Serial.println();
        Serial.println("=================================");
        Serial.println("     CONEXIÓN HIVE-MQ CLOUD");
        Serial.println("=================================");

        Serial.println(
            "Intentando conectar mediante MQTT/TLS..."
        );

        // Identificador único del cliente
        String clientId =
            "ESP32-SistemaAmbiental-";

        clientId +=
            String(random(0xffff), HEX);

        if (client.connect(
                clientId.c_str(),
                mqtt_user,
                mqtt_password)) {

            Serial.println(
                "MQTT conectado correctamente."
            );

            Serial.print("Broker: ");
            Serial.println(mqtt_server);

            Serial.print("Puerto TLS: ");
            Serial.println(mqtt_port);

            Serial.print("Topic: ");
            Serial.println(mqtt_topic);

            // Suscripción al topic
            client.subscribe(mqtt_topic);

        } else {

            Serial.print(
                "Error MQTT. Código: "
            );

            Serial.println(
                client.state()
            );

            Serial.println(
                "Reintentando en 5 segundos..."
            );

            delay(5000);
        }
    }
}

// =====================================================
// 10. ENVÍO DE DATOS A THINGSPEAK
// =====================================================

void sendToThingSpeak(
    float temperatura,
    float humedad
) {

    // Comprobar conexión Wi-Fi
    if (WiFi.status() != WL_CONNECTED) {

        Serial.println(
            "Wi-Fi no conectado."
        );

        Serial.println(
            "No se enviarán datos a ThingSpeak."
        );

        return;
    }

    HTTPClient http;

    // Construcción de la URL (include/logica.h)
    char url[256];

    if (construirUrlThingSpeak(
            url,
            sizeof(url),
            thingspeak_server,
            thingspeak_api_key,
            temperatura,
            humedad) < 0) {

        Serial.println("Error: URL de ThingSpeak demasiado larga.");
        return;
    }

    Serial.println();
    Serial.println(
        "Enviando datos a ThingSpeak..."
    );

    // Por seguridad no se muestra la API Key
    Serial.println(
        "Fields: Temperatura + Humedad"
    );

    http.begin(url);

    int httpCode =
        http.GET();

    Serial.print(
        "ThingSpeak HTTP Code: "
    );

    Serial.println(
        httpCode
    );

    if (httpCode > 0) {

        String response =
            http.getString();

        Serial.print(
            "Respuesta ThingSpeak: "
        );

        Serial.println(
            response
        );

        if (httpCode == 200) {

            Serial.println(
                "Datos enviados correctamente a ThingSpeak."
            );

        } else {

            Serial.println(
                "ThingSpeak respondió con un código diferente de 200."
            );
        }

    } else {

        Serial.println(
            "Error al enviar datos a ThingSpeak."
        );
    }

    http.end();
}

// =====================================================
// 11. CONTROL DE LEDS
// =====================================================
//
// Valores utilizados para la demostración académica:
// < 25 °C       = NORMAL
// 25 a 30 °C    = ADVERTENCIA
// > 30 °C       = ALERTA
//
// Estos umbrales corresponden al escenario de simulación
// y no representan límites ambientales universales.
// =====================================================

void controlarLEDs(
    float temperatura
) {

    // Apagar todos los LEDs
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_NARANJA, LOW);
    digitalWrite(LED_ROJO, LOW);

    // La clasificación está en include/logica.h (probada en el PC)
    EstadoAmbiental estado =
        clasificarTemperatura(temperatura);

    if (estado == ESTADO_NORMAL) {
        digitalWrite(LED_VERDE, HIGH);
    } else if (estado == ESTADO_ADVERTENCIA) {
        digitalWrite(LED_NARANJA, HIGH);
    } else {
        digitalWrite(LED_ROJO, HIGH);
    }

    Serial.print("Estado ambiental: ");
    Serial.println(nombreEstado(estado));
}

// =====================================================
// 12. ACTUALIZACIÓN DEL OLED
// =====================================================

void actualizarOLED(
    float temperatura,
    float humedad,
    int adc
) {

    display.clearDisplay();

    display.setTextSize(1);

    display.setTextColor(
        SSD1306_WHITE
    );

    // -----------------------------
    // Título
    // -----------------------------

    display.setCursor(
        0,
        0
    );

    display.println(
        "SISTEMA AMBIENTAL"
    );

    // -----------------------------
    // Temperatura
    // -----------------------------

    display.setCursor(
        0,
        16
    );

    display.print(
        "Temp: "
    );

    display.print(
        temperatura,
        1
    );

    display.println(
        " C"
    );

    // -----------------------------
    // Humedad
    // -----------------------------

    display.setCursor(
        0,
        28
    );

    display.print(
        "Hum:  "
    );

    display.print(
        humedad,
        1
    );

    display.println(
        " %"
    );

    // -----------------------------
    // ADC
    // -----------------------------

    display.setCursor(
        0,
        40
    );

    display.print(
        "ADC:  "
    );

    display.println(
        adc
    );

    // -----------------------------
    // Estado
    // -----------------------------

    display.setCursor(
        0,
        52
    );

    display.println(
        textoEstadoOLED(
            clasificarTemperatura(temperatura)
        )
    );

    display.display();
}
// =====================================================
// MODO DE AHORRO DE ENERGÍA - DEEP SLEEP
// =====================================================

// Contador que SOBREVIVE al deep sleep (memoria RTC).
RTC_DATA_ATTR int contadorCiclos = 0;

void entrarDeepSleep() {

    const uint64_t tiempo_sueno =
        (uint64_t)DEEP_SLEEP_SEGUNDOS * 1000000ULL;

    Serial.println();
    Serial.println("=================================");
    Serial.println("   ENTRANDO EN DEEP SLEEP");
    Serial.println("=================================");
    Serial.print("Ciclo completado: ");
    Serial.println(contadorCiclos);
    Serial.print("Tiempo programado: ");
    Serial.print(DEEP_SLEEP_SEGUNDOS);
    Serial.println(" segundos");

    // Mensaje visible en el OLED antes de dormir
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("MODO DEEP SLEEP");
    display.setCursor(0, 20);
    display.print("Ciclo: ");
    display.println(contadorCiclos);
    display.setCursor(0, 36);
    display.print("Despierta en ");
    display.print(DEEP_SLEEP_SEGUNDOS);
    display.println("s");
    display.display();
    delay(3000);  // tiempo para ver el mensaje / tomar captura

    // Cerrar conexiones limpiamente
    client.disconnect();
    WiFi.disconnect(true);

    // Apagar la pantalla y los LEDs (en deep sleep los GPIO pierden su estado)
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_NARANJA, LOW);
    digitalWrite(LED_ROJO, LOW);

    esp_sleep_enable_timer_wakeup(
        tiempo_sueno
    );

    Serial.flush();

    esp_deep_sleep_start();  // no retorna: al despertar se reinicia setup()
}


// =====================================================
// 13. CONFIGURACIÓN INICIAL
// =====================================================

void setup() {

    Serial.begin(
        115200
    );

    delay(1000);

    contadorCiclos++;

    Serial.println();
    Serial.print("Ciclo n.o ");
    Serial.println(contadorCiclos);
    Serial.print("Causa de arranque: ");
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("DESPERTAR POR TEMPORIZADOR (deep sleep)");
            break;
        default:
            Serial.println("encendido / reinicio normal");
            break;
    }

    Serial.println();
    Serial.println(
        "================================="
    );

    Serial.println(
        "     SISTEMA AMBIENTAL IoT"
    );

    Serial.println(
        "             ESP32"
    );

    Serial.println(
        "================================="
    );

    // =================================================
    // CONFIGURACIÓN DE PINES
    // =================================================

    pinMode(
        LED_VERDE,
        OUTPUT
    );

    pinMode(
        LED_NARANJA,
        OUTPUT
    );

    pinMode(
        LED_ROJO,
        OUTPUT
    );

    pinMode(
        POT_PIN,
        INPUT
    );

    // Apagar LEDs inicialmente
    digitalWrite(
        LED_VERDE,
        LOW
    );

    digitalWrite(
        LED_NARANJA,
        LOW
    );

    digitalWrite(
        LED_ROJO,
        LOW
    );

    // =================================================
    // INICIALIZAR DHT22
    // =================================================

    dht.begin();

    Serial.println(
        "DHT22 inicializado."
    );

    // =================================================
    // INICIALIZAR I2C Y OLED
    // =================================================

    Wire.begin(
        OLED_SDA,
        OLED_SCL
    );

    if (
        !display.begin(
            SSD1306_SWITCHCAPVCC,
            OLED_ADDRESS
        )
    ) {

        Serial.println(
            "Error: OLED no encontrado."
        );

    } else {

        Serial.println(
            "OLED inicializado correctamente."
        );

        display.clearDisplay();

        display.setTextSize(1);

        display.setTextColor(
            SSD1306_WHITE
        );

        display.setCursor(
            0,
            0
        );

        display.println(
            "Sistema Ambiental"
        );

        display.setCursor(
            0,
            15
        );

        display.println(
            "Inicializando..."
        );

        display.display();

        delay(1500);
    }

    // =================================================
    // CONEXIÓN Wi-Fi
    // =================================================

    setup_wifi();

    // =================================================
    // CONFIGURACIÓN TLS
    // =================================================

    // Se utiliza el certificado Root CA para validar
    // el servidor HiveMQ Cloud.

    espClient.setCACert(
        root_ca
    );

    Serial.println();

    Serial.println(
        "TLS configurado correctamente."
    );

    Serial.println(
        "Certificado CA configurado mediante setCACert()."
    );

    Serial.print(
        "Puerto MQTT seguro: "
    );

    Serial.println(
        mqtt_port
    );

    // =================================================
    // CONFIGURACIÓN MQTT
    // =================================================

    client.setServer(
        mqtt_server,
        mqtt_port
    );

    Serial.println();

    Serial.println(
        "Servidor MQTT:"
    );

    Serial.println(
        mqtt_server
    );

    Serial.print(
        "Puerto MQTT TLS: "
    );

    Serial.println(
        mqtt_port
    );

    Serial.print(
        "Topic MQTT: "
    );

    Serial.println(
        mqtt_topic
    );

    Serial.println();

    Serial.println(
        "Inicialización completada."
    );
}

// =====================================================
// 14. BUCLE PRINCIPAL
// =====================================================

void loop() {

    // =================================================
    // MANTENER CONEXIÓN MQTT
    // =================================================

    if (!client.connected()) {

        reconnectMQTT();
    }

    client.loop();

    // =================================================
    // LECTURA DEL DHT22
    // =================================================

    float temperatura =
        dht.readTemperature();

    float humedad =
        dht.readHumidity();

    // =================================================
    // VALIDACIÓN DE LOS DATOS
    // =================================================

    if (
        isnan(temperatura) ||
        isnan(humedad)
    ) {

        Serial.println();
        Serial.println(
            "Error al leer el DHT22."
        );

        delay(2000);

        return;
    }

    // =================================================
    // LECTURA DEL ADC
    // =================================================

    int adcValue =
        analogRead(
            POT_PIN
        );

    // =================================================
    // MOSTRAR DATOS EN MONITOR SERIAL
    // =================================================

    Serial.println();

    Serial.println(
        "---------------------------------"
    );

    Serial.print(
        "Temperatura: "
    );

    Serial.print(
        temperatura,
        2
    );

    Serial.println(
        " C"
    );

    Serial.print(
        "Humedad: "
    );

    Serial.print(
        humedad,
        2
    );

    Serial.println(
        " %"
    );

    Serial.print(
        "ADC: "
    );

    Serial.println(
        adcValue
    );

    // =================================================
    // CONTROL DE ACTUADORES
    // =================================================

    controlarLEDs(
        temperatura
    );

    // =================================================
    // ACTUALIZAR OLED
    // =================================================

    actualizarOLED(
        temperatura,
        humedad,
        adcValue
    );

    // =================================================
    // CREAR PAYLOAD JSON
    // =================================================

    char payload[128];

    if (construirPayload(
            payload,
            sizeof(payload),
            temperatura,
            humedad,
            adcValue) < 0) {

        Serial.println("Error: payload demasiado largo.");
        return;
    }

    Serial.println();

    Serial.print(
        "MQTT Payload: "
    );

    Serial.println(
        payload
    );

    // =================================================
    // PUBLICAR DATOS EN HIVEMQ
    // =================================================

    if (
        client.publish(
            mqtt_topic,
            payload
        )
    ) {

        Serial.println(
            "Datos publicados correctamente en HiveMQ."
        );

    } else {

        Serial.println(
            "Error al publicar datos en HiveMQ."
        );
    }

    // =================================================
    // ENVIAR DATOS A THINGSPEAK
    // =================================================

#if USAR_DEEP_SLEEP
    // Con deep sleep el ESP32 se reinicia en cada ciclo, así que
    // se envía siempre (el sueño de 30 s ya respeta el límite de 15 s).
    sendToThingSpeak(
        temperatura,
        humedad
    );
#else
    if (
        intervaloCumplido(
            millis(),
            lastThingSpeak,
            thingspeakInterval
        )
    ) {

        sendToThingSpeak(
            temperatura,
            humedad
        );

        lastThingSpeak =
            millis();
    }
#endif

    // =================================================
    // ESPERA ENTRE CICLOS
    // =================================================

#if USAR_DEEP_SLEEP
    client.loop();   // asegurar que el mensaje MQTT salió
    delay(500);
    entrarDeepSleep();
#else
    delay(5000);
#endif
}