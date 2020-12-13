#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>

#define READ 0
#define WRITE 1

long double fatorial(int min_num, int max_num);
long double run_process(int num, int num_process);

int main(int argc, char *argv[]){
    int num_process, num;

    // Start measuring time
    struct timeval begin, end;
    gettimeofday(&begin, 0);

    // User Input
    if(argc != 3){
        fprintf(stderr, "Arguments Error\nSyntax Example: ./program [fatorial_number] [process_number]\n");
        return 0;
    }

    num_process = atoi(argv[2]);
    num = atoi(argv[1]);

    if(num < 0 || num_process > num || num_process < 0){
        printf("\nInput Error!\n");
        return 0;
    }

    long double result;
    
    result = run_process(num, num_process);

    //printf("\nThe factorial of %d is: %Lf\n", num, result);

    // Stop measuring time and calculate the elapsed time
    gettimeofday(&end, 0);
    long seconds = end.tv_sec - begin.tv_sec;
    long microseconds = end.tv_usec - begin.tv_usec;
    double elapsed = seconds + microseconds*1e-6;
    
    // Print elapsed time in csv file
    printf("%d,%d,%.3f\n", num, num_process, elapsed);

    return 0;
}

long double fatorial(int min_num, int max_num){
    int i;
    long double result = 1;
    for(i = min_num; i <= max_num; i++){
        result *= i;
        // Delay 1ms
        usleep(1000);
    }
    return result;
}

long double run_process(int num, int num_process){
    // Process Variables and Aux. Variables
    int fd[num_process][2];
    pid_t pid;
    int child_params[num_process][2];
    int divisao, aux;
    long double final_result = 1;
    int i;

    if(num_process == 0){
        final_result = fatorial(1, num);
    } else{
        if((num) % num_process != 0){
            divisao = ((num) / num_process) + 1;
            aux = num;
        }
        else{
            divisao = ((num) / num_process);
            aux = -1;
        }

        for(i = 0; i < num_process; i++){
            if(i == 0){
                child_params[0][0] = 1;
                child_params[0][1] = divisao;
            }
            else{
                child_params[i][0] = child_params[i-1][1] + 1;
                child_params[i][1] = child_params[i-1][1] + divisao;
            }
            if(aux > 0){
                aux = aux - divisao;
                if(aux % num_process == 0 || aux <= num_process){
                    divisao = divisao - 1;
                    aux = -1;
                }
            }
        }
        child_params[num_process-1][1] = num;

        for(i = 0; i < num_process; i++){
            // Verify if the pipe was created successful
            if(pipe(fd[i]) == -1){
                printf("\nERROR in stabilish the %dth communication\n", i);
                return 1;
            }

            pid = fork();
            // Error to create the process
            if(pid < 0){
                printf("\nERROR: The children process was not created!\n");
                return 2;
            }
            else if(pid == 0){
                // Children Process
                // Process Header
                printf("Process %d Info: (PID: %d[parent] - %d[children])\n", i, getppid(), getpid());
                // Close the Read communication on the pipe
                close(fd[i][READ]);
                // Execute factorial
                long double result;
                result = fatorial(child_params[i][0], child_params[i][1]);
                write(fd[i][WRITE], &result, sizeof(result));
                wait(NULL);
                // Close the Write communication on the pipe
                close(fd[i][WRITE]);
                exit(0);
            }
            //printf("\n");
        }

        
        if(pid > 0){
            // Parent Process
            for(i = 0; i < num_process; i++){
                // Parent Process
                // Close the Write communication on the pipe
                close(fd[i][WRITE]);
                wait(NULL);
                // Receave the partial result of factorial from children process
                long double partial_result;
                read(fd[i][READ], &partial_result, sizeof(partial_result));
                // Multiply the partial factorial to the final result
                final_result = final_result * partial_result;
                // Close the Read Communication on the pipe
                close(fd[i][READ]);
                //printf("Partial Result: %.0Lf\nFinal Result: %.0Lf\n", partial_result, final_result);
            }
        }
    }
    
    return final_result;
}