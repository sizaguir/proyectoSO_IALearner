#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int n_windows;
    
    printf("=== Agentic OS Launcher ===\n");
    printf("¿Cuántas ventanas deseas abrir?: ");
    
    if (scanf("%d", &n_windows) != 1 || n_windows <= 0) {
        printf("Por favor, ingresa un número válido mayor a 0.\n");
        return 1;
    }

    printf("[Launcher] Iniciando %d procesos de ventanas...\n", n_windows);

    // Bucle para crear los N procesos hijos
    for (int i = 0; i < n_windows; i++) {
        pid_t pid = fork(); // Clonamos el proceso

        if (pid < 0) {
            // Falla al crear el hijo
            perror("[Launcher] Error en fork");
            exit(1);
            
        } else if (pid == 0) {
            // --- CÓDIGO DEL PROCESO HIJO ---
            // Reemplazamos el programa actual por el cliente de la ventana
            char *args[] = {"./bin/window_client", NULL};
            
            // execvp ejecuta el binario. Si tiene éxito, el código de abajo nunca se ejecuta
            execvp(args[0], args);
            
            // Si execvp falla (ej. no encuentra el archivo), lanza error y muere
            perror("[Launcher] Error en execvp. ¿Compilaste window_client?");
            exit(1);
        }
        // El proceso padre (Launcher) continúa el bucle para crear más ventanas
    }

    // --- CÓDIGO DEL PROCESO PADRE ---
    printf("[Launcher] Ventanas desplegadas. Esperando a que el usuario las cierre...\n");
    
    // Evitamos procesos "Zombies" esperando a que todos los hijos terminen
    while (wait(NULL) > 0);

    printf("[Launcher] Todas las ventanas han terminado. Sistema cerrado limpiamente.\n");
    return 0;
}