#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <signal.h>
#include <sys/time.h>

#define HASH_SIZE 10007 // Tamaño primo para mejor dispersión

// --- 1. ESTRUCTURAS DINÁMICAS (HASHMAP) ---
typedef struct HashNode
{
    char *palabra;
    int categoria; // 1: Email, 2: Ciencia, 3: Reporte
    struct HashNode *siguiente;
} HashNode;

HashNode **tabla_hash = NULL; // Puntero doble para arreglo dinámico

int docs_email = 0;
int docs_science = 0;
int docs_report = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int server_fd;

// Función Hash (djb2)
unsigned long hash_string(const char *str)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash % HASH_SIZE;
}

void insertar_palabra(const char *palabra, int categoria)
{
    unsigned long indice = hash_string(palabra);
    HashNode *nuevo = malloc(sizeof(HashNode));
    nuevo->palabra = strdup(palabra);
    nuevo->categoria = categoria;
    nuevo->siguiente = tabla_hash[indice];
    tabla_hash[indice] = nuevo;
}

int buscar_categoria(const char *palabra)
{
    unsigned long indice = hash_string(palabra);
    HashNode *actual = tabla_hash[indice];
    while (actual != NULL)
    {
        if (strcmp(actual->palabra, palabra) == 0)
            return actual->categoria;
        actual = actual->siguiente;
    }
    return 0; // No encontrada
}

void cargar_diccionario()
{
    tabla_hash = calloc(HASH_SIZE, sizeof(HashNode *)); // Inicializar dinámicamente
    FILE *file = fopen("diccionario.txt", "r");
    if (!file)
    {
        perror("No se encontro diccionario.txt");
        exit(1);
    }

    char *linea = NULL;
    size_t len = 0;
    while (getline(&linea, &len, file) != -1)
    {
        linea[strcspn(linea, "\r\n")] = 0; // Limpiar saltos
        char *cat_str = strtok(linea, ",");
        char *palabra = strtok(NULL, ",");

        if (cat_str && palabra)
        {
            int cat_id = 0;
            if (strcmp(cat_str, "email") == 0)
                cat_id = 1;
            else if (strcmp(cat_str, "science") == 0)
                cat_id = 2;
            else if (strcmp(cat_str, "report") == 0)
                cat_id = 3;
            if (cat_id > 0)
                insertar_palabra(palabra, cat_id);
        }
    }
    free(linea); // Libera el buffer dinámico de getline
    fclose(file);
}

// --- 2. LOG Y CLASIFICACIÓN ---
void handle_sigint(int sig)
{
    (void)sig;
    FILE *file = fopen("debug_learner.txt", "a");
    if (file)
    {
        fprintf(file, "\n=== RESUMEN FINAL ===\nDocumentos -> Correos: %d | Articulos: %d | Reportes: %d\nPERFIL: ", docs_email, docs_science, docs_report);
        if (docs_email > 0 && docs_science == 0 && docs_report == 0)
            fprintf(file, "Personal Administrativo\n");
        else if (docs_email > 0 && docs_report > 0 && docs_science == 0)
            fprintf(file, "Personal Tecnico\n");
        else if (docs_email > 0 && docs_science > 0)
            fprintf(file, "Profesor\n");
        else if (docs_science > 0 && docs_report > 0)
            fprintf(file, "Estudiante\n");
        else
            fprintf(file, "Indefinido\n");
        fclose(file);
    }
    close(server_fd);
    printf("\nServidor cerrado. Saliendo...\n");
    exit(0);
}

void procesar_oracion(char *sentence, int *count_email, int *count_science, int *count_report)
{
    char *word = strtok(sentence, " \n\r\t.,;:");
    while (word != NULL)
    {
        for (int i = 0; word[i]; i++)
            word[i] = tolower(word[i]);

        int cat = buscar_categoria(word);
        if (cat == 1)
            (*count_email)++;
        else if (cat == 2)
            (*count_science)++;
        else if (cat == 3)
            (*count_report)++;

        pthread_mutex_lock(&lock);
        FILE *f = fopen("debug_learner.txt", "a");
        if (f)
        {
            fprintf(f, "    -> Palabra aislada por strtok: '%s'\n", word);
            fclose(f);
        }
        pthread_mutex_unlock(&lock);

        word = strtok(NULL, " \n\r\t.,;:");
    }
}

