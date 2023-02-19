#include <unistd.h>
#include <stdint-gcc.h>
#include "../../util/tools.h"
#include "webSocketConnection.h"

int nextFrame(dataFrame *frame);
int sendFrame(Client *client, const unsigned char *msg, uint64_t size, int type);

char* getHandshakeAccept(char *key) {
    // merge key and handshake string
    int totalLength = WEBSOCKET_KEY_LENGTH + HANDSHAKE_STRING_LENGTH + 1;
    char str[totalLength];
    // copy key to str
    memcpy(str, key, WEBSOCKET_KEY_LENGTH);
    // copy handshake string to str
    memcpy(str + WEBSOCKET_KEY_LENGTH, HANDSHAKE_STRING, HANDSHAKE_STRING_LENGTH);
    // add null terminator
    str[totalLength - 1] = '\0';

    // get sha1 and base64
    char* b64 = sha1AndBase64(str);

    return b64;
}

void sendMessage(Client *client, char *msg) {
    uint64_t size = strlen(msg);
    // send frame of type FRAME_TEXT
    sendFrame(client, (const unsigned char *) msg, size, FRAME_TEXT);
}

int upgradeConnection(RequestHttp* request, ResponseHttp* response, WebSocketConnection* connection) {

    // ------------------ do handshake ------------------
    if (strcmp(request->methodStr, "GET") != 0) {
        logWarning("Method not allowed");
        response->statusCode = 405;
        response->contentType = HTML;
        sendResponse(response, request);
        return 0;
    }

    char* challengeKey = getHeader(request, "Sec-WebSocket-Key");
    if (challengeKey == NULL) {
        logWarning("Missing Sec-WebSocket-Key");
        response->statusCode = 400;
        response->contentType = HTML;
        sendResponse(response, request);
        return 0;
    }

    response->statusCode = 101;
    char* acceptKey = getHandshakeAccept(challengeKey);
    logInfo("Accept key: %s -> %s", challengeKey, acceptKey);

    setHeader(response, "Upgrade", "websocket");
    setHeader(response, "Connection", "Upgrade");
    setHeader(response, "Sec-WebSocket-Accept", acceptKey);
    setHeader(response, "Sec-WebSocket-Protocol", "myprotocol");

    sendResponse(response, request);
    free(acceptKey);

    // ------------------ handshake done ------------------

    Client client;
    sem_init(&client.mtx_ping, 0, 1);
    client.client_sockfd = request->client_sockfd;
    client.state = STATE_OPEN;
    client.ip_num = request->ipNum;
    strcpy(client.ip, request->ip);

    dataFrame frame;
    frame.client = &client;
    frame.cur_pos = 0;
    frame.error = 0;

    connection->client = &client;

    // allow user to send messages
    connection->sendMessage = &sendMessage;

    // on open
    connection->onOpen(connection->client);

    // ------------------ listen for frames ------------------
    while(nextFrame(&frame) >= 0) {
        // text/binary frame
        if((frame.frame_type == FRAME_TEXT || frame.frame_type == FRAME_BINARY) && !frame.error) {
            // onmessage
            char ms[frame.frame_size + 1];
            memcpy(ms, frame.msg, frame.frame_size);
            ms[frame.frame_size] = '\0';
            logInfo("Message: %s", ms);

            connection->onMessage(connection->client, frame.msg, frame.frame_size);

        } else if(frame.frame_type == OPERATION_CLOSE && !frame.error) {
            if(client.state != STATE_CLOSING) {
                // on close
                client.state = STATE_CLOSING;
                logInfo("Closing connection");

                connection->onClose(connection->client);
            }
            free(frame.msg);
            break;
        }
        free(frame.msg);
    }

    // onclose
    logInfo("Connection closed");
    sem_destroy(&client.mtx_ping);

    return 1;
}


int nextByte(dataFrame *frame) {
    ssize_t n;

    /* If empty or full. */
    if (frame->cur_pos == 0 || frame->cur_pos == frame->amt_read) {

        if ((n = read(frame->client->client_sockfd, frame->frame, sizeof(frame->frame))) <= 0) {
            frame->error = 1;
            logWarning("An error has occurred while trying to read next byte");
            return -1;
        }

        frame->amt_read = (size_t)n;
        frame->cur_pos = 0;
    }

    return frame->frame[frame->cur_pos++];
}

int isOperation(int operation) {
    return (operation == OPERATION_CLOSE || operation == OPERATION_PING || operation == OPERATION_PONG);
}

int32_t pongMsInt32(const uint8_t *msg) {
    int32_t pong_id;
    pong_id = (msg[3] << 0) | (msg[2] << 8) | (msg[1] << 16) | (msg[0] << 24);
    return pong_id;
}

