#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_ROOM_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_ROOM_H

#include <semaphore.h>
#include <stdint-gcc.h>
#include "../../../../httpServer/webSocket/webSocketConnection.h"

#define MAX_GUESTS 100

typedef enum {
    GUEST_CONNECTED,
    GUEST_DISCONNECTED,
} GuestStatus;

typedef struct Guest {
    uint32_t id;
    char* username;

    unsigned int score;

    GuestStatus status;
    Client **client;

    void (*onOpen)(); // called when the guest joins the room
    void (*onClose)(); // called when the guest leaves the room (disconnected)
    void (*onMessage)(const unsigned char *msg, uint64_t msg_size); // where I will receive the messages
    void (**onMessageToGuest)(Client *client, char *msg); // where I will send the messages
} Guest;


Guest* createGuest(const char* username, uint32_t ip_num);
void destroyGuest(Guest* guest);

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_ROOM_H
