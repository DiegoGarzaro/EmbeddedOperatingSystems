#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>

typedef struct Thread_Struct{
    pthread_t tid;
    int fat_min;
    int fat_max;
    long double result;
} Thread;

long double run_thread(int num, int num_thread);
void *threadFunction(void *arg);
long double fatorial(int min_num, int max_num);

int main(int argc, char *argv[]){
    int num_thread, num;
    int thread_size;

    // Start measuring time
    struct timeval begin, end;
    gettimeofday(&begin, 0);

    // User Input
    if(argc != 3){
        fprintf(stderr, "Arguments Error\nSyntax Example: ./program [fatorial_number] [thread_number]\n");
        return 0;
    }

    num_thread = atoi(argv[2]);
    num = atoi(argv[1]);

    if(num < 0 || num_thread > num || num_thread < 0){
        printf("\nInput Error!\n");
        return 0;
    }

    long double result;

    result = run_thread(num, num_thread);

    //printf("\nThe factorial of %d is: %Lf\n", num, result);

    // Stop measuring time and calculate the elapsed time
    gettimeofday(&end, 0);
    long seconds = end.tv_sec - begin.tv_sec;
    long microseconds = end.tv_usec - begin.tv_usec;
    double elapsed = seconds + microseconds*1e-6;
    
    // Print elapsed time in csv file
    printf("%d,%d,%.3f\n", num, num_thread, elapsed);

    return 0;
}

void *threadFunction(void *arg){
    Thread *t = arg;
    //printf("Factorial Thread range: %d - %d\n", t->fat_min, t->fat_max);
    t->result = fatorial(t->fat_min, t->fat_max);
    //printf("Partial factorial inside thread: %Lf\n", t->result);
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

long double run_thread(int num, int num_thread){
    // Thread Variables and Aux. Variables
    Thread threads[num_thread];
    int divisao, aux;
    long double final_result = 1;
    int i;

    if(num_thread == 0){
        final_result = fatorial(1, num);
    } else{
        // Organize the number of factorial for each thread
        if((num) % num_thread != 0){
            divisao = ((num) / num_thread) + 1;
            aux = num;
        }
        else{
            divisao = ((num) / num_thread);
            aux = -1;
        }

        for(i = 0; i < num_thread; i++){
            if(i == 0){
                threads[0].fat_min = 1;
                threads[0].fat_max = divisao;
            }
            else{
                threads[i].fat_min = threads[i-1].fat_max + 1;
                threads[i].fat_max = threads[i-1].fat_max + divisao;
            }
            threads[i].tid = i;
            if(aux > 0){
                aux = aux - divisao;
                if(aux % num_thread == 0 || aux <= num_thread){
                    divisao = divisao - 1;
                    aux = -1;
                }
            }
        }
        threads[num_thread-1].fat_max = num;

        // Creating and calling the threads to thread's function
        for(i = 0; i < num_thread; i++){
            int err = pthread_create(&threads[i].tid, NULL, &threadFunction, (void *)&threads[i]);
            if(err != 0){
                fprintf(stderr, "Nao foi possivel criar a thread %d\n", i);
                return 1;
            }
        }
        // Waiting the threads finish the factorial
        for(i = 0; i < num_thread; i++){
            pthread_join(threads[i].tid, NULL);
        }
        for(i = 0; i < num_thread; i++){
            final_result = final_result * threads[i].result;
        }
        //printf("Result inside function: %Lf", final_result);
    }
    return final_result;
}