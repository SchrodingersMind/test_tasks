#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "ff.h"

#define DISK_FILE "disk.iso"

void 
menu() {
    printf("~~~  Available commands  ~~~\n");
    printf("1) help - print this menu\n");
    printf("2) format - reformat disk (destroy all information)\n");
    printf("3) ls - show list of files and directories\n");
    printf("4) cd <path> - change directory\n");
    printf("5) mkdir <name> - create directory\n");
    printf("6) touch <name> - create file\n");
    // Additional commands
    printf("7) echo <content> <name> - write content to file\n");
    printf("8) cat <name> - read content of the file\n");
    printf("\n\n");
}


bool
safe_open(FIL *fsrc, const char* path, char mode) {
    FRESULT fr;
    fr = f_open(fsrc, path, mode);
    if (fr) {
        printf("Error opening file %s\n", path);
        return false;
    }
    return true;
}

bool
safe_close(FIL *fsrc) {
    FRESULT fr;
    fr = f_close(fsrc);
    if (fr) {
        printf("Error closing file\n");
        return 0;
    }
    return true;
}

bool 
safe_write(FIL *fsrc, char *buf, int len) {
    int written;
    FRESULT fr;
    fr = f_write(fsrc, buf, len, &written);           /* Write it to the destination file */
    if (fr) {
        printf("Error writing file\n");
        return false;
    }
    return true;
}

bool 
safe_read(FIL *fsrc, char* buf, int len) {
    int read;
    FRESULT fr;
    fr = f_read(fsrc, buf, len, &read);
    if (read == 0) {
        printf("Unable to read file\n");
        return false;
    }
    return true;
}

bool
safe_chdir(const char* path) {
    FRESULT fr;
    fr = f_chdir(path);
    if (fr) {
        printf("Unable to change directory\n");
        return false;
    }
    return true;
}

bool
safe_mkdir(const char *path) {
    FRESULT fr;
    fr = f_mkdir(path);
    if (fr) {
        printf("Error creating a directory\n");
        return false;
    }
    return true;
}

bool 
safe_ls(const char *path)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    int nfile, ndir;


    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        nfile = ndir = 0;
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Error or end of dir */
            if (fno.fattrib & AM_DIR) {            /* Directory */
                printf("   <DIR>   %s\n", fno.fname);
                ndir++;
            } else {                               /* File */
                printf("%10lu %s\n", fno.fsize, fno.fname);
                nfile++;
            }
        }
        f_closedir(&dir);
        printf("%d dirs, %d files.\n", ndir, nfile);
    } else {
        printf("Failed to open \"%s\". (%u)\n", path, res);
    }
    return !res;
}


void 
loop(FATFS *fs0) {
    char command[4096], tmp_buf[1024];
    char first_arg[1024], second_arg[1024];
    char *cmd;
    FIL fsrc;
    menu();
    for (;;) {
        printf("> ");fflush(stdout);
        scanf("%s", command);
        cmd = command; //strtok(line, " ");
        // first_arg = strtok(NULL, " ");
        // second_arg = strtok(NULL, " ");
        if (!strcasecmp(cmd, "help")) {
            menu();
            continue;
        } else if (!strcasecmp(cmd, "format")) {
            if (f_format()) {
                printf("Unable to format drive\n");
            }
        } else if (!strcasecmp(cmd, "ls")) {
            safe_ls(".");
        } else if (!strcasecmp(cmd, "cd")) {
            scanf("%s", first_arg);
            safe_chdir(first_arg);
        } else if (!strcasecmp(cmd, "mkdir")) {
            scanf("%s", first_arg);
            printf("Creating directory: %s\n", first_arg);
            safe_mkdir(first_arg);
        } else if (!strcasecmp(cmd, "touch")) {
            scanf("%s", first_arg);
            if (safe_open(&fsrc, first_arg, FA_WRITE|FA_CREATE_ALWAYS)) {
                safe_close(&fsrc);
            }
        } else if (!strcasecmp(cmd, "echo")) {
            scanf("%s %s", first_arg, second_arg);
            if (safe_open(&fsrc, second_arg, FA_WRITE|FA_CREATE_ALWAYS) &&
                    safe_write(&fsrc, first_arg, strlen(first_arg))) {
                safe_close(&fsrc);
            }
        } else if (!strcasecmp(cmd, "cat")) {
            scanf("%s", first_arg);
            if (safe_open(&fsrc, first_arg, FA_READ) &&
                    safe_read(&fsrc, tmp_buf, sizeof(tmp_buf)-1)) {
                printf("Content: %s\n", tmp_buf);
                safe_close(&fsrc);
            }
        } else {
            printf("!!! Unknown command: %s\n", cmd);
        }

    } 
}

int 
main(int argc, char** argv) {
    FATFS fs0;   
    FIL fsrc, fdst;   
    BYTE buffer[4096];
    FRESULT fr;
    UINT br, bw;
    char* disk_file;
    if (argc >= 2) {
        disk_file = argv[1];
    } else {
        disk_file = DISK_FILE;
    }

    fr = f_mount(&fs0, disk_file, 1);
    if (fr) {
        printf("Unknown disk format\n");
        return (int)fr;
    }
    printf("Disk mounted\n\n\n");

    loop(&fs0);
    return 0;
}