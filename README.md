# Agentic OS: Sistema Simulador de Clasificación de Perfiles (IALearner)

Este proyecto implementa un entorno de sistema operativo simulado basado en una **Arquitectura Cliente-Servidor Distribuida**. Combina el control de procesos concurrentes, la comunicación interprocesos mediante redes y la captura de interfaces gráficas nativas para analizar el comportamiento del usuario a través de un algoritmo de procesamiento de texto estructurado (*Bag of Words*).

---

##  1. Arquitectura y Diseño del Sistema 

El diseño del proyecto se fundamenta en principios sólidos de sistemas operativos, garantizando el aislamiento de fallos, el procesamiento concurrente y la exclusión mutua.

### A. Comunicación Interprocesos (IPC) y Modelo de Red
* **Mecanismo:** Sockets de Red TCP/IP (Dominio `AF_INET`, Tipo `SOCK_STREAM`).
* **Justificación:** Se seleccionó TCP sobre otros mecanismos de IPC (como Pipes o Colas de Mensajes nativas) porque es un protocolo **orientado a la conexión** que garantiza la entrega de bytes en el orden exacto y libre de errores. En un analizador de texto (*Bag of Words*), la pérdida o alteración de un solo carácter corrompería la palabra, invalidando las métricas de la IA.

### B. Gestión y Ciclo de Vida de Procesos (Launcher)
* **Llamadas al Sistema (Syscalls):** `fork()`, `execvp()`, y `wait()`.
* **Justificación:** El administrador de procesos (`launcher.c`) clona el proceso padre usando `fork()` para generar $N$ entornos aislados de ejecución. Cada hijo invoca `execvp()` para reemplazar su imagen de memoria por el ejecutable gráfico `./bin/window_client`. El proceso padre ejecuta un bucle bloqueante con `wait()` para recolectar el estado de salida de sus hijos, garantizando que el núcleo libere las entradas correspondientes en la tabla de procesos y previniendo por completo la existencia de Procesos Zombie o huérfanos.

### C. Concurrencia y Sincronización en el Servidor
* **Llamadas al Sistema:** `pthread_create()`, `pthread_detach()`, `pthread_mutex_lock()`, `pthread_mutex_unlock()`.
* **Hilos vs Procesos:** Se decidió estructurar el servidor de IA (`ialearner.c`) utilizando hilos POSIX en lugar de múltiples procesos. Los hilos comparten el mismo espacio de direccionamiento de memoria virtual, lo que permite que todas las ventanas actualicen de forma directa los contadores globales de documentos (`docs_email`, `docs_science`, `docs_report`). 
* **Atributo Detach:** El uso de `pthread_detach()` le indica al planificador del kernel que recicle los recursos físicos del hilo inmediatamente tras su terminación, optimizando el uso de la memoria RAM.
* **Exclusión Mutua (Mutex):** Para prevenir Condiciones de Carrera (Race Conditions) en el acceso al archivo de auditoría y a los contadores globales, se implementó un cerrojo estático `pthread_mutex_t lock`. Esto asegura que si múltiples ventanas envían ráfagas de palabras simultáneamente, las solicitudes se encolen de forma síncrona.

### D. Estructuras de Datos Utilizadas
* **Diccionarios Estáticos:** Matrices de punteros constantes (`const char *dict[]`) inicializadas en tiempo de compilación. Ofrecen un acceso de complejidad temporal constante $O(1)$ en memoria para la comparación léxica mediante `strcmp`.
* **Buffers Estáticos de Memoria:** Arreglos de caracteres fijos (`char sentence[4096]`). Al evitar la asignación dinámica de memoria (`malloc`/`free`) dentro de los hilos de ejecución continua, se anula el riesgo de fragmentación de memoria y se previenen por completo las fugas de memoria (**Memory Leaks**), garantizando estabilidad total en análisis con Valgrind.


##  2. Prerrequisitos e Instalación

