CFLAGS = -Wall -std=c99 -O3
CPPFLAGS = -Wall -Wno-unused-result -Wno-write-strings -Wno-sign-compare -Wno-deprecated-declarations -std=c++11 -O3
LDFLAGS = -lasound -lm -lSDL2 -lX11 -lcrypto -lstdc++

ENTRY = entry.o
BUT_ENTRY = finyl.o cJSON.o controller.o digger.o dev.o interface.o util.o
OBJS = $(ENTRY) $(BUT_ENTRY)
TESTS = test-digger test-controller test-interface
TEST_OBJS = $(addsuffix .o,$(TESTS))

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@ $(LDFLAGS)

%: %.o #%.o can be plural
	$(CC) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)

all: finyl

finyl: $(OBJS)

listdevice: listdevice.o
separate: $(BUT_ENTRY) separate.o

tests: $(TESTS)
test-digger: test-digger.o $(BUT_ENTRY)
test-controller: test-controller.o $(BUT_ENTRY)
test-interface: test-interface.o $(BUT_ENTRY)


clean:
	rm -f finyl \
		listdevice listdevice.o \
		separate separate.o\
		$(TESTS) \
		$(OBJS) \
		$(TEST_OBJS)
