

#if defined(FM_USING_ASSERT) || defined(FM_USING_ALL)

#define fm_assert( C ) \
			assert( C )

#endif



#if defined(FM_USING_RETURNS) || defined(FM_USING_ALL)

#define return_value_if( C, V ) \
			if ( (C) ) { return (V); }

#define return_false_if( C ) \
			if ( (C) ) { return false; }

#define return_true_if( C ) \
			if ( (C) ) { return true; }

#define return_null_if( C ) \
			if ( (C) ) { return NULL; }

#define return_if( C ) \
			if ( (C) ) { return; }

#endif


template < typename T >
bool fm_isAlignedBytes ( T v, int n )
{
	return (fm_uintptr(v) & (n - 1)) == 0;
}


template < typename T >
bool fm_isalignedBits ( T v, int b )
{
	return fm_isAlignedBytes(v, 1 << b)
}


template < typename T >
T fm_alignBytes ( T v, int n )
{
	fm_uintptr av = (fm_uintptr(v) + n - 1) & (~(n - 1));
	return (T)av;
}


template < typename T >
T fm_alignBits ( T v, int b )
{
	return fm_alignBytes(v, 1 << b);
}


#define fm_zeroArray( AR ) \
			::memset( (void*)AR, 0, sizeof(AR) );


template < typename T >
void fm_zeroPointed( T* t )
{
	::memset(t, 0, sizeof(T));
}


#define fm_sizeofArray( AR ) \
			(sizeof(AR)/sizeof(AR[0]))


template < typename T >
T fm_max( T t0, T t1 )
{
	return t0 > t1 ? t0 : t1;
}


template < typename T >
T fm_min( T t0, T t1 )
{
	return t0 < t1 ? t0 : t1;
}


const char* fm_va ( const char* fmt, ... );


fm_uint		fm_util_crc32					( const void *buf, fm_uint size, fm_uint crc = 0 );
fm_uint		fm_util_crc32_str				( fm_string str,				 fm_uint crc = 0 );

void		fm_util_getProcessTime			( long* outKernelMs, long* outUserMs );


