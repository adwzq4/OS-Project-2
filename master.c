// Author: Adam Wilson
// Date: 10/2/2020

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 

int main(int argc, char* argv[]) {
    int opt, nMax, sMax, tMax;
    int numStrings = 0;
    int i = 0;
    nMax = 4;
    sMax = 2;
    tMax = 100;
    FILE* fp;
    char c;

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
            printf("%d", nMax);
            break;
        case 's':
            sMax = atoi(optarg);
            printf("%d", sMax);
            break;
        case 't':
            tMax = atoi(optarg);
            printf("%d", tMax);
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

    for (c = getc(fp); c != EOF; c = getc(fp)){
        if (c == '\n')
            numStrings++;
    }
    rewind(fp);
    
    key_t key = ftok("master", 137);
    int shmid = shmget(key, numStrings * 128, 0666 | IPC_CREAT);

    char strings[numStrings][128];
    char(*addr)[128] = strings;
    addr = (char(*)[128]) shmat(shmid, (void*)0, 0);
    
    while (fgets(strings[i], 128, fp))
    {
        strings[i][strlen(strings[i]) - 1] = '\0';
        i++;
    }

    //for (i = 0; i < numStrings; i++) {
    //    printf(" %s\n", addr[i]);
    //}

    pid_t  pid;
    int ret = 1;
    int status;
    pid = fork();

    if (pid == -1) {
        printf("can't fork, error occured\n");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        printf("child process, pid = %u\n", getpid());
        printf("parent of child process, pid = %u\n", getppid());

        char n[4];
        sprintf(n, "%d", numStrings);
        execl("palin", "10", n, (char *) NULL);
        exit(0);
    }
    else {
        printf("Parent Of parent process, pid = %u\n", getppid());
        printf("parent process, pid = %u\n", getpid());

        if (waitpid(pid, &status, 0) > 0) {

            if (WIFEXITED(status) && !WEXITSTATUS(status))
                printf("program execution successful\n");

            else if (WIFEXITED(status) && WEXITSTATUS(status)) {
                if (WEXITSTATUS(status) == 127) {
                    printf("execl failed\n");
                }
                else
                    printf("program terminated normally,"
                        " but returned a non-zero status\n");
            }
            else
                printf("program didn't terminate normally\n");
        }
        else {
            printf("waitpid() failed\n");
        }
        exit(0);
    }


    if (shmdt(addr) == -1) {
        perror("shmdt");
        return -1;
    }
    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        perror("shmctl");
        return -1;
    }

    return 0;
}