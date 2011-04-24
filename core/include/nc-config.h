#ifndef NC_CONFIG__H
#define NC_CONFIG__H
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
 * Neuros cooler platform configuration header.
 *
 * REVISION:
 * 
 * 2) Fixed devices and function calls for irrtc split ---- 2006-12-20 nerochiaro
 * 1) Initial creation. ----------------------------------- 2006-02-02 MG 
 *
 */

#if BUILD_TARGET_ARM
    #define NMS_PLUGIN_DIR     "/usr/local/plugins/av/"
    #define I18N_PLUGIN_DIR    "/usr/local/plugins/i18n/"
    #define SSAVER_PLUGIN_DIR  "/usr/local/plugins/ssaver/"	

    #define IR_DEV             "/dev/neuros_ir"
    #define IR_BLASTER_DEV     "/dev/neuros_ir_blaster"
    #define RTC_DEV            "/dev/neuros_rtc"

#else
    #define NMS_PLUGIN_DIR     "PLACE-HOLDERS"
    #define I18N_PLUGIN_DIR    "PLACE-HOLDERS"
    #define SSAVER_PLUGIN_DIR  "PLACE-HOLDERS"	
    #define IR_DEV             "PLACE-HOLDERS"
    #define IR_BLASTER_DEV     "PLACE-HOLDERS"
    #define RTC_DEV            "PLACE-HOLDERS"
#endif

#define pathFONT_ROMAN			"nMyriad-roman.ttf"
#define sizeFONT_ROMAN          14
#define pathFONT_ROMAN_BOLD		"nMyriad-bold.ttf"
#define sizeFONT_ROMAN_BOLD     14
#define pathFONT_NEUROS_ICON	"neuros-ico.ttf"
#define sizeFONT_NEUROS_ICON    14

#define EXTEND_LINK             "/media/ext"
#define EXTEND_DIRECTORY        EXTEND_LINK"/.osd-extended"
#define EXTEND_SCRATCH_AREA     EXTEND_DIRECTORY"/scratch"
#define EXTEND_PROGRAMS_AREA    EXTEND_DIRECTORY"/programs"
#define EXTEND_USERS_AREA       EXTEND_LINK"/users"

#define DESKMON_SCRIPT          "/etc/init.d/deskmon-init"
#define SPLASHSCREEN_SCRIPT     "/etc/init.d/splashscreen"

#endif /* NC_CONFIG__H */
