/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS, a tool for make a LFS file system image
 *
 */

#include "lfs/lfs.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_READ_SIZE 16
#define DEFAULT_PROG_SIZE 16
#define DEFAULT_CACHE_SIZE 64
#define DEFAULT_LOOKAHEAD_SIZE 32
#define DEFAULT_BLOCK_SIZE 2048
#define DEFAULT_BLOCK_CYCLES 512

static struct lfs_config cfg;
static lfs_t lfs;
static uint8_t *data;

static int lfs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    memcpy(buffer, data + (block * c->block_size) + off, size);
    return 0;
}

static int lfs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    memcpy(data + (block * c->block_size) + off, buffer, size);
    return 0;
}

static int lfs_erase(const struct lfs_config *c, lfs_block_t block) {
    memset(data + (block * c->block_size), 0, c->block_size);
    return 0;
}

static int lfs_sync(const struct lfs_config *c) {
    return 0;
}

static void create_dir(char *src) {
    char *path;
    int ret;

    path = strchr(src, '/');
    if (path) {
        fprintf(stdout, "%s\r\n", path);

        if ((ret = lfs_mkdir(&lfs, path)) < 0) {
            fprintf(stderr, "can't create directory %s: error=%d\r\n", path, ret);
            exit(1);
        }
    }
}

static void create_file(char *src) {
    char *path;
    int ret;

    path = strchr(src, '/');
    if (path) {
        fprintf(stdout, "%s\r\n", path);

        // Open source file
        FILE *srcf = fopen(src, "rb");
        if (!srcf) {
            fprintf(stderr, "can't open source file %s: errno=%d (%s)\r\n", src, errno, strerror(errno));
            exit(1);
        }

        // Open destination file
        lfs_file_t dstf;
        if ((ret = lfs_file_open(&lfs, &dstf, path, LFS_O_WRONLY | LFS_O_CREAT)) < 0) {
            fprintf(stderr, "can't open destination file %s: error=%d\r\n", path, ret);
            exit(1);
        }

        char c = fgetc(srcf);
        while (!feof(srcf)) {
            ret = lfs_file_write(&lfs, &dstf, &c, 1);
            if (ret < 0) {
                fprintf(stderr, "can't write to destination file %s: error=%d\r\n", path, ret);
                exit(1);
            }
            c = fgetc(srcf);
        }

        // Close destination file
        ret = lfs_file_close(&lfs, &dstf);
        if (ret < 0) {
            fprintf(stderr, "can't close destination file %s: error=%d\r\n", path, ret);
            exit(1);
        }

        // Close source file
        fclose(srcf);
    }
}

static void compact(char *src) {
    DIR *dir;
    struct dirent *ent;
    struct stat s;
    char curr_path[PATH_MAX];

    dir = opendir(src);
    if (dir) {
        while ((ent = readdir(dir))) {
            // Skip . and .. directories
            if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0)) {
                // Update the current path
                strcpy(curr_path, src);
                strcat(curr_path, "/");
                strcat(curr_path, ent->d_name);

                stat(curr_path, &s);
                if (S_ISDIR(s.st_mode)) {
                    create_dir(curr_path);
                    compact(curr_path);
                } else if (S_ISREG(s.st_mode)) {
                    create_file(curr_path);
                }
            }
        }

        closedir(dir);
    }
}

static int is_number(const char *s) {
    const char *c = s;

    while (*c) {
        if ((*c < '0') || (*c > '9')) {
            return 0;
        }
        c++;
    }

    return 1;
}

static int is_hex(const char *s) {
    const char *c = s;

    if (*c++ != '0') {
        return 0;
    }

    if (*c++ != 'x') {
        return 0;
    }

    while (*c) {
        if (((*c < '0') || (*c > '9')) && ((*c < 'A') || (*c > 'F')) && ((*c < 'a') || (*c > 'f'))) {
            return 0;
        }
        c++;
    }

    return 1;
}

static int to_int(const char *s) {
    if (is_number(s)) {
        return atoi(s);
    } else if (is_hex(s)) {
        return (int)strtol(s, NULL, 16);
    }

    return -1;
}

