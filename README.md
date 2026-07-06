# Agentic OS - IALearner

IALearner es un sistema cliente-servidor multihilo escrito en C, diseñado para operar de forma encubierta (modo furtivo) como un recolector de datos. El servidor analiza las pulsaciones de teclado de múltiples ventanas gráficas simultáneas (clientes X11) y utiliza un árbol de decisión heurístico basado en una tabla hash dinámica para clasificar el perfil del usuario (Personal Administrativo, Técnico, Profesor o Estudiante) según la predominancia de las palabras escritas.

##  Requisitos Previos

Para compilar y ejecutar este proyecto en un entorno Linux, asegúrate de tener instalados los siguientes paquetes:

* Compilador **GCC** y la herramienta **Make**.
* Librerías de desarrollo de **X11** para las ventanas gráficas.
    * *(En Ubuntu/Debian, puedes instalarlas con: `sudo apt install build-essential libx11-dev`)*
* El archivo `diccionario.txt` debe estar en el directorio raíz del proyecto para que la tabla hash dinámica se cargue correctamente.

---

##  Compilación

El proyecto utiliza un `Makefile` para facilitar la construcción de los ejecutables. 
Abre tu terminal en la carpeta raíz del proyecto y ejecuta:

```bash
make
```
Esto generará automáticamente los binarios dentro de una carpeta llamada `bin/`.

> **Nota:** Para limpiar los binarios antiguos antes de recompilar, puedes usar:
>
> ```bash
> make clean
> ```

# Instrucciones de Ejecución

El sistema se compone de dos partes independientes:

- **Servidor:** `IALearner`
- **Gestor de clientes:** `Launcher`

## Paso 1: Iniciar el Servidor

En tu primera terminal, ejecuta el servidor:

```bash
./bin/ialearner
```

> **Importante:** El servidor utiliza asignación dinámica de puertos. Al iniciarse, mostrará un mensaje similar a:
>
> ```text
> Servidor activo. Escuchando en el puerto: 45678
> ```
>
> Copia ese número, ya que será necesario para iniciar el launcher.

## Paso 2: Iniciar el Launcher (Gestor de Ventanas)

Abre una segunda terminal y ejecuta el launcher, indicando el puerto proporcionado por el servidor:

```bash
./bin/launcher <PUERTO_ASIGNADO>
```

Ejemplo:

```bash
./bin/launcher 45678
```

# Uso del Menú Principal (Launcher)

Una vez iniciado el **Launcher**, se mostrará un menú interactivo con las siguientes opciones:

## 🖥️ Uso del Menú Principal (Launcher)

Una vez iniciado el Launcher, verás un menú interactivo con las siguientes opciones:

1.  **Abrir N numero de ventanas:** Te preguntará cuántas ventanas gráficas deseas abrir. Cada ventana actúa como un keylogger independiente que requiere conexión al servidor. Captura las teclas (incluyendo espacios y signos de puntuación) y las envía al servidor para formar oraciones.
2.  **Cerrar ventana con ID:** Permite destruir una ventana específica ingresando su ID corto (1, 2, 3...) o su PID (Process ID) del sistema operativo.
3.  **Cerrar todas las ventanas:** Limpia el entorno cerrando de golpe todos los clientes activos.
4.  **Ver ventanas abiertas con su ID:** Muestra una tabla que relaciona el ID interno asignado por el launcher con el PID real de la ventana en Linux.
5.  **Salir del sistema:** Cierra todas las ventanas de forma segura y termina el proceso del launcher.

# 📊 Resultados y Auditoría

Cada vez que una ventana finaliza una oración (presionando **Enter**), el servidor realiza automáticamente las siguientes acciones:

- Tokeniza la oración.
- Elimina los signos de puntuación.
- Convierte el texto a minúsculas.
- Clasifica las palabras según su categoría.

## Ver el resumen final

Para visualizar el resumen estadístico y la clasificación final del usuario, detén el servidor presionando:

```text
Ctrl + C
```

Al finalizar, el sistema:

- Libera toda la memoria dinámica utilizada.
- Muestra el número total de documentos clasificados:
  - Correo
  - Ciencia
  - Reporte
- Calcula el porcentaje de predominancia.
- Presenta el resultado final del clasificador 

## Archivo de auditoría

Toda la actividad del sistema se registra automáticamente en:

```text
debug_learner.txt
```

Este archivo incluye información detallada como:

- Conexión de hilos.
- Aislamiento de palabras.
- Clasificaciones realizadas.
- Resúmenes del procesamiento.