int sendFrame(Client *client, const unsigned char *msg, uint64_t size, int type) {
    unsigned char *response; /* Response data.     */
    unsigned char frame[10]; /* Frame.             */
    uint8_t idx_first_rData; /* Index data.        */
    uint64_t length;         /* Message length.    */
    int idx_response;        /* Index response.    */
    ssize_t output;          /* Bytes sent.        */
    uint64_t i;              /* Loop index.        */

    frame[0] = (FRAME_FIN | type);
    length = (uint64_t)size;

    /* Split the size between octets. */
    if (length <= 125) {
        frame[1] = length & 0x7F;
        idx_first_rData = 2;
    }
        /* Size between 126 and 65535 bytes. */
    else if (length <= 65535) {
        frame[1] = 126;
        frame[2] = (length >> 8) & 255;
        frame[3] = length & 255;
        idx_first_rData = 4;
    }
        /* Size between 65536 and 2^64 bytes. */
    else {
        frame[1] = 127;
        frame[2] = (length >> 56) & 255;
        frame[3] = (length >> 48) & 255;
        frame[4] = (length >> 40) & 255;
        frame[5] = (length >> 32) & 255;
        frame[6] = (length >> 24) & 255;
        frame[7] = (length >> 16) & 255;
        frame[8] = (length >> 8) & 255;
        frame[9] = length & 255;
        idx_first_rData = 10;
    }

    /* Allocate memory for the response. */
    idx_response = 0;
    response = malloc(sizeof(unsigned char) * (idx_first_rData + length + 1));
    if (!response) return -1;

    for (i = 0; i < idx_first_rData; i++) {
        response[i] = frame[i];
        idx_response++;
    }

    /* Add data bytes. */
    for (i = 0; i < length; i++) {
        response[idx_response] = msg[i];
        idx_response++;
    }

    response[idx_response] = '\0';

    /* Send to the client if there is one. */
    output = 0;
    if (client) output = write(client->client_sockfd, response, idx_response);

    /* If no client specified, broadcast to everyone. */
//    if (!client) {
//        sem_wait(&mutex);
//        for (i = 0; i < MAX_CLIENTS; i++) {
//            cli = &client_socks[i];
//            if ((cli->client_sock > -1) && get_client_state(cli) == WS_STATE_OPEN) {
//                if ((send_ret = SEND(cli, response, idx_response)) != -1)
//                    output += send_ret;
//                else
//                {
//                    output = -1;
//                    break;
//                }
//            }
//        }
//        sem_post(&mutex);
//    }

    free(response);
    return ((int)output);
}

int pong(dataFrame *frame, uint64_t frame_size) {

    if (sendFrame(frame->client, (const unsigned char *)frame->msg_ctrl, frame_size, OPERATION_PONG) < 0) {
        frame->error = 1;
        logWarning("An error has occurred while ponging!\n");
        return -1;
    }

    return 0;
}

