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


#define CACHE_LINE		64  
#define CACHE_ALIGN		__declspec(align(CACHE_LINE))  

#define SLOT_BSIZE		4096

#define SUB_MAX			64
#define SUB_NOT_ID		0



namespace ppw
{

	struct IPCRingMessenger
	{
		enum LockStatus : unsigned
		{
			LOCK_Off = 0,
			LOCK_On,
		};

		enum SlotStatus : unsigned
		{
			SLOT_Free = 0,
			SLOT_Writing,
			SLOT_Posted,
			SLOT_Continued,
			SLOT_Aborted,
		};

		CACHE_ALIGN struct Slot
		{
			SlotStatus			status;
			unsigned			fromId;
			unsigned			toId;
			unsigned			dataId;
			volatile int64_t	cliMask;
			Slot*				dataCnt;
			unsigned			dataLength;
			byte				dataBytes[ SLOT_BSIZE - 9*4 ];
		};

		static_assert( sizeof(Slot) == SLOT_BSIZE, "Inconsistent Slot size!" );

		struct Client
		{
			using Buffer = std::vector<byte>;

			unsigned			id;
			unsigned			ith;
			Slot*				polled;
			Buffer				buffer;
		};


		unsigned				magicId;
		LockStatus				lockStatus;

		Slot*					firstSlot;
		Slot*					endSlot;
		Slot* volatile			headSlot;

		unsigned				clientId[ SUB_MAX ];
		Slot*					clientSlot[ SUB_MAX ];
		int64_t					cliMask;



		static IPCRingMessenger*	init			 (	Client&			outCli,
														unsigned		inID,
														uint64_t*		inAddrLo,
														uint64_t*		inAddrHi		);

		static IPCRingMessenger*	connect			(	Client&			outCli,
														unsigned		inID,
														uint64_t*		inAddrLo,
														uint64_t*		inAddrHi		);

		bool						disconnect		(	Client&			inCli			);

		bool						tryLock			(									);
		void						lock			(									);
		bool						unlock			(									);

		bool						post			(	Client&			inCli,
														unsigned		inToID,
														unsigned		inDataId,
														unsigned		inDataLength,
														void*			inDataBytes		);

		Slot*						poll			(	Client&			inCli			);

		bool						loadBytes		(	Client&			inCli			);
		void*						getBytes		(	Client&			inCli			);
		unsigned					countBytes		(	Client&			inCli			);
		void						clearBytes		(	Client&			inCli			);
		template < typename T >
		T*							bytesAsType		(	Client&			inCli			);
	};


}



