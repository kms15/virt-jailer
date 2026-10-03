.PHONY: test clean dist-clean

BUILD=./build
SRC=./src

CFLAGS=-fpie -fstack-protector-strong -O2 -D_FORTIFY_SOURCE=3
LDFLAGS=--pie -static -Wl,-z,relro,-z,now

$(BUILD)/%.o: $(SRC)/%.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

all: test

clean:
	rm -f $(BUILD)/*

test: $(BUILD)/virt-jailer-core
	tests/run-all-tests
