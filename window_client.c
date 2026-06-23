#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> // Librerías para Sockets

#define PORT 8080

int main(void) {
    // ==========================================
    // INICIO DEL CÓDIGO DE SOCKETS (CLIENTE)
    // ==========================================
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error de creación de Socket \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convertir direcciones IPv4 e IPv6 de texto a binario (localhost)
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nDirección inválida/ No soportada \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConexión fallida al IALearner. ¿Está corriendo el servidor?\n");
        return -1;
    }
    printf("Conectado exitosamente al IALearner.\n");
    // ==========================================
    // FIN DEL CÓDIGO DE SOCKETS
    // ==========================================

    // CÓDIGO ORIGINAL DE X11
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }
    int screen = DefaultScreen(display);
    Window window = XCreateSimpleWindow(
        display,
        RootWindow(display, screen),
        10, 10, 400, 200,
        1,
        BlackPixel(display, screen),
        WhitePixel(display, screen)
    );
    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);
    XEvent event;
    
    while (1) {
        XNextEvent(display, &event);
        if (event.type == KeyPress) {
            KeySym keysym = XLookupKeysym(&event.xkey, 0);
            char *name = XKeysymToString(keysym);
            
            if (name) {
                // ==========================================
                // MODIFICACIÓN: Enviar la tecla por red
                // ==========================================
                printf("Key pressed: %s\n", name);
                send(sock, name, strlen(name), 0);
            } else {
                printf("Unknown key\n");
            }
            if (keysym == XK_Escape)
                break;
        }
    }
    
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    
    // Cerrar el socket al salir
    close(sock); 
    return 0;
}
