LDFLAGS = -lasound -lm
CFLAGS = -Wall -std=gnu99 # gnu99 so that popen is defined

ENTRY = finyl.o
BUT_ENTRY = audio.o
OBJS = $(ENTRY) $(BUT_ENTRY)
TESTS = test-digger
TEST_OBJS = $(addsuffix .o,$(TESTS))

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

%: %.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

all: finyl

finyl: $(OBJS)

listdevice: listdevice.o

test: $(TESTS)
test-digger: test-digger.o audio.o

clean:
	rm -f finyl \
		listdevice \
		$(TESTS)
		$(ENTRY) \
		$(TEST_OBJS)
