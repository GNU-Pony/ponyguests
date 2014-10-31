/**
 * ponyguests — Login wrapper to enable guest accounts
 * 
 * Copyright © 2014  Mattias Andrée (maandree@member.fsf.org)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>


#ifndef PROCDIR
#define PROCDIR  "/proc"
#endif

#ifndef SYSCONFDIR
#define SYSCONFDIR  "/etc"
#endif

#ifndef PKGNAME
#define PKGNAME  "ponyguests"
#endif


#define SCRIPTDIR  SYSCONFDIR "/" PKGNAME

/**
 * The guest should not be able to kill this program with the keyboard,
 * nor should the login process kill it when it performs a virtual hangup.
 */
#ifdef DEBUG
# define LIST_SOME_DEADLY_SIGNALS  X(SIGHUP)
#else
# define LIST_SOME_DEADLY_SIGNALS  X(SIGABRT) X(SIGHUP) X(SIGINT) X(SIGQUIT) X(SIGTERM)
#endif


static pid_t do_login(char** args)
{
  pid_t pid = fork();
  if (pid == -1)
    return perror("fork"), -1;
  
  if (pid == 0)
    {
      prctl(PR_SET_CHILD_SUBREAPER, 0); /* just to be sure. */
      /* Set to session leader. */
      if (setsid() == -1)
	return perror("setsid"), exit(1), -1;
      /* We are not longer in the login wrapper, we may die of signals now. */
#define X(S)  if (signal(S, SIG_DFL) == SIG_ERR)  return perror("signal"), exit(1), -1;
      LIST_SOME_DEADLY_SIGNALS
#undef X
      /* Spawn the login process, or guest account management script. */
      execvp(*args, args);
      perror("execvp");
      exit(1);
      return -1;
    }
  
  return pid;
}

/**
 * Create or delete guest account
 * 
 * @parma   path      The pathname of the script to run
 * @param   username  The name of the guest account
 * @return            Zero on success, -1 on error
 */
static int do_guest(char* path, char* username)
{
  char* args[3];
  pid_t pid, reaped;
  int status;
  
  args[0] = path;
  args[1] = username;
  args[2] = NULL;
  
  /* Call the script. */
  pid = do_login(args);
  if (pid == -1)
    return -1;
  
  /* Wait for the script to exit. */
  for (;;)
    {
      reaped = waitpid(-1, &status, 0);
      if (reaped == -1)
	{
	  if (errno != EINTR)
	    return perror("waitpid"), -1;
	}
      else if (reaped == pid)
	return status ? -1 : 0;
    }
}

/**
 * Create guest account
 * 
 * @param   username  The name of the guest account
 * @return            Zero on success, -1 on error
 */
static int do_start(char* username)
{
  static char path[] = SCRIPTDIR "/ponyguests-make-guest";
  return do_guest(path, username);
}

/**
 * Delete guest account
 * 
 * @param   username  The name of the guest account
 * @return            Zero on success, -1 on error
 */
static int do_exit(char* username)
{
  static char path[] = SCRIPTDIR "/ponyguests-delete-guest";
  return do_guest(path, username);
}


static int is_owned_by_uid(const char* restrict pid, const char* restrict uid)
{
  char path[1 << 10];
  char content[4 << 10];
  ssize_t got;
  size_t ptr = 0;
  int fd;
  char* p;
  char* q;
  
  sprintf(path, "%s/%s/status", PROCDIR, pid);
  if (fd = open(path, O_RDONLY), fd < 0)  return 0;
  
  content[ptr++] = '\n';
  for (;;)
    {
      got = read(fd, content + ptr, sizeof(content) - ptr * sizeof(char));
      if (got <= 0)
	break;
      ptr += (size_t)got;
    }
  close(fd);
  
  /* Check real user. */
  if (p = strstr(content, "\nUid:"), p == NULL)  return 0;
  p += strlen("\nUid:");
  while (*p && ((*p == ' ') || (*p == '\t')))    p++;
  if (*p == '\0')                                return 0;
  if (strstr(p, uid) != p)                       return 0;
  p += strlen(uid);
  if ((*p != ' ') && (*p != '\t'))               return 0;
  
  /* Lets pretend zombies are not real. */
  if (p = strstr(content, "\nState:"), p == NULL)  return 0;
  p += strlen("\nState:");
  if (q = strchr(p, '\n'), q == NULL)              return 0;
  while (*p && ((*p == ' ') || (*p == '\t')))      p++;
  if ((*p == '\0') || (*p == 'Z'))                 return 0;
  
  return 1;
}


static void ukillall(const char* restrict username, int signo)
{
  char uid[2 + 3 * sizeof(uid_t)];
  struct passwd* pwd;
  DIR* dir;
  struct dirent* file;
  int killed = 1;
  uid_t uid_;
  
  if (pwd = getpwnam(username), pwd == NULL)  return;
  if (dir = opendir(PROCDIR),   dir == NULL)  return;
  
  uid_ = pwd->pw_uid;
  sprintf(uid, "%ji", (intmax_t)uid_);
  
  while (killed)
    for (killed = 0; (file = readdir(dir)) != NULL;)
      {
	char* filename = file->d_name;
	pid_t pid;
	if ((filename == NULL) || (*filename == '\0'))
	  continue;
	for (; *filename; filename++)
	  if ((*filename < '0') || ('9' < *filename))
	    goto next;
	if (pid = (pid_t)atoll(file->d_name), pid <= 1)  continue;
	if (is_owned_by_uid(file->d_name, uid) == 0)     continue;
	killed = 1;
	kill(pid, signo);
      next:;
      }
}


int main(int argc, char** argv)
{
  char* username;
  char** args;
  int i, status;
  pid_t login_pid, reaped;
  
  /* Validate command line. */
  if (argc < 3)
    {
      printf("USAGE: %s <login-program> <arguments-for-login...>\n"
	     "\n"
	     "The last argument must be the username.\n", *argv);
      return 1;
    }
  
  /* Parse command line. */
  username = argv[argc - 1];
  args = alloca((size_t)argc * sizeof(char*));
  for (i = 1; i < argc; i++)
    args[i - 1] = argv[i];
  args[argc - 1] = NULL;
  
  /* Create guest account. */
  if (do_start(username) < 0)
    return 1;
  
  /* Do not die when the login process performs a virtual hangup,
   * nor should the guest be able to kill this process. */
#define X(S)  if (signal(S, SIG_IGN) == SIG_ERR)  return perror("signal"), do_exit(username), -1;
  LIST_SOME_DEADLY_SIGNALS
#undef X
  
  /* Stop the guest's processes form being reparent to lower subreaper (like PID 1).
   * This is do so that we do not accidentally fill other user's process because of
   * race condition between checking the process's owner and killing it. The process
   * get reaped and the PID reused between to two events. */
  if (prctl(PR_SET_CHILD_SUBREAPER, 1) == -1)
    return perror("prctl PR_SET_CHILD_SUBREAPER"), do_exit(username), 1;
  
  /* Spawn login process. */
  if (login_pid = do_login(args), login_pid == -1)
    return do_exit(username), 1;
  
  /* Wait for the login process to exit, and then fill all remaining processes. */
  for (;;)
    {
      reaped = waitpid(-1, &status, 0);
      if (reaped == -1)
	{
	  if (errno == ECHILD)  return do_exit(username), 0;
	  if (errno != EINTR)   return perror("waitpid"), 1;
	}
      else if (reaped == login_pid)
	{
	  ukillall(username, SIGKILL);
	  return do_exit(username), status & 255;
	}
    }
}

