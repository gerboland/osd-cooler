#ifndef I18N__H
#define I18N__H
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
 * i18n support module header.
 *
 * REVISION:
 * 
 * 
 * 1) Initial creation. ----------------------------------- 2006-05-11 MG 
 *
 */

#include <stdio.h>
#include <stdlib.h>

#define I18N_SHM_ID "i18n"
#define FONTNAME_LANGTH		256
#define DEFAULT_FONT_ROMAN      "nMyriad-roman.ttf"
#define DEFAULT_FONT_BOLD       "nMyriad-bold.ttf"
#define DEFAULT_FONT_ICON       "neuros-ico.ttf"
#define DEFAULT_FONT_CHINESE      "nsimsong.ttf"
#define I18N_NULL 0

typedef struct 
{
	int           langId;
	int           startOfPhraseTab;
	int           endOfPhraseTab;
	int           startOfSentenceTab;
	int           endOfSentenceTab;
	int           startOfHelpTab;
	int	          endOfHelpTab; 
	const char ** phraseTab;
	const char ** sentenceTab;
	const char ** helpTab;
	const char ** description;
} i18n_plugin_t;

typedef struct
{
	void *          lib;
	i18n_plugin_t * i18n;
} i18n_handle_t;

// language ID.
enum LANG_ID
  {
	start_LANG_ID,
	/*-----------------------------*/	
	NC_LANG_CHN,      // chinese
	NC_LANG_ENG,      // english <-- default
	NC_LANG_FRE,      // french
	NC_LANG_GER,      // german
	NC_LANG_ITA,      // italian
	NC_LANG_JAP,      // japanese 
	NC_LANG_SPA,      // spanish
	/*-----------------------------*/	
	end_LANG_ID
  };
#define MAX_LANG_ID (end_LANG_ID - start_LANG_ID - 1)

/** i18n module globlas. */
typedef struct
{
	int curLangId;   /** current language ID. */
	char font_name_ico[FONTNAME_LANGTH];
	char font_name_roman[FONTNAME_LANGTH];
	char font_name_bold[FONTNAME_LANGTH];
} i18n_global_t;

typedef enum
{
	NX_FONT_ICO,
	NX_FONT_ROMAN,
	NX_FONT_BOLD
}SYSTEM_FONT_ID;


// i18n APIs.
int             CoolIInitLanguage(void);
char *          CoolI18Ngettext(i18n_handle_t * handle, int strId);
int             CoolI18Ngetlang(void);
void            CoolI18Nsetlang(int langId);
i18n_handle_t * CoolI18NopenHandle(const char * appId);
void            CoolI18NcloseHandle(i18n_handle_t * handle);
char*            CoolI18NgetFontname(int fontid);


#endif /* I18N__H */

