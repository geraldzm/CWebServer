#include <string.h>
#include "room.h"

Guest* createGuest(const char* username, uint32_t ip_num) {
    Guest* guest = malloc(sizeof(Guest));
    guest->username = malloc(sizeof(char) * (strlen(username) + 1));
    strcpy(guest->username, username);
    guest->status = GUEST_CONNECTED;
    guest->id = ip_num;
    guest->score = 0;

    guest->onOpen = NULL;
    guest->onClose = NULL;
    guest->onMessage = NULL;
    guest->onMessageToGuest = NULL;

    return guest;
}

void destroyGuest(Guest* guest) {
    free(guest->username);
    free(guest);
}
