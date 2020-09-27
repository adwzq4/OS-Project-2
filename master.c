// Author: Adam Wilson
// Date: 10/2/2020

#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 

int nMax, sMax;

enum state { idle, want_in, in_cs };

static void interruptHandler(int s) {
    signal(SIGQUIT, SIG_IGN);
    kill(-getpid(), SIGQUIT);
    nMax = sMax = 0;
}

static int setupinterrupt(void) {
    struct sigaction sigAlrmAct;
    sigAlrmAct.sa_handler = interruptHandler;
    sigAlrmAct.sa_flags = 0;
    sigemptyset(&sigAlrmAct.sa_mask);
    return (sigaction(SIGALRM, &sigAlrmAct, NULL));
}

static int setupitimer(int t) {
    struct itimerval value = { {0, 0}, {t, 0} };
    return (setitimer(ITIMER_REAL, &value, NULL));
}

int main(int argc, char* argv[]) {
    int opt, tMax, status;
    int numStrings = 0, i = 0, currentProcesses = 0;
    FILE* fp;
    FILE* fp2;
    char c;
    struct timeval current, start;
    struct sigaction sigIntAct;

    nMax = 10; //TODO set to 4
    sMax = 2;
    tMax = 100;
    gettimeofday(&start, NULL);

    sigIntAct.sa_handler = interruptHandler;
    sigIntAct.sa_flags = 0;
    sigemptyset(&sigIntAct.sa_mask);

    if (sigaction(SIGINT, &sigIntAct, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // parses command line arguments
    while ((opt = getopt(argc, argv, "hn:s:t:")) != -1) {
        switch (opt) {
            case 'h':
                printf("\n\n\n--- palin Help Page ---\n\n\
                    palin reads a file of strings, and for each string creates a child process to determine\n\
                    whether the string is a palindrom, then writes all palindromes to one palin.out, and all\n\
                    non-palindromes to nopalin.out\n\n\
                      Invocation:\nmaster -h\nmaster [-n x] [-s x] [-t time] infile\n  Options:\n\
                    -h Describe how the project should be run and then, terminate.\n\
                    -n x Indicate the maximum total of child processes master will ever create. (Default 4, Maximum 20)\n\
                    -s x Indicate the number of children allowed to exist in the system at the same time. (Default 2)\n\
                    -t time The time in seconds after which the process will terminate, even if it has not finished. (Default 100)\n\
                    infile Input file containing strings to be tested.");
                return 0;
            // nMax equals the arg after -n, unless it is gt 20, then nMax = 20 
            case 'n':
                nMax = (atoi(optarg) < 20) ? atoi(optarg) : 20;
                break;
            case 's':
                sMax = atoi(optarg);
                break;
            case 't':
                tMax = atoi(optarg);
                break;
        }
    }

    // exits program if unable to open input file indicated in command line, or if the number of cmd args is incorrect
    if (optind = argc - 1) {
        fp = fopen(argv[optind], "r");
        if (fp == NULL) {
            printf("Unable to find that file.\n");
            return -1;
        }
    }
    else {
        printf("Wrong number of command line arguments.\n");
        return -1;
    }

    // sets up timer and timer interrupt
    if (setupitimer(tMax) == -1) {
        perror("Failed to set up the ITIMER_REAL countdown timer");
        return 1;
    }
    if (setupinterrupt() == -1) {
        perror("Failed to set up handler for SIGPROF");
        return 1;
    }

    // gets number of strings in input file by counting number of newlines, then rewinds filepointer
    for (c = getc(fp); c != EOF; c = getc(fp)){
        if (c == '\n')
            numStrings++;
    } 
    rewind(fp);
    
    struct shmseg {
        int turn;
        enum state flag[20];
        char strings[numStrings][128];
    };

    key_t key = ftok("master", 137);
    int shmid = shmget(key, sizeof(struct shmseg), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }

    struct shmseg* shmptr = shmat(shmid, (void*)0, 0);
    if (shmptr == (void*)-1) {
        perror("Shared memory attach");
        return 1;
    }

    shmptr->turn = 0;
    for (i = 0; i < 20; i++) {
        shmptr->flag[i] = idle;
    }
    

    // reads input file, string-by-string, into shared memory, adding null terminator to each
    i = 0;
    while (fgets(shmptr->strings[i], 128, fp)) {
        shmptr->strings[i][strlen(shmptr->strings[i]) - 1] = '\0';
        i++;
    }
    fclose(fp);

    pid_t  pid;
    for (i = 0; i < numStrings && i < nMax; i++) {
        while (currentProcesses >= sMax) {
            wait(&status);
            currentProcesses--;
        }

        pid = fork();
        currentProcesses++;

        if (pid == -1) {
            perror("Can't Fork");
        }
        else if (pid == 0) {
            char num[10], index[10];
            sprintf(index, "%d", i);
            sprintf(num, "%d", numStrings);
            execl("palin", index, num, (char*)NULL);
            exit(0);
        }
        else {
            while (currentProcesses >= sMax) {
                if (wait(&status) > 0) {
                    if (WIFEXITED(status) && !WEXITSTATUS(status)) ;// printf("program execution successful\n");
                    else if (WIFEXITED(status) && WEXITSTATUS(status)) {
                        if (WEXITSTATUS(status) == 127) perror("execl failed");
                        else perror("program terminated normally, but returned a non-zero status");
                    }
                    else perror("program didn't terminate normally");
                }
                currentProcesses--;
            }
        }
    }

    // waits for exit code from all remaining child processes
    for (i = 0; i < currentProcesses; i++) {
        wait(&status);
    }

    // detaches strings array from shared memory, then destroys shared memory segment
    if (shmdt(shmptr) == -1) {
        perror("shmdt");
        return -1;
    }
    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        perror("shmctl");
        return -1;
    }

    fp2 = fopen("output.log", "a");
    gettimeofday(&current, NULL);
    fprintf(fp2, "final time: %.5f s\n", ((current.tv_sec - start.tv_sec) * 1000000 + current.tv_usec - start.tv_usec) / 1000000.);
    fclose(fp2);

    return 0;
}