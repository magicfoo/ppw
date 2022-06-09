

#include <windows.h>
#include <strsafe.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <filesystem>

#include <ppw_ipc_shm.h>



//#define USE_GLOBAL_MAPPING




static bool adjust_process_to_privilege ( const char* inName )
{
	HANDLE tokenH;
	TOKEN_PRIVILEGES tp;
	LUID luid;
		
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenH))
		return false;

	if (!LookupPrivilegeValue(NULL, inName, &luid))
		return false;

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(tokenH, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),(PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
		return false;

	// The token does not have the specified privilege
	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
		return false;

	CloseHandle(tokenH);

	return true;
}



static void outputLastError ( LPTSTR lpszFunction ) 
{ 
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
	StringCchPrintf((LPTSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"), 
		lpszFunction, dw, lpMsgBuf); 
	OutputDebugString( (LPCTSTR)lpDisplayBuf );

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}



bool
ppw::createPagingFile (	PagingFile&		outPF,
						const char*		inPath,
						size_t			inBSize,
						const char*		inName,
						bool			inForceResizing		)
{
	assert( inPath );

	HANDLE			fileH;
	LARGE_INTEGER	fileSize;
	HANDLE			mapH;
	std::string		mapName;
	char			createdPath[512];

	if ( inName )
	{
		#ifdef USE_GLOBAL_MAPPING
		mapName = "Global\\" + std::string(inName);
		#else
		mapName = "Local\\"  + std::string(inName);
		#endif
	}

#ifdef USE_GLOBAL_MAPPING
	static bool se_global_adjusted = false;
	static bool se_global_granted  = false;

	if ( !se_global_adjusted )
	{
		se_global_granted  = adjust_process_to_privilege( SE_CREATE_GLOBAL_NAME );
		se_global_adjusted = true;
	}

	if ( !se_global_granted )
		return false;
#endif

	fileH = CreateFile(
				inPath,
				GENERIC_READ|GENERIC_WRITE,
				0,
				NULL,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL	);

	GetFinalPathNameByHandle( fileH, createdPath, sizeof(createdPath)-16, FILE_NAME_NORMALIZED );

	if ( fileH == INVALID_HANDLE_VALUE )
	{
		outputLastError("CreateFile");
		return false;
	}

	if ( GetFileSizeEx(fileH,&fileSize) == 0 )
	{
		outputLastError("GetFileSizeEx");
		CloseHandle( fileH );
		return false;
	}

	// even if CreateFileMapping can resize the paging file
	// one adjusts it manually here

	if ( fileSize.QuadPart != inBSize )
	{
		if ( fileSize.QuadPart == 0 || inForceResizing )
		{
			fileSize.QuadPart = inBSize;

			if ( SetFilePointerEx(fileH,fileSize,NULL,FILE_BEGIN) == 0 )
			{
				CloseHandle( fileH );
				return false;
			}

			if ( SetEndOfFile(fileH) == 0 )
			{
				CloseHandle( fileH );
				return false;
			}
		}
	}

	mapH = CreateFileMapping(
				fileH,
				NULL,
				PAGE_READWRITE,
				0, 0,
				mapName.c_str()	);

	if ( mapH == NULL )
	{
		outputLastError("CreateFileMapping");
		CloseHandle( fileH );
		return false;
	}

	outPF.filePath		= inPath;
	outPF.fileH			= fileH;
	outPF.fileBSize		= inBSize;
	outPF.mapName		= mapName;
	outPF.mapH			= mapH;
	outPF.isReadOnly	= false;

	return true;
}
	

	
bool
ppw::openPagingFile	(	PagingFile&		outPF,
						const char*		inPath,
						const char*		inName,
						bool			inReadOnly	)
{
	assert( inPath );

	HANDLE			mapH;
	std::string		mapName;
	DWORD			mapAccess;

	if ( inName )
	{
		#ifdef USE_GLOBAL_MAPPING
		mapName = "Global\\" + std::string(inName);
		#else
		mapName = "Local\\"  + std::string(inName);
		#endif
	}

	if ( inReadOnly )
		mapAccess = FILE_MAP_READ;
	else
		mapAccess = FILE_MAP_WRITE;

	mapH = OpenFileMapping(
				mapAccess,
				0,
				mapName.c_str()	);

	if ( mapH == NULL )
	{
		outputLastError("OpenFileMapping");
		return false;
	}

	outPF.filePath		= inPath;
	outPF.fileH			= NULL;
	outPF.fileBSize		= 0;
	outPF.mapName		= mapName;
	outPF.mapH			= mapH;
	outPF.isReadOnly	= inReadOnly;

	return true;
}
	


void
ppw::closePagingFile ( PagingFile& ioPF )
{
	unmapPagingFile( ioPF );

	if ( ioPF.mapH )
	{
		CloseHandle( ioPF.mapH );
		ioPF.mapH = NULL;
	}

	if ( ioPF.fileH )
	{
		CloseHandle( ioPF.fileH );
		ioPF.fileH = NULL;
	}
}



bool
ppw::mapPagingFile ( PagingFile& ioPF, uintptr_t inAddr )
{
	void* mapAddr;

	assert( ioPF.mapH );

	mapAddr = MapViewOfFileEx(
					ioPF.mapH,
					ioPF.isReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE,
					0, 0, 0,
					(void*)inAddr	);

	if ( mapAddr == NULL )
	{
		outputLastError("MapViewOfFileEx");
		return false;
	}

	assert( !inAddr || inAddr == uintptr_t(mapAddr) );

	MEMORY_BASIC_INFORMATION info;
	SIZE_T szBufferSize = ::VirtualQueryEx(::GetCurrentProcess(), mapAddr, &info, sizeof(info));

	ioPF.mapAddr	= uintptr_t( mapAddr );
	ioPF.mapBSize	= info.RegionSize;

	return true;
}



void
ppw::flushPagingFile ( PagingFile& ioPF )
{
	if ( !ioPF.isReadOnly )
	{
		if ( ioPF.mapAddr )
		{
			FlushViewOfFile( (void*) ioPF.mapAddr, 0 );
		}

		if ( ioPF.fileH )
		{
			FlushFileBuffers( ioPF.fileH );
		}
	}
}


void
ppw::unmapPagingFile ( PagingFile& ioPF )
{
	flushPagingFile( ioPF );

	if ( ioPF.mapAddr )
	{
		UnmapViewOfFile( (void*) ioPF.mapAddr );
		ioPF.mapAddr = NULL;
	}
}


