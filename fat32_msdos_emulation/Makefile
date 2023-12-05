src_files = main.c fatfs/source/*.c
out_bin = fat32_msdos_emulation
CFLAGS = -Ifatfs/source


all: $(src_files)
	gcc -o $(out_bin) $(src_files) -g -O0 $(CFLAGS)
	
debug: $(src_files)
	gcc -o $(out_bin)_coverage $(src_files) -fprofile-arcs -ftest-coverage -fPIC -g -O0  $(CFLAGS)

clean:
	rm $(out_bin)