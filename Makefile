CPPFLAGS = -Wall -Wno-unused-result -Wno-write-strings -Wno-sign-compare -Wno-deprecated-declarations -std=c++20 -O3 -Iexternal/rubberband/rubberband -Iexternal/fidlib -DT_LINUX
LDFLAGS = -lasound -lm -lSDL2 -lX11 -lcrypto -lstdc++

ENTRY = entry.o
BUT_ENTRY = finyl.o midi.o controller.o dev.o interface.o util.o action.o rekordbox.o kaitaistream.o rekordbox_pdb.o rekordbox_anlz.o dsp.o external/rubberband/single/RubberBandSingle.o external/fidlib/fidlib.o
OBJS = $(ENTRY) $(BUT_ENTRY)
TEST_OBJS = $(addsuffix .o,$(TESTS))

%.o: %.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@ $(LDFLAGS)

%: %.o #%.o can be plural
	$(CC) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)

all: finyl

finyl: $(OBJS)

listdevice: listdevice.o
separate: $(BUT_ENTRY) separate.o

test-interface: test-interface.o $(BUT_ENTRY)
test-rekordbox: test-rekordbox.o $(BUT_ENTRY)
test-dsp: test-dsp.o $(BUT_ENTRY)
test-midi: test-midi.o $(BUT_ENTRY)
test: test.o $(BUT_ENTRY)

clean:
	rm -f finyl \
		listdevice listdevice.o \
		separate separate.o\
		$(TESTS) \
		$(OBJS) \
		$(TEST_OBJS)
