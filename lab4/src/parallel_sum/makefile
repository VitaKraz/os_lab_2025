CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
LDFLAGS = -lpthread -lrt

# Целевые программы
TARGETS = parallel_sum

# Файлы для parallel_sum
SUM_SRCS = sum.c parallel_sum.c utils.c
SUM_OBJS = sum.o parallel_sum.o utils.o

# Правила по умолчанию
all: $(TARGETS)

# Сборка parallel_sum
parallel_sum: $(SUM_SRCS)
	$(CC) $(CFLAGS) -o parallel_sum $(SUM_SRCS) $(LDFLAGS)

# Отдельная компиляция объектных файлов (опционально)
sum.o: sum.c sum.h
	$(CC) $(CFLAGS) -c sum.c

parallel_sum.o: parallel_sum.c sum.h utils.h
	$(CC) $(CFLAGS) -c parallel_sum.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

# Тестирование
test: parallel_sum
	@echo "=== Тест 1: Маленький массив ==="
	./parallel_sum --threads_num 4 --array_size 100 --seed 42
	@echo ""
	@echo "=== Тест 2: Средний массив ==="
	./parallel_sum --threads_num 8 --array_size 10000 --seed 123
	@echo ""
	@echo "=== Тест 3: Большой массив ==="
	./parallel_sum --threads_num 4 --array_size 1000000 --seed 456

# Тест производительности
benchmark: parallel_sum
	@echo "=== Тест производительности ==="
	@for threads in 1 2 4 8; do \
		echo "Потоков: $$threads"; \
		./parallel_sum --threads_num $$threads --array_size 10000000 --seed 789 2>/dev/null | grep "Elapsed time"; \
	done

# Очистка
clean:
	rm -f $(TARGETS) *.o

# Справка
help:
	@echo "Доступные команды:"
	@echo "  make              - собрать parallel_sum"
	@echo "  make test         - запустить тесты"
	@echo "  make benchmark    - тест производительности"
	@echo "  make clean        - удалить скомпилированные файлы"
	@echo "  make help         - показать эту справку"

.PHONY: all test benchmark clean help