#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "banking.h"
#include "ipc.h"
#include <time.h>
#include "pa2345.h"
#include "common.h"
#include "msg_handler.h"

FILE *p_log;
FILE *e_log;
int process_number;
int pipes_to_read[12][12];
int pipes_to_write[12][12];
int balances[10];

int send_msgs(void *self, const char *msg,
              MessageType messageType, timestamp_t time);

int receive_all(void *self, local_id exceptPid);


int close_not_my_pipes(local_id pid) {
    for (int i = 0; i <= process_number; i++) {
        for (int j = 0; j <= process_number; j++) {
            if (i != j) {
                if (i != pid) {
                    if (close(pipes_to_write[i][j]) != 0) return -1;
                }
                if (j != pid) {
                    if (close(pipes_to_read[j][i]) != 0) return -1;
                }
            }
        }
    }

    return 0;
}


void transfer(void *parent_data, local_id src, local_id dst,
              balance_t amount) {

    TransferOrder transferOrder = {
            .s_src = src,
            .s_dst = dst,
            .s_amount = amount
    };

    send(pipes_to_write[0], src, prepare_msg(&transferOrder, sizeof(transferOrder), TRANSFER));


    while (1) {
        Message received_msg;
        memset(&received_msg, 0, sizeof(Message));
        if (receive(pipes_to_read[0], dst, &received_msg) == 0 && received_msg.s_header.s_type == ACK) {
            handle_msg(&received_msg);
            break;
        }
    }

//    printf("parent received ACK\n");

}

void initPipes() {
    for (int i = 0; i <= process_number; i++) {
        for (int j = 0; j <= process_number; j++) {
            int fdPipes[2];
            if (i != j) {
                pipe(fdPipes);
                fcntl(fdPipes[0], F_SETFL, O_NONBLOCK);
                fcntl(fdPipes[1], F_SETFL, O_NONBLOCK);
                pipes_to_read[j][i] = fdPipes[0];
                pipes_to_write[i][j] = fdPipes[1];
            }
        }
    }
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 12; j++) {
            if (i == j) {
                pipes_to_read[j][i] = -999;
                pipes_to_write[i][j] = -999;
            } else if (i > process_number) {
                pipes_to_read[j][i] = -1;
                pipes_to_read[i][j] = -1;
                pipes_to_write[j][i] = -1;
                pipes_to_write[i][j] = -1;
            }
        }

    }
}

void sendStartMsg(local_id i) {
    printf(log_started_fmt, get_physical_time(), i, getpid(), getppid(), balances[i]);

    fprintf(e_log, log_started_fmt, get_physical_time(), i, getpid(), getppid(), balances[i]);

    char *message_content = (char *) malloc(255 * sizeof(char));
    sprintf(message_content, log_started_fmt, get_physical_time(), i, getpid(), getppid(), balances[i]);

    send_multicast(pipes_to_write[i], prepare_msg(message_content, strlen(message_content), STARTED));

    fprintf(p_log, "I wrote in all channels descriptor\n");

}

void receiveAllStartMsg(local_id i) {

    receive_all(pipes_to_read[i], 0);

    printf(log_received_all_started_fmt, get_physical_time(), i);
    fprintf(e_log, log_received_all_started_fmt, get_physical_time(), i);
}

void sendDoneMsg(local_id i, BalanceHistory *balanceHistory) {
    printf(log_done_fmt, get_physical_time(), i,
           balanceHistory->s_history[balanceHistory->s_history_len - 1].s_balance);
    fprintf(e_log, log_done_fmt, get_physical_time(), i,
            balanceHistory->s_history[balanceHistory->s_history_len - 1].s_balance);

    char *message_content = (char *) malloc(255 * sizeof(char));
    sprintf(message_content, log_done_fmt, get_physical_time(), i,
            balanceHistory->s_history[balanceHistory->s_history_len - 1].s_balance);

    send_multicast(pipes_to_write[i], prepare_msg(message_content, strlen(message_content), DONE));

    fprintf(p_log, "I wrote in all channels descriptors\n");
//    printf("child %1d sent done in all channels descriptors\n", i);
}

void receiveAllDoneMsg(local_id i) {
    receive_all(pipes_to_read[i], 0);

    printf(log_received_all_done_fmt, get_physical_time(), i);
    fprintf(e_log, log_received_all_done_fmt, get_physical_time(), i);
//    printf("process %1d is here and it's okey\n", i);
}

void sendAckMsg(local_id from, local_id to) {
    send(pipes_to_write[from], to, prepare_msg(NULL, 0, ACK));
}