static int readFrame(dataFrame *frame,
                     int opcode,
                     unsigned char **buf,
                     uint64_t *frame_length,
                     uint64_t *frame_size,
                     uint64_t *msg_idx,
                     uint8_t *masks,
                     int is_fin) {
    unsigned char *tmp; /* Tmp message.     */
    unsigned char *msg; /* Current message. */
    int cur_byte;       /* Curr byte read.  */
    uint64_t i;         /* Loop index.      */

    msg = *buf;

    /* Decode masks and length for 16-bit messages. */
    if (*frame_length == 126) *frame_length = (((uint64_t)nextByte(frame)) << 8) | nextByte(frame);

        /* 64-bit messages. */
    else if (*frame_length == 127) {
        *frame_length =
                (((uint64_t)nextByte(frame)) << 56) | /* frame[2]. */
                (((uint64_t)nextByte(frame)) << 48) | /* frame[3]. */
                (((uint64_t)nextByte(frame)) << 40) | (((uint64_t)nextByte(frame)) << 32) |
                (((uint64_t)nextByte(frame)) << 24) | (((uint64_t)nextByte(frame)) << 16) |
                (((uint64_t)nextByte(frame)) << 8) |
                (((uint64_t)nextByte(frame))); /* frame[9]. */
    }

    *frame_size += *frame_length;

    /*
     * Check frame size
     *
     * We need to limit the amount supported here, since if
     * we follow strictly to the RFC, we have to allow 2^64
     * bytes. Also keep in mind that this is still true
     * for continuation frames.
     */
    if (*frame_size > FRAME_MAX_LENGTH) {
        logWarning("Current frame from client %d, exceeds the maximum\n"
                   "amount of bytes allowed %d/%d\n",
                   frame->client->client_sockfd, *frame_size + *frame_length, FRAME_MAX_LENGTH);
        frame->error = 1;
        return -1;
    }

    /* Read masks. */
    masks[0] = nextByte(frame);
    masks[1] = nextByte(frame);
    masks[2] = nextByte(frame);
    masks[3] = nextByte(frame);

    /*
     * Abort if error.
     *
     * This is tricky: we may have multiples error codes from the
     * previous next_bytes() calls, but, since we're only setting
     * variables and flags, there is no major issue in setting
     * them wrong _if_ we do not use their values, thing that
     * we do here.
     */
    if (frame->error) return -1;

    /*
     * Allocate memory.
     *
     * The statement below will allocate a new chunk of memory
     * if msg is NULL with size total_length. Otherwise, it will
     * resize the total memory accordingly with the message index
     * and if the current frame is a FIN frame or not, if so,
     * increment the size by 1 to accommodate the line ending \0.
     */
    if (*frame_length > 0) {
        if (!isOperation(opcode)) {
            tmp = realloc(msg, sizeof(unsigned char) * (*msg_idx + *frame_length + is_fin));
//            if (!tmp) {
//                logWarning("Cannot allocate memory, requested: % " PRId64 "\n",
//                        (*msg_idx + *frame_length + is_fin));
//
//                frame->error = 1;
//                return (-1);
//            }
            msg = tmp;
            *buf = msg;
        }

        /* Copy to the proper location. */
        for (i = 0; i < *frame_length; i++, (*msg_idx)++) {
            /* We were able to read? .*/
            cur_byte = nextByte(frame);
            if (cur_byte == -1) return -1;

            msg[*msg_idx] = cur_byte ^ masks[i % 4];
        }
    }

    /* If we're inside a FIN frame, lets... */
    if (is_fin && *frame_size > 0) {
        /* Increase memory if our FIN frame is of length 0. */
        if (!*frame_length && !isOperation(opcode)) {
            tmp = realloc(msg, sizeof(unsigned char) * (*msg_idx + 1));
//            if (!tmp) {
//                DEBUG("Cannot allocate memory, requested: %" PRId64 "\n",
//                        (*msg_idx + 1));
//
//                frame->error = 1;
//                return (-1);
//            }
            msg = tmp;
            *buf = msg;
        }
        msg[*msg_idx] = '\0';
    }

    return 0;
}

