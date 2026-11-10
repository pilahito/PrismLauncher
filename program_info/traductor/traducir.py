# SPDX-License-Identifier: GPL-3.0-only
# Traductor sin conexión para las novedades de la beta (inglés -> español).
# Uso: python traducir.py  (lee JSON por la entrada estándar: {"textos": ["..."]})
#      y escribe JSON por la salida estándar: {"textos": ["..."]}.
# Requiere argostranslate (pip install --target librerias argostranslate) y el modelo en->es.

import hashlib
import json
import os
import sys

# Carpeta de argostranslate: variable PRISM_ARGOS_DIR o, por defecto, la carpeta "librerias" junto a este script.
DIR_LIBRERIAS = os.environ.get("PRISM_ARGOS_DIR", os.path.join(os.path.dirname(os.path.abspath(__file__)), "librerias"))
if os.path.isdir(DIR_LIBRERIAS) and DIR_LIBRERIAS not in sys.path:
    sys.path.insert(0, DIR_LIBRERIAS)

import argostranslate.translate as traduccion  # noqa: E402

# Caché en la carpeta de datos del usuario, para no traducir dos veces lo mismo.
CARPETA_CACHE = os.path.join(os.environ.get("LOCALAPPDATA", os.path.expanduser("~")), "PrismLauncherBeta")
ARCHIVO_CACHE = os.path.join(CARPETA_CACHE, "traducciones-en-es.json")


def cargar_cache():
    try:
        with open(ARCHIVO_CACHE, encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def guardar_cache(cache):
    os.makedirs(CARPETA_CACHE, exist_ok=True)
    with open(ARCHIVO_CACHE, "w", encoding="utf-8") as f:
        json.dump(cache, f, ensure_ascii=False, indent=1)


# Términos de Prism que la traducción automática suele dejar mal. Se aplican al resultado,
# sin tocar la caché, así que basta con añadir una línea aquí para corregirlos.
GLOSARIO = {
    "Cambio": "Registro de cambios",
    "Fijación": "Corregido",
    "Fixed": "Corregido",
    "Changelog": "Registro de cambios",
    "Showcase": "Demostración",
    "Pasando de Twitter": "Dejamos Twitter",
}


def clave(texto):
    return hashlib.sha256(texto.encode("utf-8")).hexdigest()


def traducir_textos(textos):
    cache = cargar_cache()
    resultado = []
    cambios = False
    for texto in textos:
        if not texto.strip():
            resultado.append(texto)
            continue
        k = clave(texto)
        if k not in cache:
            cache[k] = traduccion.translate(texto, "en", "es")
            cambios = True
        resultado.append(GLOSARIO.get(cache[k].strip(), cache[k]))
    if cambios:
        guardar_cache(cache)
    return resultado


if __name__ == "__main__":
    # utf-8-sig acepta la entrada con o sin BOM (PowerShell lo añade al enviar datos).
    entrada = json.loads(sys.stdin.buffer.read().decode("utf-8-sig"))
    salida = {"textos": traducir_textos(entrada.get("textos", []))}
    # Salida siempre en UTF-8, para que las tildes no se rompan en Windows.
    sys.stdout.buffer.write(json.dumps(salida, ensure_ascii=False).encode("utf-8"))