void processTransferringByChild(local_id i, BalanceHistory *balanceHistory) {
    int isStopped = 0;
    while (!isStopped) {
        Message received_msg;
        memset(&received_msg, 0, sizeof(Message));
        while (receive_any(pipes_to_read[i], &received_msg) != 0);
        handle_msg(&received_msg);
        if (received_msg.s_header.s_type == TRANSFER) {
            TransferOrder *transferOrder = (TransferOrder *) malloc(sizeof(TransferOrder));
            memcpy(transferOrder, received_msg.s_payload, received_msg.s_header.s_payload_len);// nb
//            printf("child %1d transfer info: src -  %1d, dst - %1d, amount - $%d, payload_len - %d is transferring\n",
//                   i, transferOrder->s_src, transferOrder->s_dst, transferOrder->s_amount,
//                   received_msg.s_header.s_payload_len);


            if (transferOrder->s_src == i) {
//                printf("child %1d is transferring out\n", i);
                BalanceState previousBalSt = balanceHistory->s_history[balanceHistory->s_history_len - 1];
                BalanceState balanceState = {
                        .s_balance = previousBalSt.s_balance - transferOrder->s_amount,
                        .s_time = get_physical_time(),
                        .s_balance_pending_in = 0
                };
                timestamp_t lastTime = previousBalSt.s_time;

                for (timestamp_t time = lastTime + 1; time < balanceState.s_time; time++) {
                    balanceHistory->s_history_len = balanceHistory->s_history_len + 1;
                    BalanceState balanceStateMiddle = previousBalSt;
                    balanceStateMiddle.s_time = time;
                    balanceHistory->s_history[balanceHistory->s_history_len - 1] = balanceStateMiddle;
                }
                balanceHistory->s_history_len = balanceHistory->s_history_len + 1;
                balanceHistory->s_history[balanceHistory->s_history_len - 1] = balanceState;

                send(pipes_to_write[i], transferOrder->s_dst, &received_msg);

                printf(log_transfer_out_fmt, get_physical_time(), i, transferOrder->s_amount, transferOrder->s_dst);
                fprintf(e_log, log_transfer_out_fmt, get_physical_time(), i, transferOrder->s_amount,
                        transferOrder->s_dst);

//                printf("child %1d CHANGED balance history, where s_id = %1d, s_hist_len = %d, s_history[0]bal = %d, s_history[1]bal = %d, s_history[2]bal = %d, s_history[3]bal = %d\n",
//                       i, balanceHistory->s_id, balanceHistory->s_history_len, balanceHistory->s_history[0].s_balance,
//                       balanceHistory->s_history[1].s_balance, balanceHistory->s_history[2].s_balance,
//                       balanceHistory->s_history[3].s_balance);

            } else if (transferOrder->s_dst == i) {
//                printf("child %1d is transferring in\n", i);

                printf(log_transfer_in_fmt, get_physical_time(), i, transferOrder->s_amount, transferOrder->s_src);
                fprintf(e_log, log_transfer_out_fmt, get_physical_time(), i, transferOrder->s_amount,
                        transferOrder->s_src);

                BalanceState previousBalSt = balanceHistory->s_history[balanceHistory->s_history_len - 1];
                BalanceState balanceState = {
                        .s_balance = previousBalSt.s_balance + transferOrder->s_amount, //nb
                        .s_time = get_physical_time(),
                        .s_balance_pending_in = 0
                };
                timestamp_t lastTime = previousBalSt.s_time;

                for (timestamp_t time = lastTime + 1; time < balanceState.s_time; time++) {
                    balanceHistory->s_history_len = balanceHistory->s_history_len + 1;
                    BalanceState balanceStateMiddle = previousBalSt;
                    balanceStateMiddle.s_time = time;
                    balanceHistory->s_history[balanceHistory->s_history_len - 1] = balanceStateMiddle;
                }
                balanceHistory->s_history_len = balanceHistory->s_history_len + 1;
                balanceHistory->s_history[balanceHistory->s_history_len - 1] = balanceState;

//                printf("child %1d CHANGED balance history, where s_id = %1d, s_hist_len = %d, s_history[0]bal = %d, s_history[1]bal = %d, s_history[2]bal = %d, s_history[3]bal = %d\n",
//                       i, balanceHistory->s_id, balanceHistory->s_history_len, balanceHistory->s_history[0].s_balance,
//                       balanceHistory->s_history[1].s_balance, balanceHistory->s_history[2].s_balance,
//                       balanceHistory->s_history[3].s_balance);

                sendAckMsg(i, 0);
            }
        }
        if (received_msg.s_header.s_type == STOP) {
            timestamp_t endOfTime = received_msg.s_header.s_local_time;
            BalanceState previousBalSt = balanceHistory->s_history[balanceHistory->s_history_len - 1];
            for (timestamp_t time = previousBalSt.s_time + 1; time <= endOfTime; time++) {
                balanceHistory->s_history_len = balanceHistory->s_history_len + 1;
                BalanceState balanceStateMiddle = previousBalSt;
                balanceStateMiddle.s_time = time;
                balanceHistory->s_history[balanceHistory->s_history_len - 1] = balanceStateMiddle;
            }
            isStopped = 1;
        }
    }
}


