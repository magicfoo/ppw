
#include <Windows.h>
#include <stdint.h>
#include <assert.h>
#include <vector>

#include <ppw_ipc_msg.h>


using namespace ppw;


ppw::IPCRingMessenger*
ppw::IPCRingMessenger::init		(	Client&			outCli,
									unsigned		inID,
									uint64_t*		inAddrLo,
									uint64_t*		inAddrHi	)
{
	assert( inID != SUB_NOT_ID );
	assert( inAddrLo );
	assert( inAddrHi );
	assert( inAddrHi > inAddrLo );

	IPCRingMessenger*	rb;
	uintptr_t		base;
	uintptr_t		slotlo;
	uintptr_t		slothi;
	uintptr_t		slotcnt;

	rb		= (IPCRingMessenger*) inAddrLo;
	base	= ((sizeof(IPCRingMessenger) + 63) >> 6) << 6;
	slotlo  = uintptr_t(inAddrLo) + base;
	slotcnt = (uintptr_t(inAddrHi) - slotlo) / sizeof(IPCRingMessenger::Slot);
	slothi  = slotlo + slotcnt * sizeof(IPCRingMessenger::Slot);
	
	rb->magicId		= 0;
	rb->lockStatus	= LOCK_On;

	rb->firstSlot	= (Slot*) slotlo;
	rb->endSlot		= (Slot*) slothi;
	rb->headSlot	= rb->firstSlot;
	rb->cliMask		= 1;

	for ( unsigned i=0 ; i < SUB_MAX ; i++ )
	{
		rb->clientId[i]   = i == 0 ? inID         : SUB_NOT_ID;
		rb->clientSlot[i] = i == 0 ? rb->headSlot : nullptr;
	}

	ZeroMemory( (void*)slotlo, slothi-slotlo );

	rb->headSlot->status = SLOT_Free;

	outCli.id		= inID;
	outCli.ith		= 0;
	outCli.polled	= nullptr;

	rb->magicId		= 0x31415926;
	rb->lockStatus	= LOCK_Off;

	return rb;
}



ppw::IPCRingMessenger*
ppw::IPCRingMessenger::connect	(	Client&			outCli,
									unsigned		inID,
									uint64_t*		inAddrLo,
									uint64_t*		inAddrHi	)
{
	assert( inID != SUB_NOT_ID );
	assert( inAddrLo );
	assert( inAddrHi );
	assert( inAddrHi > inAddrLo );

	IPCRingMessenger*	rb;
	bool			already_registered;
	bool			registration_done;

	rb = (IPCRingMessenger*) inAddrLo;

	if ( rb->magicId != 0x31415926 )
		return nullptr;

	rb->lock();

	already_registered	= false;
	registration_done	= false;

	// check registration unicity

	for ( unsigned i=0 ; i < SUB_MAX ; i++ )
	{
		if ( rb->clientId[i] == inID )
		{
			already_registered = true;
			break;
		}
	}

	// try to register

	if ( !already_registered )
	{
		for ( unsigned i=0 ; i < SUB_MAX ; i++ )
		{
			if ( rb->clientId[i] == SUB_NOT_ID )
			{
				outCli.id		= inID;
				outCli.ith		= i;
				outCli.polled	= nullptr;

				registration_done = true;
				break;
			}
		}
	}

	rb->unlock();

	if ( registration_done )
	{
		rb->clientId[ outCli.ith ]    = outCli.id;
		rb->clientSlot[ outCli.ith ]  = rb->headSlot;
		_interlockedbittestandset64( &rb->cliMask, outCli.ith );
	}

	return registration_done ? rb : nullptr;
}


bool
ppw::IPCRingMessenger::disconnect ( Client& inCli )
{
	if ( inCli.id == SUB_NOT_ID )
		return false;

	assert( clientId[ inCli.ith ] == inCli.id );

	lock();

	// prevents posts with this subscriber flag
	_interlockedbittestandreset64( &cliMask, inCli.ith );

	// flush posts
	while ( poll(inCli) ) {}

	clientId[ inCli.ith ]   = SUB_NOT_ID;
	clientSlot[ inCli.ith ] = nullptr;

	inCli.id	 = SUB_NOT_ID;
	inCli.ith	 = 0;
	inCli.polled = nullptr;

	unlock();

	return true;
}


bool
ppw::IPCRingMessenger::tryLock ( )
{
	return _InterlockedCompareExchange( (long*)&lockStatus, LOCK_On, LOCK_Off ) == LOCK_Off;
}


