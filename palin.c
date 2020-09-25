// Author: Adam Wilson
// Date: 10/2/2020

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 

void criticalSection(char*, int);

//void sigquit_handler(int sig) {
//    write(0, "killed\n", 10);
//    exit(0);
//}

int main(int argc, char* argv[]) {
    int i;
    //signal(SIGQUIT, sigquit_handler);
    int index = atoi(argv[0]);
    int numStrings = atoi(argv[1]);

    key_t key = ftok("master", 137);
    int shmid = shmget(key, numStrings * 128, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }

    char(*strings)[numStrings][128] = shmat(shmid, (void*)0, 0);
    if (strings == (void*)-1) {
        perror("Shared memory attach");
        return 1;
    }

    char* str = (*strings)[index];

    //for (i = 0; i < numStrings; i++) {
    //    printf(" %s\n", (*strings)[i]);
    //}
    //printf("%s\n", (*strings)[index]);
    
    int pFlag = 1;
    int left = 0;
    int right = strlen(str) - 1;

    while (right > left) {
        if (str[left++] != str[right--]) {
            pFlag = 0;
            break;
        }
    }

    criticalSection((*strings)[index], pFlag);

    if (shmdt(strings) == -1) {
        perror("shmdt");
        return 1;
    }
//    shmctl(shmid, IPC_RMID, NULL);

	return 0;
}  

void criticalSection(char* str, int pFlag) {
    struct timeval stop, start;
    gettimeofday(&start, NULL);
    struct timespec wait;
    FILE* fp;
    srand((unsigned int)start.tv_sec);

    long r = (rand() % 2000) * 1000000;
    wait.tv_sec = r / 1000000000;
    wait.tv_nsec = r % 1000000000;

    nanosleep(&wait, NULL);
    printf("%s %.3f\n", str, r / 1000000000.);

    //if (pFlag == 1) {
    //    fp = fopen("palin.out", "a");
    //    if (fp == NULL) {
    //        perror("Error opening file.");
    //    }
    //    fprintf(fp, "%s\n", str);
    //}

    //else {
    //    fp = fopen("nopalin.out", "a");
    //    if (fp == NULL) {
    //        perror("Error opening file.");
    //    }
    //    //printf("%s\n", str);
    //    fprintf(fp, "%s\n", str);
    //}

    //fclose(fp);
}
