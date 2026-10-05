// Pruebas unitarias de la lógica pura (se ejecutan en el PC, sin ESP32):
//     pio test -e native
#include <unity.h>
#include <string.h>
#include "logica.h"

void setUp(void) {}
void tearDown(void) {}

// ---------------- LEDs / estados (T04) ----------------
void test_estado_normal_bajo_25(void) {
    TEST_ASSERT_EQUAL(ESTADO_NORMAL, clasificarTemperatura(-10.0f));
    TEST_ASSERT_EQUAL(ESTADO_NORMAL, clasificarTemperatura(20.0f));
    TEST_ASSERT_EQUAL(ESTADO_NORMAL, clasificarTemperatura(24.9f));
}

void test_estado_advertencia_limites_inclusivos(void) {
    TEST_ASSERT_EQUAL(ESTADO_ADVERTENCIA, clasificarTemperatura(25.0f));
    TEST_ASSERT_EQUAL(ESTADO_ADVERTENCIA, clasificarTemperatura(27.5f));
    TEST_ASSERT_EQUAL(ESTADO_ADVERTENCIA, clasificarTemperatura(30.0f));
}

void test_estado_alerta_sobre_30(void) {
    TEST_ASSERT_EQUAL(ESTADO_ALERTA, clasificarTemperatura(30.1f));
    TEST_ASSERT_EQUAL(ESTADO_ALERTA, clasificarTemperatura(45.0f));
}

void test_estado_nan_es_alerta(void) {
    TEST_ASSERT_EQUAL(ESTADO_ALERTA, clasificarTemperatura(NAN));
}

void test_nombres_de_estado(void) {
    TEST_ASSERT_EQUAL_STRING("NORMAL", nombreEstado(ESTADO_NORMAL));
    TEST_ASSERT_EQUAL_STRING("ADVERTENCIA", nombreEstado(ESTADO_ADVERTENCIA));
    TEST_ASSERT_EQUAL_STRING("ALERTA", nombreEstado(ESTADO_ALERTA));
}

// ---------------- OLED (T02) ----------------
void test_texto_oled_por_estado(void) {
    TEST_ASSERT_EQUAL_STRING("Estado: NORMAL", textoEstadoOLED(ESTADO_NORMAL));
    TEST_ASSERT_EQUAL_STRING("Estado: ALERTA MEDIA", textoEstadoOLED(ESTADO_ADVERTENCIA));
    TEST_ASSERT_EQUAL_STRING("Estado: PELIGRO", textoEstadoOLED(ESTADO_ALERTA));
}

void test_texto_oled_cabe_en_128px(void) {
    // Fuente 6x8 px por carácter => 21 caracteres máx. por línea
    TEST_ASSERT_TRUE(strlen(textoEstadoOLED(ESTADO_NORMAL)) <= 21);
    TEST_ASSERT_TRUE(strlen(textoEstadoOLED(ESTADO_ADVERTENCIA)) <= 21);
    TEST_ASSERT_TRUE(strlen(textoEstadoOLED(ESTADO_ALERTA)) <= 21);
}

// ---------------- DHT22 (T01) ----------------
void test_lectura_valida_normal(void) {
    TEST_ASSERT_TRUE(lecturaValida(24.0f, 40.0f));
    TEST_ASSERT_TRUE(lecturaValida(-40.0f, 0.0f));
    TEST_ASSERT_TRUE(lecturaValida(80.0f, 100.0f));
}

void test_lectura_invalida_nan(void) {
    TEST_ASSERT_FALSE(lecturaValida(NAN, 40.0f));
    TEST_ASSERT_FALSE(lecturaValida(24.0f, NAN));
    TEST_ASSERT_FALSE(lecturaValida(NAN, NAN));
}

void test_lectura_invalida_fuera_de_rango(void) {
    TEST_ASSERT_FALSE(lecturaValida(-40.1f, 50.0f));
    TEST_ASSERT_FALSE(lecturaValida(80.1f, 50.0f));
    TEST_ASSERT_FALSE(lecturaValida(25.0f, -0.1f));
    TEST_ASSERT_FALSE(lecturaValida(25.0f, 100.1f));
}

// ---------------- ADC / potenciómetro (T03) ----------------
void test_adc_valido(void) {
    TEST_ASSERT_TRUE(adcValido(0));
    TEST_ASSERT_TRUE(adcValido(2048));
    TEST_ASSERT_TRUE(adcValido(4095));
    TEST_ASSERT_FALSE(adcValido(-1));
    TEST_ASSERT_FALSE(adcValido(4096));
}

