#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

// Функция для создания зомби-процессов
void create_zombies(int count, int zombie_time) {
    printf("Родительский процесс (PID: %d) создает %d зомби-процессов...\n", 
           getpid(), count);
    
    for (int i = 0; i < count; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        
        if (pid == 0) {
            // Дочерний процесс
            printf("  Дочерний процесс %d создан (PID: %d)\n", i+1, getpid());
            
            // Завершаем дочерний процесс немедленно, делая его зомби
            printf("  Дочерний процесс %d (PID: %d) завершается...\n", i+1, getpid());
            exit(EXIT_SUCCESS);
        } else {
            // Родительский процесс
            // НЕ вызываем wait() для дочернего процесса,
            // оставляя его в состоянии зомби
            printf("  Родитель не ждет завершения дочернего процесса %d\n", i+1);
            
            // Небольшая пауза между созданием процессов
            sleep(1);
        }
    }
    
    // Родительский процесс спит, оставляя зомби-процессы в системе
    printf("\nЗомби-процессы активны. Проверьте их с помощью команды:\n");
    printf("  ps aux | grep 'Z' | grep -v grep\n");
    printf("или\n");
    printf("  ps -eo pid,stat,command | grep 'Z'\n\n");
    
    printf("Родительский процесс будет спать %d секунд, прежде чем завершиться...\n", 
           zombie_time);
    printf("Когда родитель завершится, зомби-процессы будут убраны init\n");
    sleep(zombie_time);
    
    printf("\nРодительский процесс завершается. Зомби исчезнут.\n");
}

// Функция для демонстрации правильного завершения процессов
void proper_cleanup(int count) {
    printf("\n=== Правильное завершение процессов ===\n");
    printf("Родительский процесс (PID: %d) создает %d дочерних процессов...\n", 
           getpid(), count);
    
    for (int i = 0; i < count; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        
        if (pid == 0) {
            // Дочерний процесс
            printf("  Дочерний процесс %d (PID: %d) запущен\n", i+1, getpid());
            sleep(1); // Имитация работы
            printf("  Дочерний процесс %d (PID: %d) завершается\n", i+1, getpid());
            exit(EXIT_SUCCESS);
        }
    }
    
    // Родительский процесс ЖДЕТ завершения всех дочерних процессов
    printf("\nРодительский процесс ждет завершения всех дочерних процессов...\n");
    
    int status;
    pid_t wpid;
    
    while ((wpid = wait(&status)) > 0) {
        printf("  Дочерний процесс (PID: %d) завершен корректно\n", wpid);
    }
    
    printf("Все дочерние процессы завершены. Зомби не создано.\n");
}

int main(int argc, char *argv[]) {
    printf("=== Демонстрация зомби-процессов ===\n\n");
    
    // Параметры по умолчанию
    int zombie_count = 3;
    int zombie_time = 30;
    int demo_mode = 0; // 0 - создание зомби, 1 - правильное завершение
    
    // Обработка аргументов командной строки
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--count") == 0 && i+1 < argc) {
                zombie_count = atoi(argv[++i]);
            } else if (strcmp(argv[i], "--time") == 0 && i+1 < argc) {
                zombie_time = atoi(argv[++i]);
            } else if (strcmp(argv[i], "--proper") == 0) {
                demo_mode = 1;
            } else if (strcmp(argv[i], "--help") == 0) {
                printf("Использование: %s [опции]\n", argv[0]);
                printf("  --count N   Создать N зомби-процессов (по умолчанию: 3)\n");
                printf("  --time T    Держать зомби T секунд (по умолчанию: 30)\n");
                printf("  --proper    Показать правильное завершение процессов\n");
                printf("  --help      Показать эту справку\n");
                return 0;
            }
        }
    }
    
    if (zombie_count <= 0) zombie_count = 1;
    if (zombie_time <= 0) zombie_time = 10;
    
    if (demo_mode == 1) {
        proper_cleanup(zombie_count);
    } else {
        printf("Режим: создание зомби-процессов\n");
        printf("Количество зомби: %d\n", zombie_count);
        printf("Время жизни зомби: %d секунд\n", zombie_time);
        printf("\n");
        create_zombies(zombie_count, zombie_time);
    }
    
    return 0;
}