OBJS = main.o ssu_score.o blank.o
SRCS = main.c ssu_score.c blank.c

CC = gcc
CFLAGS = -g
TARGET = ssu_score

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)
	rm $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET) core
