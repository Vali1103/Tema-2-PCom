#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <limits.h>
#include <math.h>
#include "server.h"

int accept_connection(int server_socket, struct sockaddr_in *new_tcp, socklen_t *dim) {
    int socket_primit = accept(server_socket, (struct sockaddr*)new_tcp, dim);
    if (socket_primit < 0)
        DIE("accept");
    return socket_primit;
}

int receive_id(int socket_primit, char *buffer) {
    if (recv(socket_primit, buffer, 10, 0) < 0)
        DIE("recv");
    return 0;
}

int find_client(client *clienti, char *buffer, int max) {
    int found = -1;
    for (int j = 0; j <= max; j++) {
        if (strcmp(clienti[j].id, buffer) == 0) {
            found = j;
            break;
        }
    }
    return found;
}

void handle_new_client(client *clienti, int socket_primit, struct sockaddr_in new_tcp, int *max, fd_set *write_file, char* buffer) {
    FD_SET(socket_primit, write_file);
    if (socket_primit > *max)
        *max = socket_primit;

    strcpy(clienti[*max].id, buffer);
    clienti[*max].socket = socket_primit;
    clienti[*max].online = 1;
    printf("New client %s connected from %s:%d\n", clienti[*max].id,
           inet_ntoa(new_tcp.sin_addr), ntohs(new_tcp.sin_port));
}

void handle_existing_client(client *clienti, int found, int socket_primit, struct sockaddr_in new_tcp, fd_set *write_file) {
    clienti[found].socket = socket_primit;
    clienti[found].online = 1;
    printf("New client %s connected from %s:%d.\n", clienti[found].id,
           inet_ntoa(new_tcp.sin_addr), ntohs(new_tcp.sin_port));

}

void process_and_send_udp_data(msg_udp *send_to_udp, msg_tcp *send_to_tcp, struct client *clients, int max) {
    uint32_t num;
    double real;

    switch (send_to_udp->type) {
        case 0: // INT
            num = ntohl(*(uint32_t *)(send_to_udp->continut + 1));
            if (send_to_udp->continut[0] == 1) {
                num = num * (-1);
            }
            sprintf(send_to_tcp->continut, "%d", num);
            strcpy(send_to_tcp->type, "INT");
            break;

        case 1: // SHORT_REAL
            real = abs(ntohs(*(uint16_t *)(send_to_udp->continut)));
            real = real / 100;
            if (send_to_udp->continut[0] == 1) {
                    real = real * (-1);
                }
            sprintf(send_to_tcp->continut, "%.2f", real);
            strcpy(send_to_tcp->type, "SHORT_REAL");
            break;

        case 2: // FLOAT
            {
                int n = 1;
                real = ntohl(*(uint32_t *)(send_to_udp->continut + 1));

                n = pow(10, send_to_udp->continut[5]);

                real = real / n;
                strcpy(send_to_tcp->type, "FLOAT");

                if (send_to_udp->continut[0] == 1) {
                    real = real * (-1);
                }
                sprintf(send_to_tcp->continut, "%lf", real);
            }
            break;

        default: // STRING
            strcpy(send_to_tcp->type, "STRING");
            strcpy(send_to_tcp->continut, send_to_udp->continut);
            break;
    }

    for (int j = 0; j <= max; j++) {
        client cl = clients[j];
        for (int k = 0; k < cl.dim_topics; k++) {
            if (strcmp(cl.nume_topic[k], send_to_tcp->topic) == 0) {
                if (cl.online) {
                    if( send(cl.socket, send_to_tcp, sizeof(struct tcp_struct), 0) < 0)
                        DIE("send");
                } else if (cl.sf_topic[k] == 1) {
                    cl.unsent[cl.dim_unsent++] = *send_to_tcp;
                }
                break;
            }
        }
    }
}

void process_subscribe(client *c, Packet *input) {
    int topicIndex = -1;

    for (int k = 0; k < c->dim_topics; k++) {
        if (strcmp(c->nume_topic[k], input->topic) == 0) {
            topicIndex = k;
            break;
        }
    }

    if (topicIndex < 0) {
        strcpy(c->nume_topic[c->dim_topics], input->topic);
        c->sf_topic[c->dim_topics] = input->tip_date;
        c->dim_topics++;
    }
}

void process_unsubscribe(client *c, Packet *input) {
    int topicIndex = -1;
    for (int k = 0; k < c->dim_topics; k++) {
        if (strcmp(c->nume_topic[k], input->topic) == 0) {
            topicIndex = k;
            break;
        }
    }

    if (topicIndex >= 0) {
        for (int l = topicIndex; l < c->dim_topics; l++) {
            c->sf_topic[l] = c->sf_topic[l + 1];
            strcpy( c->nume_topic[l] , c->nume_topic[l + 1]);
        }
        c->dim_topics--;
    }
}

void process_exit(client *clients, int i, int max, fd_set *file_descr) {
    for (int j = 0; j <= max; j++) {
        if (clients[j].socket == i) {
            printf("Client %s disconnected.\n", clients[j].id);
            clients[j].online = 0;
            clients[j].socket = -1;
            FD_CLR(i, file_descr);
            close(i);
            break;
        }
    }
}

