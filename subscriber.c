#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "server.h"

void check_arguments(int argc) {
    if(argc < 4)
        DIE("arguments");
}

int create_tcp_socket() {
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(tcp_sock < 0)
        DIE("socket");
    return tcp_sock;
}

struct sockaddr_in configure_server_data(char *ip_address, char *port) {
    struct sockaddr_in server_data;
    server_data.sin_family = AF_INET;
    server_data.sin_port = htons(atoi(port));
    inet_aton(ip_address, &server_data.sin_addr);
    return server_data;
}

void connect_to_server(int tcp_sock, struct sockaddr_in *server_data) {
    if (connect(tcp_sock, (struct sockaddr *)server_data, sizeof(*server_data)) < 0)
        DIE("connect");
}

void send_initial_data(int tcp_sock, char *client_id) {
    if (send(tcp_sock, client_id, 10, 0) < 0)
        DIE("send");
}

void disable_nagle_algorithm(int tcp_sock) {
    int flag = 1;
    if(setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0) {
        DIE("nagle");
    }
}

void handle_stdin_input(int tcp_sock);
void handle_server_message(int tcp_sock);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    check_arguments(argc);

    int tcp_sock = create_tcp_socket();

    struct sockaddr_in server_data = configure_server_data(argv[2], argv[3]);

    fd_set write_file;
    FD_ZERO(&write_file);

    FD_SET(tcp_sock, &write_file);
    FD_SET(STDIN_FILENO, &write_file);

    connect_to_server(tcp_sock, &server_data);

    send_initial_data(tcp_sock, argv[1]);

    disable_nagle_algorithm(tcp_sock);

    while(1) {
        fd_set read_file = write_file;

        if (select(tcp_sock + 1, &read_file, NULL, NULL, NULL) < 0)
            DIE("select");

        if (FD_ISSET(STDIN_FILENO, &read_file)) {
            handle_stdin_input(tcp_sock);
        }

        if(FD_ISSET(tcp_sock, &read_file)) {
            handle_server_message(tcp_sock);
        }
    }

    close(tcp_sock);
    return 0;
}

void handle_stdin_input(int tcp_sock) {
    char buffer[100];
    memset(buffer, 0, 100);
    fgets(buffer, 100, stdin);
    Packet pack;
    memset(&pack, 0, PACKLEN);

    CommandType command = get_command_type(buffer);

    switch (command) {
        case CMD_EXIT:
            strcpy(pack.type, "exit");
            if (send(tcp_sock, &pack, PACKLEN, 0) < 0)
                DIE("send");
            exit(0);

        case CMD_SUBSCRIBE: {
            sscanf(buffer, "%10s %50s %c", pack.type, pack.topic, &pack.tip_date);
            if (send(tcp_sock, &pack, PACKLEN, 0) < 0)
                DIE("send");
            printf("Subscribed to topic.\n");
            break;
        }

        case CMD_UNSUBSCRIBE: {
            sscanf(buffer, "%10s %50s %c", pack.type, pack.topic, &pack.tip_date);

            if (send(tcp_sock, &pack, PACKLEN, 0) < 0)
                DIE("send");
            printf("Unsubscribed from topic.\n");
            break;
        }

        default:
            printf("Invalid command.\n");
            break;
    }
}

void handle_server_message(int tcp_sock) {
    char buffer[sizeof(struct tcp_struct)];
    memset(buffer, 0, sizeof(struct tcp_struct));

    int ret = recv(tcp_sock, buffer, sizeof(struct tcp_struct), 0);
    if (ret < 0)
        DIE("receive");

    if (ret == 0) {
        exit(0);
    }

    struct tcp_struct *pack_send = (struct tcp_struct *)buffer;
    printf("%s:%u - %s - %s - %s\n", pack_send->ip, pack_send->port,
           pack_send->topic, pack_send->type, pack_send->continut);
}
