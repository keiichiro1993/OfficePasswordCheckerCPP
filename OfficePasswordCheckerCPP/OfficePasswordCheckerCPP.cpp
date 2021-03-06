//+===================================================================
//
//  新形式の Office ファイルがパスワード/RMSで暗号化されているかどうかを確認する
//  Following code verifies if the Office document is encrypted by RMS/Password
//  only for OOXML files.
//  
//  以下の公開情報をもとに作成
//  built based on the sample code in the following document.
//  <EnumAll Sample>
//  https://docs.microsoft.com/ja-jp/windows/desktop/Stg/enumall-sample
//
//+===================================================================


#define UNICODE
#define _UNICODE

#include "pch.h"
#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <atlstr.h>
#include <vector>

#pragma comment( lib, "ole32.lib" )

//+-------------------------------------------------------------------
//
//  DisplayStorageTree
//
//  この関数では、引数に与えられたList(vector)に対してストリーム名の一覧を返します。
//  Dump all the property sets in the given storage and recursively in
//  all its child objects.
//
//+-------------------------------------------------------------------

void DisplayStorageTree(const WCHAR *pwszStorageName, IStorage *pStg, std::vector<std::wstring> *pStreamList)
{
	IPropertySetStorage *pPropSetStg = NULL;
	IStorage *pStgChild = NULL;
	WCHAR *pwszChildStorageName = NULL;
	IEnumSTATSTG *penum = NULL;
	HRESULT hr = S_OK;
	STATSTG statstg;

	memset(&statstg, 0, sizeof(statstg));

	try
	{
		// Dump the property sets at this storage level

		hr = pStg->QueryInterface(IID_IPropertySetStorage,
			reinterpret_cast<void**>(&pPropSetStg));
		if (FAILED(hr))
			throw
			L"Failed IStorage::QueryInterface(IID_IPropertySetStorage)";

		// Get an enumerator for this storage.

		hr = pStg->EnumElements(NULL, NULL, NULL, &penum);
		if (FAILED(hr)) throw L"failed IStorage::Enum";

		// Get the name of the first element (stream/storage)
		// in the enumeration.  As usual, 'Next' will return
		// S_OK if it returns an element of the enumerator,
		// S_FALSE if there are no more elements, and an
		// error otherwise.

		hr = penum->Next(1, &statstg, 0);

		// Loop through all the child objects of this storage.

		while (S_OK == hr)
		{
			// Verify that this is a storage that is not a property
			// set, because the property sets are displayed above).
			// If the first character of its name is the '\005'
			// reserved character, it is a stream /storage property
			// set.

			//ストリーム名の一覧を表示するには、このコメントアウトを解除します。
			//wprintf(L"\nParentName: %s Stream Name: %s\n", pwszStorageName, statstg.pwcsName);
			
			pStreamList->push_back(statstg.pwcsName);
			// Indicates normal storage, not a propset.
			// Open the storage.

			hr = pStg->OpenStorage(statstg.pwcsName,
				NULL,
				STGM_READ | STGM_SHARE_EXCLUSIVE,
				NULL, 0,
				&pStgChild);
			if (!FAILED(hr)) {
				// Build the name.
				std::wstring tmpPath(pwszStorageName);
				std::wstring tmpChildPath(statstg.pwcsName);
				if (tmpChildPath[0] == '\x6' || tmpChildPath[0] == '\x5')
				{
					tmpChildPath.erase(0, 1);
				}
				std::wstring path = tmpPath + L"\\" + tmpChildPath;
				const WCHAR * pwszPath = path.c_str();
				// Dump all property sets under this child storage.
				DisplayStorageTree(pwszPath, pStgChild, pStreamList);

				pStgChild->Release();
				pStgChild = NULL;

				delete[] pwszChildStorageName;
				pwszChildStorageName = NULL;
			}

			// Move to the next element in the enumeration of 
			// this storage.

			CoTaskMemFree(statstg.pwcsName);
			statstg.pwcsName = NULL;

			hr = penum->Next(1, &statstg, 0);
		}
		if (FAILED(hr)) throw L"failed IEnumSTATSTG::Next";
	}
	catch (LPCWSTR pwszErrorMessage)
	{
		wprintf(L"Error in DumpStorageTree: %s (hr = %08x)\n",
			pwszErrorMessage, hr);
	}

	// Cleanup before returning.

	if (NULL != statstg.pwcsName)
		CoTaskMemFree(statstg.pwcsName);
	if (NULL != pStgChild)
		pStgChild->Release();
	if (NULL != pStg)
		pStg->Release();
	if (NULL != penum)
		penum->Release();
	if (NULL != pwszChildStorageName)
		delete[] pwszChildStorageName;

}

//+-------------------------------------------------------------------
//
//  IsRMSEncrypted/IsPasswordEncrypted
//
//  それぞれ、RMS 暗号化されている際に True, パスワード暗号化されている際に True
//  を返答する関数です。
//
//+-------------------------------------------------------------------

bool IsRMSEncrypted(std::vector<std::wstring> *pStreamList)
{
	return !(std::find(pStreamList->begin(), pStreamList->end(), L"DRMEncryptedDataSpace") == pStreamList->end());
}


bool IsPasswordEncrypted(std::vector<std::wstring> *pStreamList)
{
	return (!(std::find(pStreamList->begin(), pStreamList->end(), L"StrongEncryptionTransform") == pStreamList->end())
		&& !(std::find(pStreamList->begin(), pStreamList->end(), L"EncryptionInfo") == pStreamList->end()));
}

//+-------------------------------------------------------------------
//
//  wmain
//
//  Dump all the property sets in a file which is a storage.
//
//+-------------------------------------------------------------------

extern "C" void wmain(int cArgs, WCHAR *rgwszArgs[])
{
	HRESULT hr = S_OK;
	IStorage *pStg = NULL;
	std::vector<std::wstring> streamList;

	// Display usage information if necessary.

	if (1 == cArgs
		||
		0 == wcscmp(L"-?", rgwszArgs[1])
		||
		0 == wcscmp(L"/?", rgwszArgs[1]))
	{
		printf("\n"
			"Purpose:  Enumerate all stream nemes and\n"
			"          check if it's encrypted\n"
			"Usage:    *.exe <filename>\n"
			"E.g.:     PasswordChecker.exe word.doc\n"
			"\n");
		exit(0);
	}

	// Open the root storage.

	hr = StgOpenStorageEx(rgwszArgs[1],
		STGM_READ | STGM_SHARE_DENY_WRITE,
		STGFMT_ANY,
		0,
		NULL,
		NULL,
		IID_IStorage,
		reinterpret_cast<void**>(&pStg));

	// DisplayStorageTreeですべてのStorage/Stream名をstd::vectorに追加し、その後Is* 関数を利用して状態を確認する。

	if (FAILED(hr))
	{
		wprintf(L"Error: couldn't open storage \"%s\" (hr = %08x)\nPlease check the file specified is encrypted Office file.\n",
			rgwszArgs[1], hr);
	}
	else
	{
		printf("\nSeeking for all property sets ...\n");
		DisplayStorageTree(rgwszArgs[1], pStg, &streamList);

		wprintf(L"RMS encrypted: %d \n", IsRMSEncrypted(&streamList));
		wprintf(L"Password encrypted: %d \n", IsPasswordEncrypted(&streamList));

		pStg->Release();
	}
}