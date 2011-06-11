/******************************************************************
 *
 * Project: Helper
 * File: Bookmark.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Implements a bookmark system, with both bookmark folders
 * and bookmarks.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include <algorithm>
#include "Bookmark.h"
#include "RegistrySettings.h"
#include "Helper.h"


CBookmark::CBookmark(const std::wstring &strName,const std::wstring &strLocation,const std::wstring &strDescription) :
	m_strName(strName),
	m_strLocation(strLocation),
	m_strDescription(strDescription),
	m_iVisitCount(0)
{
	CoCreateGuid(&m_guid);
	GetSystemTimeAsFileTime(&m_ftCreated);
}

CBookmark::~CBookmark()
{

}

std::wstring CBookmark::GetName() const
{
	return m_strName;
}

std::wstring CBookmark::GetLocation() const
{
	return m_strLocation;
}

std::wstring CBookmark::GetDescription() const
{
	return m_strDescription;
}

void CBookmark::SetName(const std::wstring &strName)
{
	m_strName = strName;

	CBookmarkItemNotifier::GetInstance().NotifyObserversBookmarkItemModified(m_guid);
}

void CBookmark::SetLocation(const std::wstring &strLocation)
{
	m_strLocation = strLocation;

	CBookmarkItemNotifier::GetInstance().NotifyObserversBookmarkItemModified(m_guid);
}

void CBookmark::SetDescription(const std::wstring &strDescription)
{
	m_strDescription = strDescription;

	CBookmarkItemNotifier::GetInstance().NotifyObserversBookmarkItemModified(m_guid);
}

GUID CBookmark::GetGUID() const
{
	return m_guid;
}

int CBookmark::GetVisitCount() const
{
	return m_iVisitCount;
}

FILETIME CBookmark::GetDateLastVisited() const
{
	return m_ftLastVisited;
}

void CBookmark::UpdateVisitCount()
{
	++m_iVisitCount;
	GetSystemTimeAsFileTime(&m_ftLastVisited);

	CBookmarkItemNotifier::GetInstance().NotifyObserversBookmarkItemModified(m_guid);
}

FILETIME CBookmark::GetDateCreated() const
{
	return m_ftCreated;
}

FILETIME CBookmark::GetDateModified() const
{
	return m_ftModified;
}

CBookmarkFolder CBookmarkFolder::Create(const std::wstring &strName)
{
	return CBookmarkFolder(strName,INITIALIZATION_TYPE_NORMAL);
}

CBookmarkFolder *CBookmarkFolder::CreateNew(const std::wstring &strName)
{
	return new CBookmarkFolder(strName,INITIALIZATION_TYPE_NORMAL);
}

CBookmarkFolder CBookmarkFolder::UnserializeFromRegistry(const std::wstring &strKey)
{
	return CBookmarkFolder(strKey,INITIALIZATION_TYPE_REGISTRY);
}

CBookmarkFolder::CBookmarkFolder(const std::wstring &str,InitializationType_t InitializationType)
{
	switch(InitializationType)
	{
	case INITIALIZATION_TYPE_REGISTRY:
		InitializeFromRegistry(str);
		break;

	default:
		Initialize(str);
		break;
	}
}

CBookmarkFolder::~CBookmarkFolder()
{

}

void CBookmarkFolder::Initialize(const std::wstring &strName)
{
	CoCreateGuid(&m_guid);

	m_strName = strName;
	m_nChildFolders = 0;

	GetSystemTimeAsFileTime(&m_ftCreated);

	m_ftModified = m_ftCreated;
}

void CBookmarkFolder::InitializeFromRegistry(const std::wstring &strKey)
{
	HKEY hKey;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER,strKey.c_str(),0,KEY_READ,&hKey);

	if(lRes == ERROR_SUCCESS)
	{
		/* TODO: Write GUID. */
		//NRegistrySettings::ReadDwordFromRegistry(hKey,_T("ID"),reinterpret_cast<DWORD *>(&m_ID));
		NRegistrySettings::ReadStringFromRegistry(hKey,_T("Name"),m_strName);
		NRegistrySettings::ReadDwordFromRegistry(hKey,_T("DateCreatedLow"),&m_ftCreated.dwLowDateTime);
		NRegistrySettings::ReadDwordFromRegistry(hKey,_T("DateCreatedHigh"),&m_ftCreated.dwHighDateTime);
		NRegistrySettings::ReadDwordFromRegistry(hKey,_T("DateModifiedLow"),&m_ftModified.dwLowDateTime);
		NRegistrySettings::ReadDwordFromRegistry(hKey,_T("DateModifiedHigh"),&m_ftModified.dwHighDateTime);

		TCHAR szSubKeyName[256];
		DWORD dwSize = SIZEOF_ARRAY(szSubKeyName);
		int iIndex = 0;

		while(RegEnumKeyEx(hKey,iIndex,szSubKeyName,&dwSize,NULL,NULL,NULL,NULL) == ERROR_SUCCESS)
		{
			TCHAR szSubKey[256];
			StringCchPrintf(szSubKey,SIZEOF_ARRAY(szSubKey),_T("%s\\%s"),strKey.c_str(),szSubKeyName);

			if(CheckWildcardMatch(_T("BookmarkFolder_*"),szSubKeyName,FALSE))
			{
				CBookmarkFolder BookmarkFolder = CBookmarkFolder::UnserializeFromRegistry(szSubKey);
				m_ChildList.push_back(BookmarkFolder);
			}
			else if(CheckWildcardMatch(_T("Bookmark_*"),szSubKeyName,FALSE))
			{
				/* TODO: Create bookmark. */
			}

			dwSize = SIZEOF_ARRAY(szSubKeyName);
			iIndex++;
		}

		RegCloseKey(hKey);
	}
}

