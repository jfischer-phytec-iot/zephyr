CC= gcc
CFLAGS= -Wall -O3

all: hid-issue

hid-issue: hid-issue.c
	$(CC) $(CFLAGS) $(LIBS) -o $@ $<

clean:
	rm -f hid-issue *~
