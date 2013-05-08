// Copyright (C) 2012 - Will Glozer.  All rights reserved.

#include <inttypes.h>
#include <stdlib.h>
#include <math.h>

#include "stats.h"
#include "zmalloc.h"

stats *stats_alloc(uint64_t samples) {
    stats *stats = zcalloc(sizeof(stats) + sizeof(uint64_t) * samples);
    stats->samples = samples;
    return stats;
}

void stats_free(stats *stats) {
    zfree(stats);
}

void stats_record(stats *stats, uint64_t x) {
    stats->data[stats->index++] = x;
    if (stats->limit < stats->samples)  stats->limit++;
    if (stats->index == stats->samples) stats->index = 0;
}

static int stats_compare(const void *a, const void *b) {
    uint64_t *x = (uint64_t *) a;
    uint64_t *y = (uint64_t *) b;
    return *x - *y;
}

long double stats_summarize(stats *stats, int64_t *min, uint64_t *max) {
    qsort(stats->data, stats->limit, sizeof(uint64_t), &stats_compare);

    if (min) *min = stats->data[0];
    if (max) *max = stats->data[stats->limit - 1];

    if (stats->limit == 0) return 0.0;

    uint64_t sum = 0;
    for (uint64_t i = 0; i < stats->limit; i++) {
        sum += stats->data[i];
    }
    return sum / (long double) stats->limit;
}

long double stats_stdev(stats *stats, long double mean) {
    long double sum = 0.0;
    if (stats->limit < 2) return 0.0;
    for (uint64_t i = 0; i < stats->limit; i++) {
        sum += powl(stats->data[i] - mean, 2);
    }
    return sqrtl(sum / (stats->limit - 1));
}

long double stats_within_stdev(stats *stats, long double mean, long double stdev, uint64_t n) {
    long double upper = mean + (stdev * n);
    long double lower = mean - (stdev * n);
    uint64_t sum = 0;

    for (uint64_t i = 0; i < stats->limit; i++) {
        uint64_t x = stats->data[i];
        if (x >= lower && x <= upper) sum++;
    }

    return (sum / (long double) stats->limit) * 100;
}

uint64_t stats_percentile(stats *stats, long double p) {
    uint64_t rank = round((p / 100.0) * stats->limit + 0.5);
    return stats->data[rank - 1];
}
