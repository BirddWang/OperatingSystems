#include "sender.h"
#include <time.h>
struct timespec start, end;

void send(message_t message, mailbox_t* mailbox_ptr){
    /*  TODO: 
        1. Use flag to determine the communication method
        2. According to the communication method, send the message
    */
    if(mailbox_ptr->flag == 1) {
        // 1: Message Passing
        if(msgsnd(mailbox_ptr->storage.msqid, &message, sizeof(message_t), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
    }
    else if(mailbox_ptr->flag == 2) {
        // 2: Shared Memory
        message_t* shared_memory = (message_t*)mailbox_ptr->storage.shm_addr;
        memcpy(shared_memory, &message, sizeof(message_t));
    }
    return;
}

double send_and_get_time(message_t message, mailbox_t* mailbox_ptr) {
    clock_gettime(1, &start);
    send(message, mailbox_ptr);
    clock_gettime(1, &end);
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
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
        1) Call send(message, &mailbox) according to the flow in slide 4
        2) Measure the total sending time
        3) Get the mechanism and the input file from command line arguments
            • e.g. ./sender 1 input.txt
                    (1 for Message Passing, 2 for Shared Memory)
        4) Get the messages to be sent from the input file
        5) Print information on the console according to the output format
        6) If the message form the input file is EOF, send an exit message to the receiver.c
        7) Print the total sending time and terminate the sender.c
    */
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <mechanism> <input_file>\n", argv[0]);
        exit(1);
    }
    int mechanism = atoi(argv[1]);
    char* input_file = argv[2];

    mailbox_t mailbox;
    mailbox.flag = mechanism;
    if(mechanism == 1) {
        fprintf(stdout, "Message Passing\n");
        int msqid = create_msq_id();
        mailbox.storage.msqid = msqid;
    }
    else if(mechanism == 2) {
        fprintf(stdout, "Shared Memory\n");
        int shmid = create_shm_id();
        mailbox.storage.shm_addr = (char*)shmat(shmid, NULL, 0);
        if(mailbox.storage.shm_addr == (char*)-1) {
            perror("shmat");
            exit(1);
        }
        memset(mailbox.storage.shm_addr, 0, sizeof(message_t));
    }
    else {
        fprintf(stderr, "Invalid mechanism\n");
        exit(1);
    }

    message_t message;
    message.msg_type = 1;
    FILE* fp = fopen(input_file, "r");
    if(fp == NULL) {
        perror("fopen");
        exit(1);
    }

    // semaphoe
    sem_t* sender_sem = sem_open("sender_sem", O_CREAT, 0666, 0);
    sem_t* receiver_sem = sem_open("receiver_sem", O_CREAT, 0666, 0);

    // time record
    double time_taken = 0.0;
    
    // main
    sem_post(sender_sem);
    while(fgets(message.msg_text, sizeof(message.msg_text), fp) != NULL) {
        sem_wait(sender_sem);
        time_taken += send_and_get_time(message, &mailbox);
        fprintf(stderr, "Sending message: %s", message.msg_text);
        sem_post(receiver_sem);
    }

    // edit exit message
    strcpy(message.msg_text, "\n");

    // send exit message
    sem_wait(sender_sem);
    time_taken += send_and_get_time(message, &mailbox);
    fprintf(stdout, "\nEnd of input file! exit!\n");
    sem_post(receiver_sem);

    // end of sender
    fprintf(stdout, "Total time taken in sending msg: %lf s", time_taken);

    // get end call
    sem_wait(sender_sem);
    // clean up
    if(mechanism == 1) {
        if(msgctl(mailbox.storage.msqid, IPC_RMID, NULL) == -1) {
            perror("msgctl");
            exit(1);
        }
    }
    else if(mechanism == 2) {
        if(shmdt(mailbox.storage.shm_addr) == -1) {
            perror("shmdt");
            exit(1);
        }
        if(shmctl(create_shm_id(), IPC_RMID, NULL) == -1) {
            perror("shmctl");
            exit(1);
        }
    }
    sem_close(sender_sem);
    sem_unlink("sender_sem");
    // close file and return
    fclose(fp);
    return 0;
}