#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "multmodulo.h"

struct Server {
    char ip[255];
    int port;
};

struct ThreadArgs {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
};

bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;
    unsigned long long i = strtoull(str, &end, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Out of uint64_t range: %s\n", str);
        return false;
    }
    if (errno != 0)
        return false;
    *val = i;
    return true;
}

void *ThreadServer(void *args) {
    struct ThreadArgs *targs = (struct ThreadArgs *)args;
    
    struct hostent *hostname = gethostbyname(targs->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed with %s\n", targs->server.ip);
        targs->result = 1;
        return NULL;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(targs->server.port);
    
    // Исправление: используем h_addr_list вместо h_addr
    if (hostname->h_addr_list[0] != NULL) {
        memcpy(&server.sin_addr.s_addr, hostname->h_addr_list[0], hostname->h_length);
    } else {
        fprintf(stderr, "No address found for %s\n", targs->server.ip);
        targs->result = 1;
        return NULL;
    }

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed!\n");
        targs->result = 1;
        return NULL;
    }

    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
        fprintf(stderr, "Connection failed to %s:%d\n", 
                targs->server.ip, targs->server.port);
        close(sck);
        targs->result = 1;
        return NULL;
    }

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &targs->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &targs->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &targs->mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < 0) {
        fprintf(stderr, "Send failed to %s:%d\n", 
                targs->server.ip, targs->server.port);
        close(sck);
        targs->result = 1;
        return NULL;
    }

    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
        fprintf(stderr, "Receive failed from %s:%d\n", 
                targs->server.ip, targs->server.port);
        close(sck);
        targs->result = 1;
        return NULL;
    }

    uint64_t answer = 0;
    memcpy(&answer, response, sizeof(uint64_t));
    targs->result = answer;

    close(sck);
    return NULL;
}

int main(int argc, char **argv) {
    uint64_t k = 0;
    uint64_t mod = 0;
    char servers_path[255] = {'\0'};
    bool k_set = false, mod_set = false, servers_set = false;

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"k", required_argument, 0, 0},
            {"mod", required_argument, 0, 0},
            {"servers", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
            case 0: {
                switch (option_index) {
                    case 0:
                        k_set = ConvertStringToUI64(optarg, &k);
                        if (!k_set || k == 0) {
                            fprintf(stderr, "k must be positive\n");
                            return 1;
                        }
                        break;
                    case 1:
                        mod_set = ConvertStringToUI64(optarg, &mod);
                        if (!mod_set || mod == 0) {
                            fprintf(stderr, "mod must be positive\n");
                            return 1;
                        }
                        break;
                    case 2:
                        memcpy(servers_path, optarg, strlen(optarg));
                        servers_path[strlen(optarg)] = '\0';
                        servers_set = true;
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
            } break;
            case '?':
                printf("Arguments error\n");
                break;
            default:
                fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (!k_set || !mod_set || !servers_set) {
        fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
                argv[0]);
        return 1;
    }

    FILE *servers_file = fopen(servers_path, "r");
    if (!servers_file) {
        perror("Failed to open servers file");
        return 1;
    }

    struct Server servers[100];
    int servers_count = 0;
    char line[255];

    while (fgets(line, sizeof(line), servers_file) && servers_count < 100) {
        line[strcspn(line, "\n")] = 0;
        
        char *colon = strchr(line, ':');
        if (colon == NULL) {
            fprintf(stderr, "Invalid server format: %s\n", line);
            continue;
        }
        
        *colon = '\0';
        strncpy(servers[servers_count].ip, line, 254);
        servers[servers_count].port = atoi(colon + 1);
        servers_count++;
    }
    fclose(servers_file);

    if (servers_count == 0) {
        fprintf(stderr, "No valid servers found in file\n");
        return 1;
    }

    pthread_t threads[servers_count];
    struct ThreadArgs thread_args[servers_count];
    
    uint64_t numbers_per_server = k / servers_count;
    uint64_t remainder = k % servers_count;
    uint64_t current_start = 1;

    for (int i = 0; i < servers_count; i++) {
        thread_args[i].server = servers[i];
        thread_args[i].begin = current_start;
        
        thread_args[i].end = current_start + numbers_per_server - 1;
        if ((uint64_t)i < remainder) {  // Приведение типов для сравнения
            thread_args[i].end++;
        }
        
        if (thread_args[i].end > k) {
            thread_args[i].end = k;
        }
        
        thread_args[i].mod = mod;
        thread_args[i].result = 1;
        
        current_start = thread_args[i].end + 1;
        
        if (pthread_create(&threads[i], NULL, ThreadServer, 
                           (void *)&thread_args[i]) != 0) {
            fprintf(stderr, "Failed to create thread for server %d\n", i);
            thread_args[i].result = 1;
        }
    }

    for (int i = 0; i < servers_count; i++) {
        pthread_join(threads[i], NULL);
    }

    uint64_t final_result = 1;
    for (int i = 0; i < servers_count; i++) {
        final_result = MultModulo(final_result, thread_args[i].result, mod);
    }

    printf("Result: %lu\n", final_result);
    return 0;
}