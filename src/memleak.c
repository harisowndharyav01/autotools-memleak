/*
 * memleak.c - small demo program that intentionally leaks memory for testing.
 *
 * Build: use autotools (see README) or compile directly:
 *   gcc -std=c11 -O2 -o memleak memleak.c
 *
 * WARNING: This program intentionally leaks memory. Do NOT run on production gateways.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

static volatile sig_atomic_t keep_running = 1;
static void on_sigint(int sig) { (void)sig; keep_running = 0; }

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [OPTIONS]\n"
        "Options:\n"
        "  --mode=MODE           leak|leak-slab|transient|steady (default: leak)\n"
        "  --size=BYTES          bytes per allocation (default: 65536)\n"
        "  --interval=MS         interval between allocations in milliseconds (default: 1000)\n        --count=N            number of allocations (0 = unlimited) (default: 0)\n"
        "  --report=N            print report every N seconds (default: 10)\n"
        "  --no-daemon           run in foreground (default: foreground)\n"
        "  -h, --help            show this help\n",
        prog);
}

static long now_seconds(void) {
    return time(NULL);
}

int main(int argc, char **argv) {
    const char *mode = "leak";
    size_t size = 64 * 1024; /* 64 KB */
    unsigned int interval_ms = 1000;
    unsigned long count = 0; /* 0 = unlimited */
    unsigned int report_interval = 10;
    int foreground = 1;

    static struct option longopts[] = {
        {"mode", required_argument, 0, 0},
        {"size", required_argument, 0, 0},
        {"interval", required_argument, 0, 0},
        {"count", required_argument, 0, 0},
        {"report", required_argument, 0, 0},
        {"no-daemon", no_argument, 0, 0},
        {"help", no_argument, 0, 'h'},
        {0,0,0,0}
    };

    int optindex=0;
    while (1) {
        int c = getopt_long(argc, argv, "h", longopts, &optindex);
        if (c == -1) break;
        if (c == 'h') { usage(argv[0]); return 0; }
        if (c == 0) {
            const char *name = longopts[optindex].name;
            if (strcmp(name,"mode")==0) mode = optarg;
            else if (strcmp(name,"size")==0) size = (size_t) strtoul(optarg, NULL, 10);
            else if (strcmp(name,"interval")==0) interval_ms = (unsigned int) atoi(optarg);
            else if (strcmp(name,"count")==0) count = strtoul(optarg, NULL, 10);
            else if (strcmp(name,"report")==0) report_interval = (unsigned int) atoi(optarg);
            else if (strcmp(name,"no-daemon")==0) foreground = 1;
        }
    }

    signal(SIGINT, on_sigint);
    signal(SIGTERM, on_sigint);

    fprintf(stderr, "memleak: mode=%s size=%zu interval=%ums count=%lu report=%u\n",
            mode, size, interval_ms, count, report_interval);

    /* simple reporting */
    unsigned long allocations = 0;
    unsigned long leaked_bytes = 0;
    long last_report = now_seconds();

    /* For leak-slab mode we keep pointers in an expanding array */
    void **slab = NULL;
    size_t slab_capacity = 0;

    while (keep_running) {
        if (count && allocations >= count) {
            fprintf(stderr, "memleak: reached requested count %lu, exiting\n", count);
            break;
        }

        void *p = malloc(size);
        if (!p) {
            fprintf(stderr, "memleak: malloc failed at allocation %lu: %s\n", allocations+1, strerror(errno));
            /* if malloc fails we pause and try again; could be OOM on device */
            sleep(1);
            continue;
        }

        /* touch memory to ensure allocation committed (avoid lazy allocation) */
        memset(p, 0x41, size);

        if (strcmp(mode,"leak")==0) {
            /* intentionally never free */
            /* keep no pointer reference so it becomes irretrievable (true leak) */
        } else if (strcmp(mode,"leak-slab")==0) {
            /* store pointer so pointer remains reachable and memory persists */
            if (allocations + 1 > slab_capacity) {
                size_t newcap = (slab_capacity == 0) ? 1024 : slab_capacity * 2;
                void **tmp = realloc(slab, newcap * sizeof(void*));
                if (!tmp) {
                    fprintf(stderr, "memleak: slab realloc failed\n");
                    free(p);
                    break;
                }
                slab = tmp;
                slab_capacity = newcap;
            }
            slab[allocations] = p;
        } else if (strcmp(mode,"transient")==0) {
            /* free immediately (no leak) */
            free(p);
        } else if (strcmp(mode,"steady")==0) {
            /* allocate and free but also sleep; minimal memory growth */
            free(p);
        } else {
            /* unknown mode */
            fprintf(stderr,"memleak: unknown mode '%s'\n", mode);
            free(p);
            break;
        }

        allocations++;
        leaked_bytes += (strcmp(mode,"transient")==0 || strcmp(mode,"steady")==0) ? 0 : size;

        /* periodic report */
        long now = now_seconds();
        if ((now - last_report) >= (long)report_interval) {
            fprintf(stderr, "memleak: time=%ld allocations=%lu leaked_bytes=%lu (approx)\n",
                    now, allocations, leaked_bytes);
            last_report = now;
        }

        /* sleep interval_ms */
        if (interval_ms >= 1000) {
            sleep(interval_ms / 1000);
            if (interval_ms % 1000) usleep((interval_ms % 1000) * 1000);
        } else {
            usleep(interval_ms * 1000);
        }
    }

    /* cleanup: in slab mode free stored pointers so OS reclaims if shutting down gracefully */
    if (slab) {
        for (size_t i=0; i < slab_capacity && i < allocations; ++i) {
            free(slab[i]);
        }
        free(slab);
    }

    fprintf(stderr, "memleak: exiting. allocations=%lu estimated_leaked_bytes=%lu\n",
            allocations, leaked_bytes);
    return 0;
}
