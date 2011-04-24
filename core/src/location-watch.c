/*
 *  Copyright(C) 2007 Neuros Technology International LLC. 
 *               <www.neurostechnology.com>
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
 * Routines to monitor the availability of a file or directory using inotify.
 *
 * REVISION:
 *
 * 1) Initial creation. ------------------------------- 2007-09-20 nerochiaro
 *
 * TODO:
 * - Implement thread safety with mutexes on the state variable.
 * - Better error handling in case of errors during event read.
 * - Proper tracing facility.
 */

#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/inotify.h>
#include <linux/inotify-syscalls.h>

#define INOTIFY_DEBUG_EVENT
#define OSD_DBG_MSG
#include "nc-err.h"
#include "nc-type.h"

#include "location-watch.h"

#define EVENT_GUESS_SIZE_INC 256
#define EVENT_GUESS_SIZE (sizeof(struct inotify_event) + EVENT_GUESS_SIZE_INC)
#define MAX_RETRIES 3
#define FLAGS_NOT_LOCATION (IN_CREATE | IN_MOVED_TO | IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT)
#define FLAGS_LOCATION (IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT)

/* ------------------- internals --------------------------*/

typedef struct {
	location_watch_state_t status;
	char *location;
	char *current_watch;
	int watch;
	int notify_fd;
	location_watch_cb_t callback;
	void *user_data;
} lw_context_t;

#ifdef INOTIFY_DEBUG_EVENT
char * inotifytools_event_to_str_sep(int events);
#endif

static int set_watch(lw_context_t *c, int do_callback);
static int rem_watch(lw_context_t *c);
static void find_prev_dir(char *path);

/* ------------------- public API -------------------- */

/** 
 * Start monitoring (or "watching") a path.
 * You will be notified using the callback when the location becomes available or disappears.
 * Notice that you will not be called back with the initial state of the location, only of changes.
 * You can call CoolLocationWatchState at any time to get current location state.
 * When you are finished with the watch, you must call CoolLocationWatchFinish to clean up.
 *
 * @param location The path to watch. Must be an absolute path to a file or directory.
 * @param callback A function that will be called back when the location changes state. 
 *                 It cannot be NULL.
 * @param user_data A pointer to any data you like, or NULL. Will be passed back to you on callback.
 * @return NULL if failed.
 *         Otherwise a watch token that you should use in all other CoolLocationWatch* functions.
 *
 * @note The mountpoint parameter is needed because on the OSD interaction between inotify and hotplug
 *       seems a bit problematic. Hotplug 
 */
location_watch_context_t CoolLocationWatchInit(const char *location, location_watch_cb_t callback, void* user_data)
{
	lw_context_t *c;
	int ret;

	if (location == NULL || strlen(location) == 0 || location[0] != '/') return NULL; 

	c = calloc(1, sizeof(lw_context_t));
	if (c == NULL) return NULL;

	c->notify_fd = inotify_init();
	if (c->notify_fd == -1) goto bail;

	c->location = strdup(location); 
	c->watch = -1;
	c->callback = callback;
	c->user_data = user_data;

	ret = set_watch(c, FALSE);
	if (ret != 0) {
		ERRLOG("Cannot setup a watch anywhere. bailing out\n");
		goto bail;
	} 

	return (location_watch_context_t) c;

bail:
	if (c != NULL) {
		if (c->current_watch != NULL) free(c->current_watch);
		if (c->location != NULL) free(c->location);
		free(c);
	}
	return NULL;
}

/** 
 * Return the current state of the watched location at the moment of the call.
 * @param context The watch token for the watch you're interested in. If NULL the return will always be WSTATE_ABSENT
 * @return The current state of the watched location.
 */
location_watch_state_t CoolLocationWatchState(location_watch_context_t context)
{
	lw_context_t *c;
	
	if (context == NULL) return WSTATE_ABSENT;
	c = (lw_context_t*) context;

	// Simply trust that the internal state is consistent.
	return c->status;
}

/** 
 * Get the a file descriptor that should be monitored for read availability
 * only, using poll(), select() and similar functions.
 * You should not close or otherwise manipulate the file descriptor.
 * When data is available to be read on the file descriptor, call CoolLocationWatchFDRead
 * to process it. 
 *
 * Note that you generally won't need to call this function directly
 * in Neux GUI applications, since it's called internall by the NHelperLocationWatch* 
 * functions.
 *
 * @param context The watch token for the watch you're interested in.
 * @return The file descriptor that is used for the watch, or -1 if error.
 */
