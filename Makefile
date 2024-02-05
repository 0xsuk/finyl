CFLAGS = -Wall -std=c99 -O3
CPPFLAGS = -Wall -Wno-unused-result -Wno-write-strings -Wno-sign-compare -Wno-deprecated-declarations -std=c++20 -O3
LDFLAGS = -lasound -lm -lSDL2 -lX11 -lcrypto -lstdc++

ENTRY = entry.o
BUT_ENTRY = finyl.o controller.o dev.o interface.o util.o action.o json.o rekordbox.o  kaitaistream.o rekordbox_pdb.o rekordbox_anlz.o
OBJS = $(ENTRY) $(BUT_ENTRY)
TESTS = test-controller test-interface test-json
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
test-controller: test-controller.o $(BUT_ENTRY)
test-interface: test-interface.o $(BUT_ENTRY)
test-json: test-json.o json.o
test-rekordbox: test-rekordbox.o $(BUT_ENTRY)
test: test.o $(BUT_ENTRY)

clean:
	rm -f finyl \
		listdevice listdevice.o \
		separate separate.o\
		$(TESTS) \
		$(OBJS) \
		$(TEST_OBJS)
