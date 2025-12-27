#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void* thread1_func(void* arg) {
    printf("Thread 1: locking mutex1\n");
    pthread_mutex_lock(&mutex1);
    
    sleep(1);  // Даем время второму потоку захватить mutex2
    
    printf("Thread 1: trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2);  // DEADLOCK
    
    printf("Thread 1: locked both mutexes\n");
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return NULL;
}

void* thread2_func(void* arg) {
    printf("Thread 2: locking mutex2\n");
    pthread_mutex_lock(&mutex2);
    
    sleep(1);  // Даем время первому потоку захватить mutex1
    
    printf("Thread 2: trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1);  // DEADLOCK
    
    printf("Thread 2: locked both mutexes\n");
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    
    printf("Starting deadlock demonstration...\n");
    printf("Both threads will lock mutexes in different order:\n");
    printf("Thread 1: mutex1 -> mutex2\n");
    printf("Thread 2: mutex2 -> mutex1\n\n");
    
    pthread_create(&thread1, NULL, thread1_func, NULL);
    pthread_create(&thread2, NULL, thread2_func, NULL);
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    printf("This line will never be printed (deadlock!)\n");
    return 0;
}