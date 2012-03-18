#ifndef __UNITS_H
#define __UNITS_H

char *format_binary(long double);
char *format_metric(long double);
char *format_time_us(long double);

int scan_metric(char *, uint64_t *);

#endif /* __UNITS_H */

