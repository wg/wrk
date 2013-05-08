#ifndef STATS_H
#define STATS_H

typedef struct {
    uint64_t samples;
    uint64_t index;
    uint64_t limit;
    uint64_t data[];
} stats;

stats *stats_alloc(uint64_t);
void stats_free(stats *);
void stats_record(stats *, uint64_t);
long double stats_summarize(stats *, int64_t *, uint64_t *);
long double stats_stdev(stats *stats, long double);
long double stats_within_stdev(stats *, long double, long double, uint64_t);
uint64_t stats_percentile(stats *, long double);

#endif /* STATS_H */