BalanceHistory *handleBalanceState(local_id i, timestamp_t time) {

    BalanceState *balanceState = malloc(sizeof(BalanceState));
    balanceState->s_balance = balances[i],
    balanceState->s_time = time,
    balanceState->s_balance_pending_in = 0;

    BalanceHistory *balanceHistory = malloc(sizeof(BalanceHistory) + sizeof(BalanceState) * 255);
    memset(balanceHistory, 0, sizeof(BalanceHistory) + sizeof(BalanceState) * 255);


    balanceHistory->s_id = i;
    balanceHistory->s_history_len = 1;
    memcpy(&(balanceHistory->s_history[0]), balanceState, sizeof(BalanceState) * balanceHistory->s_history_len);

//    printf("child %1d has real_s_time = %d, s_time = %d, s_bal = %d\n", i, balanceState->s_time,
//           balanceHistory->s_history[0].s_time, balanceHistory->s_history[0].s_balance);

    return balanceHistory;
}

void sendBalanceHistory(local_id i, BalanceHistory *balanceHistory) {
    send(pipes_to_write[i], 0, prepare_msg(balanceHistory,sizeof(balanceHistory->s_id) + sizeof(balanceHistory->s_history_len) +
                                                          sizeof(BalanceState) * balanceHistory->s_history_len, BALANCE_HISTORY ));

//    printf("child %1d sent balance history, where s_id = %1d, s_hist_len = %d, s_history[0]time = %d, s_history[1]time = %d, s_history[2]time = %d, s_history[3]time = %d\n",
//           i, balanceHistory->s_id, balanceHistory->s_history_len, balanceHistory->s_history[0].s_time,
//           balanceHistory->s_history[1].s_time, balanceHistory->s_history[2].s_time,
//           balanceHistory->s_history[3].s_time);
}

void initChildProcesses() {
    for (int i = 1; i <= process_number; i++) // loop will run n times (n=5)
    {
        pid_t child_pid = fork();
        if (child_pid == 0) {
            timestamp_t time = get_physical_time();
            BalanceHistory *history = handleBalanceState(i, time);
            close_not_my_pipes(i);
            sendStartMsg(i);
            receiveAllStartMsg(i);
            //полезная работа
            processTransferringByChild(i, history);
            //конец полезной работы
            sendDoneMsg(i, history);
            receiveAllDoneMsg(i);
            sendBalanceHistory(i, history);
            exit(0);
        }
    }
}

void waitForChildrenTerminating() {
    for (int i = 1; i <= process_number; i++) // loop will run n times (n=5)
        wait(NULL);
}

void sendStopMsg(local_id i) {
    fprintf(p_log, "parent is sending STOP to all children\n");
//    printf("parent is sending STOP to all children\n");

    char *message_content = ""; //nb

    send_multicast(pipes_to_write[i], prepare_msg(message_content, strlen(message_content), STOP));

    fprintf(p_log, "parent wrote STOP to all children\n");
//    printf("parent wrote STOP to all children\n");

}

AllHistory *receiveAllBalHis(local_id i) {
    fprintf(p_log, "parent start receiving all balance history messages\n");
//    printf("parent start receiving all balance history messages\n");


    AllHistory *allHistory = malloc(sizeof(allHistory->s_history_len) + sizeof(BalanceHistory) * (process_number + 1));
    allHistory->s_history_len = process_number;


    for (int j = 1; j <= process_number; j++) {
        if (i != j) {

            while (1) {
                Message message;
                memset(&message, 0, sizeof(Message));
                if (receive(pipes_to_read[i], j, &message) == 0 &&
                    message.s_header.s_type == BALANCE_HISTORY) {
                    handle_msg(&message);
//                    printf("Got from child %d his_len = %d, balance[0] = %d, time[0] = %d\n", j,
//                           allHistory->s_history_len, allHistory->s_history[j].s_history[0].s_balance,
//                           allHistory->s_history[j].s_history[0].s_time);
                    memcpy(&(allHistory->s_history[j - 1]), message.s_payload,
                           message.s_header.s_payload_len); // !!! nb  (sizeof мб неверный)
                    break;
                }
            }
        }
    }
//    printf("I received all balance history messages\n");
    fprintf(p_log, "I received all balance history messages\n");

    return allHistory;
}

void parent_routine() {
    initChildProcesses();
    close_not_my_pipes(0);
    receiveAllStartMsg(0);
    bank_robbery(NULL, process_number);
    sendStopMsg(0);
    receiveAllDoneMsg(0);
//    printf("BLOP\n");
    AllHistory *allHistory = receiveAllBalHis(0);
    waitForChildrenTerminating();
    print_history(allHistory);
}

void start() {
    initPipes();
    parent_routine();
}

int main(int argc, char *argv[]) {
    if (0 != strcmp(argv[1], "-p")) {
        return 1;
    }
    process_number = atoi(argv[2]);
    for (int i = 1; i <= process_number; i++) {
        balances[i] = atoi(argv[2 + i]);
    }

    p_log = fopen(pipes_log, "wa+");
    if (p_log == NULL) { return 1; }

    e_log = fopen(events_log, "wa+");
    if (e_log == NULL) { return 1; }


    start();


}
