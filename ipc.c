//
// Created by Хумай Байрамова on 07.11.2022.
//
#include "ipc.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


int send(void *self, local_id dst, const Message *msg) {
    if (msg == NULL) return -1;
    if (write(((int *) self)[dst], msg, sizeof(MessageHeader) + msg->s_header.s_payload_len) !=
        sizeof(MessageHeader) + msg->s_header.s_payload_len)
        return -1;
    return 0;
}

int send_multicast(void *self, const Message *msg) {

    local_id i = 0;
    while (((int *) self)[i] != -1) {
        if (((int *) self)[i] != -999) {
            while (send(self, i, msg) != 0);
        }
        i++;
    }
    return 0;
}

int receive(void *self, local_id from, Message *msg) {
    if (msg == NULL) return -1;

    int res = read(((int *) self)[from], &(msg->s_header), sizeof(MessageHeader));
    if (res == -1 && errno == EAGAIN) {
        return 1;
    }
    if (res == sizeof(MessageHeader)) {
        int res2 = read(((int *) self)[from], msg->s_payload, msg->s_header.s_payload_len);
        if (res2 == msg->s_header.s_payload_len) return 0;
        if (res2 == -1 && errno == EAGAIN) return 1;
        else return -2;
    }

    return -2;
}

int receive_any(void *self, Message *msg) {
    local_id i = 0;
    int res;
    while (((int *) self)[i] != -1) {
        if (((int *) self)[i] != -999) {
            res = receive(self, i, msg);
            if (res == 0) return 0;
        }
        i++;
    }
    return -1;
}

int send_msgs(void *self, const char *msg,
              MessageType messageType, timestamp_t time) {

    MessageHeader message_header = {
            .s_magic = MESSAGE_MAGIC,
            .s_payload_len = strlen(msg),
            .s_type = messageType,
            .s_local_time = time
    };


    Message message = {
            .s_header = message_header,
            .s_payload = {0}
    };

    strcpy(message.s_payload, msg);

    int res = send_multicast(self, &message);

    return res;

}

int receive_all(void *self, local_id exceptPid) {
    local_id i = 0;
    int res;
    Message message;
    memset(&message, 0, sizeof(Message));
    while (((int *) self)[i] != -1) {
        if (((int *) self)[i] != -999 && i != exceptPid) {
            res = receive(self, i, &message);
            if (res == 1) {
                i--;
            }
            if (res == -2) {
//                printf("ERROR\n");
                return -1;
            }
        }
        i++;
    }
    return 0;
}



