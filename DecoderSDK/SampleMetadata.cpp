/*! @file SampleMetadata.cpp

*  @brief Internal routines used for processing sample metadata.
*  
*  Interface to the CineForm HD decoder.  The decoder API uses an opaque
*  data type to represent an instance of a decoder.  The decoder reference
*  is returned by the call to CFHD_OpenDecoder.
*
*  @version 1.0.0
*
*  (C) Copyright 2017 GoPro Inc (http://gopro.com/).
*
*  Licensed under either:
*  - Apache License, Version 2.0, http://www.apache.org/licenses/LICENSE-2.0  
*  - MIT license, http://opensource.org/licenses/MIT
*  at your option.
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*/


#include "StdAfx.h"

#ifdef _WINDOWS
#include "../Common/Settings.h"
#else
#define MAX_PATH	260
#if __APPLE__
#include <string.h>
#include "CoreFoundation/CoreFoundation.h"
#else
#include  <mm_malloc.h>
#endif
#endif

#include "CFHDMetadata.h"
#include "../Common/AVIExtendedHeader.h"
#include "SampleMetadata.h"
#include "../Codec/metadata.h"
#include "../Codec/lutpath.h"


/*
 * TODO: Should change this routine to pass the length of the output strings
 * or use the string class from the standard template library to avoid writing
 * beyond the end of the output strings.
 */


void InitGetLUTPaths(char *pPathStr, size_t pathSize, char *pDBStr, size_t DBSize)
{
	if (pPathStr && pDBStr)
	{
#ifdef _WINDOWS
		USES_CONVERSION;

		TCHAR defaultLUTpath[260] = "C:\\Program Files\\Common Files\\CineForm\\LUTs";
		char DbNameStr[64] = "db";

		CSettings cfg;


		if(cfg.Open(HKEY_CURRENT_USER, _T("SOFTWARE\\CineForm\\ColorProcessing")))
		{
			LPCTSTR pDBPathStr = NULL;
			LPCTSTR pLUTPathStr = NULL;

			CComBSTR path(cfg.GetString(_T("DBPath"), _T("db")));
			pDBPathStr = OLE2T(path);
			//strcpy(DbNameStr, pDBPathStr);
			strcpy_s(DbNameStr, sizeof(DbNameStr), pDBPathStr);

			CComBSTR path2(cfg.GetString(_T("LUTPath"), _T("NONE")));
			pLUTPathStr = OLE2T(path2);

			cfg.Close();

			if(0 == strcmp(pLUTPathStr, "NONE"))
			{
				int n;
				char PublicPath[80];

				if(n = GetEnvironmentVariable("PUBLIC",PublicPath,79)) // Vista and Win7
				{
					_stprintf_s(defaultLUTpath, sizeof(defaultLUTpath), _T("%s\\%s"), PublicPath, _T("CineForm\\LUTs")); //Vista & 7 default
				}
				else
				{
					if(cfg.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion")))
					{
						CComBSTR commonpath(cfg.GetString(_T("CommonFilesDir (x86)"), _T("NONE")));
						LPCTSTR pCommonPathStr = OLE2T(commonpath);
						if(0 == strcmp(pCommonPathStr, "NONE"))
						{
							commonpath = cfg.GetString(_T("CommonFilesDir"), _T("C:\\Program Files\\Common Files"));
							pCommonPathStr = OLE2T(commonpath);
						}
						_stprintf_s(defaultLUTpath, sizeof(defaultLUTpath), _T("%s\\%s"), pCommonPathStr, _T("CineForm\\LUTs"));

						cfg.Close();
					}
				}
			}
			else
			{
				strcpy_s(defaultLUTpath, sizeof(defaultLUTpath), pLUTPathStr);
			}
		}

		strncpy(pPathStr, defaultLUTpath, 259);
		strncpy(pDBStr, DbNameStr, 63);

#elif __APPLE_REMOVE__

		// This code has not been tested
		CFPropertyListRef	appValue;

		CFPreferencesAppSynchronize( CFSTR("com.cineform.codec") );

		/*  encoder not defined in this.
			OverridePathString not needed

		appValue = CFPreferencesCopyAppValue( CFSTR("OverridePath"),  CFSTR("com.cineform.codec") );
		//fprintf(stderr,"(Enc)AppValue for OverridePath = %08x\n",appValue);
		if ( appValue && CFGetTypeID(appValue) == CFStringGetTypeID()  ) {
			const char	*	pathStr = CFStringGetCStringPtr( (CFStringRef)appValue, kCFStringEncodingASCII);
			if(pathStr) {
				strcpy(encoder->OverridePathStr, pathStr);
			} else
			{
				strcpy(encoder->OverridePathStr, "/Library/Application Support/CineForm");
			}
		}
		else
		{
			strcpy(encoder->OverridePathStr, "/Library/Application Support/CineForm");
		}
		 */
		appValue = CFPreferencesCopyAppValue( CFSTR("LUTsPath"),  CFSTR("com.cineform.codec") );
		if ( appValue && CFGetTypeID(appValue) == CFStringGetTypeID()  ) {
			const char	*	pathStr = CFStringGetCStringPtr( (CFStringRef)appValue, kCFStringEncodingASCII);
			if(pathStr) {
				strcpy(pPathStr, pathStr);
			}
			else
			{
				strcpy(pPathStr, "/Library/Application Support/CineForm/LUTs");
			}
		}
		else
		{
			strcpy(pPathStr, "/Library/Application Support/CineForm/LUTs");
		}
		//strcpy(decoder->OverridePathStr, "/Library/Application Support/CineForm/LUTs");
		appValue = CFPreferencesCopyAppValue( CFSTR("CurrentDBPath"),  CFSTR("com.cineform.codec") );
		if ( appValue && CFGetTypeID(appValue) == CFStringGetTypeID()  ) {
			const char	*	pathStr = CFStringGetCStringPtr( (CFStringRef)appValue, kCFStringEncodingASCII);
			if(pathStr) {
				strcpy(pDBStr, pathStr);
			}
			else
			{
				if( !CFStringGetCString( (CFStringRef)appValue, pDBStr, 260, kCFStringEncodingASCII) ) {
					strcpy(pDBStr, "db");
				}
			}
		}
		else
		{
			strcpy(pDBStr, "db");
		}

#else
		// Initialize the default locations on Linux
		strcpy(pPathStr, LUT_PATH_STRING);
		strcpy(pDBStr, DATABASE_PATH_STRING);

		// Open the first user preferences file that exists
		//FILE *file = fopen(SETTINGS_PATH_STRING, "r");
		char pathname[PATH_MAX];
		FILE *file = OpenUserPrefsFile(pathname, sizeof(pathname));
		if (file)
		{
			SCANNER scanner;

			// Parse the preferences file and set parameters in the decoder
			CODEC_ERROR error = ParseUserMetadataPrefs(file, &scanner,
													   pPathStr, pathSize,
													   pDBStr, DBSize);
			if (error != CODEC_ERROR_OKAY)
			{
				// Restore the default paths
				strcpy(pPathStr, LUT_PATH_STRING);
				strcpy(pDBStr, DATABASE_PATH_STRING);

				// Report the error code and line number from the scanner
				FILE *logfile = OpenLogFile();
				if (logfile)
				{
					int error = scanner.error;
					fprintf(logfile, "Error %s line %d: %s (%d)\n", pathname, scanner.line, Message(error), error);
					fclose(logfile);
				}
			}

			fclose(file);
		}
#endif
	}
}