void test_adc_a_voltaje(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, adcAVoltaje(0));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.3f, adcAVoltaje(4095));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.65f, adcAVoltaje(2048));
}

// ---------------- Payload MQTT (T06) ----------------
void test_payload_formato_json(void) {
    char buf[128];
    int n = construirPayload(buf, sizeof(buf), 24.0f, 40.0f, 1213);
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_EQUAL_STRING(
        "{\"temperatura\":24.00,\"humedad\":40.00,\"adc\":1213}", buf);
}

void test_payload_valores_extremos_caben(void) {
    char buf[128];
    int n = construirPayload(buf, sizeof(buf), -40.0f, 100.0f, 4095);
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_TRUE(n < 128);
}

void test_payload_buffer_pequeno_devuelve_error(void) {
    char buf[10];
    TEST_ASSERT_EQUAL_INT(-1, construirPayload(buf, sizeof(buf), 24.0f, 40.0f, 1213));
}

// ---------------- URL ThingSpeak (T07) ----------------
void test_url_thingspeak_dos_campos(void) {
    char url[256];
    int n = construirUrlThingSpeak(url, sizeof(url),
        "http://api.thingspeak.com/update", "ABC123", 24.5f, 40.25f);
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_EQUAL_STRING(
        "http://api.thingspeak.com/update?api_key=ABC123&field1=24.50&field2=40.25", url);
}

void test_url_thingspeak_tres_campos(void) {
    char url[256];
    construirUrlThingSpeak(url, sizeof(url),
        "http://api.thingspeak.com/update", "ABC123", 24.5f, 40.0f, 1213);
    TEST_ASSERT_EQUAL_STRING(
        "http://api.thingspeak.com/update?api_key=ABC123&field1=24.50&field2=40.00&field3=1213", url);
}

void test_url_thingspeak_buffer_pequeno(void) {
    char url[20];
    TEST_ASSERT_EQUAL_INT(-1, construirUrlThingSpeak(url, sizeof(url),
        "http://api.thingspeak.com/update", "ABC123", 24.5f, 40.0f));
}

// ---------------- Temporización (T07) ----------------
void test_intervalo_no_cumplido(void) {
    TEST_ASSERT_FALSE(intervaloCumplido(19999, 0, 20000));
}

void test_intervalo_cumplido_exacto(void) {
    TEST_ASSERT_TRUE(intervaloCumplido(20000, 0, 20000));
    TEST_ASSERT_TRUE(intervaloCumplido(45000, 20000, 20000));
}

void test_intervalo_con_desbordamiento_de_millis(void) {
    // millis() da la vuelta a los ~49.7 días: 0xFFFFFFFF -> 0
    uint32_t ultimo = 0xFFFFFF00u;
    TEST_ASSERT_FALSE(intervaloCumplido(0x00000100u, ultimo, 20000));
    TEST_ASSERT_TRUE(intervaloCumplido(0x00005000u, ultimo, 20000));
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_estado_normal_bajo_25);
    RUN_TEST(test_estado_advertencia_limites_inclusivos);
    RUN_TEST(test_estado_alerta_sobre_30);
    RUN_TEST(test_estado_nan_es_alerta);
    RUN_TEST(test_nombres_de_estado);
    RUN_TEST(test_texto_oled_por_estado);
    RUN_TEST(test_texto_oled_cabe_en_128px);
    RUN_TEST(test_lectura_valida_normal);
    RUN_TEST(test_lectura_invalida_nan);
    RUN_TEST(test_lectura_invalida_fuera_de_rango);
    RUN_TEST(test_adc_valido);
    RUN_TEST(test_adc_a_voltaje);
    RUN_TEST(test_payload_formato_json);
    RUN_TEST(test_payload_valores_extremos_caben);
    RUN_TEST(test_payload_buffer_pequeno_devuelve_error);
    RUN_TEST(test_url_thingspeak_dos_campos);
    RUN_TEST(test_url_thingspeak_tres_campos);
    RUN_TEST(test_url_thingspeak_buffer_pequeno);
    RUN_TEST(test_intervalo_no_cumplido);
    RUN_TEST(test_intervalo_cumplido_exacto);
    RUN_TEST(test_intervalo_con_desbordamiento_de_millis);
    return UNITY_END();
}
