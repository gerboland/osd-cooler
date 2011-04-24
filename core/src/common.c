/*
 *  Copyright(C) 2006 Neuros Technology International LLC. 
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
 * Common routines.
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ----------------------------------- 2006-09-19 CL
 *
 */

#include "common.h"
//#define OSD_DBG_MSG
#include "nc-err.h"
#include <fcntl.h>
#include <time.h>



/**
 * Convert int to string
 *
 * @param value
 *       int value to convert to string
 * @return 
 *       string buffer containing NULL terminated string, it is the caller's responsibility
 *        to free the string.
 *
 */
char* CoolIntToString(int value)
{
	char buf[32];

	snprintf(buf,32,"%d",value);

	return strdup(buf);

}




/**
 * Convert seconds to time string in the format of HH:MM:SS.
 *
 * @param total
 *       seconds.
 * @param time
 *       time string buffer, it is the caller's responsibility to make sure
 *       buffer length is greater than or equal to 9 chars.
 *
 */
void CoolInt2Time(int total, char * time)
{
    int second = 0, minute = 0, hour = 0;
	strcpy(time, "00:00:00");
	second = total % 60;
	minute = total / 60;
	if (minute >= 60)
	  {
		hour = minute / 60;
		minute = (minute - hour*60);
	  }

	sprintf(time, "%02d:%02d:%02d", hour, minute,second);
}

/*
 *Function Name:CoolTimeInt2String
 *
 *Parameters:
 *	seconds: 	time
 *  format: 	string format( "%H:%M:%S" or "%M:%S", etc )
 *  strbuf:		the result(string) buf
 *  bufsize:	the result(string) buf size
 *
 *Description:
 *		change "int" time type into string by format
 *
 *Returns:
 *
 */
void CoolTimeInt2String(int seconds, const char* format, char *strbuf, int bufsize)
{
	struct tm *gmtm;
	int totalseconds = seconds;
	gmtm = gmtime((time_t*)&totalseconds);
	strftime(strbuf,bufsize,format,gmtm);
}
	
