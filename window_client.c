#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

int main(int argc, char *argv[])
{
    // 1. Validar que el launcher nos haya pasado el puerto
    if (argc != 2)
    {
        printf("\nError: Se requiere el puerto del servidor como argumento.\n");
        return 1;
    }

    // INICIO DEL CÓDIGO DE SOCKETS (CLIENTE)
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error de creación de Socket \n");
        return -1;
    }

    // 2. Extraer el puerto de los argumentos (dinámico)
    int port = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port); // <-- Aquí usamos la variable 'port'

    // Convertir direcciones IPv4 e IPv6 de texto a binario (localhost)
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("\nDirección inválida/ No soportada \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConexión fallida al IALearner. ¿Está corriendo el servidor en el puerto %d?\n", port);
        return -1;
    }
    printf("Conectado exitosamente al IALearner en el puerto %d.\n", port);

    // CÓDIGO X11
    Display *display = XOpenDisplay(NULL);
    if (!display)
    {
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
        WhitePixel(display, screen));
    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);
    XEvent event;

    while (1)
    {
        XNextEvent(display, &event);
        if (event.type == KeyPress)
        {
            KeySym keysym = XLookupKeysym(&event.xkey, 0);
            char *name = XKeysymToString(keysym);

            if (name)
            {
                printf("Key pressed: %s\n", name);
                if (send(sock, name, strlen(name), 0) < 0)
                {
                    printf("\n[!] Error: El servidor IALearner se ha cerrado o te ha desconectado por inactividad.\n");
                    break; // Sale del while y cierra la ventana X11 limpiamente
                }
            }
            else
            {
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