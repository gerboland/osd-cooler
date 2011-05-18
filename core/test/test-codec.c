/*
 *  Copyright(C) 2005 Neuros Technology International LLC. 
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
 * Neuros-Cooler platform codec test entry point.
 *
 * REVISION:
 * 
 * 1) Initial creation. ----------------------------------- 2005-01-11 MG
 *
 */

#include "nc-test.h"
#include "cooler-media.h"
#include "nc-err.h"
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <asm/fcntl.h>
#include <string.h>

extern ssize_t getline(char ** lineptr, size_t * n, FILE * stream);
static int test = 0;
#define CHECK_TEST() do { \
	  if (test) {\
		ERRLOG("Decoer and Encoder test can not be run at the same time!\n");\
		return;\
	  } else test = 1;\
  } while(0)

static char test_scan_kbd(void)
{
    char * line = NULL;
	size_t len;
	ssize_t readsize;

	readsize = getline(&line, &len, stdin);
	if (readsize > 0)
	  {
		if (line != NULL)
		  {
			char c = (len > 0) ? line[0] : '\0';
			free(line);
			return (toupper(c));
		  }
	  }
	return 0;
}


//!! decoder and encoder test can not run at the same time.
static int test_exit;
static int test_paused;
static int test_time;

/************************************************************/
/*-------------------- decoder test code. ------------------*/
/************************************************************/
static int test_seek;
static int test_mark;
static int volL, volR;
static void * test_loop(void * arg)
{
	int cur_time;
	while (!test_exit)
	{
		usleep(125000);
		if (test_paused) continue;

		/* display elapsed time. */
		cur_time = NmsGetPlaytime();
		if (cur_time/1000 != test_time/1000)
		{
			test_time = cur_time;
			DBGLOG("\tElapsed time = %d\n", test_time/1000);
		}
		//DBGLOG("in test_loop\n");
	}
	return NULL;
}
static void test_imgdec(char * imagefile)
{
	CHECK_TEST();

	DBGLOG("connecting to nms\n");
	if (NmsConnect()) 
	{
		ERRLOG("NMS connection failed!\n");
	}
	
	while (1)
	  {
		switch (test_scan_kbd())
		  {
		  case 'R':
		  	NmsSetSlideShowImage(imagefile);
			NmsStartSlideShow();
		  	break;
		  case 'C':
		  	break;
		  case 'Q':	
		  	NmsStopSlideShow(); 			
			return;
		  default: 
			DBGLOG("-------------\n");
			break;			
		  }
	  }
	
	return;
}
static void test_decoder(char * fname)
{
    static pthread_t test_thread;
	CHECK_TEST();

	test_exit = 0;
	test_paused = 1;
	test_time = 0;
	test_seek = 0;
	test_mark = 0;

	pthread_create(&test_thread, NULL, test_loop, NULL);

	DBGLOG("connecting to nms\n");
	if (NmsConnect()) 
	{
		ERRLOG("NMS connection failed!\n");
	}
	DBGLOG("satrt to play!\n");
	NmsPlay(0, fname);
	NmsSetOutputMode(1);
	test_paused = 0;
	while (1)
	  {
		switch (test_scan_kbd())
		  {
		  case 'Q':	NmsStopPlay(); goto out;
		  case 'A':
			NmsGetVolume(&volL, &volR);
			volL++;
			volR++;
			NmsSetVolume(volL, volR);
			// read back to verify.
			NmsGetVolume(&volL, &volR);
			DBGLOG("current volume: L = %d, R = %d\n", volL, volR);
			break;
		  case 'B':
			NmsGetVolume(&volL, &volR);
			volL--;
			volR--;
			NmsSetVolume(volL, volR);
			// read back to verify.
			NmsGetVolume(&volL, &volR);
			DBGLOG("current volume: L = %d, R = %d\n", volL, volR);
			break;
		  case 'P':
			test_paused ^= 1;
			if (test_paused) 
			  {
				DBGLOG("playback paused.\n");
			  }
			else
			  {
				DBGLOG("playback resumed.\n");			
			  }
			NmsPauseUnpause();
			break;
		  case 'S':
			NmsStopPlay(); goto out;
		  case 'Y': // bookmark.
			test_mark = test_time;
			break;
		  case 'T': // seek to time stamp
			NmsSeek(test_mark);
			NmsPauseUnpause();
			break;
		  case 'F':
			test_seek += 1;
			NmsFfRw(test_seek);
			DBGLOG("F\n");
			break;
		  case 'R':
			test_seek -= 1;
			NmsFfRw(test_seek);
			DBGLOG("B\n");
			break;
		  default: 
			DBGLOG("-------------\n");
			break;			
		  }
	  }
 out: 
	test_exit = 1;
	pthread_join(test_thread, NULL);
	return;
}

