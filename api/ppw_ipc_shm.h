/*
 *
 * FatModel: An empowered data model.
 *
 * Copyright 2017 Gerald Gainant.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#pragma once

#include <windows.h>
#include <filesystem>


namespace ppw
{

	struct PagingFile
	{
		using fs_path = std::experimental::filesystem::path;

		bool			isReadOnly;

		fs_path			filePath;
		HANDLE			fileH;
		size_t			fileBSize;

		std::string		mapName;
		HANDLE			mapH;
		uintptr_t 		mapAddr;
		size_t			mapBSize;
	};


	bool	createPagingFile	(	PagingFile&		outPF,
									const char*		inPath,
									size_t			inBSize,
									const char*		inName,
									bool			inForceResizing		);

	bool	openPagingFile		(	PagingFile&		outPF,
									const char*		inPath,
									const char*		inName,
									bool			inReadOnly			);

	void	closePagingFile		(	PagingFile&		ioPF				);

	bool	mapPagingFile		(	PagingFile&		ioPF,
									uintptr_t		inAddr				);

	void	flushPagingFile		(	PagingFile&		ioPF				);
		
	void	unmapPagingFile		(	PagingFile&		ioPF				);

}



