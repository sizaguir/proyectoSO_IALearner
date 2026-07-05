#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

// Nodo para Lista Enlazada Dinámica
typedef struct NodoVentana {
    int id;
    pid_t pid;
    struct NodoVentana *siguiente;
} NodoVentana;

NodoVentana *cabeza = NULL; // Inicio de la lista
int contador_ids = 1;
char *puerto_servidor = NULL;

// --- FUNCIONES DE LISTA ENLAZADA ---
void insertar_ventana(int id, pid_t pid) {
    NodoVentana *nuevo = malloc(sizeof(NodoVentana));
    nuevo->id = id;
    nuevo->pid = pid;
    nuevo->siguiente = cabeza;
    cabeza = nuevo;
}

void eliminar_ventana_por_pid(pid_t pid) {
    NodoVentana *actual = cabeza;
    NodoVentana *anterior = NULL;

    while (actual != NULL && actual->pid != pid) {
        anterior = actual;
        actual = actual->siguiente;
    }
    if (actual == NULL) return; // No se encontró
    if (anterior == NULL) cabeza = actual->siguiente; // Era la cabeza
    else anterior->siguiente = actual->siguiente;
    
    free(actual); // Liberar memoria del nodo
}

void limpiar_ventanas_cerradas() {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        eliminar_ventana_por_pid(pid);
    }
}

// --- LOGICA DEL LAUNCHER ---
void abrir_ventanas(int n) {
    int creadas = 0;
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("[Launcher] Error en fork");
        } else if (pid == 0) {
            // El hijo ejecuta el cliente pasándole el puerto
            char *args[] = {"./bin/window_client", puerto_servidor, NULL};
            execvp(args[0], args);
            perror("Error. ¿Compilaste window_client?");
            exit(1);
        } else {
            insertar_ventana(contador_ids++, pid);
            creadas++;
        }
    }
    printf("\n[+] Se abrieron %d ventanas exitosamente.\n", creadas);
}

void ver_ventanas() {
    printf("\n--- VENTANAS ABIERTAS ---\n");
    NodoVentana *actual = cabeza;
    int activas = 0;
    while (actual != NULL) {
        printf("ID: %d \t PID OS: %d\n", actual->id, actual->pid);
        activas++;
        actual = actual->siguiente;
    }
    if (activas == 0) printf("No hay ninguna ventana abierta.\n");
    printf("-------------------------\n");
}

void cerrar_ventana_id(int id_buscar) {
    NodoVentana *actual = cabeza;
    while (actual != NULL) {
        if (actual->id == id_buscar) {
            kill(actual->pid, SIGTERM);
            printf("\n[-] Ventana con ID %d cerrada.\n", id_buscar);
            return;
        }
        actual = actual->siguiente;
    }
    printf("\n[!] No se encontro ventana activa con ID %d.\n", id_buscar);
}

void cerrar_todas_las_ventanas() {
    NodoVentana *actual = cabeza;
    int cerradas = 0;
    while (actual != NULL) {
        kill(actual->pid, SIGTERM);
        NodoVentana *a_borrar = actual;
        actual = actual->siguiente;
        free(a_borrar); // Limpiar memoria de la lista
        cerradas++;
    }
    cabeza = NULL; // Lista vacía
    if (cerradas > 0) printf("\n[-] Se cerraron %d ventanas.\n", cerradas);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("ERROR de uso. Ejecuta asi: %s <PUERTO>\n", argv[0]);
        printf("Ejemplo: %s 45678\n", argv[0]);
        return 1;
    }
    puerto_servidor = argv[1];
    
    int opcion = 0;
    printf("=== Agentic OS Launcher Iniciado (Apuntando a puerto %s) ===\n", puerto_servidor);

    while (1) {
        limpiar_ventanas_cerradas(); 

        printf("\n=== MENU PRINCIPAL ===\n");
        printf("1. Abrir N numero de ventanas\n");
        printf("2. Cerrar ventana con ID\n");
        printf("3. Cerrar todas las ventanas\n");
        printf("4. Ver ventanas abiertas con su ID\n");
        printf("5. Salir del sistema\n");
        printf("Selecciona una opcion: ");
        
        if (scanf("%d", &opcion) != 1) {
            while(getchar() != '\n'); 
            continue;
        }

        if (opcion == 1) {
            int n;
            printf("¿Cuantas ventanas deseas abrir?: ");
            if (scanf("%d", &n) == 1 && n > 0) abrir_ventanas(n);
        } else if (opcion == 2) {
            int id_cerrar;
            printf("Ingresa ID de la ventana a cerrar: ");
            if (scanf("%d", &id_cerrar) == 1) cerrar_ventana_id(id_cerrar);
        } else if (opcion == 3) {
            cerrar_todas_las_ventanas();
        } else if (opcion == 4) {
            ver_ventanas();
        } else if (opcion == 5) {
            printf("\nCerrando sistema...\n");
            cerrar_todas_las_ventanas();
            while (wait(NULL) > 0); 
            printf("Sistema cerrado limpiamente.\n");
            break;
        }
    }
    return 0;
}