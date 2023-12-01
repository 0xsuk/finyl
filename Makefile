LDFLAGS = -lasound -lm
CFLAGS = -Wall -std=gnu99 # gnu99 so that popen is defined

ENTRY = entry.o
BUT_ENTRY = finyl.o cJSON.o
OBJS = $(ENTRY) $(BUT_ENTRY)
TESTS = test
TEST_OBJS = $(addsuffix .o,$(TESTS))

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

%: %.o #%.o can be plural
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

all: finyl listdevice

finyl: $(OBJS)

listdevice: listdevice.o

tests: $(TESTS)
test: test.o $(BUT_ENTRY) # this is for interactive development. not for automated test. so test.c is gitignored

clean:
	rm -f finyl \
		listdevice \
		$(TESTS) \
		$(OBJS) \
		$(TEST_OBJS)
