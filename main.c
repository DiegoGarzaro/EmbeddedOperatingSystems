#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#define READ 0
#define WRITE 1

long double fatorial(int min_num, int max_num);
long double run_process(int num, int num_process);

int main(int argc, char *argv[]){
    int num_process = atoi(argv[2]), num = atoi(argv[1]);

    // User Input
    printf("\n");

    if(argc != 3){
        printf("\nInput Error\n");
        return 0;
    }

    if(num < 0 || num_process > num || num_process < 0){
        printf("\nInput Error!\n");
        return 0;
    }

    long double result;
    
    result = run_process(num, num_process);

    printf("\nThe factorial of %d is: %Lf\n", num, result);

    return 0;
}

long double fatorial(int min_num, int max_num){
    int i;
    long double result = 1;
    for(i = min_num; i <= max_num; i++){
        result *= i;
        usleep(1000);
    }
    return result;
}

long double run_process(int num, int num_process){
    // Process Variables and Aux. Variables
    int fd[num_process][2];
    pid_t pid;
    int child_params[num_process][2];
    int divisao;
    long double final_result = 1;
    int i;

    if(num_process == 0){
        final_result = fatorial(1, num);
    } else{
        if(num % num_process != 0){
            divisao = (num / num_process) + 1;
        }
        else{
            divisao = (num / num_process);
        }

        for(i = 0;i < num_process; i++){
            child_params[i][0] = (i*divisao) + 1;
            child_params[i][1] = (i+1) * divisao;
        }
        child_params[-1][1] = num;

        for(i = 0; i < num_process; i++){
            if(pipe(fd[i]) == -1){
                printf("\nERROR in stabilish the %dth communication\n", i);
                return 1;
            }

            pid = fork();

            if(pid < 0){
                printf("\nERROR: The children process was not created!\n");
                return 2;
            }
            else if(pid == 0){
                // Children Process
                // Process Header
                //printf("Process %d Info: (PID: %d[parent] - %d[children])\n", i, getppid(), getpid());
                close(fd[i][READ]);
                //printf("\nThe son receaved the range: %d - %d\n", child_params[i][0], child_params[i][1]);
                // Execute factorial
                long double result;
                result = fatorial(child_params[i][0], child_params[i][1]);
                //printf("The result of factorial inside the son process: %.0Lf\n", result);
                write(fd[i][WRITE], &result, sizeof(result));
                wait(NULL);
                close(fd[i][WRITE]);
                exit(0);
            }
            //printf("\n");
        }

        
        if(pid > 0){
            // Parent Process
            for(i = 0; i < num_process; i++){
                // Parent Process
                close(fd[i][WRITE]);
                wait(NULL);
                long double partial_result;
                read(fd[i][READ], &partial_result, sizeof(partial_result));
                final_result = final_result * partial_result;
                close(fd[i][READ]);
                //printf("Partial Result: %.0Lf\nFinal Result: %.0Lf\n", partial_result, final_result);
            }
        }
    }
    
    return final_result;
}










/*
int main(){
    int fd[2];

    if(pipe(fd) == -1){
        printf("\nERROR in stabilish the communication\n");
    }

    int id = fork();
    if(id == 0){
        close(fd[READ]);
        int x;
        printf("Input a number in child PID: %d[parent] - %d[son]: ", getppid(), getpid());
        scanf("%d", &x);
        write(fd[WRITE], &x, sizeof(int));
        close(fd[WRITE]);
    }
    else{
        close(fd[WRITE]);
        int y;
        read(fd[READ], &y, sizeof(int));
        close(fd[READ]);
        printf("\nThe number entered in child is: %d\n", y);
    }
}
*/