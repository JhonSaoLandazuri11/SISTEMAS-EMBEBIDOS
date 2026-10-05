"""Pruebas del simulador Python (simulador.py) - no necesita red ni paho instalado.

Ejecutar desde la raiz del proyecto:
    python -m unittest discover -s pruebas/pc -v
"""
import io
import json
import os
import re
import sys
import types
import unittest
from unittest import mock

RAIZ = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, RAIZ)


# ---- paho-mqtt falso (para probar sin red) ----
class FakeInfo:
    def __init__(self, rc=0):
        self.rc = rc

    def wait_for_publish(self):
        pass


class FakeClient:
    instancia = None
    rc_publish = 0  # atributo de clase: las pruebas lo cambian para simular fallos

    def __init__(self, *a, **k):
        self.publicados = []
        self.usuario = None
        self.tls = False
        self.conexion = None
        FakeClient.instancia = self

    def username_pw_set(self, u, p):
        self.usuario = (u, p)

    def tls_set(self):
        self.tls = True

    def connect(self, host, port, keepalive):
        self.conexion = (host, port, keepalive)

    def loop_start(self):
        pass

    def loop_stop(self):
        pass

    def disconnect(self):
        pass

    def publish(self, topic, msg):
        self.publicados.append((topic, msg))
        return FakeInfo(self.rc_publish)


def instalar_paho_falso():
    paho = types.ModuleType("paho")
    mqtt_pkg = types.ModuleType("paho.mqtt")
    cliente = types.ModuleType("paho.mqtt.client")
    cliente.Client = FakeClient
    cliente.CallbackAPIVersion = types.SimpleNamespace(VERSION2=2)
    cliente.MQTT_ERR_SUCCESS = 0
    mqtt_pkg.client = cliente
    paho.mqtt = mqtt_pkg
    sys.modules.update({"paho": paho, "paho.mqtt": mqtt_pkg, "paho.mqtt.client": cliente})


instalar_paho_falso()
sys.modules.pop("simulador", None)
import simulador  # noqa: E402


class TestGeneracionDeLecturas(unittest.TestCase):
    """S1 - Los datos simulados tienen formato y rangos correctos."""

    def test_s1_1_claves_y_tipos(self):
        lec = simulador.generar_lectura()
        self.assertEqual(set(lec), {"temperatura", "humedad", "adc"})
        self.assertIsInstance(lec["temperatura"], float)
        self.assertIsInstance(lec["humedad"], float)
        self.assertIsInstance(lec["adc"], int)

    def test_s1_2_rangos_en_1000_lecturas(self):
        for _ in range(1000):
            lec = simulador.generar_lectura()
            self.assertGreaterEqual(lec["temperatura"], 22.5)
            self.assertLessEqual(lec["temperatura"], 29.5)
            self.assertGreaterEqual(lec["humedad"], 35.0)
            self.assertLessEqual(lec["humedad"], 50.0)
            self.assertTrue(0 <= lec["adc"] <= 4095)

    def test_s1_3_un_decimal(self):
        for _ in range(200):
            lec = simulador.generar_lectura()
            self.assertEqual(lec["temperatura"], round(lec["temperatura"], 1))
            self.assertEqual(lec["humedad"], round(lec["humedad"], 1))

    def test_s1_4_adc_coherente_con_temperatura_alta(self):
        with mock.patch.object(simulador.random, "uniform", side_effect=[8.0, 0.0]):
            lec = simulador.generar_lectura()  # fuerza T = 32 C
        self.assertGreater(lec["temperatura"], 30.0)
        self.assertTrue(1500 <= lec["adc"] <= 3000)

    def test_s1_5_json_serializable_igual_que_el_firmware(self):
        lec = simulador.generar_lectura()
        recuperado = json.loads(json.dumps(lec))
        self.assertEqual(recuperado, lec)

    @unittest.expectedFailure
    def test_s1_6_DEFECTO_el_simulador_nunca_llega_a_estado_alerta(self):
        """Defecto conocido: T maxima = 24.0 + 5.5 = 29.5 C, por lo que nunca supera
        los 30 C (ALERTA) y la rama 'adc 1500-3000' es codigo muerto.
        Si amplias el rango (p. ej. uniform(-1.5, 8.5)) esta prueba pasara a ser
        'unexpected success': quita el decorador @expectedFailure."""
        alcanza_alerta = any(
            simulador.generar_lectura()["temperatura"] > 30.0 for _ in range(5000)
        )
        self.assertTrue(alcanza_alerta)


