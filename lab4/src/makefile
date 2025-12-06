CC = gcc
CFLAGS = -std=c11

# Целевые программы
TARGETS = parallel_min_max process_memory

# Правила по умолчанию
all: $(TARGETS)

# Сборка parallel_min_max
parallel_min_max: parallel_min_max.c find_min_max.c utils.c find_min_max.h utils.h
	$(CC) $(CFLAGS) -o parallel_min_max parallel_min_max.c find_min_max.c utils.c

# Сборка process_memory
process_memory: process_memory.c
	$(CC) $(CFLAGS) -o process_memory process_memory.c

# Очистка
clean:
	rm -f $(TARGETS) *.o

# Справка
help:
	@echo "Доступные команды:"
	@echo "  make              - собрать обе программы"
	@echo "  make clean        - удалить скомпилированные файлы"
	@echo "  make help         - показать эту справку"

.PHONY: all clean help