Si el repositorio se descarga en una computadora nueva, es obligatorio instalar las herramientas de desarrollo básicas y las librerías gráficas de X11 mediante la terminal de Linux (Ubuntu/Debian/WSL):

```bash
# 1. Actualizar los índices de los paquetes
sudo apt update

# 2. Instalar el compilador GCC y utilitarios de desarrollo (Make)
sudo apt install build-essential

# 3. Instalar la librería de desarrollo nativa de X11 (Crítico)
sudo apt install libx11-dev

```

## 3. Compilación Automatizada
El proyecto implementa un archivo de automatización Makefile que gestiona las dependencias de enlazado de librerías (-lX11 y -lpthread) y compila con banderas estrictas de depuración (-Wall -Wextra -g).

Para compilar todo el sistema desde cero, ejecute en la raíz:
```bash
make clean
make
```
Los binarios resultantes se organizarán automáticamente dentro del directorio estandarizado ./bin/.


## 4. Protocolo de Ejecución y Pruebas
El sistema opera bajo un modelo distribuido, por lo que requiere el uso de dos terminales de comando independientes:

#### Paso 1: Inicializar el Servidor (Terminal 1)
Inicie el proceso analítico central. Se mantendrá a la escucha de conexiones entrantes en el puerto 8080:

```bash
./bin/ialearner
```
#### Paso 2: Desplegar los Clientes mediante el Launcher (Terminal 2)
En una nueva ventana de la terminal, ejecute el gestor de procesos:
```bash
./bin/launcher
```
El programa solicitará el número de ventanas gráficas a desplegar.

Ingrese el número deseado (ej. 2). Al instante, se dibujarán las ventanas blancas de X11 en su pantalla.

#### Paso 3: Simulación de Carga de Datos e Interacción
- Haga clic en una de las ventanas generadas para asignarle el foco del teclado.

- Escriba cadenas de texto combinando las palabras clave de los diccionarios del sistema.

- El sistema cuenta con soporte de puntuación avanzado. Puede separar palabras usando Espacios, Puntos (.), Comas (,), Puntos y Comas (;) o Dos Puntos (:).

- Presione la tecla Enter para forzar el análisis de la oración. (El sistema soporta tanto la tecla Return principal como KP_Enter de teclados extendidos).

- Presione Escape para cerrar la ventana actual.

Nota: Si un usuario escribe un texto y cierra la ventana usando Escape sin presionar Enter, el servidor ejecutará automáticamente un algoritmo de captura de texto huérfano, procesando las palabras remanentes antes de destruir el hilo para evitar la pérdida de información.

#### Paso 4: Cierre del Sistema y Evaluación de Perfil
Una vez cerradas todas las ventanas de X11, diríjase a la Terminal 1 (servidor) y envíe una señal de interrupción presionando Ctrl + C.

El servidor interceptará la señal SIGINT, detendrá el bucle de escucha, liberará los recursos de red y desplegará en la terminal el Reporte Final de la IA con la clasificación global del perfil del usuario basado en la matriz de documentos recolectada:

- **Personal Administrativo:** Solo correos detectados.

- **Personal Técnico:** Correos y reportes técnicos detectados.

- **Profesor:** Correos y artículos científicos detectados.

- **Estudiante:** Artículos científicos y reportes técnicos detectados.

## 5. Sistema de Auditoría (Logs de Memoria)
El programa genera dinámicamente un archivo llamado debug_learner.txt en la raíz del proyecto. Este archivo registra paso a paso el comportamiento interno de los descriptores de archivos y los hilos en el kernel:

Conexión y asignación de memoria para nuevos hilos concurrentes.

Volcado de la oración cruda exacta recibida a través del buffer de red.

Trazabilidad del tokenizador strtok aislando palabras y eliminando signos de puntuación.

Puntuación parcial léxica obtenida antes de aplicar la regla de clasificación de documentos.