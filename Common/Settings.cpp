/*! @file settings.cpp

*  @brief Windows Registry I/O
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

#ifdef _WINDOWS

#include "StdAfx.h"

//#include <qedit.h>
//-------------------------------------------------------------------------------------------
// Settings.cpp
//-------------------------------------------------------------------------------------------
#include "Settings.h"
//#include <comdef.h>

//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------
CSettings::CSettings()
{
	m_key = 0;
}

//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------
CSettings::~CSettings()
{
	Close();
}

//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------
BOOL CSettings::Open(HKEY key, LPCTSTR pSubKey)
{
	_ASSERT(m_key == 0);
	if(ERROR_SUCCESS != ::RegCreateKeyEx(key, pSubKey, 0, 0, 0, KEY_ALL_ACCESS, 0, &m_key, 0))
	{
		return FALSE;
	}
	return TRUE;
}

//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------
void CSettings::Close()
{
	if(m_key)
	{
		::RegCloseKey(m_key);
		m_key = 0;
	}
}

//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------
CComBSTR CSettings::GetString(LPCTSTR pName, LPCTSTR defaultValue)
{
	_ASSERT(m_key);
	DWORD lengthNeeded = 0;
	::RegQueryValueEx(m_key, pName, 0, 0, 0, &lengthNeeded);
	if(lengthNeeded)
	{
		TCHAR* pBuf = (TCHAR*)_alloca(lengthNeeded);
		if(pBuf)
		{
			if(::RegQueryValueEx(m_key, pName, 0, 0, (BYTE*)pBuf, &lengthNeeded) == ERROR_SUCCESS)
			{
				return CComBSTR(pBuf);
			}
		}
	}
	return CComBSTR(defaultValue);
}

//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------
void CSettings::PutString(LPCTSTR pName, LPCTSTR pValue)
{
	_ASSERT(m_key);
	::RegSetValueEx(m_key, pName, 0, REG_SZ, (BYTE*)pValue, lstrlen(pValue));
	// TODO: catch error?
}

//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------
void CSettings::PutDWORD(LPCTSTR pName, DWORD value)
{
	_ASSERT(m_key);
	::RegSetValueEx(m_key, pName, 0, REG_DWORD, (BYTE*)&value, sizeof(value));
	// TODO: catch error?
}

//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------
void CSettings::DeleteValue(LPCTSTR pName)
{
	_ASSERT(m_key);
	::RegDeleteValue(m_key, pName);
}

//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------
DWORD CSettings::GetDWORD(LPCTSTR pName, DWORD defaultValue)
{
	_ASSERT(m_key);
	DWORD value = 0, valueSize;
	valueSize = sizeof(value);
	if(ERROR_SUCCESS != ::RegQueryValueEx(m_key, pName, 0, 0, (BYTE*)&value, &valueSize))
	{
		value = defaultValue;
	}

	return value;
}

#endif