#include "receiver.h"
#include <time.h>
struct timespec start, end;

void receive(message_t* message_ptr, mailbox_t* mailbox_ptr){
    /*  TODO: 
        1. Use flag to determine the communication method
        2. According to the communication method, receive the message
    */
    if(mailbox_ptr->flag == 1) {
        // 1: Message Passing
        if(msgrcv(mailbox_ptr->storage.msqid, message_ptr, sizeof(message_t), message_ptr->msg_type, 0) == -1) {
            perror("msgrcv");
            return;
        }
    }
    else if(mailbox_ptr->flag == 2) {
        // 2: Shared Memory
        message_t* shared_memory = (message_t*)mailbox_ptr->storage.shm_addr;
        memcpy(message_ptr, shared_memory, sizeof(message_t));
    }
    return;
}
// create message queue
int create_msq_id() {
    key_t key = ftok("msqid", 1);
    int msqid = msgget(key, IPC_CREAT | 0666);
    if(msqid == -1) {
        perror("msgget");
        exit(1);
    }
    return msqid;
}
// create shared memory
int create_shm_id() {
    key_t key = ftok("shmid", 1);
    int shmid = shmget(key, sizeof(message_t), IPC_CREAT | 0666);
    if(shmid == -1) {
        perror("shmget");
        exit(1);
    }
    return shmid;
}

int main(int argc, char* argv[]){
    /*  TODO: 
        1) Call receive(&message, &mailbox) according to the flow in slide 4
        2) Measure the total receiving time
        3) Get the mechanism from command line arguments
            • e.g. ./receiver 1
        4) Print information on the console according to the output format
        5) If the exit message is received, print the total receiving time and terminate the receiver.c
    */
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <mechanism>\n", argv[0]);
        exit(1);
    }
    int mechanism = atoi(argv[1]);

    mailbox_t mailbox;
    mailbox.flag = mechanism;
    if(mechanism == 1) {
        fprintf(stdout, "Message Passing\n");
        mailbox.storage.msqid = create_msq_id();
    }
    else if(mechanism == 2) {
        fprintf(stdout, "Shared Memory\n");
        int shmid = create_shm_id();
        mailbox.storage.shm_addr = shmat(shmid, NULL, 0);
        if(mailbox.storage.shm_addr == (char*)-1) {
            perror("shmat");
            exit(1);
        }
    }
    else {
        fprintf(stderr, "Invalid mechanism\n");
        exit(1);
    }

    message_t message;

    // semaphore
    sem_t* sender_sem = sem_open("sender_sem", 0);
    sem_t* receiver_sem = sem_open("receiver_sem", 0);
    if(sender_sem == SEM_FAILED || receiver_sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    // time record
    double time_taken = 0.0;

    // main
    while(1) {
        sem_wait(receiver_sem);
        clock_gettime(CLOCK_MONOTONIC, &start);
        receive(&message, &mailbox);
        clock_gettime(CLOCK_MONOTONIC, &end);
        time_taken += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
        if(strcmp(message.msg_text, "exit") == 0 || message.msg_type == 2) {
            break;
        }
        fprintf(stderr, "Receiving message: %s", message.msg_text);
        sem_post(sender_sem);
    }
    sem_post(sender_sem);
    fprintf(stdout, "\nSender exit!\n");
    fprintf(stdout, "Total time in receiving msg: %lf s\n", time_taken);
    sem_close(receiver_sem);
    sem_unlink("receiver_sem");
    return 0;
}