/************************************************************/
/*-------------------- encoder test code. ------------------*/
/************************************************************/
static int gainL, gainR;
static void * test_loop_enc(void * arg)
{
    int cur_time;
	
	while (!test_exit)
	  {
#if 1
		usleep(125000);
#else
		usleep(1250);
		DBGLOG("stress test.\n");
#endif
		if (test_paused) continue;
		
		/* display elapsed time. */
		cur_time = NmsGetRecordtime();
		if (cur_time/1000 != test_time/1000)
		  {
			test_time = cur_time;
			DBGLOG("\tElapsed time = %d\n", test_time/1000);
		  }
		//DBGLOG("in test_loop\n");
	  }
	return NULL;
}


static void test_encoder( char * fname)
{
    static pthread_t test_thread_enc;
	rec_ctrl_t ctrl;
	CHECK_TEST();
	NMS_SRV_ERROR_DETAIL error;

	test_exit = 0;
	test_paused = 1;
	test_time = 0;

	pthread_create(&test_thread_enc, NULL, test_loop_enc, NULL);

	DBGLOG("connecting to nms\n");
	if (NmsConnect()) 
	{
		ERRLOG("NMS connection failed!\n");
	}

	DBGLOG("start video monitor\n");
	NmsStartMonitor();

	ctrl.v_codec = NMS_VC_MPEG4;
	//ctrl.vcodec = NMS_VC_NO_VIDEO;
	ctrl.a_codec = NMS_AC_ARM_MP3;
	//ctrl.a_codec = NMS_AC_ARM_AAC;
	// not used yet.	
	ctrl.v_w = 640;
	ctrl.v_h = 480;
	ctrl.a_samplerate = 16000;
	ctrl.a_bitrate = 128;          //studio
	ctrl.v_frame_rate = 30;
	ctrl.v_bitrate = 2500;
	ctrl.is_pal = 1;
	while (1)
	  {
		switch (test_scan_kbd())
		  {
		  case 'Q':	
		  	NmsStopRecord();
			NmsStopMonitor();
			goto out;
		  case 'A':
			NmsGetGain(&gainL, &gainR);
			gainL++;
			gainR++;
			NmsSetGain(gainL, gainR);
			// read back to verify.
			NmsGetGain(&gainL, &gainR);
			DBGLOG("current gain: L = %d, R = %d\n", gainL, gainR);
			break;
		  case 'B':
			NmsGetGain(&gainL, &gainR);
			gainL--;
			gainR--;
			NmsSetGain(gainL, gainR);
			// read back to verify.
			NmsGetGain(&gainL, &gainR);
			DBGLOG("current gain: L = %d, R = %d\n", gainL, gainR);
			break;
		  case 'M':
			DBGLOG("stop video monitor\n");
			NmsStopMonitor();
			break;
		  case 'N':
			DBGLOG("start video monitor\n");
			NmsStartMonitor();
			break;
		  case 'R':
			NmsStopMonitor();
			DBGLOG("satrt to record!\n");
			NmsRecord(&ctrl, fname, &error);
			test_paused = 0;
			break;
		  case 'P':
			test_paused ^= 1;
			NmsPauseRecord(test_paused);
			if (test_paused) 
			  {
				DBGLOG("recording paused.\n");
			  }
			else
			  {
				DBGLOG("recording resumed.\n");			
			  }
			break;
		  case 'S':
			NmsStopRecord();
			NmsStopMonitor();
			goto out;
		  default: 
			DBGLOG("-------------\n");
			break;			
		  }
	  }
 out: 
	test_exit = 1;	
	pthread_join(test_thread_enc, NULL);
	return;
}

void test_getinfo(char *filename)
{
   media_info_t media_info;

   CHECK_TEST();
	DBGLOG("connecting to nms\n");
	if (NmsConnect()) 
	{
		ERRLOG("NMS connection failed!\n");
	}
   DBGLOG("satrt to get media info!\n");

   NmsGetMediaInfo(filename,(void *)&media_info);

   DBGLOG("Genre: %s\n", &media_info.genre[0]);
   DBGLOG("Title: %s\n",&media_info.title[0]);
   DBGLOG("Artist: %s\n",  &media_info.artist[0]);
   DBGLOG("Album: %s\n", &media_info.album[0]);
   DBGLOG("Comment: %s\n", &media_info.comment[0]);
   DBGLOG("Year: %s\n",  &media_info.year[0]);
   DBGLOG("Duration: %ld\n", media_info.duration);

}

int main(int argc, char ** argv)
{
    if ((argc != 3) ||(argv[1][0] != '-'))
	  {
		goto bail;
	  }

	switch (argv[1][1])
	  {
	  case 'd':
		test_decoder(argv[2]); break;
	  case 'e':
		test_encoder(argv[2]); break;
	  case 'i':
		test_getinfo(argv[2]); break;
	  case 'm':
		test_imgdec(argv[2]); break;
	  default: goto bail;
	  }
	return 0;

 bail:
	DBGLOG("USAGE: %s [-d|-e|-i|-m] [filename]\n", argv[0]);
	exit(1);
}

