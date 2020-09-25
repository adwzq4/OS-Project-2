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

static void timerhandler(int s) {
    signal(SIGQUIT, SIG_IGN);
    kill(-getpid(), SIGQUIT);
    nMax = sMax = 0;
}

static int setupinterrupt(void) {
    struct sigaction act;
    act.sa_handler = timerhandler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    return (sigaction(SIGALRM, &act, NULL));
}

static int setupitimer(int t) {
    struct itimerval value;
    value.it_value.tv_sec = t;
    value.it_value.tv_usec = 0;
    value.it_interval.tv_sec = value.it_interval.tv_usec = 0;
    return (setitimer(ITIMER_REAL, &value, NULL));
}

int main(int argc, char* argv[]) {
    int opt, tMax;
    int numStrings = 0;
    int i = 0;
    nMax = 10;
    sMax = 2;
    tMax = 100;
    FILE* fp;
    //FILE* fp2;
    char c;
    struct timeval current, start;
    
    gettimeofday(&start, NULL);

    while ((opt = getopt(argc, argv, "hn:s:t:")) != -1) {
        switch (opt) {
        case 'h':
            printf("\n\n\n--- palin Help Page ---\n\n\
                palin reads a file of strings, and for each string creates a child process to determine\n\
                whether the string is a palindrom, then writes all palindromes to one palin.out, and all\n\
                non-palindromes to nopalin.out\n\n\
                  Invocation:\nmaster -h\nmaster [-n x] [-s x] [-t time] infile\n  Options:\n\
                -h Describe how the project should be run and then, terminate.\n\
                -n x Indicate the maximum total of child processes master will ever create. (Default 4)\n\
                -s x Indicate the number of children allowed to exist in the system at the same time. (Default 2)\n\
                -t time The time in seconds after which the process will terminate, even if it has not finished. (Default 100)\n\
                infile Input file containing strings to be tested.");
            return 0;
        case 'n':
            nMax = atoi(optarg);
            break;
        case 's':
            sMax = atoi(optarg);
            break;
        case 't':
            tMax = atoi(optarg);
            break;
        }
    }

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

    if (setupinterrupt() == -1) {
        perror("Failed to set up handler for SIGPROF");
        return 1;
    }
    if (setupitimer(tMax) == -1) {
        perror("Failed to set up the ITIMER_PROF interval timer");
        return 1;
    }

    for (c = getc(fp); c != EOF; c = getc(fp)){
        if (c == '\n')
            numStrings++;
    } 
    rewind(fp);
    
    key_t key = ftok("master", 137);
    int shmid = shmget(key, numStrings * 128, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("Shared memory");
        if (shmctl(shmid, IPC_RMID, 0) == -1) {
            perror("shmctl");
            return -1;
        }
        return 1;
    }
    //fprintf(stderr, "Value of errno: %d\n", errno); printf("%lu", shmid);
    
    char(*strings)[numStrings][128] = shmat(shmid, (void*)0, 0);
    if (strings == (void*)-1) {
        perror("Shared memory attach");
        return 1;
    }
    
    while (fgets((*strings)[i], 128, fp)) {
        (*strings)[i][strlen((*strings)[i]) - 1] = '\0';
        i++;
    }

    //for (i = 0; i < numStrings; i++) {
    //    printf("%d%s\n", i, (*strings)[i]);
    //}

    fclose(fp);

    //fp2 = fopen("output.log", "a");
    int currentProcesses = 0;
    int status;
    pid_t  pid;
    i = 0;
    for (i = 0; i < numStrings && i < nMax; i++) {
        while (currentProcesses >= sMax) {
            wait(&status);
            currentProcesses--;
        };

        pid = fork();
        currentProcesses++;

        if (pid == -1) {
            perror("Can't Fork");
        }
        else if (pid == 0) {
            //fprintf(fp2, "--%u   %d   %s\n", getpid(), i, (*strings)[i]);
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
                        if (WEXITSTATUS(status) == 127) {
                            perror("execl failed");
                        }
                        else
                            perror("program terminated normally, but returned a non-zero status");
                    }
                    else
                        perror("program didn't terminate normally");
                }
                //else perror("wait() failed\n");
                currentProcesses--;
            }
        }
    }

    for (i = 0; i < currentProcesses; i++) {
        wait(&status);
    }

    gettimeofday(&current, NULL);
    printf("time: %.5f s\n", ((current.tv_sec - start.tv_sec) * 1000000 + current.tv_usec - start.tv_usec) / 1000000.);

    if (shmdt(strings) == -1) {
        perror("shmdt");
        return -1;
    }
    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        perror("shmctl");
        return -1;
    }

    //fclose(fp2);
    return 0;
}