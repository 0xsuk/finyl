CFLAGS = -Wall -Wno-unused-result -std=gnu99 -O3 # gnu99 so that popen is defined
LDFLAGS = -lasound -lm -lSDL2 -lX11 -lcrypto

ENTRY = entry.o
BUT_ENTRY = finyl.o cJSON.o controller.o digger.o dev.o interface.o util.o
OBJS = $(ENTRY) $(BUT_ENTRY)
TESTS = test-digger test-controller test-interface
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
separate: $(BUT_ENTRY) separate.o

tests: $(TESTS)
test-digger: test-digger.o $(BUT_ENTRY)
test-controller: test-controller.o $(BUT_ENTRY)
test-interface: test-interface.o $(BUT_ENTRY)


clean:
	rm -f finyl \
		listdevice \
		separate \
		$(TESTS) \
		$(OBJS) \
		$(TEST_OBJS)