int CoolLocationWatchGetFD(location_watch_context_t context)
{
	lw_context_t *c;
	
	if (context == NULL) return -1;
	c = (lw_context_t*) context;

	DBGLOG("Getting Inotify fd [%d]\n", c->notify_fd);

	return c->notify_fd;
}

/** 
 * Call this function when you know that the file descriptor obtained
 * by CoolLocationWatchGetFD has data to be read. This function process
 * that data and calls the callback in case of changes in the state of 
 * the watched location.
 *
 * Note that you generally won't need to call this function directly
 * in Neux GUI applications, since it's called internall by the NHelperLocationWatch* 
 * functions.
 *
 * @param context The watch token for the watch you're interested in.
 */
void CoolLocationWatchFDRead(location_watch_context_t context)
{
	lw_context_t *c;
	struct inotify_event *event;
	char *buf;
	int bufsize;
	int len,i;

	if (context == NULL) return;
	c = (lw_context_t*) context;

	bufsize = EVENT_GUESS_SIZE;
	buf = malloc(bufsize);

	while (TRUE)
	{	
		len = read(c->notify_fd, buf, EVENT_GUESS_SIZE);
		if (len < 0) {
			if (errno == EINTR) {
				WARNLOG ("Reading from the inotify fd interrupted as EINTR"); 
				continue;
			}
			else { 
				ERRLOG ("Reading from the inotify fd errored (%d)", errno); 
				break; 
			}
		} else if (len == 0) {
			// in this case inotify told us that it has data for us, but we don't have 
			// enough room in the buffer to hold it. Let's get some more space.
			bufsize += EVENT_GUESS_SIZE_INC;
			DBGLOG("Inotify event buffer too small, enlarging it to %d\n", bufsize);
			if (realloc(buf, bufsize) == NULL) {
				ERRLOG ("Not enough memory to enlarge inotify event buffer.");
				break;
			};
			continue;
		}

		// Event(s) received successfully, process them all
		for (i = 0; i < len; ) 
		{
			event = (struct inotify_event *) &buf[i];

#ifdef INOTIFY_DEBUG_EVENT
			DBGLOG ("----- Received inotify event -------\n");
			char *strevent = inotifytools_event_to_str_sep(event->mask);
			DBGLOG ("%s : wd=%d mask=%u cookie=%u len=%u\n",
					strevent, event->wd, event->mask,
					event->cookie, event->len);
			if (event->len) DBGLOG ("name=%s\n", event->name);
#endif

			// When errors occour here what can we do ?
			// TODO: maybe call the callback, with a 3rd state LW_ERROR, and instruct people watch will be 
			// unreliable when this happens, and remove the watch and try setting it up again ?

			if (c->watch == event->wd)
			{
				if ((event->mask & IN_DELETE_SELF || event->mask & IN_MOVE_SELF || event->mask & IN_UNMOUNT) ||
					(event->mask & IN_MOVED_TO || event->mask & IN_CREATE))
				{
					if (!(event->mask & IN_UNMOUNT)) //watch is already removed in this case.
						rem_watch(c);

					if (set_watch(c, TRUE) != 0) {
						//TODO: add a provision here or in set_watch to report the error to user, so he can restart the watch if he wants.
					}
				}
			}
			else if (!(event->mask & IN_IGNORED))
				WARNLOG("Event for a different watch than ours"); //: event: %d, ours: %d\n", event->wd, c->watch);

			i += sizeof(struct inotify_event) + event->len;
		}
		break;
	}

	free(buf);
}

void CoolLocationWatchClose(location_watch_context_t context)
{
	lw_context_t *c;
	c = (lw_context_t*) context;

	if (c != NULL) {
		if (c->current_watch != NULL) free(c->current_watch);
		if (c->location != NULL) free(c->location);
		if (c->notify_fd != -1) {
			if (c->watch != -1) inotify_rm_watch(c->notify_fd, c->watch);
			close(c->notify_fd);
		}		
		free(c);
	}

	DBGLOG("Done freeing inotify data\n");
}

/* ------------------- debug --------------------------*/

#ifdef INOTIFY_DEBUG_EVENT