void CBookmarkFolder::SerializeToRegistry(const std::wstring &strKey)
{
	HKEY hKey;
	LONG lRes = RegCreateKeyEx(HKEY_CURRENT_USER,strKey.c_str(),
	0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,NULL);

	if(lRes == ERROR_SUCCESS)
	{
		/* These details don't need to be saved for the root bookmark. */
		/* TODO: Read GUID. */
		//NRegistrySettings::SaveDwordToRegistry(hKey,_T("ID"),m_ID);
		NRegistrySettings::SaveStringToRegistry(hKey,_T("Name"),m_strName.c_str());
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateCreatedLow"),m_ftCreated.dwLowDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateCreatedHigh"),m_ftCreated.dwHighDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateModifiedLow"),m_ftModified.dwLowDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateModifiedHigh"),m_ftModified.dwHighDateTime);

		int iItem = 0;

		for each(auto Variant in m_ChildList)
		{
			TCHAR szSubKey[256];

			if(CBookmarkFolder *pBookmarkFolder = boost::get<CBookmarkFolder>(&Variant))
			{
				StringCchPrintf(szSubKey,SIZEOF_ARRAY(szSubKey),_T("%s\\BookmarkFolder_%d"),strKey.c_str(),iItem);
				pBookmarkFolder->SerializeToRegistry(szSubKey);
			}
			else if(CBookmark *pBookmark = boost::get<CBookmark>(&Variant))
			{
				StringCchPrintf(szSubKey,SIZEOF_ARRAY(szSubKey),_T("%s\\Bookmark_%d"),strKey.c_str(),iItem);

				/* TODO: Serialize. */
			}

			iItem++;
		}

		RegCloseKey(hKey);
	}
}

std::wstring CBookmarkFolder::GetName() const
{
	return m_strName;
}

void CBookmarkFolder::SetName(const std::wstring &strName)
{
	m_strName = strName;

	CBookmarkItemNotifier::GetInstance().NotifyObserversBookmarkItemModified(m_guid);
}

GUID CBookmarkFolder::GetGUID() const
{
	return m_guid;
}

FILETIME CBookmarkFolder::GetDateCreated() const
{
	return m_ftCreated;
}

FILETIME CBookmarkFolder::GetDateModified() const
{
	return m_ftModified;
}

void CBookmarkFolder::InsertBookmark(const CBookmark &Bookmark)
{
	InsertBookmark(Bookmark,m_ChildList.size());
}

void CBookmarkFolder::InsertBookmark(const CBookmark &Bookmark,std::size_t Position)
{
	if(Position > (m_ChildList.size() - 1))
	{
		m_ChildList.push_back(Bookmark);
	}
	else
	{
		auto itr = m_ChildList.begin();
		std::advance(itr,Position);
		m_ChildList.insert(itr,Bookmark);
	}

	GetSystemTimeAsFileTime(&m_ftModified);

	CBookmarkItemNotifier::GetInstance().NotifyObserversBookmarkAdded(Bookmark);
}

void CBookmarkFolder::InsertBookmarkFolder(const CBookmarkFolder &BookmarkFolder)
{
	InsertBookmarkFolder(BookmarkFolder,m_ChildList.size());
}