void
ppw::IPCRingMessenger::lock ( )
{
	while ( !tryLock() ) { }
}


bool
ppw::IPCRingMessenger::unlock ( )
{
	return _InterlockedExchange( (long*)&lockStatus, LOCK_Off ) == LOCK_On;
}


static IPCRingMessenger::Slot* rb_alloc_slot ( IPCRingMessenger* rb, unsigned trycnt )
{
	IPCRingMessenger::Slot* s;
	IPCRingMessenger::Slot* h;

	for ( ; trycnt ; trycnt-- )
	{
		s = rb->headSlot;

		if ( _InterlockedCompareExchange( (long*)&s->status, IPCRingMessenger::SLOT_Writing, IPCRingMessenger::SLOT_Free ) == IPCRingMessenger::SLOT_Free )
		{
			// got a slot!

			// update the head
			h = s + 1;

			if ( h == rb->endSlot )
				h = rb->firstSlot;

			rb->headSlot = h;

			return s;
		}
	}

	return nullptr;
}


static void rb_dealloc_slot ( IPCRingMessenger::Slot* s )
{
	assert( s );

	// recursively deallocs sub slots

	if ( s->dataCnt )
	{
		rb_dealloc_slot( s->dataCnt );
		s->dataCnt = nullptr;
	}

	s->status = IPCRingMessenger::SLOT_Free;
}


static void rb_abort_slot ( IPCRingMessenger::Slot* s )
{
	assert( s );

	// recursively aborts sub slots

	if ( s->dataCnt )
	{
		rb_abort_slot( s->dataCnt );
		s->dataCnt = nullptr;
	}

	s->status = IPCRingMessenger::SLOT_Aborted;
}


bool
ppw::IPCRingMessenger::post ( Client& inCli, unsigned inToID, unsigned inDataId, unsigned inDataLength, void* inDataBytes )
{
	if ( inCli.id == SUB_NOT_ID )
		return false;

	if ( !inDataLength )
		return false;

	if ( !inDataBytes )
		return false;

	unsigned	slot_bs;
	char*		data;
	Slot*		head;
	Slot*		last;
	Slot*		s;
	bool		aborted;
	
	slot_bs		= sizeof(Slot::dataBytes);
	head		= nullptr;
	last		= nullptr;
	data		= (char*) inDataBytes;
	aborted		= false;

	while ( inDataLength )
	{
		s = rb_alloc_slot( this, 32 );

		if ( !s )
		{
			aborted = true;
			break;
		}

		s->fromId		= inCli.id;
		s->toId			= inToID;
		s->cliMask		= cliMask;
		s->dataId		= inDataId;
		s->dataCnt		= nullptr;
		s->dataLength	= __min( inDataLength, slot_bs );

		::memcpy( s->dataBytes, data, s->dataLength );

		inDataLength -= s->dataLength;
		data         += s->dataLength;

		if ( last )
		{
			// chains the sub slots
			s->status = SLOT_Continued;
			last->dataCnt = s;
		}

		last = s;

		if ( !head )
			head = s;
	}
	
	if ( aborted )
	{
		if ( head )
			rb_abort_slot( head );
		return false;
	}

	assert( head );
	assert( head->status == SLOT_Writing );

	_InterlockedExchange( (long*)&head->status, SLOT_Posted );

	return true;
}



ppw::IPCRingMessenger::Slot*
ppw::IPCRingMessenger::poll ( Client& inCli )
{
	if ( inCli.id == SUB_NOT_ID )
		return nullptr;

	Slot*		call_slot;
	Slot*		s;
	BOOLEAN		b;

	call_slot	= clientSlot[ inCli.ith ];
	s			= call_slot;

	assert( s );

	for ( ;; )
	{
		if ( s->status == SLOT_Free || s->status == SLOT_Writing )
		{
			// nothing to read ...
			return nullptr;
		}

		else if ( s->status == SLOT_Posted )
		{
			// already polled?
			if ( inCli.polled == s )
			{
				b = _interlockedbittestandreset64( &s->cliMask, inCli.ith );
				assert( b );

				// recycles it?
				if ( s->cliMask == 0 )
				{
					rb_dealloc_slot( s );
				}
			}

			// polls it
			else
			{
				inCli.polled = s;
				return s;
			}
		}

		s += 1;

		if ( s == endSlot )
			s = firstSlot;

		clientSlot[ inCli.ith ] = s;
		inCli.polled = nullptr;

		// avoid infinite loop!

		if ( s == call_slot )
			return nullptr;
	}
}