int nextFrame(dataFrame *frame) {
    unsigned char *msg_data; /* Data frame.                */
    unsigned char *msg_ctrl; /* Control frame.             */
    uint8_t masks_data[4];   /* Masks data frame array.    */
    uint8_t masks_ctrl[4];   /* Masks control frame array. */
    uint64_t msg_idx_data;   /* Current msg index.         */
    uint64_t msg_idx_ctrl;   /* Current msg index.         */
    uint64_t frame_length;   /* Frame length.              */
    uint64_t frame_size;     /* Current frame size.        */
    uint32_t utf8_state;     /* Current UTF-8 state.       */
    int32_t pong_id;         /* Current PONG id.           */
    uint8_t opcode;          /* Frame opcode.              */
    uint8_t is_fin;          /* Is FIN frame flag.         */
    uint8_t mask;            /* Mask.                      */
    int cur_byte;            /* Current frame byte.        */

    msg_data = NULL;
    msg_ctrl = frame->msg_ctrl;
    is_fin = 0;
    frame_length = 0;
    frame_size = 0;
    msg_idx_data = 0;
    msg_idx_ctrl = 0;
    frame->frame_size = 0;
    frame->frame_type = -1;
    frame->msg = NULL;

    /* Read until find a FIN or a unsupported frame. */
    do
    {
        /*
         * Obs: next_byte() can return error if not possible to read the
         * next frame byte, in this case, we return an error.
         *
         * However, please note that this check is only made here and in
         * the subsequent next_bytes() calls this also may occur too.
         * wsServer is assuming that the client only create right
         * frames and we will do not have disconnections while reading
         * the frame but just when waiting for a frame.
         */
        cur_byte = nextByte(frame);
        if (cur_byte == -1) return -1;

        is_fin = (cur_byte & 0xFF) >> 7;
        opcode = (cur_byte & 0xF);

        /*
         * Check for RSV field.
         *
         * Since wsServer do not negotiate extensions if we receive
         * a RSV field, we must drop the connection.
         */
        if (cur_byte & 0x70) {
            logWarning("RSV is set while wsServer do not negotiate extensions!\n");
            frame->error = 1;
            break;
        }

        /*
         * Check if the current opcode makes sense:
         * a) If we're inside a cont frame but no previous data frame
         *
         * b) If we're handling a data-frame and receive another data
         *    frame. (it's expected to receive only CONT or control
         *    frames).
         *
         * It is worth to note that in a), we do not need to check
         * if the previous frame was FIN or not: if was FIN, an
         * on_message event was triggered and this function returned;
         * so the only possibility here is a previous non-FIN data
         * frame, ;-).
         */
        if ((frame->frame_type == -1 && opcode == 0) ||
            (frame->frame_type != -1 && !isOperation(opcode) && opcode != FRAME_CONTINUATION)) {
            logWarning("Unexpected frame was received!, opcode: %d, previous: %d\n", opcode, frame->frame_type);
            frame->error = 1;
            break;
        }

        /* Check if one of the valid opcodes. */
        if (opcode == FRAME_TEXT || opcode == FRAME_BINARY ||
            opcode == FRAME_CONTINUATION || opcode == OPERATION_PING ||
            opcode == OPERATION_PONG || opcode == OPERATION_CLOSE) {
            /*
             * Check our current state: if CLOSING, we only accept close
             * frames.
             *
             * Since the server may, at any time, asynchronously, asks
             * to close the client connection, we should terminate
             * immediately.
             */
//            if (get_client_state(frame->client) == WS_STATE_CLOSING &&
//                opcode != WS_FR_OP_CLSE)
//            {
//                logWarning("Unexpected frame received, expected CLOSE (%d), "
//                      "received: (%d)",
//                      OPERATION_CLOSE, opcode);
//                frame->error = 1;
//                break;
//            }

            /* Only change frame type if not a CONT frame. */
            if (opcode != FRAME_CONTINUATION && !isOperation(opcode))
                frame->frame_type = opcode;

            mask = nextByte(frame);
            frame_length = mask & 0x7F;
            msg_idx_ctrl = 0;
            frame_size = 0;

            /*
             * We should deny non-FIN control frames or that have
             * more than 125 octets.
             */
            if (isOperation(opcode) && (!is_fin || frame_length > 125)) {
                logWarning("Control frame bigger than 125 octets or not a FIN frame!\n");
                frame->error = 1;
                break;
            }

            /* Normal data frames. */
            if (opcode == FRAME_TEXT || opcode == FRAME_BINARY || opcode == FRAME_CONTINUATION) {
                if (readFrame(frame, opcode, &msg_data, &frame_length, &frame->frame_size, &msg_idx_data, masks_data, is_fin) < 0) break;
            }

                /*
                 * We _may_ send a PING frame if the ws_ping() routine was invoked.
                 *
                 * If the content is invalid and/or differs the size, ignore it.
                 * (maybe unsolicited PONG).
                 */
            else if (opcode == OPERATION_PONG) {
                if (readFrame(frame, opcode, &msg_ctrl, &frame_length, &frame_size,
                              &msg_idx_ctrl, masks_ctrl, is_fin) < 0)
                    break;

                is_fin = 0;

                /* If there is no content and/or differs the size, ignore it. */
                if (frame_size != sizeof(frame->client->last_pong_id)) continue;

                /*
                 * Our PONG id should be positive and smaller than our
                 * current PING id. If not, ignore.
                 */
                /* clang-format off */
                sem_wait(&frame->client->mtx_ping);

                pong_id = pongMsInt32(msg_ctrl);
                if (pong_id < 0 || pong_id > frame->client->current_ping_id) {
                    sem_post(&frame->client->mtx_ping);
                    continue;
                }
                frame->client->last_pong_id = pong_id;

                sem_post(&frame->client->mtx_ping);
                /* clang-format on */
                continue;
            }

                /* We should answer to a PING frame as soon as possible. */
            else if (opcode == OPERATION_PING) {
                if (readFrame(frame, opcode, &msg_ctrl, &frame_length, &frame_size,
                              &msg_idx_ctrl, masks_ctrl, is_fin) < 0)
                    break;

                if (pong(frame, frame_size) < 0)
                    break;

                /* Quick hack to keep our loop. */
                is_fin = 0;
            }

                /* We interrupt the loop as soon as we find a CLOSE frame. */
            else {
                if (readFrame(frame, opcode, &msg_ctrl, &frame_length, &frame_size,
                              &msg_idx_ctrl, masks_ctrl, is_fin) < 0)
                    break;

                /* Since we're aborting, we can scratch the 'data'-related
                 * vars here. */
                frame->frame_size = frame_size;
                frame->frame_type = OPERATION_CLOSE;
                free(msg_data);
                return 0;
            }
        }
            /* Anything else (unsupported frames). */
        else {
            logWarning("Unsupported frame opcode: %d\n", opcode);
            /* We should consider as error receive an unknown frame. */
            frame->frame_type = opcode;
            frame->error = 1;
        }

    } while (!is_fin && !frame->error);

    /* Check for error. */
    if (frame->error) {
        free(msg_data);
        frame->msg = NULL;
        return -1;
    }

    frame->msg = msg_data;
    return 0;
}