void CBookmarkFolder::InsertBookmarkFolder(const CBookmarkFolder &BookmarkFolder,std::size_t Position)
{
	if(Position > (m_ChildList.size() - 1))
	{
		m_ChildList.push_back(BookmarkFolder);
	}
	else
	{
		auto itr = m_ChildList.begin();
		std::advance(itr,Position);
		m_ChildList.insert(itr,BookmarkFolder);
	}

	m_nChildFolders++;

	GetSystemTimeAsFileTime(&m_ftModified);

	CBookmarkItemNotifier::GetInstance().NotifyObserversBookmarkFolderAdded(BookmarkFolder);
}

std::list<boost::variant<CBookmarkFolder,CBookmark>>::iterator CBookmarkFolder::begin()
{
	return m_ChildList.begin();
}

std::list<boost::variant<CBookmarkFolder,CBookmark>>::iterator CBookmarkFolder::end()
{
	return m_ChildList.end();
}

std::list<boost::variant<CBookmarkFolder,CBookmark>>::const_iterator CBookmarkFolder::begin() const
{
	return m_ChildList.begin();
}

std::list<boost::variant<CBookmarkFolder,CBookmark>>::const_iterator CBookmarkFolder::end() const
{
	return m_ChildList.end();
}

bool CBookmarkFolder::HasChildFolder() const
{
	if(m_nChildFolders > 0)
	{
		return true;
	}

	return false;
}

CBookmarkItemNotifier::CBookmarkItemNotifier()
{

}

CBookmarkItemNotifier::~CBookmarkItemNotifier()
{

}

CBookmarkItemNotifier& CBookmarkItemNotifier::GetInstance()
{
	static CBookmarkItemNotifier bin;
	return bin;
}

void CBookmarkItemNotifier::AddObserver(NBookmark::IBookmarkItemNotification *pbin)
{
	m_listObservers.push_back(pbin);
}

void CBookmarkItemNotifier::RemoveObserver(NBookmark::IBookmarkItemNotification *pbin)
{
	auto itr = std::find_if(m_listObservers.begin(),m_listObservers.end(),
		[pbin](const NBookmark::IBookmarkItemNotification *pbinCurrent){return pbinCurrent == pbin;});

	if(itr != m_listObservers.end())
	{
		m_listObservers.erase(itr);
	}
}

void CBookmarkItemNotifier::NotifyObserversBookmarkItemModified(const GUID &guid)
{
	NotifyObservers(NOTIFY_BOOKMARK_ITEM_MODIFIED,guid);
}

void CBookmarkItemNotifier::NotifyObserversBookmarkAdded(const CBookmark &Bookmark)
{
	NotifyObservers(NOTIFY_BOOKMARK_ADDED,Bookmark);
}

void CBookmarkItemNotifier::NotifyObserversBookmarkFolderAdded(const CBookmarkFolder &BookmarkFolder)
{
	NotifyObservers(NOTIFY_BOOKMARK_FOLDER_ADDED,BookmarkFolder);
}

void CBookmarkItemNotifier::NotifyObserversBookmarkRemoved(const GUID &guid)
{
	NotifyObservers(NOTIFY_BOOKMARK_REMOVED,guid);
}

void CBookmarkItemNotifier::NotifyObserversBookmarkFolderRemoved(const GUID &guid)
{
	NotifyObservers(NOTIFY_BOOMARK_FOLDER_REMOVED,guid);
}

void CBookmarkItemNotifier::NotifyObservers(NotificationType_t NotificationType,
	boost::variant<const GUID &,const CBookmark &,const CBookmarkFolder &> variantData)
{
	for each(auto pbin in m_listObservers)
	{
		switch(NotificationType)
		{
		case NOTIFY_BOOKMARK_ITEM_MODIFIED:
			pbin->OnBookmarkItemModified(boost::get<const GUID &>(variantData));
			break;

		case NOTIFY_BOOKMARK_ADDED:
			pbin->OnBookmarkAdded(boost::get<const CBookmark &>(variantData));
			break;

		case NOTIFY_BOOKMARK_FOLDER_ADDED:
			pbin->OnBookmarkFolderAdded(boost::get<const CBookmarkFolder &>(variantData));
			break;

		case NOTIFY_BOOKMARK_REMOVED:
			pbin->OnBookmarkRemoved(boost::get<const GUID &>(variantData));
			break;

		case NOTIFY_BOOMARK_FOLDER_REMOVED:
			pbin->OnBookmarkFolderRemoved(boost::get<const GUID &>(variantData));
			break;
		}
	}
}