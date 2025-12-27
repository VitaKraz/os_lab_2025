# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -pthread
LDFLAGS = -pthread

# Цели по умолчанию
all: client server

# Объектные файлы
COMMON_OBJS = multmodulo.o
CLIENT_OBJS = client.o $(COMMON_OBJS)
SERVER_OBJS = server.o $(COMMON_OBJS)

# Клиент
client: $(CLIENT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Сервер
server: $(SERVER_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Правила компиляции объектных файлов
client.o: client.c multmodulo.h
	$(CC) $(CFLAGS) -c $< -o $@

server.o: server.c multmodulo.h
	$(CC) $(CFLAGS) -c $< -o $@

multmodulo.o: multmodulo.c multmodulo.h
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка
clean:
	rm -f client server *.o

# Запуск тестового сервера
run-server: server
	./server --port 20001 --tnum 4

# Запуск тестового клиента
run-client: client
	echo "127.0.0.1:20001" > servers.txt
	echo "127.0.0.1:20002" >> servers.txt
	./client --k 10 --mod 12345 --servers servers.txt

# Полный тест
test: all run-client

# Проверка на утечки памяти
valgrind-client: client
	valgrind --leak-check=full ./client --k 10 --mod 12345 --servers servers.txt

valgrind-server: server
	valgrind --leak-check=full ./server --port 20001 --tnum 2

# Сборка с отладочной информацией
debug: CFLAGS += -g -O0 -DDEBUG
debug: clean all

# Сборка с оптимизацией
release: CFLAGS += -O3
release: clean all

.PHONY: all clean run-server run-client test valgrind-client valgrind-server debug release