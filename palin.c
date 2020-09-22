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

    printf("child: %s\n%s\n", argv[0], argv[1]);

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
