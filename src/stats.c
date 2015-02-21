// Copyright (C) 2012 - Will Glozer.  All rights reserved.

#include <inttypes.h>
#include <stdlib.h>
#include <math.h>

#include "stats.h"
#include "zmalloc.h"

stats *stats_alloc(uint64_t max) {
    uint64_t limit = max + 1;
    stats *s = zcalloc(sizeof(stats) + sizeof(uint64_t) * limit);
    s->limit = limit;
    s->min   = UINT64_MAX;
    return s;
}

void stats_free(stats *stats) {
    zfree(stats);
}

int stats_record(stats *stats, uint64_t n) {
    if (n >= stats->limit) return 0;
    __sync_fetch_and_add(&stats->data[n], 1);
    __sync_fetch_and_add(&stats->count, 1);
    uint64_t min = stats->min;
    uint64_t max = stats->max;
    while (n < min) min = __sync_val_compare_and_swap(&stats->min, min, n);
    while (n > max) max = __sync_val_compare_and_swap(&stats->max, max, n);
    return 1;
}

void stats_correct(stats *stats, int64_t expected) {
    for (uint64_t n = expected * 2; n <= stats->max; n++) {
        uint64_t count = stats->data[n];
        int64_t m = (int64_t) n - expected;
        while (count && m > expected) {
            stats->data[m] += count;
            stats->count += count;
            m -= expected;
        }
    }
}

long double stats_mean(stats *stats) {
    if (stats->count == 0) return 0.0;

    uint64_t sum = 0;
    for (uint64_t i = stats->min; i <= stats->max; i++) {
        sum += stats->data[i] * i;
    }
    return sum / (long double) stats->count;
}

long double stats_stdev(stats *stats, long double mean) {
    long double sum = 0.0;
    if (stats->count < 2) return 0.0;
    for (uint64_t i = stats->min; i <= stats->max; i++) {
        if (stats->data[i]) {
            sum += powl(i - mean, 2) * stats->data[i];
        }
    }
    return sqrtl(sum / (stats->count - 1));
}

long double stats_within_stdev(stats *stats, long double mean, long double stdev, uint64_t n) {
    long double upper = mean + (stdev * n);
    long double lower = mean - (stdev * n);
    uint64_t sum = 0;

    for (uint64_t i = stats->min; i <= stats->max; i++) {
        if (i >= lower && i <= upper) {
            sum += stats->data[i];
        }
    }

    return (sum / (long double) stats->count) * 100;
}

uint64_t stats_percentile(stats *stats, long double p) {
    uint64_t rank = round((p / 100.0) * stats->count + 0.5);
    uint64_t total = 0;
    for (uint64_t i = stats->min; i <= stats->max; i++) {
        total += stats->data[i];
        if (total >= rank) return i;
    }
    return 0;
}

uint64_t stats_popcount(stats *stats) {
    uint64_t count = 0;
    for (uint64_t i = stats->min; i <= stats->max; i++) {
        if (stats->data[i]) count++;
    }
    return count;
}

uint64_t stats_value_at(stats *stats, uint64_t index, uint64_t *count) {
    *count = 0;
    for (uint64_t i = stats->min; i <= stats->max; i++) {
        if (stats->data[i] && (*count)++ == index) {
            *count = stats->data[i];
            return i;
        }
    }
    return 0;
}
