

#pragma once



namespace ddl
{

	struct memcursor
	{
		union AnyAddr
		{
			byte*		bp;
			uint32_t	u32p;
			uint64_t	u64p;
			uintptr_t	uptr;
		};

		byte*		bufferStart;
		byte*		bufferEnd;
		AnyAddr		bufferAddr;

		void init ( void* inBufferStart, unsigned inBufferSize )
		{
			bufferStart		= (byte*) inBufferStart;
			bufferEnd		= bufferStart + inBufferSize;
			bufferAddr.bp	= bufferStart;

			assert( bufferAddr.bp >= bufferStart );
			assert( bufferAddr.bp <= bufferEnd	 );
		}

		byte* start ( )
		{
			return bufferStart;
		}

		bool isAligned ( unsigned n )
		{
			return (bufferAddr.uptr % n) == 0;
		}

		void align ( unsigned n )
		{
			while ( !isAligned(n) )
				bufferAddr.uptr++;
		}
		
		void align32 ( )
		{
			align( 4 );
		}
		
		void align64 ( )
		{
			align( 8 );
		}

		unsigned left ( )
		{
			assert( bufferAddr.bp >= bufferStart );
			assert( bufferAddr.bp <= bufferEnd	 );
			return unsigned( bufferEnd - bufferAddr.bp );
		}

		unsigned used ( )
		{
			assert( bufferAddr.bp >= bufferStart );
			assert( bufferAddr.bp <= bufferEnd	 );
			return unsigned( bufferAddr.bp - bufferStart );
		}

		template < typename T >
		T* allocate ( bool initMem = true )
		{
			unsigned n = sizeof(T);
			if ( n > left() )
				return NULL;

			T* addr = (T*) bufferAddr.bp;
			bufferAddr.bp += n;

			if( initMem )
				::memset( addr, 0, n );

			return addr;
		}

		template < typename T >
		T* write ( T& t )
		{
			T* addr = allocate<T>( false );
			if( addr )
				::memcpy( addr, &t, sizeof T );
			return addr;
		}

		template < typename T >
		T* writeList ( T* first, std::vector<int>& list )
		{
			T* addr = NULL;
			
			for ( auto& i : list )
			{
				T* p = write( first[i] );
				addr = addr ? addr : p;
			}

			return addr;
		}

		char* writeString ( const char* s )
		{
			assert(s);

			unsigned n = unsigned( strlen(s) );
			assert( n < left() );

			char* addr = (char*) bufferAddr.bp;

			strcpy_s( addr, left(), s );

			bufferAddr.bp += n+1;
			return addr;
		}
	};

}


