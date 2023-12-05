src_files = src/dns_server.c src/parse_config.c
out_bin = dns_server
CFLAGS = -Isrc/include -linih -lldns


debug: $(src_files)
	gcc -o $(out_bin) $(src_files) -g -O0  $(CFLAGS)

all: $(src_files)
	gcc -o $(out_bin) $(src_files) $(CFLAGS)

clean:
	rm $(out_bin)