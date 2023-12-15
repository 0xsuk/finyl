LDFLAGS = -lasound -lm
CFLAGS = -Wall -std=gnu99 -O3 # gnu99 so that popen is defined

ENTRY = entry.o
BUT_ENTRY = finyl.o cJSON.o controller.o digger.o dev.o
OBJS = $(ENTRY) $(BUT_ENTRY)
TESTS = test-digger test-controller
TEST_OBJS = $(addsuffix .o,$(TESTS))

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

%: %.o #%.o can be plural
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

all: finyl listdevice

shared:
	$(CC) $(CFLAGS) -c finyl.c -fPIC -o finyl.o $(LDFLAGS)
	$(CC) $(CFLAGS) -shared $^ -o finyl.so $(LDFLAGS)

finyl: $(OBJS)

listdevice: listdevice.o

tests: $(TESTS)
test-digger: test-digger.o $(BUT_ENTRY)
test-controller: test-controller.o $(BUT_ENTRY)

clean:
	rm -f finyl \
		listdevice \
		$(TESTS) \
		$(OBJS) \
		$(TEST_OBJS)
