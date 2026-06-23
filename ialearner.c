#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// --- 1. DICCIONARIOS ---
const char *dict_email[] = {"thank", "thanks", "please", "regards", "meeting", "attached", "information", "update", "schedule", "team", "project", NULL};
const char *dict_science[] = {"data", "analysis", "results", "method", "study", "model", "research", "system", "significant", "effect", NULL};
const char *dict_report[] = {"system", "data", "network", "security", "application", "server", "user", "performance", "service", "infrastructure", NULL};

// --- 2. VARIABLES GLOBALES ---
int docs_email = 0;
int docs_science = 0;
int docs_report = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int server_fd; 

// Función para escribir en el archivo TXT de forma segura
void log_to_file(const char *message) {
    // Abrimos en modo "a" (append) para no borrar lo anterior
    FILE *file = fopen("debug_learner.txt", "a");
    if (file) {
        fprintf(file, "%s", message);
        fclose(file);
    }
}

int word_in_dict(char *word, const char **dict) {
    for(int i = 0; dict[i] != NULL; i++) {
        if(strcmp(word, dict[i]) == 0) return 1;
    }
    return 0;
}

// --- FASE 6: CLASIFICACIÓN FINAL ---
void handle_sigint(int sig) {
    (void)sig; 
    char buffer_log[512];
    
    printf("\n\n--- EVALUANDO PERFIL DE USUARIO ---\n");
    
    sprintf(buffer_log, "\n=== RESUMEN FINAL ===\nDocumentos -> Correos: %d | Articulos: %d | Reportes: %d\nPERFIL: ", docs_email, docs_science, docs_report);
    log_to_file(buffer_log);

    if (docs_email > 0 && docs_science == 0 && docs_report == 0) {
        log_to_file("Personal Administrativo\n");
        printf("Personal Administrativo\n");
    } else if (docs_email > 0 && docs_report > 0 && docs_science == 0) {
        log_to_file("Personal Tecnico\n");
        printf("Personal Técnico\n");
    } else if (docs_email > 0 && docs_science > 0) {
        log_to_file("Profesor\n");
        printf("Profesor\n");
    } else if (docs_science > 0 && docs_report > 0) {
        log_to_file("Estudiante\n");
        printf("Estudiante\n");
    } else {
        log_to_file("Indefinido\n");
        printf("Indefinido\n");
    }
    
    close(server_fd);
    exit(0);
}

