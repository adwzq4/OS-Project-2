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

extern enum state { idle, want_in, in_cs } flag[20];
extern int turn;
//extern int n;
extern enum state flag[20];

void criticalSection(char*, int, int);

//void sigquit_handler(int sig) {
//    write(0, "killed\n", 10);
//    exit(0);
//}

int main(int argc, char* argv[]) {
    int i, j, turn;
    enum state flag[20];
    //signal(SIGQUIT, sigquit_handler);
    i = atoi(argv[0]);
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

    char* str = (*strings)[i];

    //for (i = 0; i < numStrings; i++) printf(" %s\n", (*strings)[i]);
    
    int pFlag = 1;
    int left = 0;
    int right = strlen(str) - 1;

    while (right > left) {
        if (str[left++] != str[right--]) {
            pFlag = 0;
            break;
        }
    }

    //do
    //{
    //    flag[i] = want_in; // Raise my flag
    //    j = turn; // Set local variable
    //    while (j != i)
    //        j = (flag[j] != idle) ? turn : (j + 1) % 20;
    //    // Declare intention to enter critical section
    //    flag[i] = in_cs;
    //    // Check that no one else is in critical section
    //    for (j = 0; j < 20; j++)
    //        if ((j != i) && (flag[j] == in_cs))
    //            break;
    //} while ((j < 20) || (turn != i && flag[turn] != idle));
    //// Assign turn to self and enter critical section
    //turn = i;
    criticalSection((*strings)[i], pFlag, i);
    //// Exit section
    //j = (turn + 1) % 20;
    //while (flag[j] == idle)
    //    j = (j + 1) % 20;
    //// Assign turn to next waiting process; change own flag to idle
    //turn = j;
    //flag[i] = idle;

    if (shmdt(strings) == -1) {
        perror("shmdt");
        return 1;
    }

	return 0;
}  

void criticalSection(char* str, int pFlag, int index) {
    struct timeval stop, start;
    struct timespec wait;
    FILE* fp;
    FILE* fp2;

    gettimeofday(&start, NULL);
    srand((unsigned int)start.tv_usec);

    long r = (rand() % 2000) * 1000000;
    wait.tv_sec = r / 1000000000;
    wait.tv_nsec = r % 1000000000;

    nanosleep(&wait, NULL);
    printf("%s %.3f\n", str, r / 1000000000.);

    //fp2 = fopen("output.log", "a");
    //if (fp2 == NULL) {
    //    perror("Error opening file.");
    //}
    //fprintf(fp2, "--%u   %d   %s\n", getpid(), index, str);
    //fclose(fp2);

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
