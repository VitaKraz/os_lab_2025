#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdatomic.h>

// Структура для передачи данных в потоки
typedef struct {
    long long k;                // Число для вычисления факториала
    long long mod;              // Модуль
    long long* current;         // Текущее число для обработки (разделяемая переменная)
    long long* result;          // Результат (разделяемая переменная)
    pthread_mutex_t* result_mutex;  // Мьютекс для синхронизации результата
} ThreadData;

// Функция, которую выполняет каждый поток
void* worker(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    long long local_result = 1;
    long long num = 0;
    
    // Каждый поток берет следующее число для обработки
    while (1) {
        // Атомарно увеличиваем current и получаем предыдущее значение
        num = __atomic_fetch_add(data->current, 1, __ATOMIC_RELAXED);
        
        if (num > data->k) {
            break;  // Все числа обработаны
        }
        
        // Вычисляем локальный результат
        local_result = (local_result * num) % data->mod;
    }
    
    // Синхронизация итогового результата с использованием мьютекса
    pthread_mutex_lock(data->result_mutex);
    *data->result = (*data->result * local_result) % data->mod;
    pthread_mutex_unlock(data->result_mutex);
    
    return NULL;
}

// Функция для вычисления факториала
long long calculate_factorial(long long k, int pnum, long long mod) {
    long long current = 1;
    long long result = 1;
    pthread_t* threads = NULL;
    ThreadData* thread_data = NULL;
    pthread_mutex_t result_mutex;
    
    // Инициализируем мьютекс
    if (pthread_mutex_init(&result_mutex, NULL) != 0) {
        fprintf(stderr, "Ошибка инициализации мьютекса\n");
        return -1;
    }
    
    // Выделяем память для потоков и их данных
    threads = (pthread_t*)malloc(pnum * sizeof(pthread_t));
    thread_data = (ThreadData*)malloc(pnum * sizeof(ThreadData));
    
    if (threads == NULL || thread_data == NULL) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        free(threads);
        free(thread_data);
        pthread_mutex_destroy(&result_mutex);
        return -1;
    }
    
    // Создаем и запускаем потоки
    for (int i = 0; i < pnum; i++) {
        thread_data[i].k = k;
        thread_data[i].mod = mod;
        thread_data[i].current = &current;
        thread_data[i].result = &result;
        thread_data[i].result_mutex = &result_mutex;
        
        if (pthread_create(&threads[i], NULL, worker, &thread_data[i]) != 0) {
            fprintf(stderr, "Ошибка создания потока %d\n", i);
            // Завершаем уже созданные потоки
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            free(threads);
            free(thread_data);
            pthread_mutex_destroy(&result_mutex);
            return -1;
        }
    }
    
    // Ожидаем завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Освобождаем ресурсы
    free(threads);
    free(thread_data);
    pthread_mutex_destroy(&result_mutex);
    
    return result;
}

void print_usage(const char* program_name) {
    printf("Использование: %s -k <число> --pnum=<количество_потоков> --mod=<модуль>\n", program_name);
    printf("Пример: %s -k 10 --pnum=4 --mod=10\n", program_name);
}

int main(int argc, char* argv[]) {
    long long k = 0;
    int pnum = 1;
    long long mod = 0;
    int opt;
    int option_index = 0;
    
    // Определение длинных опций
    struct option long_options[] = {
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };
    
    // Парсинг аргументов командной строки
    while ((opt = getopt_long(argc, argv, "k:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'k':
                k = atoll(optarg);
                break;
            case 'p':
                pnum = atoi(optarg);
                break;
            case 'm':
                mod = atoll(optarg);
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Проверка корректности введенных данных
    if (k <= 0 || pnum <= 0 || mod <= 0) {
        fprintf(stderr, "Ошибка: все параметры должны быть положительными числами!\n");
        print_usage(argv[0]);
        return 1;
    }
    
    
    // Если потоков больше чем чисел для обработки
    if (pnum > k) {
        pnum = k;
        printf("Предупреждение: количество потоков уменьшено до %d (так как k = %lld)\n", pnum, k);
    }
    
    // Вычисляем факториал
    long long result = calculate_factorial(k, pnum, mod);
    
    if (result == -1) {
        fprintf(stderr, "Ошибка при вычислении факториала\n");
        return 1;
    }
    
    // Выводим результат
    printf("%lld! mod %lld = %lld\n", k, mod, result);
    
    return 0;
}