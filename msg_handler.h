//
// Created by Хумай Байрамова on 04.12.2022.
//
#include "banking.h"
#include <unistd.h>

#ifndef DISTRIBUTED_SYSTEMS_2_MSG_HANDLER_H
#define DISTRIBUTED_SYSTEMS_2_MSG_HANDLER_H

Message* prepare_msg(void *payload, uint16_t payload_len, int16_t type);

void handle_msg(Message* msg);

#endif //DISTRIBUTED_SYSTEMS_2_MSG_HANDLER_H
