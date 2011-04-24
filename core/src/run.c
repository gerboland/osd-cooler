/*
 *  Copyright(C) 2007 Neuros Technology International LLC. 
 *					<www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
 *  
 *
 *  This program is distributed in the hope that, in addition to its 
 *  original purpose to support Neuros hardware, it will be useful 
 *  otherwise, but WITHOUT ANY WARRANTY; without even the implied 
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 *
 * Command execution routines
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ----------------------------------- 2007-10-10 DF
 * 2) Add NoWait version----------------------------------- 2007-12-20 nerochiaro
 *
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include "run.h"

static const unsigned MAX_CMD_ARGS=16;	/**< Largest number of arguments */

/**
 * Waits for the given process to exit and returns its status code or -1 on
 * waitpid error.
 */
static int WaitProcessExit(int pid)
{
	int status;
	errno = 0;
	do {
		if ((waitpid(pid, &status, 0) == -1) || !WIFEXITED(status)) {
			if (errno != EINTR)
				return -1;
		} else
				return status;
	} while(1);
}

/**
 * Runs an external command until completion.
 *
 * Operates like system(3) but takes a list of arguments (like execlp(3)) and
 * does NOT spawn a shell so escaping them is unnecessary. It also doesn't
 * mask signals, so the process can be interrupted.
 *
 * @param command Command name; will be searched on PATH if not an absolute path
 *
 * @param ... Arguments to the command, with NULL as the last element of the
 *	list; these are equivalent to argv[1]..argv[n] where n <= 15
 *
 * @return Exit status, in the same format as system(3)
 */
int CoolRunCommand(const char *command, ...) {
	int pid;

	if (command == NULL)
		return -1;

	pid = fork();

	if (pid == -1)
		return -1;

	if (pid == 0) {
		/* child */
		const char* argv[MAX_CMD_ARGS];
		va_list args;
		va_start(args, command);
		unsigned i=0;
		argv[i++] = command;
		for (; (i < MAX_CMD_ARGS) && (argv[i] = va_arg(args, char *)) != NULL; ++i)
			;
		va_end(args);
		if (i >= MAX_CMD_ARGS)
			/* Too many arguments--abort */
			exit(-1);

		execvp(command, (char **)argv);

		/* Will only get here if command cannot be run */
		exit(127);	/* This is what the shell returns in this case */
	}

	/* parent */
	return WaitProcessExit(pid);
}

/**
 * Runs an external command until completion, redirecting stdout to a file.
 * it may cause issues and unknown side effect, use with caution
 * and test thoroughly!
 *
 * Operates like system(3) but takes a list of arguments (like execlp(3)) and
 * does NOT spawn a shell so escaping them is unnecessary. It also doesn't
 * mask signals, so the process can be interrupted.
 *
 * @param outfile File name which will be created or overwritten with the command output
 *
 * @param command Command name; will be searched on PATH if not an absolute path
 *
 * @param ... Arguments to the command, with NULL as the last element of the
 *	list; these are equivalent to argv[1]..argv[n] where n <= 15
 *
 * @return Exit status, in the same format as system(3)
 */
int CoolRunCommandOut(const char *outfile, const char *command, ...)
{
	int pid;

	if (command == NULL || outfile == NULL)
		return -1;

	pid = fork();

	if (pid == -1)
		return -1;

	if (pid == 0) {
		/* child */
		const char* argv[MAX_CMD_ARGS];
		va_list args;
		va_start(args, command);
		unsigned i=0;
		argv[i++] = command;
		for (; (i < MAX_CMD_ARGS) && (argv[i] = va_arg(args, char *)) != NULL; ++i)
			;
		va_end(args);
		if (i >= MAX_CMD_ARGS)
			/* Too many arguments--abort */
			exit(-1);

		int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (fd == -1)
			exit(-1);

		/* Make this file stdout for this process */
		int rc = dup2(fd, 1);
		close(fd);
		if (rc == -1)
			exit(-1);

		execvp(command, (char **)argv);

		/* Will only get here if command cannot be run */
		exit(127);	/* This is what the shell returns in this case */
	}

	/* parent */
	return WaitProcessExit(pid);
}

/**
 * Runs an external command without waiting for it to complete.
 *
 * Operates like system(3) takes a list of arguments (like execlp(3)) and
 * does NOT spawn a shell so escaping them is unnecessary. It also doesn't
 * mask signals, so the process can be interrupted. It also doesn't wait for
 * it to finish.
 *
 * @param command Command name; will be searched on PATH if not an absolute path
 *
 * @param ... Arguments to the command, with NULL as the last element of the
 *	list; these are equivalent to argv[1]..argv[n] where n <= 15
 *
 * @return Pid of the new process, -1 if something failed
 */
int CoolRunCommandNoWait(const char *command, ...) {
	int pid;

	if (command == NULL)
		return -1;

	pid = fork();

	if (pid == -1)
		return -1;

	if (pid == 0) {
		/* child */
		const char* argv[MAX_CMD_ARGS];
		va_list args;
		va_start(args, command);
		unsigned i=0;
		argv[i++] = command;
		for (; (i < MAX_CMD_ARGS) && (argv[i] = va_arg(args, char *)) != NULL; ++i)
			;
		va_end(args);
		if (i >= MAX_CMD_ARGS)
			/* Too many arguments--abort */
			exit(-1);

		execvp(command, (char **)argv);

		/* Will only get here if command cannot be run */
		exit(127);	/* This is what the shell returns in this case */
	}

	/* parent */
	return pid;
}