// --- FASE 5: LA LÓGICA DE CADA HILO ---
void *handle_client(void *socket_desc) {
    int sock = *(int*)socket_desc;
    free(socket_desc); 
    
    char buffer[BUFFER_SIZE] = {0};
    char sentence[4096] = {0}; 
    int pos = 0;
    int valread;
    char log_buffer[4500];

    int count_email = 0, count_science = 0, count_report = 0;

    // Escribimos en el log que se conectó una nueva ventana
    sprintf(log_buffer, "\n[Hilo %lu] NUEVA VENTANA CONECTADA\n", pthread_self());
    log_to_file(log_buffer);

    while ((valread = read(sock, buffer, BUFFER_SIZE)) > 0) {
        buffer[valread] = '\0'; 

        if (strcmp(buffer, "space") == 0) {
            sentence[pos++] = ' ';
        } else if (strcmp(buffer, "period") == 0) { // <-- Agregamos soporte manual para el punto
            sentence[pos++] = '.';
        } else if (strcmp(buffer, "semicolon") == 0) { // <-- NUEVO
            sentence[pos++] = ';';
        } else if (strcmp(buffer, "colon") == 0) {     // <-- NUEVO
            sentence[pos++] = ':';
        } else if (strcmp(buffer, "comma") == 0) {  // <-- Agregamos soporte para la coma
            sentence[pos++] = ',';
        } else if (strcmp(buffer, "Return") == 0 || strcmp(buffer, "KP_Enter") == 0) {
            sentence[pos] = '\0';
            
            // Log de la oración completa recibida
            pthread_mutex_lock(&lock);
            sprintf(log_buffer, "[Hilo %lu] Oracion cruda recibida: '%s'\n", pthread_self(), sentence);
            log_to_file(log_buffer);
            pthread_mutex_unlock(&lock);

            char *word = strtok(sentence, " \n\r\t.,;:");
            while (word != NULL) {
                for(int i = 0; word[i]; i++) word[i] = tolower(word[i]);

                // Log de cada palabra extraída
                pthread_mutex_lock(&lock);
                sprintf(log_buffer, "    -> Palabra aislada por strtok: '%s'\n", word);
                log_to_file(log_buffer);
                pthread_mutex_unlock(&lock);

                if (word_in_dict(word, dict_email)) count_email++;
                if (word_in_dict(word, dict_science)) count_science++;
                if (word_in_dict(word, dict_report)) count_report++;

                word = strtok(NULL, " \n\r\t.,;:");
            }
            
            memset(sentence, 0, sizeof(sentence));
            pos = 0;
            
        } else if (strlen(buffer) == 1) { 
            sentence[pos++] = buffer[0];
        }
    }
    // ==========================================
    // EL SALVAVIDAS: Evaluar texto huérfano
    // ==========================================
    if (pos > 0) {
        sentence[pos] = '\0'; // Cerramos la cadena
        
        pthread_mutex_lock(&lock);
        char log_huerfano[512];
        snprintf(log_huerfano, sizeof(log_huerfano), "[Hilo %lu] Evaluando texto huerfano al cerrar: '%s'\n", pthread_self(), sentence);
        log_to_file(log_huerfano);
        pthread_mutex_unlock(&lock);

        char *word = strtok(sentence, " \n\r\t.,;:");
        while (word != NULL) {
            for(int i = 0; word[i]; i++) word[i] = tolower(word[i]);

            if (word_in_dict(word, dict_email)) count_email++;
            if (word_in_dict(word, dict_science)) count_science++;
            if (word_in_dict(word, dict_report)) count_report++;

            word = strtok(NULL, " \n\r\t.,;:");
        }
    }

    // CLASIFICACIÓN AL CERRAR VENTANA
    int is_email = 0, is_science = 0, is_report = 0;

    if (count_email >= 3) is_email = 1;
    if (count_science >= 3) is_science = 1;
    if (count_report >= 3) is_report = 1;

    if (is_email + is_science + is_report > 1) {
        is_email = is_science = is_report = 0; 
        if (count_email >= count_science && count_email >= count_report) is_email = 1;
        else if (count_science >= count_email && count_science >= count_report) is_science = 1;
        else is_report = 1;
    }

    pthread_mutex_lock(&lock);
    sprintf(log_buffer, "[Hilo %lu] RESULTADO: Email(%d) | Ciencia(%d) | Reporte(%d)\n", pthread_self(), count_email, count_science, count_report);
    log_to_file(log_buffer);
    
    if (is_email) {
        docs_email++;
        log_to_file("[Hilo] Clasificado como: CORREO\n");
    } else if (is_science) {
        docs_science++;
        log_to_file("[Hilo] Clasificado como: CIENCIA\n");
    } else if (is_report) {
        docs_report++;
        log_to_file("[Hilo] Clasificado como: REPORTE\n");
    } else {
        log_to_file("[Hilo] Clasificado como: DESCONOCIDO\n");
    }
    pthread_mutex_unlock(&lock);

    close(sock);
    pthread_exit(NULL);
}

int main() {
    // Limpiamos el log anterior al arrancar el servidor
    FILE *f = fopen("debug_learner.txt", "w");
    if(f) { fprintf(f, "--- INICIO DEL SERVIDOR ---\n"); fclose(f); }

    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    signal(SIGINT, handle_sigint);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) exit(EXIT_FAILURE);
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) exit(EXIT_FAILURE);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) exit(EXIT_FAILURE);
    if (listen(server_fd, 10) < 0) exit(EXIT_FAILURE);
    
    printf("Servidor activo. Revisa 'debug_learner.txt' para ver la auditoria en tiempo real.\n");

    while (1) {
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) continue;

        int *new_sock = malloc(sizeof(int));
        *new_sock = new_socket;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, (void*)new_sock);
        pthread_detach(thread_id); 
    }
    return 0;
}