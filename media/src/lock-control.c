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
 * Neuros-Cooler media lock control interface.
 *
 * REVISION:
 * 
 * 1) Initial creation. ----------------------------------- 2007-08-10 nerochiaro
 *
 */
#include "nc-err.h"
#include "cooler-core.h"
#include "nmsplugin.h"
#include "lock-control.h"

/**
 * Returns the current state of one of the three global media locks.
 * There's no guarantee that the state of the lock will remain the same after the call.
 *
 * @param media the lock you want to check
 * @return TRUE if someone is already holding the lock, FALSE otherwise
 */
BOOL CoolIsMediaLocked(MEDIA_LOCK_TYPE media)
{
	int value;
	char lock;
	int sem;
	
	switch (media)
	{
	case LOCK_AUDIO: lock = DM320_AUDIO_SEM_ID; break;
	case LOCK_VIDEO: lock = DM320_VEDEO_SEM_ID; break;
	case LOCK_IMAGE: lock = DM320_IMAGE_SEM_ID; break;
	default: return FALSE;
	}

	if (CoolSemGet(DM320_MEDIA_SEM_PATH, lock, &sem) != 0) return FALSE;

	if (CoolSemGetValue(sem, &value) != 0) return FALSE;

	return (value <= 0);
}