void process_received_packet(client *clients, int i, int max, char *buffer, fd_set *file_descr) {
    Packet *input = (Packet *)buffer;
    client *c = NULL;

    for (int j = 0; j <= max; j++) {
        if (i == clients[j].socket) {
            c = &clients[j];
            break;
        }
    }

    if (strcmp(input->type, "subscribe") == 0) {
        process_subscribe(c, input);
    } else if (strcmp(input->type, "unsubscribe") == 0) {
        process_unsubscribe(c, input);
    } else if (strcmp(input->type, "exit") == 0) {
        process_exit(clients, i, max, file_descr);
    }
}

int main(int argc, char** argv) {
    // Dezactivează buffering-ul pentru stdout
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // Verifică dacă numărul de argumente este corect
	if(argc <  2){
        printf("Format ./server PORT\n");
        DIE("arguments");
    }

    int max = 0;
    // Alocă memorie pentru structurile clienti
	// int enable = 1;
	client *clienti = malloc (500 * sizeof(client));

    // Crează socket-uri pentru UDP și TCP
	int *sockets = malloc(2 * sizeof(int));
    sockets[0] = socket(PF_INET, SOCK_DGRAM, 0);    //UDP
    sockets[1] = socket(AF_INET, SOCK_STREAM, 0);  // TCP
    if(sockets[0] < 0  ||  sockets[1] < 0)
        DIE("socket");
    else{
        if(sockets[0] > sockets[1])
            max = sockets[0];
        else max = sockets[1];
    }

	// Inițializează structurile de adrese pentru server
    struct sockaddr_in servaddr, udp_addr, new_tcp;
    char buffer[BUFFER_SIZE];

    // Convertește portul din argumentele programului și setează structura de adrese pentru server
    uint16_t PORT = htons(atoi(argv[1]));

    memset((char *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = PORT;
    
	// Asociază socket-ul TCP cu adresa și portul serverului
    if(bind(sockets[1], (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) < 0){
        DIE("bind");
    }

	// Setează structura de adrese pentru UDP
    memset((char *)&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = PORT;


	// Asociază socket-ul UDP cu adresa și portul serverului
    if(bind(sockets[0], (struct sockaddr *)&udp_addr, sizeof(struct sockaddr)) < 0)
        DIE("bind");

	// Dezactivez nagle
    int flag = 1;
    if(setsockopt(sockets[1], IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0) {
        DIE("nagle");
    }

	if(listen(sockets[1], INT_MAX) < 0)
        DIE("listen");

	fd_set write_file;
    FD_ZERO(&write_file);


    FD_SET(sockets[1], &write_file);
	FD_SET(sockets[0], &write_file);
	FD_SET(STDIN_FILENO, &write_file);

	while(1) {
		fd_set read_file;
        FD_ZERO(&read_file);
        read_file = write_file;

		if(select(max + 1, &read_file, NULL, NULL, NULL) < 0)
            DIE("select");



		for (int i = 0; i <= max; i++) {
			if (FD_ISSET(i, &read_file)) {
				int k = 0;
                while (k < sizeof(buffer)) {
                    buffer[k] = 0;
                    k++;
                }
				if (i == sockets[1]) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
                    // pe care serverul o accepta
                    socklen_t dim = sizeof(struct sockaddr);
                    int socket_primit = accept_connection(sockets[1], &new_tcp, &dim);
                    receive_id(socket_primit, buffer);
                    int found = find_client(clienti, buffer, max);

                    if (found == -1) {
                        handle_new_client(clienti, socket_primit, new_tcp, &max, &write_file, buffer);
                    } else if (found != -1 && !clienti[found].online) {
                        handle_existing_client(clienti, found, socket_primit, new_tcp, &write_file);
                    } 

				} else if (i == sockets[0]) {
					socklen_t dim = sizeof(struct sockaddr);
                    if ( recvfrom(sockets[0], buffer, 1551, 0, (struct sockaddr *)&udp_addr, &dim) < 0)
                        DIE("recvfrom");

                    msg_tcp send_to_tcp;
                    memset(&send_to_tcp, 0, sizeof(struct tcp_struct));
                    msg_udp *send_to_udp;

                    send_to_tcp.port = PORT;
                    send_to_udp = (msg_udp *)buffer;
                    strcpy(send_to_tcp.topic, send_to_udp->topic);
                    strcpy(send_to_tcp.ip, inet_ntoa(udp_addr.sin_addr));

                    process_and_send_udp_data(send_to_udp, &send_to_tcp, clienti, max);



				}
                 else if (i == STDIN_FILENO) {
					fgets(buffer, 100, stdin);

					if(strncmp(buffer, "exit", 4) == 0){
                        DIE("exit");
                        close(sockets[0]);
                        close(sockets[1]);
                        free(clienti);
                        return 0;
                    }

				} 
                else {
					int k = 0;
                    while (k < sizeof(buffer)) {
                        buffer[k] = 0;
                        k++;
                    }

                    int x = recv(i, buffer, sizeof(Packet), 0);
                    if(x < 0)
                        DIE("recv");
                    if(x){
                        process_received_packet(clienti, i, max, buffer, &write_file);
                    } else {
                        process_exit(clienti, i, max, &write_file);
                    }
				}
			}
		}
	}
	close(sockets[0]);
    close(sockets[1]);

	return 0;
}

