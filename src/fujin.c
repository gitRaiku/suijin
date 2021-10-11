#include "fujin.h"

void cleanup(int32_t severity) {
  log_format(severity, "Exiting with exit code %i", severity);

  glfwTerminate();

  log_message(severity, "GLFW_TERMINATE");

  exit(severity);
}

void sig_handler(int signum) {
  log_format(signum, "Caught signal %s (%s); Aborting!", signum, sigabbrev_np(signum), sigdescr_np(signum));
  cleanup(signum);
}

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
