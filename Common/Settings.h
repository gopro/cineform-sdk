/*! @file settings.h

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

#pragma once
#include "StdAfx.h"

//-------------------------------------------------------------------------------------------
// class CSettings
//-------------------------------------------------------------------------------------------
class CSettings
{
public:
	CSettings();
	~CSettings();

public:
	BOOL		Open(HKEY key, LPCTSTR pSubKey);
	void		Close();

	CComBSTR	GetString(LPCTSTR pName, LPCTSTR defaultValue);
	void		PutString(LPCTSTR pName, LPCTSTR pValue);
	DWORD		GetDWORD(LPCTSTR pName, DWORD defaultValue);
	void		PutDWORD(LPCTSTR pName, DWORD value);

	void		DeleteValue(LPCTSTR pName);

	BOOL		IsOpen() { return m_key != 0; }
private:
	HKEY	m_key;

};

