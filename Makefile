.PHONY: test clean dist-clean

BUILD=./build
SRC=./src

CFLAGS=-fpie -fstack-protector-strong -O2 -D_FORTIFY_SOURCE=3 \
	   -Wformat -Werror=format-security -Werror=format-extra-args
LDFLAGS=--pie -static -Wl,-z,relro,-z,now

$(BUILD)/%.o: $(SRC)/%.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

$(BUILD)/test/%.o: tests/test-namespaces/%.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

$(BUILD)/test/%.o: tests/test-fd-passing/%.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

all: test

clean:
	rm -f $(BUILD)/*

test: \
		$(BUILD)/virt-jailer-core \
		$(BUILD)/get-namespace-ids \
		./tests/run-all-tests \
		./tests/test-basic-wrapper/run-all-tests \
		./tests/test-namespaces/run-all-tests \
		./tests/test-fd-passing/run-all-tests \
		$(BUILD)/test/test-i-am-pid-1 \
		$(BUILD)/test/list-open-fds \
		#
	tests/run-all-tests
