/**
 * TE355 - Trabalho 2 - Semaphore
 * Author: Diego R. Garzaro
 * GRR20172364
 * 
 * */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_THREADS 6

typedef struct Letter_Sequence
{
    char letter;
    int quantity;
} Letter_Sequence;

sem_t my_sem;
int count = 0;

Letter_Sequence sequence[6] = {{'d', 6}, {'c', 9}, {'a', 11}, {'c', 17}, {'e', 8}, {'d', 28}};
char thread_letters[NUM_THREADS] = {'a', 'b', 'c', 'd', 'e', 'f'};

void *thread_handler(void *arg)
{
    // Hold semaphore
    int *t_letter = (int *)arg;
    // Wait
    sem_wait(&my_sem);

    // Verify if the correct thread was called
    if (sequence[count].letter == thread_letters[*t_letter])
    {
        int j;
        // Print the letter
        for (j = 0; j < sequence[count].quantity; j++)
        {
            printf("- %c\n", thread_letters[*t_letter]);
        }
        count++;
    }

    usleep(1000);
    // Release semaphore
    sem_post(&my_sem);
    pthread_exit(NULL);
}

int main()
{
    pthread_t threads[NUM_THREADS];

    // Header
    printf("TE355 - Trabalho 2 - Semaphore\n");
    printf("Aluno: Diego R. Garzaro - GRR20172364\n");

    // Create semaphore
    int create_sem = sem_init(&my_sem, 0, 0);
    // Verify if semaphore was created successfully
    if (create_sem < 0)
    {
        perror("Failed to create semaphore\n");
        return 1;
    }

    // Loop
    do
    {
        int i;
        for (i = 0; i < NUM_THREADS; i++)
        {
            int *parameter;
            parameter = calloc(1, sizeof(int *));
            *parameter = i;
            int rc = pthread_create(&threads[i], NULL, thread_handler, parameter);
            if (rc)
            {
                perror("Failed to create thread\n");
                return 1;
            }
        }

        sem_post(&my_sem);

        for (i = 0; i < NUM_THREADS; i++)
        {
            pthread_join(threads[i], NULL);
        }
    } while (count < 6);

    return 0;
}