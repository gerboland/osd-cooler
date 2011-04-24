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
 * I18n support routines.
 *
 * REVISION:
 * 
 *
 * 2) New i18n algorithm. --------------------------------- 2007-04-19 MG
 * 1) Initial creation. ----------------------------------- 2006-05-01 MG
 *
 */
//#define OSD_DBG_MSG
#include <dlfcn.h>
#include "cooler-core.h"

#define I18N_PLUGIN_API "NeurosI18nDescriptor"
#define LANGUAGE_CONFIG_FILE     "/mnt/OSD/.language"

static const char* const strLanguageKey[] = {
	"LANGUAGE",
	"CHINESE",
	"ENGLISH",
	"FRENCH",
	"GERMAN",
	"ITALIAN",
	"JAPANESE",
	"SPANISH"
};

static void
SetLanguageConfig(i18n_global_t* i18n)
{
	char value[FONTNAME_LANGTH];

	snprintf(value,FONTNAME_LANGTH-1,"%d",i18n->curLangId);
	CoolSetCharValue( LANGUAGE_CONFIG_FILE,strLanguageKey[0], "LanguageId", value);

	snprintf(value,FONTNAME_LANGTH-1,"%s",i18n->font_name_roman);
	CoolSetCharValue( LANGUAGE_CONFIG_FILE,strLanguageKey[i18n->curLangId], "FontnameRoman", value );

	snprintf(value,FONTNAME_LANGTH-1,"%s",i18n->font_name_bold);
	CoolSetCharValue( LANGUAGE_CONFIG_FILE,strLanguageKey[i18n->curLangId], "FontnameBold", value );

	snprintf(value,FONTNAME_LANGTH-1,"%s",i18n->font_name_ico);
	CoolSetCharValue( LANGUAGE_CONFIG_FILE,strLanguageKey[i18n->curLangId], "FontnameIcon", value );
}

static void
GetLanguageConfig(i18n_global_t* i18n)
{
	char value[FONTNAME_LANGTH];

	i18n->curLangId = NC_LANG_ENG;
	CoolGetIntValue( LANGUAGE_CONFIG_FILE,strLanguageKey[0], "LanguageId", &i18n->curLangId);
	// initialized to default language: ENGLISH
	if( i18n->curLangId<=start_LANG_ID || i18n->curLangId>=end_LANG_ID )
		i18n->curLangId = NC_LANG_ENG;

	int configLangId = i18n->curLangId;
	if ( i18n->curLangId != NC_LANG_ENG )
	{
#ifndef LANG_CHINESE_SUPPORT
		if ( i18n->curLangId == NC_LANG_CHN )
			i18n->curLangId = NC_LANG_ENG;
#endif 
#ifndef LANG_FRENCH_SUPPORT
		if ( i18n->curLangId == NC_LANG_FRE )
			i18n->curLangId = NC_LANG_ENG;
#endif 
#ifndef LANG_GERMAN_SUPPORT
		if ( i18n->curLangId == NC_LANG_GER )
			i18n->curLangId = NC_LANG_ENG;
#endif 
#ifndef LANG_JAPANESE_SUPPORT
		if ( i18n->curLangId == NC_LANG_JAP )
			i18n->curLangId = NC_LANG_ENG;
#endif 
#ifndef LANG_ITALIAN_SUPPORT
		if ( i18n->curLangId == NC_LANG_ITA )
			i18n->curLangId = NC_LANG_ENG;
#endif 
#ifndef LANG_SPANISH_SUPPORT
		if ( i18n->curLangId == NC_LANG_SPA )
			i18n->curLangId = NC_LANG_ENG;
#endif 
	}

	strlcpy(value, DEFAULT_FONT_ROMAN, sizeof(value));
	CoolGetCharValue( LANGUAGE_CONFIG_FILE,strLanguageKey[i18n->curLangId], "FontnameRoman", value, FONTNAME_LANGTH );
	strlcpy( i18n->font_name_roman, value, sizeof(i18n->font_name_roman) ); 

	strlcpy(value, DEFAULT_FONT_BOLD, sizeof(value));
	CoolGetCharValue( LANGUAGE_CONFIG_FILE,strLanguageKey[i18n->curLangId], "FontnameBold", value, FONTNAME_LANGTH );
	strlcpy( i18n->font_name_bold, value, sizeof(i18n->font_name_bold) ); 

	strlcpy(value, DEFAULT_FONT_ICON, sizeof(value));
	CoolGetCharValue( LANGUAGE_CONFIG_FILE,strLanguageKey[i18n->curLangId], "FontnameIcon", value, FONTNAME_LANGTH );
	strlcpy( i18n->font_name_ico, value, sizeof(i18n->font_name_ico) ); 

	// if language id in config file isn't supported, save current language id to config file
	if ( configLangId != i18n->curLangId )
		SetLanguageConfig(i18n);
}

// public APIs.

/** Function fetches current language ID.
 *
 * @return 
 *        language ID.
 *
 */