// --- 3. HILO CLIENTE ---
void *handle_client(void *socket_desc)
{
    int sock = *(int *)socket_desc;
    free(socket_desc);
    
    pid_t client_pid = 0;
    read(sock, &client_pid, sizeof(pid_t));

    int buffer_cap = 256;
    char *buffer = malloc(buffer_cap * sizeof(char));

    int sentence_cap = 256;
    char *sentence = malloc(sentence_cap * sizeof(char));
    sentence[0] = '\0';
    int pos = 0;
    int valread;

    int count_email = 0, count_science = 0, count_report = 0;

    pthread_mutex_lock(&lock);
    FILE *f = fopen("debug_learner.txt", "a");
    if (f)
    {
        fprintf(f, "\n[Hilo %lu] NUEVA VENTANA CONECTADA\n", pthread_self());
        fclose(f);
    }
    pthread_mutex_unlock(&lock);

    // --- NUEVO: Configurar un Timeout de 30 segundos ---
    struct timeval tv;
    tv.tv_sec = 30; // Si pasan 30 segundos sin recibir teclas, se desconecta
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);
    // ----------------------------------------------------

    while (1)
    {
        valread = read(sock, buffer, buffer_cap - 1);

        if (valread == 0)
        {
            break; // La ventana se cerró limpiamente (Escape)
        }
        else if (valread < 0)
        {
            printf("\n[!] TIMEOUT: Ventana PID %d inactiva por 30s. Conexion cerrada por el servidor.\n", client_pid);
            break; // Rompe el ciclo y evalúa el texto huérfano si es que quedó alguno
        }

        buffer[valread] = '\0';

        // Ampliar memoria dinámica de la oración si es necesario
        if (pos + 2 >= sentence_cap)
        {
            sentence_cap *= 2;
            sentence = realloc(sentence, sentence_cap * sizeof(char));
        }

        if (strcmp(buffer, "space") == 0)
            sentence[pos++] = ' ';
        else if (strcmp(buffer, "period") == 0)
            sentence[pos++] = '.';
        else if (strcmp(buffer, "semicolon") == 0)
            sentence[pos++] = ';';
        else if (strcmp(buffer, "colon") == 0)
            sentence[pos++] = ':';
        else if (strcmp(buffer, "comma") == 0)
            sentence[pos++] = ',';
        else if (strcmp(buffer, "Return") == 0 || strcmp(buffer, "KP_Enter") == 0)
        {
            sentence[pos] = '\0';

            pthread_mutex_lock(&lock);
            FILE *f = fopen("debug_learner.txt", "a");
            if (f)
            {
                fprintf(f, "[Hilo %lu] Oracion cruda recibida: '%s'\n", pthread_self(), sentence);
                fclose(f);
            }
            pthread_mutex_unlock(&lock);

            procesar_oracion(sentence, &count_email, &count_science, &count_report);

            // Limpiar memoria sin destruirla
            pos = 0;
            sentence[0] = '\0';
        }
        else if (strlen(buffer) == 1)
        {
            sentence[pos++] = buffer[0];
        }
    }

    if (pos > 0)
    {
        sentence[pos] = '\0';
        pthread_mutex_lock(&lock);
        FILE *f = fopen("debug_learner.txt", "a");
        if (f)
        {
            fprintf(f, "[Hilo %lu] Evaluando texto huerfano al cerrar: '%s'\n", pthread_self(), sentence);
            fclose(f);
        }
        pthread_mutex_unlock(&lock);
        procesar_oracion(sentence, &count_email, &count_science, &count_report);
    }

    // Lógica de Desempate (Opción 3 - Estricta)
    int is_email = 0, is_science = 0, is_report = 0;
    if (count_email >= 3)
        is_email = 1;
    if (count_science >= 3)
        is_science = 1;
    if (count_report >= 3)
        is_report = 1;

    if (is_email + is_science + is_report > 1)
    {
        int max = count_email;
        if (count_science > max)
            max = count_science;
        if (count_report > max)
            max = count_report;

        int empatados = 0;
        if (count_email == max)
            empatados++;
        if (count_science == max)
            empatados++;
        if (count_report == max)
            empatados++;

        is_email = is_science = is_report = 0;
        if (empatados == 1)
        {
            if (count_email == max)
                is_email = 1;
            else if (count_science == max)
                is_science = 1;
            else if (count_report == max)
                is_report = 1;
        }
    }

    pthread_mutex_lock(&lock);
    f = fopen("debug_learner.txt", "a");
    if (f)
    {
        fprintf(f, "[Hilo %lu] RESULTADO: Email(%d) | Ciencia(%d) | Reporte(%d)\n", pthread_self(), count_email, count_science, count_report);
        if (is_email)
        {
            docs_email++;
            fprintf(f, "[Hilo] Clasificado como: CORREO\n");
        }
        else if (is_science)
        {
            docs_science++;
            fprintf(f, "[Hilo] Clasificado como: CIENCIA\n");
        }
        else if (is_report)
        {
            docs_report++;
            fprintf(f, "[Hilo] Clasificado como: REPORTE\n");
        }
        else
        {
            fprintf(f, "[Hilo] Clasificado como: DESCONOCIDO\n");
        }
        fclose(f);
    }
    pthread_mutex_unlock(&lock);

    // Liberar memoria dinámica
    free(buffer);
    free(sentence);
    close(sock);
    pthread_exit(NULL);
}

int main()
{
    cargar_diccionario();

    FILE *f = fopen("debug_learner.txt", "w");
    if (f)
    {
        fprintf(f, "--- INICIO DEL SERVIDOR ---\n");
        fclose(f);
    }

    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    signal(SIGINT, handle_sigint);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        exit(EXIT_FAILURE);
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
        exit(EXIT_FAILURE);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(0); // PUERTO DINÁMICO ASIGNADO POR EL OS

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
        exit(EXIT_FAILURE);
    if (listen(server_fd, 10) < 0)
        exit(EXIT_FAILURE);

    // Extraer y mostrar el puerto asignado
    if (getsockname(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen) == -1)
        exit(EXIT_FAILURE);
    printf("Servidor activo. Escuchando en el puerto: %d\n", ntohs(address.sin_port));

    while (1)
    {
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            continue;
        int *new_sock = malloc(sizeof(int));
        *new_sock = new_socket;
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, (void *)new_sock);
        pthread_detach(thread_id);
    }
    return 0;
}