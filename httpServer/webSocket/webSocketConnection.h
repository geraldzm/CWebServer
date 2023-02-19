#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_WEBSOCKETCONNECTION_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_WEBSOCKETCONNECTION_H

#include "../requestHttp.h"
#include "../responseHttp.h"
#include "webSocketConnection.h"
#include <semaphore.h>
#include <stdint.h>

#define MAX_CLIENTS 100

#define WEBSOCKET_KEY_LENGTH 24
#define HANDSHAKE_STRING_LENGTH 36
//#define HANDSHAKE_HEADER_LENGTH   130
#define HANDSHAKE_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

// socket operations
#define OPERATION_CLOSE 8
#define OPERATION_PING 9
#define OPERATION_PONG 10

// frame operations
#define FRAME_FIN   128
#define FRAME_TEXT 1
#define FRAME_BINARY 2
#define FRAME_CONTINUATION 0
#define FRAME_MAX_LENGTH (16*1024*1024)

// client states
#define STATE_CONNECTING 0
#define STATE_OPEN 1
#define STATE_CLOSING 2
#define STATE_CLOSED 3


static sem_t mutex;

typedef struct Client {
    int client_sockfd;
    int state;       /**< WebSocket current state. */

    /* Timeout thread and locks. */
    sem_t mtx_state;
    pthread_cond_t cnd_state_close;
    pthread_t thrd_tout;
    char close_thrd;

    /* Send lock. */
    sem_t mtx_snd;

    /* IP address. */
    char ip[46];
    /* IP address as a number */
    uint32_t ip_num;

    /* Ping/Pong IDs and locks. */
    int32_t current_ping_id;
    int32_t last_pong_id;
    sem_t mtx_ping;
} Client;

typedef struct dataFrame {
    unsigned char frame[2048];
    unsigned char *msg;
    /**
     * @brief Control frame payload
     */
    unsigned char msg_ctrl[125];
    /**
     * @brief Current byte position.
     */
    size_t cur_pos;
    /**
     * @brief Amount of read bytes.
     */
    size_t amt_read;
    /**
     * @brief Frame type, like text or binary.
     */
    int frame_type;
    /**
     * @brief Frame size.
     */
    uint64_t frame_size;
    /**
     * @brief Error flag, set when a read was not possible.
     */
    char error;

    Client *client;
} dataFrame;

typedef struct WebSocketConnection {
    Client *client;

    void (*onOpen)(Client *client);
    void (*onClose)(Client *client);
    void (*onMessage)(Client *client, const unsigned char *msg, uint64_t msg_size);
    void (*sendMessage)(Client *client, char *msg);

} WebSocketConnection;

int upgradeConnection(RequestHttp* request, ResponseHttp* response, WebSocketConnection *connection);
//int sendFrame(Client *client, const char *msg, uint64_t size, int type);

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_WEBSOCKETCONNECTION_H