int CoolI18Ngetlang(void)
{
	int langId = -1;
	int first;
	shm_lock_t * shmlock;
	i18n_global_t * i18n;

	i18n = CoolShmGet(I18N_SHM_ID, sizeof(i18n_global_t), &first, &shmlock);

	if (!i18n) 
	{
		WARNLOG("Unable to fetch i18n shared memory!\n");
		goto bail_no_shm;
	}
	if (first)
	{
		CoolShmWriteLock(shmlock);
		GetLanguageConfig( i18n );
		CoolShmWriteUnlock(shmlock);
	}
	else
	{
		CoolShmReadLock(shmlock);
		langId = i18n->curLangId;
		CoolShmReadUnlock(shmlock);
	}

	CoolShmPut(I18N_SHM_ID);
 bail_no_shm:
	return langId;
}

/**
 * Function sets system language ID.
 *
 * @param langId
 *       language ID.
 *
 */
void CoolI18Nsetlang(int langId)
{
	int first;
	shm_lock_t * shmlock;
	i18n_global_t * i18n;

	if (langId <= start_LANG_ID || langId >= end_LANG_ID)
	{
		langId = NC_LANG_ENG;
	}

	i18n = CoolShmGet(I18N_SHM_ID, sizeof(i18n_global_t), &first, &shmlock);

	if (!i18n) 
	{
		WARNLOG("Unable to fetch i18n shared memory!\n");
		goto bail_no_shm;
	}

	CoolShmWriteLock(shmlock);
	i18n->curLangId = langId;
	snprintf(i18n->font_name_ico,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_ICON);
	switch( i18n->curLangId )
	{
#ifdef LANG_CHINESE_SUPPORT
	case NC_LANG_CHN:
		snprintf(i18n->font_name_roman,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_CHINESE);
		snprintf(i18n->font_name_bold,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_CHINESE);
		break;
#endif
#ifdef LANG_FRENCH_SUPPORT
	case NC_LANG_FRE:
		snprintf(i18n->font_name_roman,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_ROMAN);
		snprintf(i18n->font_name_bold,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_BOLD);
		break;
#endif
#ifdef LANG_GERMAN_SUPPORT
	case NC_LANG_GER:
		snprintf(i18n->font_name_roman,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_ROMAN);
		snprintf(i18n->font_name_bold,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_BOLD);
		break;
#endif
#ifdef LANG_ITALIAN_SUPPORT
	case NC_LANG_ITA:
		snprintf(i18n->font_name_roman,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_ROMAN);
		snprintf(i18n->font_name_bold,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_BOLD);
		break;
#endif
#ifdef LANG_JAPANESE_SUPPORT
	case NC_LANG_JAP:
		snprintf(i18n->font_name_roman,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_ROMAN);
		snprintf(i18n->font_name_bold,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_BOLD);
		break;
#endif
#ifdef LANG_SPANISH_SUPPORT
	case NC_LANG_SPA:
		snprintf(i18n->font_name_roman,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_ROMAN);
		snprintf(i18n->font_name_bold,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_BOLD);
		break;
#endif
	default:
		snprintf(i18n->font_name_roman,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_ROMAN);
		snprintf(i18n->font_name_bold,FONTNAME_LANGTH-1,"%s",DEFAULT_FONT_BOLD);
		break;
	}
	SetLanguageConfig( i18n );
	CoolShmWriteUnlock(shmlock);

	CoolShmPut(I18N_SHM_ID);
 bail_no_shm:
	return;
}


/** Function fetches i18n handle. 
 *
 * @param appID
 *        unique application ID.
 * @return
 *        i18n handle.
 *
 */
i18n_handle_t * CoolI18NopenHandle(const char * appId)
{
	int langId = CoolI18Ngetlang();
	dir_node_t node;
	char path[PATH_MAX];
	char apppath[PATH_MAX];
	int ii;
	void * lib;
	void * ld;
	i18n_handle_t * handle = NULL;
	i18n_handle_t * defhandle = NULL;

	DBGLOG("open i18n handle.\n");
	strlcpy(apppath, I18N_PLUGIN_DIR, sizeof(apppath));
	strlcat(apppath, appId, sizeof(apppath));
	strlcat(apppath, "/", sizeof(apppath));
	if (CoolBlockOpenDir(apppath, &node) > 0)
	{
		DBGLOG(" filter out directory...\n");
		CoolFilterDirectory(&node, CoolDirFilterHiddenEntry, DF_ALL);
	}
	else
	{
		WARNLOG("i18n not available.\n");
		return NULL;
	}

	while(node.ffnum--)
	{
		ii = node.ffindex[node.ffnum];
		strlcpy(path, apppath, sizeof(path));
		strlcat(path, node.namelist[ii]->d_name, sizeof(path));

		lib = dlopen(path, RTLD_LAZY);
		DBGLOG("opening %s\n", path);
		if (!lib)
		{
			DBGLOG("DLERROR: %s\n", dlerror());
			continue;
		}

		ld = dlsym(lib, I18N_PLUGIN_API);
		if (!ld) 
		{
			DBGLOG("unable to find symbol: %s\n", I18N_PLUGIN_API);
			dlclose(lib);
		}
		else
		{
			i18n_plugin_t * pp = (i18n_plugin_t *)ld;

			ii = pp->langId;
			if (ii == langId)
			{
				DBGLOG("language: %s\n",*(pp->description));
				handle = malloc(sizeof(i18n_handle_t));
				handle->i18n =  pp;
				handle->lib = lib;
				break;
			} 
			else if (ii == NC_LANG_ENG)
			{
				defhandle = malloc(sizeof(i18n_handle_t));
				defhandle->i18n = pp;
				defhandle->lib = lib;
			}
			else dlclose(lib);
		}
	}

	CoolCloseDirectory(&node);

	if (NULL == handle)
	{
		//use default handle.
		handle = defhandle;
	}
	else
	{
		//close default handle.
		if (defhandle)
		{
			dlclose(defhandle->lib);
			free(defhandle);
		}
	}
	
	return handle;
}