static struct option long_options[] = {
    {"read-size", required_argument, NULL, 'r'},
    {"prog-size", required_argument, NULL, 'p'},
    {"block-size", required_argument, NULL, 'b'},
    {"cache-size", required_argument, NULL, 'c'},
    {"lookahead-size", required_argument, NULL, 'L'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {0, 0, NULL, 0},
};

static const char option_string[] = {
    "r:"
    "p:"
    "b:"
    "c:"
    "L:"
    "hv",
};

static void print_help(const char *prog) {
    printf("Usage: %s [options] DIRECTORY IMAGE SIZE\n", basename((char *)prog));
    printf("\n");
    printf("Create a littlefs IMAGE from DIRECTORY with SIZE allocated.\n");
    printf("\n");
    printf("Options:\n");
    printf("  -r, --read-size=SIZE          minimum size of a block read (default: %d)\n", DEFAULT_READ_SIZE);
    printf("  -p, --prog-size=SIZE          minimum size of a block program (default: %d)\n", DEFAULT_PROG_SIZE);
    printf("  -b, --block-size=SIZE         size of block in bytes (default: %d)\n", DEFAULT_BLOCK_SIZE);
    printf("  -c, --cache-size=SIZE         size of block caches in bytes (default: %d)\n", DEFAULT_CACHE_SIZE);
    printf("  -L, --lookahead-size=SIZE     size of lookahead buffer in bytes (default: %d)\n", DEFAULT_LOOKAHEAD_SIZE);
    printf("\n");
    printf("Other options:\n");
    printf("  -h, --help                    print this help\n");
    printf("  -v, --version                 print program version\n");
}

#ifndef GIT_TAG
#define GIT_TAG "HEAD"
#endif

#ifndef GIT_COMMIT
#define GIT_COMMIT "unknown"
#endif

static void print_version(void) {
    printf("%s (%s)\n", GIT_TAG, GIT_COMMIT);
    printf("LFS library : %d.%d\n", LFS_VERSION_MAJOR, LFS_VERSION_MINOR);
    printf("LFS data    : %d.%d\n", LFS_DISK_VERSION_MAJOR, LFS_DISK_VERSION_MINOR);
}

int main(int argc, char **argv) {
    char *src = NULL; // Source directory
    char *dst = NULL; // Destination image
    int read_size = DEFAULT_READ_SIZE;
    int prog_size = DEFAULT_PROG_SIZE;
    int block_size = DEFAULT_BLOCK_SIZE;
    int cache_size = DEFAULT_CACHE_SIZE;
    int lookahead_size = DEFAULT_LOOKAHEAD_SIZE;
    int fs_size = 0;
    int err;

    int long_index = -1;
    while (1) {
        int c = getopt_long(argc, argv, option_string, long_options, &long_index);
        if (c == EOF)
            break;
        switch (c) {
        case 'r':
            read_size = to_int(optarg);
            break;
        case 'p':
            prog_size = to_int(optarg);
            break;
        case 'b':
            block_size = to_int(optarg);
            break;
        case 'c':
            cache_size = to_int(optarg);
            break;
        case 'L':
            lookahead_size = to_int(optarg);
            break;
        case 's':
            fs_size = to_int(optarg);
            break;
        case 'v':
            print_version();
            return 0;
        case 'h':
        case '?':
            print_help(argv[0]);
            return 0;
        }
    }

    if (optind + 3 != argc) {
        print_help(argv[0]);
        return -1;
    }

    src = argv[optind];
    dst = argv[optind + 1];
    fs_size = to_int(argv[optind + 2]);

    if (read_size <= 0 || prog_size <= 0 || block_size <= 0 || cache_size <= 0 ||
        lookahead_size <= 0 || fs_size <= 0) {
        print_help(argv[0]);
        return -1;
    }

    printf("Making littlefs image...\n");
    printf("  source: %s\n", src);
    printf("  target: %s\n", dst);
    printf("  size  : %d\n", fs_size);
    printf("\n");

    // Mount the file system
    cfg.read = lfs_read;
    cfg.prog = lfs_prog;
    cfg.erase = lfs_erase;
    cfg.sync = lfs_sync;

    cfg.read_size = read_size;
    cfg.prog_size = prog_size;
    cfg.block_size = block_size;
    cfg.block_count = fs_size / cfg.block_size;
    cfg.block_cycles = -1; // disable leveling since we're making image locally
    cfg.cache_size = cache_size;
    cfg.lookahead_size = lookahead_size;
    cfg.context = NULL;

    data = calloc(1, fs_size);
    if (!data) {
        fprintf(stderr, "no memory for mount\r\n");
        return -1;
    }

    err = lfs_format(&lfs, &cfg);
    if (err < 0) {
        fprintf(stderr, "format error: error=%d\r\n", err);
        return -1;
    }

    err = lfs_mount(&lfs, &cfg);
    if (err < 0) {
        fprintf(stderr, "mount error: error=%d\r\n", err);
        return -1;
    }

    compact(src);

    FILE *img = fopen(dst, "wb+");

    if (!img) {
        fprintf(stderr, "can't create image file: errno=%d (%s)\r\n", errno, strerror(errno));
        return -1;
    }

    fwrite(data, 1, fs_size, img);

    fclose(img);

    return 0;
}