bool
ppw::IPCRingMessenger::loadBytes ( Client& inCli )
{
	Slot*		cnt;
	size_t		sz;

	inCli.buffer.clear();

	if ( !inCli.polled )
		return false;

	// stores data to the client buffer

	for ( cnt=inCli.polled ; cnt ; cnt=cnt->dataCnt )
	{
		sz = inCli.buffer.size();
		inCli.buffer.resize( sz + cnt->dataLength );
		::memcpy( &inCli.buffer[sz], cnt->dataBytes, cnt->dataLength );
	}

	return true;
}


void*
ppw::IPCRingMessenger::getBytes ( Client& inCli )
{
	if ( inCli.buffer.empty() )
		return nullptr;
	else
		return inCli.buffer.data();
}


unsigned
ppw::IPCRingMessenger::countBytes ( Client& inCli )
{
	return (unsigned) inCli.buffer.size();
}


void
ppw::IPCRingMessenger::clearBytes ( Client& inCli )
{
	inCli.buffer.clear();
}


template < typename T >
T*
ppw::IPCRingMessenger::bytesAsType ( Client& inCli )
{
	if ( inCli.buffer.size() < sizeof(T) )
		return nullptr;

	return (T*) inCli.buffer.data();
}




#if 0


#define SLOT_BSIZE		2048

#define CLI_MAX			64
#define CLI_NOT_ID		0

//#define SINGLE_TH
#define DEBUG_TH
#define OUTPUT
//#define TIMED		1
#define DUMMY_TEST



void output ( const char* fmt, ... )
{
#if 1
	char buffer[256];
	va_list args;
	va_start (args, fmt);
	vsprintf_s (buffer,fmt, args);
	va_end (args);
	OutputDebugStringA( buffer );
#endif
}


void debug_output ( const char* fmt, ... )
{
#if defined(OUTPUT)
	char buffer[256];
	va_list args;
	va_start (args, fmt);
	vsprintf_s (buffer,fmt, args);
	va_end (args);
	OutputDebugStringA( buffer );
#endif
}


void debug_th_output ( const char* fmt, ... )
{
#if defined(DEBUG_TH) && defined(OUTPUT)
	char buffer[256];
	va_list args;
	va_start (args, fmt);
	vsprintf_s (buffer,fmt, args);
	va_end (args);
	OutputDebugStringA( buffer );
#endif
}


void PostDummyLongMessage ( IPCRingMessenger* rb, IPCRingMessenger::Client& cli )
{
#if defined(DUMMY_TEST)
	unsigned k = rand() % 16;

	if ( k != 7 )
		return;

	unsigned n = rand() % 32*1024 + 4*1024;
	byte*    bytes = (byte*) _malloca( n );
	assert( bytes );

	for ( unsigned i=0 ; i<n ; i++ )
		bytes[i] = i & 255;

	debug_th_output( "[%d] <post dummy %d bytes>\n", cli.ith, n );
	rb->post( cli, 0, 0xdeadf00d, n, bytes );
#endif // DUMMY_TEST
}


struct FactData
{
	static const unsigned DataID = 0x4545;

	unsigned	asked;
	unsigned	result;
	unsigned	tosolve;
};


void FactPost ( IPCRingMessenger* rb, IPCRingMessenger::Client& cli, FactData& f )
{
	unsigned toID;

#if defined(SINGLE_TH)
	toID = 0;
#else
	toID = rand() % 4;
#endif

	debug_output( "[%d] (post fact(%d) to solve by [%d])\n", cli.ith, f.asked, toID );

	rb->post( cli, toID, FactData::DataID, sizeof(f), &f );
}


void FactStart ( IPCRingMessenger* rb, IPCRingMessenger::Client& cli )
{
	FactData f;
	f.asked   = rand() % 10 + 1;
	f.result  = 1;
	f.tosolve = f.asked;
	
	FactPost( rb, cli, f );
}


