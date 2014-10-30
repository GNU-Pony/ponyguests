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


#ifndef SYSCONFDIR
#define SYSCONFDIR  "/etc"
#endif

#ifndef PKGNAME
#define PKGNAME  "ponyguests"
#endif


#define SCRIPTDIR  SYSCONFDIR "/" PKGNAME


static pid_t do_login(char** args)
{
  pid_t pid = fork();
  if (pid == -1)
    return perror("fork"), -1;
  
  if (pid == 0)
    {
      execvp(*args, args);
      perror("execvp");
      exit(1);
      return -1;
    }
  
  return pid;
}

static int do_guest(char* path, char* username)
{
  char* args[3];
  pid_t pid, reaped;
  int status;
  
  args[0] = path;
  args[1] = username;
  args[2] = NULL;
  
  pid = do_login(args);
  if (pid == -1)
    return -1;
  
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

static int do_start(char* username)
{
  static char path[] = SCRIPTDIR "/ponyguests-make-guest";
  return do_guest(path, username);
}

static int do_exit(char* username)
{
  static char path[] = SCRIPTDIR "/ponyguests-delete-guest";
  return do_guest(path, username);
}


int main(int argc, char** argv)
{
  char* username;
  char** args;
  int i, status;
  pid_t login_pid, reaped;
  
  if (argc < 3)
    {
      printf("USAGE: %s <login-program> <arguments-for-login...>\n"
	     "\n"
	     "The last argument must be the username.\n", *argv);
      return 1;
    }
  
  username = argv[argc - 1];
  args = alloca((size_t)argc * sizeof(char*));
  
  for (i = 1; i < argc; i++)
    args[i - 1] = argv[i];
  args[argc - 1] = NULL;
  
  if (do_start(username) < 0)
    return 1;
  
  if (prctl(PR_SET_CHILD_SUBREAPER, 1) == -1)
    return perror("prctl PR_SET_CHILD_SUBREAPER"), do_exit(username), 1;
  
  if (login_pid = do_login(args), login_pid == -1)
    return do_exit(username), 1;
  
  for (;;)
    {
      reaped = waitpid(-1, &status, 0);
      if (reaped == -1)
	{
	  if (errno == ECHILD)  return do_exit(username), 0;
	  if (errno != EINTR)   return perror("waitpid"), 1;
	}
      else if (reaped == login_pid)
	for (;;)
	  {
	    if (reaped = waitpid(-1, NULL, WNOHANG), reaped == 0)
	      {
		if (login_pid = do_login(args), login_pid == -1)
		  return do_exit(username), 1;
		break;
	      }
	    if (reaped != -1)     continue;
	    if (errno == EINTR)   continue;
	    if (errno == ECHILD)  return do_exit(username), status & 255;
	    return perror("waitpid"), 1;
	  }
    }
}

