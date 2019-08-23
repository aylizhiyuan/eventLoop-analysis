SRC = src/ae.c src/anet.c
OBJ = ${SRC:.c=.o}
CFLAGS = -Wno-parentheses -Wno-switch-enum -Wno-unused-value

libae.a: $(OBJ)
	$(AR) -rc $@ $(OBJ)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@ -g

timer: example/timer.o libae.a
	$(CC) $^ -o $@ -g

echo: example/echo.o libae.a
	$(CC) $^ -o $@ -g

clean:
	rm -f $(OBJ) libae.a example/timer.o timer example/echo.o echo

.PHONY: clean
