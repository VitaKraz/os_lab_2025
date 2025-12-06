#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

// Глобальные переменные для хранения PID дочерних процессов
static pid_t *child_pids = NULL;
static int timeout_seconds = -1;  // -1 означает отсутствие таймаута

// Обработчик сигнала SIGALRM
void timeout_handler(int sig) {
    if (child_pids != NULL) {
        for (int i = 0; i < optind; i++) {
            if (child_pids[i] > 0) {
                kill(child_pids[i], SIGKILL);
            }
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;
  timeout_seconds = -1;  // Инициализация таймаута

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {"timeout", required_argument, 0, 0},  // Добавлена опция timeout
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:  // seed
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("Seed must be positive\n");
              return 1;
            }
            break;
          case 1:  // array_size
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("Array size must be positive\n");
              return 1;
            }
            break;
          case 2:  // pnum
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be positive\n");
              return 1;
            }
            break;
          case 3:  // by_files
            with_files = true;
            break;
          case 4:  // timeout
            timeout_seconds = atoi(optarg);
            if (timeout_seconds < 0) {
              printf("Timeout must be non-negative\n");
              return 1;
            }
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"seconds\"]\n",
           argv[0]);
    return 1;
  }

  // Выделяем память для хранения PID дочерних процессов
  child_pids = malloc(sizeof(pid_t) * pnum);
  if (child_pids == NULL) {
    perror("malloc");
    return 1;
  }

  // Инициализируем массив PID
  for (int i = 0; i < pnum; i++) {
    child_pids[i] = 0;
  }

  int *array = malloc(sizeof(int) * array_size);
  if (array == NULL) {
    perror("malloc");
    free(child_pids);
    return 1;
  }
  
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // массив каналов для общения с процессами
  int pipefd[pnum][2];
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipefd[i]) == -1) {
        perror("pipe");
        free(array);
        free(child_pids);
        return 1;
      }
    }
  }

  // Регистрируем обработчик сигнала SIGALRM, если задан таймаут
  if (timeout_seconds >= 0) {
    struct sigaction sa;
    sa.sa_handler = timeout_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
      perror("sigaction");
      free(array);
      free(child_pids);
      return 1;
    }
  }

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process
        
        // Устанавливаем игнорирование SIGALRM в дочернем процессе
        if (timeout_seconds >= 0) {
          signal(SIGALRM, SIG_IGN);
        }
        
        int chunk_size = array_size / pnum;
        int start = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;

        struct MinMax local_minmax = GetMinMax(array, start, end);

        if (with_files) {
          // use files here
          char filename[256];
          sprintf(filename, "temp_%d.txt", i);
          FILE *f = fopen(filename, "w");
          if (f == NULL) {
            perror("fopen");
            return 1;
          }
          fprintf(f, "%d %d\n", local_minmax.min, local_minmax.max);
          fclose(f);
        } else {
          // use pipe here
          close(pipefd[i][0]);
          write(pipefd[i][1], &local_minmax.min, sizeof(int));
          write(pipefd[i][1], &local_minmax.max, sizeof(int));
          close(pipefd[i][1]);
        }
        return 0;
      } else {
        // родительский процесс сохраняет PID дочернего
        child_pids[i] = child_pid;
      }
    } else {
      printf("Fork failed!\n");
      free(array);
      free(child_pids);
      return 1;
    }
  }

  // Устанавливаем таймаут, если задан
  if (timeout_seconds >= 0) {
    alarm(timeout_seconds);
  }

  // Ожидание завершения дочерних процессов с обработкой таймаута
  int status;
  pid_t pid;
  int completed_processes = 0;
  
  while (completed_processes < pnum) {
    pid = wait(&status);
    
    if (pid == -1) {
      if (errno == EINTR) {
        // Был получен сигнал SIGALRM (таймаут)
        printf("Timeout reached! Sending SIGKILL to all child processes.\n");
        
        // Отправляем SIGKILL всем оставшимся дочерним процессам
        for (int i = 0; i < pnum; i++) {
          if (child_pids[i] > 0) {
            kill(child_pids[i], SIGKILL);
          }
        }
        
        // Ждем завершения всех процессов после отправки SIGKILL
        while (wait(NULL) > 0 || errno != ECHILD) {
          // Продолжаем ждать
        }
        
        break;
      } else {
        perror("wait");
        break;
      }
    } else {
      completed_processes++;
      
      // Находим и удаляем PID из массива
      for (int i = 0; i < pnum; i++) {
        if (child_pids[i] == pid) {
          child_pids[i] = 0;
          break;
        }
      }
    }
  }

  // Отменяем таймер, если он еще не сработал
  if (timeout_seconds >= 0) {
    alarm(0);
  }

  active_child_processes = 0;  // Все процессы завершены

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  // Чтение результатов только от завершенных процессов
  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // read from files
      char filename[256];
      sprintf(filename, "temp_%d.txt", i);
      FILE *f = fopen(filename, "r");
      if (f != NULL) {
        fscanf(f, "%d %d", &min, &max);
        fclose(f);
        remove(filename);
      }
    } else {
      // read from pipes
      // Закрываем записывающий конец, если он еще не закрыт
      close(pipefd[i][1]);
      
      // Пытаемся прочитать, но если процесс был убит, чтение может завершиться с ошибкой
      ssize_t bytes_read = read(pipefd[i][0], &min, sizeof(int));
      if (bytes_read == sizeof(int)) {
        read(pipefd[i][0], &max, sizeof(int));
      }
      close(pipefd[i][0]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  
  if (timeout_seconds >= 0 && completed_processes < pnum) {
    printf("Warning: Some child processes were terminated due to timeout.\n");
  }
  
  fflush(NULL);
  return 0;
}