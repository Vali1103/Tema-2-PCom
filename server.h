#ifndef _HELPERS_H
#define _HELPERS_H 1

#define DIE( call_description)	\
	do {									\
											\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
											\
	} while(0)

typedef enum {
    CMD_EXIT,
    CMD_SUBSCRIBE,
    CMD_UNSUBSCRIBE,
    CMD_INVALID
} CommandType;

typedef struct tcp_struct {
	char ip[16];
	uint16_t port;
	char tip_date;
	char type[11];  
	char topic[51];
	char continut[1501];
} msg_tcp, Packet;

typedef struct udp_struct {
	char topic[50];
	uint8_t type;
	char continut[1501];
} msg_udp;

typedef struct client{
	char id[10];
	int socket;
	int dim_topics;
	int dim_unsent;
	struct tcp_struct unsent[100];
	char nume_topic[100][51];
	int sf_topic[100];
	int online; // 1 online, 0 nu
} client;

CommandType get_command_type(const char *buffer) {
    if (strncmp(buffer, "exit", 4) == 0) {
        return CMD_EXIT;
    } else if (strncmp(buffer, "subscribe", 9) == 0) {
        return CMD_SUBSCRIBE;
    } else if (strncmp(buffer, "unsubscribe", 11) == 0) {
        return CMD_UNSUBSCRIBE;
    } else {
        return CMD_INVALID;
    }
}

#define PACKLEN sizeof(Packet)
#define BUFFER_SIZE sizeof(Packet)

#endif












