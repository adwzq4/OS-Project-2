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

// declare state possibilities
enum state { idle, want_in, in_cs };

void criticalSection(char*, int, int);

int main(int argc, char* argv[]) {
    int i, j;
    
    // read execl args into local variables
    i = atoi(argv[0]);
    int numStrings = atoi(argv[1]);

    // shared memory segment contains 2d string array and variables to control 
    // critical section
    struct shmseg {
        int turn;
        enum state flag[20];
        char strings[numStrings][128];
    };


    // get same shmid as in master
    key_t key = ftok("master", 137);
    int shmid = shmget(key, sizeof(struct shmseg), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }

    // attach struct pointer to shared memory segment
    struct shmseg * shmptr  = shmat(shmid, (void*)0, 0);
    if (shmptr == (void*)-1) {
        perror("Shared memory attach");
        return 1;
    }

    // puts strings[i] into a local variable for easier handling
    char* str = shmptr->strings[i];
    
    // sets pFlag to true if str is a palindrome and false if not
    int pFlag = 1;
    int left = 0;
    int right = strlen(str) - 1;
    while (right > left) {
        if (str[left++] != str[right--]) {
            pFlag = 0;
            break;
        }
    }

    // makes sure only one process can enter its critical section at a time
    do
    {
        shmptr->flag[i] = want_in; // Raise my flag
        j = shmptr->turn; // Set local variable
        while (j != i)
            j = (shmptr->flag[j] != idle) ? shmptr->turn : (j + 1) % 20;
        // Declare intention to enter critical section
        shmptr->flag[i] = in_cs;
        // Check that no one else is in critical section
        for (j = 0; j < 20; j++)
            if ((j != i) && (shmptr->flag[j] == in_cs))
                break;
    } while ((j < 20) || (shmptr->turn != i && shmptr->flag[shmptr->turn] != idle));
    // Assign turn to self and enter critical section
    shmptr->turn = i;
    criticalSection(str, pFlag, i);
    // Exit section
    j = (shmptr->turn + 1) % 20;
    while (shmptr->flag[j] == idle)
        j = (j + 1) % 20;
    // Assign turn to next waiting process; change own flag to idle
    shmptr->turn = j;
    shmptr->flag[i] = idle;

    // detach from shared memory
    if (shmdt(shmptr) == -1) {
        perror("shmdt");
        return 1;
    }

	return 0;
}  

// handles the process's critical section
void criticalSection(char* str, int pFlag, int index) {
    struct timeval start;
    struct timespec wait;
    FILE* fp;
    FILE* fp2;

    // seed random number generator
    gettimeofday(&start, NULL);
    srand((unsigned int)start.tv_usec);

    // get random wait from 0-2 seconds, with nanosecond precision,
    // then sleep for that amount of time
    long r = (rand() % 2000) * 1000000;
    wait.tv_sec = r / 1000000000;
    wait.tv_nsec = r % 1000000000;
    nanosleep(&wait, NULL);

    // print process id, array index, and string to log file
    fp2 = fopen("output.log", "a");
    if (fp2 == NULL) {
        perror("Error opening file.");
    }
    fprintf(fp2, "%u   %d   %s\n", getpid(), index, str);
    fclose(fp2);

    // print string to palin.out if it is a palindrome
    if (pFlag == 1) {
        fp = fopen("palin.out", "a");
        if (fp == NULL) {
            perror("Error opening file.");
        }
        fprintf(fp, "%s\n", str);
    }

    // print string to nopalin.out if it is not a palindrome
    else {
        fp = fopen("nopalin.out", "a");
        if (fp == NULL) {
            perror("Error opening file.");
        }
        fprintf(fp, "%s\n", str);
    }

    fclose(fp);
}
