"""Pruebas del cableado (diagrama de Wokwi) contra los pines definidos en el firmware.

Ejecutar desde la raiz del proyecto:
    python -m unittest discover -s pruebas/pc -v
"""
import json
import os
import re
import unittest

RAIZ = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

PINES_VALIDOS = {
    "wokwi-led": {"A", "C"},
    "wokwi-resistor": {"1", "2"},
    "wokwi-potentiometer": {"VCC", "SIG", "GND"},
    "wokwi-dht22": {"VCC", "SDA", "NC", "GND"},
    "board-ssd1306": {"GND", "VCC", "SCL", "SDA", "DATA", "CLK", "DC", "RST", "CS"},
}


def leer_pines_h():
    with open(os.path.join(RAIZ, "include", "pines.h"), encoding="utf-8") as f:
        texto = f.read()
    return {n: int(v, 0) for n, v in re.findall(r"#define\s+(\w+)\s+(0x[0-9A-Fa-f]+|\d+)\b", texto)}


def cargar_diagrama():
    with open(os.path.join(RAIZ, "diagrama.json"), encoding="utf-8") as f:
        return json.load(f)


class TestCableado(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pines = leer_pines_h()
        cls.diagrama = cargar_diagrama()
        cls.partes = {p["id"]: p for p in cls.diagrama["parts"]}
        cls.conexiones = [(a, b) for a, b, *_ in cls.diagrama["connections"]]

    # ---- utilidades ----
    def conectados(self, a, b):
        """True si a y b estan unidos directamente (en cualquier sentido)."""
        return (a, b) in self.conexiones or (b, a) in self.conexiones

    def alcanzables(self, origen):
        """Todos los terminales unidos (directa o indirectamente) a 'origen'."""
        visto, pendientes = {origen}, [origen]
        while pendientes:
            actual = pendientes.pop()
            for a, b in self.conexiones:
                for x, y in ((a, b), (b, a)):
                    if x == actual and y not in visto:
                        visto.add(y)
                        pendientes.append(y)
        return visto

    # ---- pruebas ----
    def test_w0_pines_h_define_todo_lo_necesario(self):
        for nombre in ("DHT_PIN", "OLED_SDA", "OLED_SCL", "POT_PIN",
                       "LED_VERDE", "LED_NARANJA", "LED_ROJO"):
            self.assertIn(nombre, self.pines)

    def test_w1_dht22_senal_en_dht_pin(self):
        self.assertTrue(self.conectados(f"esp:{self.pines['DHT_PIN']}", "dht1:SDA"))

    def test_w2_dht22_alimentacion(self):
        self.assertIn("esp:3V3", self.alcanzables("dht1:VCC"))
        self.assertTrue(any(t.startswith("esp:GND") for t in self.alcanzables("dht1:GND")))

    def test_w3_oled_i2c(self):
        self.assertTrue(self.conectados(f"esp:{self.pines['OLED_SDA']}", "oled1:SDA"))
        self.assertTrue(self.conectados(f"esp:{self.pines['OLED_SCL']}", "oled1:SCL"))

    def test_w4_oled_alimentacion_y_direccion(self):
        self.assertIn("esp:3V3", self.alcanzables("oled1:VCC"))
        self.assertTrue(any(t.startswith("esp:GND") for t in self.alcanzables("oled1:GND")))
        direccion = int(self.partes["oled1"]["attrs"]["i2cAddress"], 0)
        self.assertEqual(direccion, self.pines["OLED_ADDRESS"])

    def test_w5_potenciometro(self):
        self.assertTrue(self.conectados(f"esp:{self.pines['POT_PIN']}", "pot1:SIG"))
        self.assertIn("esp:3V3", self.alcanzables("pot1:VCC"))
        self.assertTrue(any(t.startswith("esp:GND") for t in self.alcanzables("pot1:GND")))

    def test_w6_pot_en_pin_adc1(self):
        # ADC2 no funciona con Wi-Fi activo: el pin debe ser ADC1 (GPIO 32-39)
        self.assertIn(self.pines["POT_PIN"], range(32, 40))

    def test_w7_cada_led_tiene_resistencia_en_serie_y_tierra(self):
        cadenas = [("LED_VERDE", "r1", "led1"), ("LED_NARANJA", "r2", "led2"),
                   ("LED_ROJO", "r3", "led3")]
        for pin, r, led in cadenas:
            with self.subTest(led=led):
                self.assertTrue(self.conectados(f"esp:{self.pines[pin]}", f"{r}:1"))
                self.assertTrue(self.conectados(f"{r}:2", f"{led}:A"))
                self.assertTrue(any(t.startswith("esp:GND") for t in self.alcanzables(f"{led}:C")))

    def test_w8_colores_de_leds(self):
        self.assertEqual(self.partes["led1"]["attrs"]["color"], "green")
        self.assertEqual(self.partes["led2"]["attrs"]["color"], "orange")
        self.assertEqual(self.partes["led3"]["attrs"]["color"], "red")

    def test_w9_resistencias_limitadoras_220_ohm(self):
        for r in ("r1", "r2", "r3"):
            self.assertEqual(self.partes[r]["attrs"]["value"], "220")

    def test_w10_no_hay_pines_inexistentes_en_partes(self):
        for a, b in self.conexiones:
            for terminal in (a, b):
                parte, _, pin = terminal.partition(":")
                if parte.startswith("$") or parte == "esp":
                    continue
                tipo = self.partes[parte]["type"]
                if tipo in PINES_VALIDOS:
                    self.assertIn(pin, PINES_VALIDOS[tipo], f"{terminal} no existe en {tipo}")

    def test_w11_pines_del_esp_sin_prefijo_D(self):
        # En board-esp32-devkit-c-v4 los pines se llaman "21", "34"... (no "D21")
        for a, b in self.conexiones:
            for terminal in (a, b):
                if terminal.startswith("esp:D") and terminal[5:].isdigit():
                    self.fail(f"Pin inexistente en devkit-c-v4: {terminal}")

    def test_w12_sin_conexiones_duplicadas(self):
        vistos = set()
        for a, b in self.conexiones:
            clave = frozenset((a, b))
            self.assertNotIn(clave, vistos, f"duplicada: {a} - {b}")
            vistos.add(clave)

    def test_w13_ningun_pin_gpio_repetido_entre_perifericos(self):
        usados = [self.pines[n] for n in ("DHT_PIN", "OLED_SDA", "OLED_SCL", "POT_PIN",
                                          "LED_VERDE", "LED_NARANJA", "LED_ROJO")]
        self.assertEqual(len(usados), len(set(usados)))


if __name__ == "__main__":
    unittest.main(verbosity=2)
