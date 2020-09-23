// Author: Adam Wilson
// Date: 10/2/2020

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 

void palinCheck(char*);

int main(int argc, char* argv[]) {
    int i;

    printf("child: %s\n%s\n", argv[0], argv[1]);
    int index = atoi(argv[0]);
    int numStrings = atoi(argv[1]);

    key_t key = ftok("master", 137);
    int shmid = shmget(key, numStrings * 128, 0666 | IPC_CREAT);

    char(*addr)[128] = (char(*)[128]) shmat(shmid, (void*)0, 0);
    printf(" %s\n", addr[0]);
    //for (i = 0; i < numStrings; i++) {
    //    printf(" %s\n", addr[i]);
    //}

	return 0;
}

void palinCheck(char* str) {
    int left = 0;
    int right = strlen(str) - 1;

    while (left > right) {
        if (str[left++] != str[right--]) {
            printf("no\n");
        }
    }
    printf("yes\n");
}
