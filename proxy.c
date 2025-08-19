#include <stdio.h>
#include <stdlib.h>
#include <argtable3.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>

#define BUF_SIZE 256
pthread_mutex_t mutex;
volatile sig_atomic_t running = 1;

typedef struct SerialThreadArgs SerialThreadArgs;

struct SerialThreadArgs
{
    char *port_device;
    int port_baud;
    volatile int fd;
    SerialThreadArgs *output;
};

void handle_sigint(int sig)
{
    printf("\nSignal %d received. Stopping...\n", sig);
    running = 0;
}

void thread_settings(void *raw_args)
{
    SerialThreadArgs *args = (SerialThreadArgs *)raw_args;
    int fd_input = open(args->port_device, O_RDWR | O_NOCTTY);

    struct termios tty;
    tcgetattr(fd_input, &tty);
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ICANON;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;
    tcsetattr(fd_input, TCSANOW, &tty);
}

void *thread_function(void *raw_args)
{
    char buf[BUF_SIZE];
    SerialThreadArgs *args = (SerialThreadArgs *)raw_args;
    while (running)
    {
        pthread_mutex_lock(&mutex);
        args->fd = -1;
        pthread_mutex_unlock(&mutex);

        int fd_input = open(args->port_device, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (ioctl(fd_input, TIOCEXCL) == -1)
        {
            perror("Error opening serial port");
            fprintf(stderr, "Error opening file: %s %s\n", args->port_device, strerror(errno));
            sleep(3);
            continue;
        }

        printf("Port %s connected\n", args->port_device);

        pthread_mutex_lock(&mutex);
        args->fd = fd_input;
        pthread_mutex_unlock(&mutex);

        thread_settings(args);

        while (running)
        {
            ssize_t n = read(fd_input, buf, BUF_SIZE);
            if (n > 0)
            {
                int fd_output;

                pthread_mutex_lock(&mutex);
                fd_output = args->output->fd;
                pthread_mutex_unlock(&mutex);

                if (fd_output != -1)
                {
                    write(fd_output, buf, n);
                    write(STDOUT_FILENO, buf, n);
                }
            }
            else if (n == 0)
            {
                perror("Error 1 with port");
                break;
                // continue;
            }
            else if (n == -1)
            {
                if (errno == EIO || errno == ENXIO || errno == ENODEV)
                {
                    perror("Error 2 with port");
                    break;
                }
            }
        }

        ioctl(fd_input, TIOCNXCL);
        close(fd_input);
    }
    pthread_exit(NULL);
}

int runtime(char *serial_device_port1, char *serial_device_port2, int baud)
{
    printf("Serial Port 1: %s\n", serial_device_port1);
    printf("Serial Port 2: %s\n", serial_device_port2);
    printf("Baud: %d\n", baud);

    signal(SIGINT, handle_sigint);

    SerialThreadArgs args1;
    SerialThreadArgs args2;

    args1.port_device = serial_device_port1;
    args2.port_device = serial_device_port2;
    args1.port_baud = baud;
    args2.port_baud = baud;
    args1.fd = -1;
    args2.fd = -1;
    args1.output = &args2;
    args2.output = &args1;

    pthread_t th1, th2;
    pthread_mutex_init(&mutex, NULL);
    pthread_create(&th1, NULL, thread_function, &args1);
    pthread_create(&th2, NULL, thread_function, &args2);

    pthread_join(th1, NULL);
    pthread_join(th2, NULL);

    printf("Threads is over\n");
    pthread_mutex_destroy(&mutex);
    return 0;
}

int main(int argc, char *argv[])
{
    struct arg_str *serial_device_1 = arg_str1(NULL, "serial-port-1", "<device>", "Serial port 1 (e.g., /dev/ttyUSB0)");
    struct arg_str *serial_device_2 = arg_str1(NULL, "serial-port-2", "<device>", "Serial port 2 (e.g., /dev/ttyUSB1)");
    struct arg_int *baud = arg_int1(NULL, "baud", "<n>", "Baud rate (e.g., 115200)");
    struct arg_lit *help = arg_litn(NULL, "help", 0, 1, "display this help and exit");
    struct arg_end *end = arg_end(20);

    void *argtable[] = {serial_device_1, serial_device_2, baud, help, end};

    int nerrors = arg_parse(argc, argv, argtable);

    if (help->count > 0)
    {
        printf("Usage: %s", argv[0]);
        arg_print_syntax(stdout, argtable, "\n");
        printf("Options:\n");
        arg_print_glossary(stdout, argtable, "  %-25s %s\n");
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 0;
    }

    if (nerrors > 0)
    {
        arg_print_errors(stdout, end, argv[0]);
        printf("Usage:\n");
        arg_print_glossary(stdout, argtable, "  %-25s %s\n");
        return 1;
    }

    char *port1 = (char *)serial_device_1->sval[0];
    char *port2 = (char *)serial_device_2->sval[0];
    int baudrate = baud->ival[0];

    int exit_code = runtime(port1, port2, baudrate);

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return exit_code;
}
