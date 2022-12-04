//
// Created by Хумай Байрамова on 04.12.2022.
//

#include "msg_handler.h"

#include <malloc.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <time.h>
//#include <string.h>
//

//#include "pa2345.h"


Message *prepare_msg(void *payload, uint16_t payload_len, int16_t type) {

    Message *msg = malloc(sizeof(MessageHeader) + payload_len);
    msg->s_header.s_magic = MESSAGE_MAGIC;
    msg->s_header.s_type = type;
    msg->s_header.s_local_time = get_physical_time();
    msg->s_header.s_payload_len = payload_len;

    memcpy(msg->s_payload, payload, msg->s_header.s_payload_len);

    return msg;
}

void handle_msg(Message* msg) {

}
