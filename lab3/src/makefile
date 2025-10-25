CC := gcc

TARGETS := parallel_min_max sequential_min_max run_sequential

.PHONY: all clean

all: $(TARGETS)

parallel_min_max: parallel_min_max.o find_min_max.o utils.o
	$(CC) $(CFLAGS) -o $@ $^

sequential_min_max: sequential_min_max.o find_min_max.o utils.o
	$(CC) $(CFLAGS) -o $@ $^

run_sequential: run_sequential.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS)
