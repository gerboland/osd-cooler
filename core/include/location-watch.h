typedef void* location_watch_context_t;

typedef enum {
	WSTATE_PRESENT = 0,
	WSTATE_ABSENT,
} location_watch_state_t;

typedef void (*location_watch_cb_t)(location_watch_state_t state, void *user_data);

location_watch_context_t CoolLocationWatchInit(const char *location, location_watch_cb_t callback, void* user_data);
location_watch_state_t CoolLocationWatchState(location_watch_context_t context);
int CoolLocationWatchGetFD(location_watch_context_t context);
void CoolLocationWatchFDRead(location_watch_context_t context);
void CoolLocationWatchClose(location_watch_context_t context);
