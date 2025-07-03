#include <X11/Xlib.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signalfd.h>
#include <time.h>
#include <unistd.h>
#define LENGTH(X) (sizeof(X) / sizeof(X[0]))
#define CMDLENGTH 50

typedef struct {
  char *icon;
  char *command;
  unsigned int interval;
  unsigned int signal;
} Block;
void sighandler();
void replace(char *str, char old, char new);
void remove_all(char *str, char to_remove);
void getcmds(int time);
void getsigcmds(int signal, int button);
void setupsignals();
int getstatus(char *str, char *last);
void setroot();
void statusloop();
void termhandler(int signum);

#include "config.h"

static Display *dpy;
static int screen;
static Window root;
static char statusbar[LENGTH(blocks)][CMDLENGTH] = {0};
static char statusstr[2][256];
static int statusContinue = 1;
static int signalFD;
static int timerInterval = -1;
static void (*writestatus)() = setroot;

void replace(char *str, char old, char new) {
  for (char *c = str; *c; c++)
    if (*c == old)
      *c = new;
}

// the previous function looked nice but unfortunately it didnt work if
// to_remove was in any position other than the last character theres probably
// still a better way of doing this
void remove_all(char *str, char to_remove) {
  char *read = str;
  char *write = str;
  while (*read) {
    if (*read != to_remove) {
      *write++ = *read;
    }
    ++read;
  }
  *write = '\0';
}

int gcd(int a, int b) {
  int temp;
  while (b > 0) {
    temp = a % b;

    a = b;
    b = temp;
  }
  return a;
}

void getcmdbuttoned(const Block *block, char *output, int button) {
  if (block->signal) {
    output[0] = block->signal;
    output++;
  }
  char *cmd = block->command;
  FILE *cmdf;

  if (button != 0) {
    char full_cmd[1024];
    snprintf(full_cmd, sizeof(full_cmd), "BLOCK_BUTTON=%d %s", button, cmd);
    cmdf = popen(full_cmd, "r");
  } else {
    cmdf = popen(cmd, "r");
  }

  if (!cmdf) {
    // printf("failed to run: %s, %d\n", block->command, errno);
    return;
  }
  char tmpstr[CMDLENGTH] = "";
  char *s;
  int e;
  do {
    errno = 0;
    s = fgets(tmpstr, CMDLENGTH - (strlen(delim) + 1), cmdf);
    e = errno;
  } while (!s && e == EINTR);
  pclose(cmdf);
  int i = strlen(block->icon);
  strcpy(output, block->icon);
  strcpy(output + i, tmpstr);
  remove_all(output, '\n');
  i = strlen(output);
  if ((i > 0 && block != &blocks[LENGTH(blocks) - 1])) {
    strcat(output, delim);
  }
  i += strlen(delim);
  output[i++] = '\0';
}

void getcmd(const Block *block, char *output) {
  getcmdbuttoned(block, output, 0);
}

void getcmds(int time) {
  const Block *current;
  for (int i = 0; i < LENGTH(blocks); i++) {
    current = blocks + i;
    if ((current->interval != 0 && time % current->interval == 0) ||
        time == -1) {
      getcmd(current, statusbar[i]);
    }
  }
}

void getsigcmds(int signal, int button) {
  pid_t process_id = getpid();
  const Block *current;
  int i = 0;
  for (; i < LENGTH(blocks); i++) {
    current = blocks + i;
    if (current->signal == signal)
      break;
  }

  getcmdbuttoned(current, statusbar[i], button);
}

void setupsignals() {
  sigset_t signals;
  sigemptyset(&signals);
  sigaddset(&signals, SIGALRM); // Timer events
  sigaddset(&signals, SIGUSR1); // Button events
  // All signals assigned to blocks
  for (size_t i = 0; i < LENGTH(blocks); i++)
    if (blocks[i].signal > 0)
      sigaddset(&signals, SIGRTMIN + blocks[i].signal);
  // Create signal file descriptor for pooling
  signalFD = signalfd(-1, &signals, 0);
  // Block all real-time signals
  for (int i = SIGRTMIN; i <= SIGRTMAX; i++)
    sigaddset(&signals, i);
  sigprocmask(SIG_BLOCK, &signals, NULL);
  // Do not transform children into zombies
  struct sigaction sigchld_action = {.sa_handler = SIG_DFL,
                                     .sa_flags = SA_NOCLDWAIT};
  sigaction(SIGCHLD, &sigchld_action, NULL);
}

int getstatus(char *str, char *prev) {
  strcpy(prev, str);
  str[0] = '\0';
  for (int i = 0; i < LENGTH(blocks); i++) {
    char block[50];
    strcpy(block, statusbar[i]);
    strcat(str, block);
    if (i == LENGTH(blocks) - 1)
      strcat(str, " ");
  }
  str[strlen(str) - 1] = '\0';
  return strcmp(str, prev); // 0 if they are the same
}

void setroot() {
  if (!getstatus(statusstr[0],
                 statusstr[1])) // Only set root if text has changed.
    return;
  Display *d = XOpenDisplay(NULL);
  if (d) {
    dpy = d;
  }
  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);
  XStoreName(dpy, root, statusstr[0]);
  XCloseDisplay(dpy);
}

void statusloop() {
  setupsignals();
  // first figure out the default wait interval by finding the
  // greatest common denominator of the intervals
  for (int i = 0; i < LENGTH(blocks); i++) {
    if (blocks[i].interval) {
      timerInterval = gcd(blocks[i].interval, timerInterval);
    }
  }
  getcmds(-1);    // Fist time run all commands
  raise(SIGALRM); // Schedule first timer event
  int ret;
  struct pollfd pfd[] = {{.fd = signalFD, .events = POLLIN}};
  while (statusContinue) {
    // Wait for new signal
    ret = poll(pfd, sizeof(pfd) / sizeof(pfd[0]), -1);
    if (ret < 0 || !(pfd[0].revents & POLLIN))
      break;
    sighandler(); // Handle signal
  }
}

void sighandler() {
  static int time = 0;
  struct signalfd_siginfo si;
  int ret = read(signalFD, &si, sizeof(si));
  if (ret < 0)
    return;
  int signal = si.ssi_signo;
  if (signal == SIGALRM) {
    getcmds(time);
    alarm(timerInterval);
    time += timerInterval;
  } else {
    if (si.ssi_int == 7) {
      printf("yaya\n");
      system(
          "cd ~/.local/bin/dwmblocks-modules/ && cp template sb-CHANGEME && code ./");
    }
    getsigcmds(signal - SIGRTMIN, si.ssi_int);
  }
  writestatus();
}

void termhandler(int signum) { statusContinue = 0; }

int main() {
  signal(SIGTERM, termhandler);
  signal(SIGINT, termhandler);
  statusloop();
  close(signalFD);
}
