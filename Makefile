CFLAGS += -static
shell: shell.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) 
clean:
	rm -f shell fake *.o
.PHONY: clean
