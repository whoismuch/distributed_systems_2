//#include <string.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <stdio.h>
//#include "banking.h"
//#include "ipc.c"
//#include <time.h>
//#include "pa2345.h"
//#include "common.h"
//#include "bank_robbery.c"
//
//FILE *p_log;
//FILE *e_log;
//int process_number;
//int pipes_to_read[12][12];
//int pipes_to_write[12][12];
//
//
//int close_not_my_pipes(local_id pid) {
//    for (int i = 0; i <= process_number; i++) {
//        if (i != pid) {
//            for (int j = 0; j <= process_number; j++) {
//                if (i != j) {
//                    if (close(pipes_to_read[i][j]) != 0) return -1;
//                    if (close(pipes_to_write[i][j]) != 0) return -1;
//                    pipes_to_read[i][j] = -999;
//                    pipes_to_write[i][j] = -999;
//                }
//            }
//        }
//    }
//    return 0;
//}
//
//
//void transfer(void *parent_data, local_id src, local_id dst,
//              balance_t amount) {
//
//}
//
//void initPipes() {
//    for (int i = 0; i <= process_number; i++) {
//        for (int j = 0; j <= process_number; j++) {
//            int fdPipes[2];
//            if (i != j) {
//                pipe(fdPipes);
//                pipes_to_read[j][i] = fdPipes[0];
//                pipes_to_write[i][j] = fdPipes[1];
//            }
//        }
//        pipes_to_read[i][process_number + 1] = -1;
//        pipes_to_write[i][process_number + 1] = -1;
//        pipes_to_read[process_number + 1][i] = -1;
//        pipes_to_write[process_number + 1][i] = -1;
//    }
//    for (int i = 0; i < 12; i++) {
//        for (int j = 0; j < 12; j++) {
//            if (i == j) {
//                pipes_to_read[j][i] = -999;
//                pipes_to_write[i][j] = -999;
//            } else if (i > process_number){
//                pipes_to_read[j][i] = -1;
//                pipes_to_read[i][j] = -1;
//                pipes_to_write[j][i] = -1;
//                pipes_to_write[i][j] = -1;
//            }
//        }
//
//    }
//}
//
//void sendStartMsg(local_id i) {
//    printf(log_started_fmt, i, getpid(), getppid(), );
//
//    fprintf(e_log, log_started_fmt, i, getpid(), getppid());
//
//    char *message_content = (char *) malloc(100 * sizeof(char));
//    sprintf(message_content, log_started_fmt, i, getpid(), getppid());
//
//    send_msg(process_number, pipes_to_write, i, message_content, STARTED);
//
//    fprintf(p_log, "I wrote in all channels descriptor\n");
//
//}
//
//void receiveAllStartMsg(local_id i) {
//    receive_msg(process_number, pipes_to_read, i, p_log);
//
//    printf(log_received_all_started_fmt, i);
//    fprintf(e_log, log_received_all_started_fmt, i);
//}
//
//void sendDoneMsg(local_id i) {
//    printf(log_done_fmt, i);
//    fprintf(e_log, log_done_fmt, i);
//
//    char *message_content = (char *) malloc(100 * sizeof(char));
//    sprintf(message_content, log_done_fmt, i);
//
//    send_msg(process_number, pipes_to_write, i, message_content, DONE);
//
//    fprintf(p_log, "I wrote in all channels descriptors\n");
//
//}
//
//void receiveAllDoneMsg(local_id i) {
//    receive_msg(process_number, pipes_to_read, i, p_log);
//
//    printf(log_received_all_done_fmt, i);
//    fprintf(e_log, log_received_all_done_fmt, i);
//}
//
//void initChildProcesses() {
//    for (int i = 1; i <= process_number; i++) // loop will run n times (n=5)
//    {
//        pid_t child_pid = fork();
//        if (child_pid == 0) {
//
//            close_not_my_pipes(i);
//
//            sendStartMsg(i);
//
//            receiveAllStartMsg(i);
//
//            sendDoneMsg(i);
//
//            receiveAllDoneMsg(i);
//
//            exit(0);
//        }
//    }
//}
//
//void waitForChildrenTerminating() {
//    for (int i = 1; i <= process_number; i++) // loop will run n times (n=5)
//        wait(NULL);
//}
//
//void parent_routine() {
//    initChildProcesses();
//    close_not_my_pipes(0);
//    receiveAllStartMsg(0);
//    receiveAllDoneMsg(0);
//    waitForChildrenTerminating();
//}
//
//void start() {
//    initPipes();
//    parent_routine();
//}
//
//int main(int argc, char *argv[]) {
//    if (0 != strcmp(argv[1], "-p")) {
//        return 1;
//    }
//    process_number = atoi(argv[2]);
////    int balances[process_number];
////    for (int i=0; i<process_number; i++) {
////        balances[i] = atoi(argv[3+i]);
////    }
//
//    p_log = fopen(pipes_log, "wa+");
//    if (p_log == NULL) { return 1; }
//
//    e_log = fopen(events_log, "wa+");
//    if (e_log == NULL) { return 1; }
//
//
//    start();
//
//
//}