char * inotifytools_event_to_str_sep(int events)
{
	static char ret[1024];
	ret[0] = '\0';
	ret[1] = '\0';

	if ( IN_ACCESS & events ) {
		strcat( ret, " - " );
		strcat( ret, "ACCESS" );
	}
	if ( IN_MODIFY & events ) {
		strcat( ret, " - " );
		strcat( ret, "MODIFY" );
	}
	if ( IN_ATTRIB & events ) {
		strcat( ret, " - " );
		strcat( ret, "ATTRIB" );
	}
	if ( IN_CLOSE_WRITE & events ) {
		strcat( ret, " - " );
		strcat( ret, "CLOSE_WRITE" );
	}
	if ( IN_CLOSE_NOWRITE & events ) {
		strcat( ret, " - " );
		strcat( ret, "CLOSE_NOWRITE" );
	}
	if ( IN_OPEN & events ) {
		strcat( ret, " - " );
		strcat( ret, "OPEN" );
	}
	if ( IN_MOVED_FROM & events ) {
		strcat( ret, " - " );
		strcat( ret, "MOVED_FROM" );
	}
	if ( IN_MOVED_TO & events ) {
		strcat( ret, " - " );
		strcat( ret, "MOVED_TO" );
	}
	if ( IN_CREATE & events ) {
		strcat( ret, " - " );
		strcat( ret, "CREATE" );
	}
	if ( IN_DELETE & events ) {
		strcat( ret, " - " );
		strcat( ret, "DELETE" );
	}
	if ( IN_DELETE_SELF & events ) {
		strcat( ret, " - " );
		strcat( ret, "DELETE_SELF" );
	}
	if ( IN_UNMOUNT & events ) {
		strcat( ret, " - " );
		strcat( ret, "UNMOUNT" );
	}
	if ( IN_Q_OVERFLOW & events ) {
		strcat( ret, " - " );
		strcat( ret, "Q_OVERFLOW" );
	}
	if ( IN_IGNORED & events ) {
		strcat( ret, " - " );
		strcat( ret, "IGNORED" );
	}
	if ( IN_CLOSE & events ) {
		strcat( ret, " - " );
		strcat( ret, "CLOSE" );
	}
	if ( IN_MOVE_SELF & events ) {
		strcat( ret, " - " );
		strcat( ret, "MOVE_SELF" );
	}
	if ( IN_ISDIR & events ) {
		strcat( ret, " - " );
		strcat( ret, "ISDIR" );
	}
	if ( IN_ONESHOT & events ) {
		strcat( ret, " - " );
		strcat( ret, "ONESHOT" );
	}

	if (ret[0] == '\0') {
		sprintf( ret, " - %08x", events );
	}
	return &ret[1];
}

#endif

/* ------------------- internals --------------------------*/

// path is modified in place. path buffer length must be at least 2 (including null-term).
static void find_prev_dir(char *path)
{
	char* loc;

	// Look up previous path element
	loc = strrchr(path, '/');
	if (loc == path) {
		path[1] = '\0';
		return;
	}
	else if (loc == NULL) return; //relative path ?

	*loc = '\0';
}

static int rem_watch(lw_context_t *c)
{
	//remove current watch, if any.
	if (c->watch != -1) {
		if (inotify_rm_watch(c->notify_fd, c->watch) < 0) {
			ERRLOG("Failed to remove inotify watch (err %d)", errno);
			return -1;
		}
		c->watch = -1;	
	}

	return 0;
}

static int set_watch(lw_context_t *c, int do_callback)
{
	int isloc = TRUE;
	int flags = 0;

	if (c->current_watch != NULL) {
		free(c->current_watch);
	}
	c->current_watch = strdup(c->location);

	while (TRUE) 
	{
		flags = (isloc) ? FLAGS_LOCATION : FLAGS_NOT_LOCATION;
	
		DBGLOG("Inotify trying to watch path [%s]\n", c->current_watch);
		c->watch = inotify_add_watch(c->notify_fd, c->current_watch, flags);
		if (c->watch != -1) 
		{
			DBGLOG("Inotify watch set OK (id %d)\n", c->watch);
	
			location_watch_state_t oldstate = c->status;
			c->status = (isloc) ? WSTATE_PRESENT : WSTATE_ABSENT;
			if (do_callback && c->status != oldstate && c->callback != NULL) 
				c->callback(c->status, c->user_data);
	
			return 0;
		}
		else {
			if (errno != ENOENT) {
				ERRLOG("Setting inotify watch fails with error %d", errno);
				return -1;
			}
		}
	
		find_prev_dir(c->current_watch);
		isloc = FALSE;
	}
}
