import json
import os
import random
import time

import paho.mqtt.client as mqtt


MQTT_SERVER = "72132f2da69e492e9e8cef84550f5c24.s1.eu.hivemq.cloud"
MQTT_PORT = 8883
MQTT_TOPIC = "sistema/ambiente"
# Las credenciales se leen de variables de entorno (NO se escriben en el código):
#   Windows (PowerShell):  $env:MQTT_USER="usuario"; $env:MQTT_PASS="clave"
#   Linux/macOS:           export MQTT_USER=usuario MQTT_PASS=clave
MQTT_USER = os.getenv("MQTT_USER")
MQTT_PASS = os.getenv("MQTT_PASS")

TEMPERATURA_BASE = 24.0
HUMEDAD_BASE = 40.0


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Conectado de forma segura a HiveMQ Cloud.")
    else:
        print(f"Fallo de conexión MQTT. Código: {reason_code}")


def generar_lectura():
    """Devuelve una lectura simulada {temperatura, humedad, adc}."""
    temperatura = round(TEMPERATURA_BASE + random.uniform(-1.5, 5.5), 1)
    humedad = round(HUMEDAD_BASE + random.uniform(-5.0, 10.0), 1)
    valor_adc = (
        random.randint(1500, 3000)
        if temperatura > 30.0
        else random.randint(0, 500)
    )
    return {"temperatura": temperatura, "humedad": humedad, "adc": valor_adc}


def main():
    if not MQTT_USER or not MQTT_PASS:
        raise SystemExit(
            "Configura las variables de entorno MQTT_USER y MQTT_PASS antes de ejecutar."
        )

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.username_pw_set(MQTT_USER, MQTT_PASS)
    client.tls_set()
    client.on_connect = on_connect

    print("Estableciendo conexión TLS con HiveMQ Cloud...")
    client.connect(MQTT_SERVER, MQTT_PORT, 60)
    client.loop_start()

    try:
        while True:
            payload = generar_lectura()
            message = json.dumps(payload)
            result = client.publish(MQTT_TOPIC, message)
            result.wait_for_publish()

            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                print(f"Publicado en '{MQTT_TOPIC}': {message}")
            else:
                print(f"No se pudo publicar. Código: {result.rc}")

            time.sleep(4)
    except KeyboardInterrupt:
        print("\nSimulación finalizada por el usuario.")
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