/** Function closes i18n handle. 
 *
 * @param handle
 *        i18n handle.
 * @return
 *        i18n handle.
 *
 */
void CoolI18NcloseHandle(i18n_handle_t * handle)
{
	if (handle)
	{
		if(handle->lib)
		{
			dlclose(handle->lib);
			handle->lib = NULL;
			free(handle);
		}
	}
}

/**
 * Function creates gets i18n string.
 * 
 * @param handle
 *        i18n handle
 * @param strId
 *       Enumerated string ID.
 * @return 
 *       i18n string, default to English if not found in current locale.
 *
 */
char * CoolI18Ngettext(i18n_handle_t * handle, int strId)
{
	int id = strId;

	if ( strId == 0)
		return NULL;
	
	if ((strId <0)||(NULL == handle)) 
	{
		WARNLOG("invalid string ID detected!\n");
		goto bail;
	}

	if (NULL == handle->i18n)
	{
		WARNLOG("invalid handle detected!\n");
		goto bail;
	}

	DBGLOG("strId = %d\n"
		   "phrase start: %d end: %d\n", 
		   strId, 
		   handle->i18n->startOfPhraseTab,
		   handle->i18n->endOfPhraseTab);

    if ((strId >= handle->i18n->startOfPhraseTab) &&
		(strId <= handle->i18n->endOfPhraseTab))
	{
		id -= handle->i18n->startOfPhraseTab;
		return (char*)handle->i18n->phraseTab[id];
	}
	else if ((strId >= handle->i18n->startOfSentenceTab) &&
		(strId <= handle->i18n->endOfSentenceTab))
	{
		id -= handle->i18n->startOfSentenceTab;
		return (char*)handle->i18n->sentenceTab[id];
	}
	if ((strId >= handle->i18n->startOfHelpTab) &&
		(strId <= handle->i18n->endOfHelpTab))
	{
		id -= handle->i18n->startOfHelpTab;
		return (char*)handle->i18n->phraseTab[id];
	}

 bail:
	return " ";
}

/*
 *Function Name:CoolI18NgetFontname
 *
 *Parameters:fontid
 *      defferent font type
 *Description:
 *      get the system font name
 *Returns:
 *      return font name of the current language, the font name
 *      must be free by caller
 */
char* CoolI18NgetFontname(int fontid)
{
	char* fontname = NULL;
	int first;
	shm_lock_t * shmlock;
	i18n_global_t * i18n;
	i18n = CoolShmGet(I18N_SHM_ID, sizeof(i18n_global_t), &first, &shmlock);

	if( !i18n || first )
	{
		WARNLOG("Unable to fetch i18n shared memory or don't load language config!\n");
		switch( fontid )
		{
		case NX_FONT_ICO:
			fontname = strdup(DEFAULT_FONT_ICON);
			break;
		case NX_FONT_ROMAN:
			fontname = strdup(DEFAULT_FONT_ROMAN);
			break;
		case NX_FONT_BOLD:
			fontname = strdup(DEFAULT_FONT_BOLD);
			break;
		default:
			break;
		}
		if( i18n )
			CoolShmPut(I18N_SHM_ID);
		return fontname;
	}
	else
	{
		CoolShmReadLock(shmlock);
		switch( fontid )
		{
		case NX_FONT_ICO:
			fontname = strdup(i18n->font_name_ico);
			break;
		case NX_FONT_ROMAN:
			fontname = strdup(i18n->font_name_roman);
			break;
		case NX_FONT_BOLD:
			fontname = strdup(i18n->font_name_bold);
			break;
		default:
			break;
		}
		CoolShmReadUnlock(shmlock);
		CoolShmPut(I18N_SHM_ID);
		return fontname;
	}
}

int CoolIInitLanguage(void)
{
	int first;
	shm_lock_t * shmlock;
	i18n_global_t * i18n;

	i18n = CoolShmGet(I18N_SHM_ID, sizeof(i18n_global_t), &first, &shmlock);

	if(!i18n)
	{
		WARNLOG("Unable to fetch i18n shared memory!\n");
		return -1;
	}
	CoolShmWriteLock(shmlock);
	GetLanguageConfig( i18n );
	CoolShmWriteUnlock(shmlock);
	return 0;
}