void FactProcess ( IPCRingMessenger* rb, IPCRingMessenger::Client& cli )
{
	IPCRingMessenger::Slot*	s;
	FactData*				f;
	FactData				r;

	for ( ;; )
	{
		s = rb->poll( cli );

		if ( !s )
			return;

		if ( s->dataId != FactData::DataID )
			continue;

		if ( s->toId != cli.ith )
			continue;

		// process on step of the fact computation

		rb->loadBytes( cli, s );

		assert( rb->countBytes(cli) == sizeof(FactData) );
		assert( rb->getBytes(cli) );

		f = rb->bytesAsType<FactData>( cli );

		if ( f->tosolve == 1 )
		{
			output( "[%d] fact(%d) = %d\n", cli.ith, f->asked, f->result );

			FactStart( rb, cli );
		}
		else
		{
			debug_output( "[%d] (processing fact(%d) at step %d)\n", cli.ith, f->asked, f->tosolve );

			r.asked   = f->asked;
			r.result  = f->result * f->tosolve;
			r.tosolve = f->tosolve - 1;
			
			FactPost( rb, cli, r );
		}

		break;
	}
}


void server_th ( unsigned id, uint64_t* lo, uint64_t* hi )
{
	IPCRingMessenger*			rb;
	IPCRingMessenger::Client	cli;

	debug_output( "[%d] initializing ... \n", id );
	rb = IPCRingMessenger::init( cli, id, lo, hi	);
	debug_output( "[%x] initialized as #%d.\n", id, cli.ith );

	rb->halt( cli );
	std::this_thread::sleep_for( std::chrono::seconds(2) );
	rb->resume( cli );

	FactStart( rb, cli );

	for ( ;; )
	{
		#if defined(TIMED)
		std::this_thread::sleep_for( std::chrono::milliseconds( rand()%TIMED) );
		#endif

		FactProcess( rb, cli );
		PostDummyLongMessage( rb, cli );
	}
}



void cli_1_th ( unsigned id, uint64_t* lo, uint64_t* hi )
{
	IPCRingMessenger*			rb;
	IPCRingMessenger::Client	cli;

	debug_output( "[%x] connecting ... \n", id );
	while ( !(rb = IPCRingMessenger::connect( cli, id, lo, hi	) ) );
	debug_output( "[%x] connected as #%d.\n", id, cli.ith );

	for ( ;; )
	{
		#if defined(TIMED)
		std::this_thread::sleep_for( std::chrono::milliseconds( rand()%TIMED) );
		#endif

		FactProcess( rb, cli );
		PostDummyLongMessage( rb, cli );
	}
}


void cli_2_th ( unsigned id, uint64_t* lo, uint64_t* hi )
{
	IPCRingMessenger*			rb;
	IPCRingMessenger::Client	cli;

	debug_output( "[%x] connecting ... \n", id );
	while ( !(rb = IPCRingMessenger::connect( cli, id, lo, hi	) ) );
	debug_output( "[%x] connected as #%d.\n", id, cli.ith );

	for ( ;; )
	{
		#if defined(TIMED)
		std::this_thread::sleep_for( std::chrono::milliseconds( rand()%TIMED) );
		#endif

		FactProcess( rb, cli );
		PostDummyLongMessage( rb, cli );
	}
}


void cli_3_th ( unsigned id, uint64_t* lo, uint64_t* hi )
{
	IPCRingMessenger*			rb;
	IPCRingMessenger::Client	cli;

	debug_output( "[%x] connecting ... \n", id );
	while ( !(rb = IPCRingMessenger::connect( cli, id, lo, hi	) ) );
	debug_output( "[%x] connected as #%d.\n", id, cli.ith );

	for ( ;; )
	{
		#if defined(TIMED)
		std::this_thread::sleep_for( std::chrono::milliseconds( rand()%TIMED) );
		#endif

		FactProcess( rb, cli );
		PostDummyLongMessage( rb, cli );
	}
}



int main()
{
	std::vector< uint64_t > rb_heap;

	rb_heap.resize( 16*1024*1024 );

	std::thread t0( server_th, 0xf00, &rb_heap.front(), &rb_heap.back() );

#if !defined(SINGLE_TH)
	std::thread t1( cli_1_th, 0xba1, &rb_heap.front(), &rb_heap.back() );
	std::thread t2( cli_2_th, 0xba2, &rb_heap.front(), &rb_heap.back() );
	std::thread t3( cli_3_th, 0xba3, &rb_heap.front(), &rb_heap.back() );
#endif

	std::this_thread::sleep_for( std::chrono::seconds(20) );

	output( "\n\n\n***** TERMINATED ******\n" );
	
	exit(0);
	return 0;
}



#endif // 0