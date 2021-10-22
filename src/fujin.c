#include "fujin.h"

void get_time_log(char *timestr, uint32_t length) {
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);

  strftime(timestr, length, "%c", tm);
}

void log_message(uint32_t severity, const char *message) {
    char timestr[128];
    get_time_log(timestr, 128);
    FILE *__restrict outfile = severity > 5 ? stderr : stdout;
    fprintf(outfile, "[%s] %2u: ", timestr, severity);
    fputs(message, outfile);
    fputc('\n', outfile);
}

void log_format(uint32_t severity, char *format, ...) {
    char timestr[128];
    get_time_log(timestr, 128);
    FILE *__restrict outfile = severity > 5 ? stderr : stdout;

    fprintf(outfile, "[%s] %2u: ", timestr, severity);

    va_list vars;
    va_start(vars, format);
    vfprintf(outfile, format, vars); 
    va_end(vars);

    fputc('\n', outfile);
}