class TestConfiguracion(unittest.TestCase):
    """S2 - Configuracion de conexion."""

    def test_s2_1_servidor_puerto_y_topic(self):
        self.assertEqual(simulador.MQTT_PORT, 8883)
        self.assertEqual(simulador.MQTT_TOPIC, "sistema/ambiente")
        self.assertTrue(simulador.MQTT_SERVER.endswith(".hivemq.cloud"))

    def test_s2_2_no_hay_credenciales_escritas_en_el_codigo(self):
        with open(os.path.join(RAIZ, "simulador.py"), encoding="utf-8") as f:
            texto = f.read()
        # Las unicas claves leidas del entorno deben ser MQTT_USER y MQTT_PASS
        nombres = re.findall(r'os\.getenv\("([^"]+)"\)', texto)
        self.assertEqual(nombres, ["MQTT_USER", "MQTT_PASS"])
        # y no debe haber asignaciones literales de usuario/clave
        self.assertIsNone(re.search(r'(?m)^MQTT_(USER|PASS)\s*=\s*"', texto))

    def test_s2_3_sin_variables_de_entorno_termina_con_mensaje(self):
        with mock.patch.object(simulador, "MQTT_USER", None), \
             mock.patch.object(simulador, "MQTT_PASS", None):
            with self.assertRaises(SystemExit) as ctx:
                simulador.main()
        self.assertIn("MQTT_USER", str(ctx.exception))


class TestPublicacion(unittest.TestCase):
    """S3 - Flujo de publicacion con un broker falso."""

    def correr_una_iteracion(self, rc=0):
        def dormir_y_salir(_):
            raise KeyboardInterrupt()

        with mock.patch.object(simulador, "MQTT_USER", "usuario"), \
             mock.patch.object(simulador, "MQTT_PASS", "clave"), \
             mock.patch.object(simulador.time, "sleep", side_effect=dormir_y_salir), \
             mock.patch.object(FakeClient, "rc_publish", rc), \
             mock.patch("sys.stdout", new_callable=io.StringIO) as salida:
            simulador.main()
        self.salida = salida.getvalue()
        return FakeClient.instancia

    def test_s3_1_conecta_con_tls_y_credenciales(self):
        c = self.correr_una_iteracion()
        self.assertTrue(c.tls)
        self.assertEqual(c.usuario, ("usuario", "clave"))
        self.assertEqual(c.conexion[1], 8883)

    def test_s3_2_publica_json_valido_en_el_topic(self):
        c = self.correr_una_iteracion()
        self.assertEqual(len(c.publicados), 1)
        topic, msg = c.publicados[0]
        self.assertEqual(topic, "sistema/ambiente")
        datos = json.loads(msg)
        self.assertEqual(set(datos), {"temperatura", "humedad", "adc"})

    def test_s3_3_error_de_publicacion_no_rompe_el_bucle(self):
        c = self.correr_una_iteracion(rc=4)  # rc != MQTT_ERR_SUCCESS
        self.assertEqual(len(c.publicados), 1)
        self.assertIn("No se pudo publicar", self.salida)
        self.assertIn("finalizada", self.salida)

    def test_s3_4_publicacion_exitosa_informa_el_mensaje(self):
        self.correr_una_iteracion(rc=0)
        self.assertIn("Publicado en 'sistema/ambiente'", self.salida)
        self.assertNotIn("No se pudo publicar", self.salida)


if __name__ == "__main__":
    unittest.main(verbosity=2)
