LDFLAGS = -lasound -lm -lrubberband
CFLAGS = -Wall -std=c++20 -O3 # gnu99 so that popen is defined

ENTRY = entry.o
BUT_ENTRY = finyl.o cJSON.o controller.o
OBJS = $(ENTRY) $(BUT_ENTRY)
TESTS = test test-digger test-controller
TEST_OBJS = $(addsuffix .o,$(TESTS))

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

%: %.o #%.o can be plural
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

all: finyl listdevice

finyl: $(OBJS)

listdevice: listdevice.o

tests: $(TESTS)
test: test.o $(BUT_ENTRY) # this is for interactive development. not for automated test. so test.c is gitignored
test-digger: test-digger.o $(BUT_ENTRY)
test-controller: test-controller.o $(BUT_ENTRY)

clean:
	rm -f finyl \
		listdevice \
		$(TESTS) \
		$(OBJS) \
		$(TEST_OBJS)
