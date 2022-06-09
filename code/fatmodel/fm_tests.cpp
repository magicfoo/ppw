
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>


#ifdef NDEBUG

#define fm_test(expression) \
			if( (!(expression)) ) { *((char*)0xdeadf00dLL)=0; }

#define fm_assert(expression) \
			if( (!(expression)) ) { *((char*)0xdeadf00dLL)=0; }

#else

#define fm_test(expression) \
			assert(expression)

#define fm_assert(expression) \
			assert(expression)

#endif


#include <ppw_fat_model.h>
#include "fm_utils.h"



void fm_test_init ( )
{
	bool res;

	res = fm_initAndActive();
	fm_test( res );

	fm_test( fm_getActive() );
}


void fm_test_shut ( )
{
	fm_test( fm_getActive() );
	fm_test( fm_checkIntegrity() );

	fm_shutAndForget();
	fm_test( !fm_getActive() );
}


void fm_test_probe ( const char* title )
{
	size_t	memUsage[ FM_UT_DATA-FM_UT_PHYSICAL+1 ];
	size_t	edtUsage[ FM_UT_DATA-FM_UT_PHYSICAL+1 ];

	fm_test( fm_checkIntegrity() );

	for ( int i=FM_UT_PHYSICAL ; i <= FM_UT_DATA ; i++ )
	{
		memUsage[i] = fm_probeMemoryUsage( fm_usageType(i) );
		edtUsage[i] = fm_probeEditableUsage( fm_usageType(i) );
	}

	OutputDebugString( fm_va("\n## %s\n", title?title:"") );
	OutputDebugString( fm_va("\tfm_probeMemoryUsage(FM_UT_PHYSICAL) = %ld\n", (long)memUsage[FM_UT_PHYSICAL]) );
	OutputDebugString( fm_va("\tfm_probeMemoryUsage(FM_UT_LOGICAL) = %ld\n",  (long)memUsage[FM_UT_LOGICAL])  );
	OutputDebugString( fm_va("\tfm_probeMemoryUsage(FM_UT_PAYLOAD) = %ld\n",  (long)memUsage[FM_UT_PAYLOAD])  );
	OutputDebugString( fm_va("\tfm_probeMemoryUsage(FM_UT_DATA) = %ld\n",     (long)memUsage[FM_UT_DATA])     );

	OutputDebugString( fm_va("\tedtUsage(FM_UT_PHYSICAL) = %ld\n", (long)edtUsage[FM_UT_PHYSICAL]) );
	OutputDebugString( fm_va("\tedtUsage(FM_UT_LOGICAL) = %ld\n",  (long)edtUsage[FM_UT_LOGICAL])  );
	OutputDebugString( fm_va("\tedtUsage(FM_UT_PAYLOAD) = %ld\n",  (long)edtUsage[FM_UT_PAYLOAD])  );
	OutputDebugString( fm_va("\tedtUsage(FM_UT_DATA) = %ld\n",     (long)edtUsage[FM_UT_DATA])     );

	OutputDebugString( fm_va("\tdensity = %ld bytes/element\n",  (long)memUsage[FM_UT_PAYLOAD]/(long)edtUsage[FM_UT_PAYLOAD]) );
	OutputDebugString( "\n" );
}


void fm_test_types ( )
{
	fm_int		v0 = 1;
	fm_uint		v1 = 2U;
	fm_fp		v2 = 0.5f;
	fm_ref		v3 = FM_BAD_REF;

	fm_data		d0;
	fm_data		d1;
	fm_data		d2;
	fm_data		d3;

	fm_data		d;

	d0.set_si( v0 );
	d1.set_ui( v1 );
	d2.set_fp( v2 );
	d3.set_ref( v3 );

	fm_test( d0.type == FM_DT_INT );
	fm_test( d1.type == FM_DT_UINT );
	fm_test( d2.type == FM_DT_FLOAT );
	fm_test( d3.type == FM_DT_REF );

	fm_test( d0.content.si == v0 );
	fm_test( d1.content.ui == v1 );
	fm_test( d2.content.fp == v2 );
	fm_test( d3.content.ref== v3 );

	d = d3;
	fm_test( d.equals(d3) );
	fm_test( !d.equals(d0) );
	fm_test( d == d3 );
	fm_test( d != d0 );

	fm_test( d0.compare(d0) == 0 );
	fm_test( d1.compare(d1) == 0 );
	fm_test( d2.compare(d2) == 0 );
	fm_test( d3.compare(d3) == 0 );

	fm_test( d0.compare(d1) < 0 );
	fm_test( d1.compare(d2) < 0 );
	fm_test( d2.compare(d3) < 0 );

	d.set_si( -1 );
	fm_test( d.compare(d0) < 0 );
	fm_test( d < d0 );

	d.set_si( 5 );
	fm_test( d.compare(d0) > 0 );
	fm_test( d > d0 );

	d.set_ui( 1 );
	fm_test( d.compare(d1) < 0 );
	fm_test( d < d1 );

	d.set_ui( 5 );
	fm_test( d.compare(d1) > 0 );
	fm_test( d > d1 );

	d.set_fp( -1 );
	fm_test( d.compare(d2) < 0 );
	fm_test( d < d2 );

	d.set_fp( 9.08f );
	fm_test( d.compare(d2) > 0 );
	fm_test( d > d2 );
}


void fm_test_extends ( )
{
	fm_test( fm_extend(FM_RT_NONE) == false );
	fm_test( fm_extend(FM_RT_NONE) == false );

	fm_test( fm_extend(FM_RT_SSTRING) );
	fm_test( fm_extend(FM_RT_SSTRING) );

	fm_test( fm_extend(FM_RT_MSTRING) );
	fm_test( fm_extend(FM_RT_MSTRING) );

	fm_test( fm_extend(FM_RT_LSTRING) );
	fm_test( fm_extend(FM_RT_LSTRING) );

	fm_test( fm_extend(FM_RT_NODE) );
	fm_test( fm_extend(FM_RT_NODE) );

	fm_test( fm_extend(FM_RT_BRUSH) );
	fm_test( fm_extend(FM_RT_BRUSH) );

	fm_test( fm_extend(FM_RT_ARRAY) );
	fm_test( fm_extend(FM_RT_ARRAY) );
}


void fm_test_ref_1 ( )
{
	fm_test( fm_refIsValid( FM_BAD_REF ) == false );
	fm_test( fm_refIsValid( 8 ) == false );
	fm_test( !fm_refIsValid(5000) );

	fm_test( fm_refGetType( FM_BAD_REF ) == FM_RT_NONE );
	fm_test( fm_refGetType( 8 ) == FM_RT_NONE );

	fm_extend( FM_RT_NONE );
	fm_extend( FM_RT_NONE );

	fm_test( fm_refGetType( FM_BAD_REF ) == FM_RT_NONE );
	fm_test( fm_refGetType( 8 ) == FM_RT_NONE );
}


void fm_test_ref_2 ( )
{
	fm_ref r;
	fm_ref n[2];

	fm_test( fm_begin() );
	fm_test( fm_end() );

	// a transaction should be opened
	r = fm_refAlloc( FM_RT_SSTRING );
	fm_test( r == FM_BAD_REF );

	fm_test( fm_begin() );
		r = fm_refAlloc( FM_RT_NONE );
		fm_test( r == FM_BAD_REF );
		fm_test( fm_refIsValid( r ) == false );
		fm_test( fm_refIsString( r ) == false );
		fm_test( fm_refIsHead( r ) == false );
		fm_test( fm_refRevise( r, FM_VERSION_HEAD ) == r );
		fm_test( fm_refDeref( r ) == NULL );
		fm_test( fm_refDealloc( r ) == false );
	fm_test( fm_end() );
	
	fm_test( fm_begin() );
		r = fm_refAlloc( FM_RT_SSTRING );
		fm_test( r != FM_BAD_REF );
		fm_test( fm_refIsValid( r ) );
		fm_test( fm_refIsString( r ) );
		fm_test( fm_refGetType( r ) == FM_RT_SSTRING );
		fm_test( fm_refIsHead( r ) );
		fm_test( fm_refRevise( r, FM_VERSION_HEAD ) == r );
		fm_test( fm_refDeref( r ) );
	fm_test( fm_end() );

	for ( int i=0 ; i < 2 ; i++ )
	{
		fm_test( fm_begin() );
			n[i] = fm_refAlloc( FM_RT_NODE );
			fm_test( n[i] != FM_BAD_REF );
			fm_test( fm_refIsValid( n[i] ) );
			fm_test( fm_refIsString( n[i] ) == false );
			fm_test( fm_refGetType( n[i] ) == FM_RT_NODE );
			fm_test( fm_refIsHead( n[i] ) );
		fm_test( fm_refRevise( r, FM_VERSION_HEAD ) == r );
			fm_test( fm_refDeref( n[i] ) );
		fm_test( fm_end() );
	}

	fm_test( fm_refIsHead( n[0] ) );
	fm_test( fm_refIsHead( n[1] ) );
	fm_test( fm_refGetVersion( n[0] ) != FM_VERSION_NONE );
	fm_test( fm_refGetVersion( n[1] ) != FM_VERSION_NONE );
	fm_test( fm_refGetVersion( n[1] ) == fm_headVersion() );
	fm_test( fm_refGetVersion( n[0] ) < fm_refGetVersion( n[1] ) );

	fm_test( fm_begin() );
		fm_test( fm_refLink(n[0],n[1]) );
	fm_test( fm_end() );

	fm_test( fm_refIsHead( n[0] ) == false );
	fm_test( fm_refIsHead( n[1] ) == true );
}


void fm_test_ref_3 ( )
{
	int			N;
	fm_ref*		refs;
	fm_refType	type;

	N    = rand() % 16000;
	refs = new fm_ref[ N ];
	fm_test( refs );

	fm_test( fm_begin() );

	for ( int i=0 ; i < N ; i++ )
	{
		type = fm_refType( (rand() % (FM_RT_LAST-1)) + 1 );
		fm_assert( type > FM_RT_NONE );
		fm_assert( type < FM_RT_LAST );

		refs[i] = fm_refAlloc( type );
		fm_test( fm_refIsValid( refs[i] ) == true );
		fm_test( fm_refGetType( refs[i] ) == type );
	}

	fm_test( fm_end() );

	delete refs;
}


void fm_test_string_1 ( )
{
	fm_string	str0 = "123";
	fm_string	str1 = "1234567890123456789012345678901234567890";
	fm_string	str2 = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";

	fm_uint		s0;
	fm_uint		s1;
	fm_uint		s2;

	fm_test( fm_stringHash(NULL) == 0 );
	fm_test( fm_stringHash("") == 0 );
	fm_test( fm_stringHash("abc") > 0 );
	fm_test( fm_stringHash("abc__________________________abc") > 0 );

	fm_test( fm_stringFind(NULL) == FM_BAD_REF );
	fm_test( fm_stringFind("") == FM_BAD_REF );
	fm_test( fm_stringFind("abc") == FM_BAD_REF );

	fm_test( fm_stringAlloc(NULL) == FM_BAD_REF );
	fm_test( fm_stringAlloc("") == FM_BAD_REF );
	fm_test( fm_stringAlloc("abc") == FM_BAD_REF );

	fm_test( fm_begin() );
		fm_test( fm_stringAlloc(NULL) == FM_BAD_REF );
		fm_test( fm_stringAlloc("") != FM_BAD_REF );
		s0 = fm_stringAlloc(str0);
		s1 = fm_stringAlloc(str1);
		s2 = fm_stringAlloc(str2);
	fm_test( fm_end() );

	fm_test( s0 != FM_BAD_REF );
	fm_test( s1 != FM_BAD_REF );
	fm_test( s2 != FM_BAD_REF );
	fm_test( s0 != s1 );
	fm_test( s0 != s2 );
	fm_test( s1 != s2 );

	fm_test( fm_refIsString(s0) );
	fm_test( fm_refIsString(s1) );
	fm_test( fm_refIsString(s2) );

	fm_test( fm_refGetType(s0) == FM_RT_SSTRING );
	fm_test( fm_refGetType(s1) == FM_RT_MSTRING );
	fm_test( fm_refGetType(s2) == FM_RT_LSTRING );

	fm_test( s0 == fm_stringFind(str0) );
	fm_test( s1 == fm_stringFind(str1) );
	fm_test( s2 == fm_stringFind(str2) );

	fm_test( fm_stringDeref(FM_BAD_REF) == NULL );
	fm_test( fm_stringDeref(999) == NULL );
	fm_test( fm_stringDeref(s0) != NULL );
	fm_test( fm_stringDeref(s1) != NULL );
	fm_test( fm_stringDeref(s2) != NULL );

	fm_test( _stricmp( fm_stringDeref(s0), str0 ) == 0 );
	fm_test( _stricmp( fm_stringDeref(s1), str1 ) == 0 );
	fm_test( _stricmp( fm_stringDeref(s2), str2 ) == 0 );
}


void fm_test_string_2 ( )
{
	char		str[ 300 ];

	int			N, P;
	fm_ref*		refs;

	N = rand() % 16000;

	refs = new fm_ref[ N ];
	fm_test( refs );

	fm_test( fm_begin() );

	for ( int i=0 ; i < N ; i++ )
	{
		// generate a random in length unique str

		memset( str, 'x', sizeof(str) );
		
		sprintf_s( str, "%d", i );
		str[ strlen(str) ] = 'x';

		P = (rand() % (sizeof(str)-16)) + 15;
		str[P] = 0;

		refs[i] = fm_stringAlloc( str );

		if ( P <= fm_editSString::MaxLength )
		{
			fm_test( fm_refIsValid( refs[i] ) == true );
			fm_test( fm_refIsString( refs[i] ) == true );
			fm_test( fm_refGetType( refs[i] ) == FM_RT_SSTRING );
			fm_test( fm_stringFind( str ) == refs[i] );
		}
		else if ( P <= fm_editMString::MaxLength )
		{
			fm_test( fm_refIsValid( refs[i] ) == true );
			fm_test( fm_refIsString( refs[i] ) == true );
			fm_test( fm_refGetType( refs[i] ) == FM_RT_MSTRING );
			fm_test( fm_stringFind( str ) == refs[i] );
		}
		else if ( P <= fm_editLString::MaxLength )
		{
			fm_test( fm_refIsValid( refs[i] ) == true );
			fm_test( fm_refIsString( refs[i] ) == true );
			fm_test( fm_refGetType( refs[i] ) == FM_RT_LSTRING );
			fm_test( fm_stringFind( str ) == refs[i] );
		}
		else
		{
			fm_test( fm_refIsValid( refs[i] ) == false );
			fm_test( fm_refIsString( refs[i] ) == false );
		}
	}

	fm_test( fm_end() );

	delete refs;
}


void fm_test_node_1 ( )
{
	fm_ref	n0;
	fm_ref	n1;
	fm_ref	n2;

	fm_test( fm_nodeAlloc() == FM_BAD_REF );

	fm_test( fm_begin() );
	fm_test( fm_nodeAlloc() != FM_BAD_REF );
	fm_test( fm_end() );

	fm_test( fm_begin() );
	n0 = fm_nodeAlloc();
	n1 = fm_nodeAlloc();
	n2 = fm_nodeAlloc();
	fm_test( fm_end() );

	fm_test( n0 != FM_BAD_REF );
	fm_test( n1 != FM_BAD_REF );
	fm_test( n2 != FM_BAD_REF );

	fm_test( n0 != n1 );
	fm_test( n0 != n2 );
	fm_test( n1 != n2 );

	fm_test( fm_refIsValid(n0) );
	fm_test( fm_refIsValid(n1) );
	fm_test( fm_refIsValid(n2) );

	fm_test( !fm_refIsString(n0) );
	fm_test( !fm_refIsString(n1) );
	fm_test( !fm_refIsString(n2) );

	fm_test( fm_refGetType(n0) == FM_RT_NODE );
	fm_test( fm_refGetType(n1) == FM_RT_NODE );
	fm_test( fm_refGetType(n2) == FM_RT_NODE );

	fm_test( fm_refIsHead(n0) );
	fm_test( fm_refIsHead(n1) );
	fm_test( fm_refIsHead(n2) );

	fm_test( fm_nodeDeref(n0) != NULL );
	fm_test( fm_nodeDeref(n1) != NULL );
	fm_test( fm_nodeDeref(n2) != NULL );

	fm_test( (void*)fm_nodeDeref(n0) == (void*)fm_refDeref(n0) );
	fm_test( (void*)fm_nodeDeref(n1) == (void*)fm_refDeref(n1) );
	fm_test( (void*)fm_nodeDeref(n2) == (void*)fm_refDeref(n2) );
}


void fm_test_node_2 ( )
{
	int			N;
	fm_ref*		refs;

	N    = rand() % 25000;
	refs = new fm_ref[ N ];
	fm_test( refs );

	fm_test( fm_begin() );

	for ( int i=0 ; i < N ; i++ )
	{
		refs[i] = fm_nodeAlloc();
		fm_test( fm_refIsValid( refs[i] ) == true );
		fm_test( fm_refGetType( refs[i] ) == FM_RT_NODE );
		fm_test( fm_refIsString( refs[i] ) == false );
		fm_test( fm_refIsHead( refs[i] ) == true );
		fm_test( fm_nodeDeref( refs[i] ) != NULL );
		fm_test( (void*)fm_nodeDeref( refs[i] ) == (void*)fm_refDeref( refs[i] ) );
	}

	fm_test( fm_end() );

	delete refs;
}


void fm_test_node_3 ( )
{
	fm_ref		n0;
	
	fm_int		v0 = 1;
	fm_uint		v1 = 2U;
	fm_fp		v2 = 0.5f;
	fm_ref		v3 = FM_BAD_REF;

	fm_data		d0;
	fm_data		d1;
	fm_data		d2;
	fm_data		d3;
	fm_data		d4;
	fm_data		d5;

	fm_data		d;
	fm_data		v;

	fm_version	ver0;
	fm_version	ver1;

	d0.set_si( v0 );
	d1.set_ui( v1 );
	d2.set_fp( v2 );
	d3.set_ref( v3 );

	fm_test( d0.type == FM_DT_INT );
	fm_test( d1.type == FM_DT_UINT );
	fm_test( d2.type == FM_DT_FLOAT );
	fm_test( d3.type == FM_DT_REF );

	fm_test( d0.content.si == v0 );
	fm_test( d1.content.ui == v1 );
	fm_test( d2.content.fp == v2 );
	fm_test( d3.content.ref== v3 );

	d = d3;
	fm_test( d.equals(d3) );
	fm_test( !d.equals(d0) );

	fm_test( fm_begin() );
	d4.set_ref( fm_nodeAlloc() );
	fm_test( fm_end() );

	fm_test( fm_begin() );
	d5.set_ref( fm_stringAlloc("key") );
	fm_test( d5.type == FM_DT_REF );
	fm_test( d5.content.ref != FM_BAD_REF );
	fm_test( fm_refIsString(d5.content.ref) );
	fm_test( fm_end() );

	fm_test( fm_begin() );
	n0 = fm_nodeAlloc();
	fm_test( fm_end() );

	fm_test( n0 != FM_BAD_REF );

	fm_test( fm_nodeGet(n0,FM_VERSION_NONE,d0,v) == false );
	fm_test( fm_nodeGet(n0,FM_VERSION_HEAD,d0,v) == false );
	fm_test( fm_nodeGet(n0,FM_VERSION_HEAD,d1,v) == false );
	fm_test( fm_nodeGet(n0,FM_VERSION_HEAD,d2,v) == false );
	fm_test( fm_nodeGet(n0,FM_VERSION_HEAD,d3,v) == false );
	fm_test( fm_nodeGet(n0,FM_VERSION_HEAD,d5,v) == false );

	fm_test( fm_nodeSet(n0,d0,d0) == false );

	fm_test( fm_begin() );
	fm_test( fm_nodeSet( n0, d3, d0 ) == false );		// NONE is an uncorrect key
	fm_test( fm_nodeSet( n0, d4, d0 ) == false );		// REF!=string is an uncorrect key
	fm_test( fm_end() );

	fm_test( fm_begin() );
	fm_test( fm_nodeSet( n0, d0, d0 ) );
	fm_test( fm_end() );

	fm_test( fm_nodeGet(n0,FM_VERSION_HEAD,d0,v) == true );
	fm_test( fm_nodeGet(n0,FM_VERSION_HEAD,d1,v) == false );
	fm_test( v.equals(d0) );

	ver0 = fm_headVersion();

	fm_test( fm_nodeGet(n0,ver0,d0,v) == true );
	fm_test( fm_nodeGet(n0,ver0,d1,v) == false );
	fm_test( v.equals(d0) );

	fm_test( fm_begin() );
	fm_test( fm_nodeSet( n0, d0, d1 ) );
	fm_test( fm_end() );

	ver1 = fm_headVersion();

	fm_test( fm_nodeGet(n0,FM_VERSION_HEAD,d0,v) == true );
	fm_test( fm_nodeGet(n0,FM_VERSION_HEAD,d1,v) == false );
	fm_test( v.equals(d1) );

	fm_test( fm_nodeGet(n0,ver0,d0,v) == true );
	fm_test( v.equals(d0) );

	fm_test( fm_nodeGet(n0,ver1,d0,v) == true );
	fm_test( v.equals(d1) );

	fm_test( fm_begin() );
	fm_test( fm_nodeUnset( n0, d0 ) );
	fm_test( fm_end() );

	fm_test( fm_nodeGet(n0,ver0,d0,v) == true );
	fm_test( v.equals(d0) );

	fm_test( fm_nodeGet(n0,ver1,d0,v) == true );
	fm_test( v.equals(d1) );

	fm_test( fm_nodeGet(n0,FM_VERSION_HEAD,d0,v) == false );
}


void fm_test_node_4 ( )
{
	int				N, P;
	fm_ref			ref0 ,ref;
	fm_data			key;
	fm_data			val;

	fm_ref*			refs;
	fm_version*		vers;
	fm_int*			vals;

	fm_uint			k, n;
	fm_int			v;

	fm_int			headVals[32];
	fm_keyValueMod	headKvps[FM_NODE_LOAD];

	N = rand() % 25000;

	refs = new fm_ref[ N ];
	vers = new fm_version[ N ];
	vals = new fm_int[ N ];
	fm_test( refs );
	fm_test( vers );
	fm_test( vals );

	fm_test( fm_begin() );
	ref0 = fm_nodeAlloc();
	fm_test( fm_end() );

	for ( int i=0 ; i < fm_sizeofArray(headVals)  ; i++ )
	{
		headVals[i] = -1;
	}

	// assign random values to many keys in [0-9]

	for ( int i=0 ; i < N ; i++ )
	{
		ref = (i == 0) ? ref0 : refs[i-1];

		fm_test( fm_begin() );

		// n[0] = random
		k			= 0;
		v			= rand() % 9999;
		
		vals[i]		= v;
		headVals[k] = v;
		
		key.set_ui( k );
		val.set_si( v );

		fm_test( fm_nodeSet( ref, key, val ) );

		// n[1-9] = random
		P = rand() % 9;
		for ( int p=0 ; p < P ; p++ )
		{
			k			= (rand() % 9)+1;
			v			= rand() % 9999;
			
			headVals[k] = v;
			
			key.set_ui( k );
			val.set_si( v );
			fm_test( fm_nodeSet( ref, key, val ) );
		}

		fm_test( fm_end() );
		
		refs[i] = fm_refRevise( ref, FM_VERSION_HEAD );

		vers[i] = fm_headVersion();
	}

	// check back the value at key 0 for each version from the revised reference

	for ( int i=0 ; i < N ; i++ )
	{
		key.set_ui( 0 );
		fm_test( fm_nodeGet( refs[i], vers[i], key, val ) );
		fm_test( val.get_si() == vals[i] );
	}

	// check back the value at key 0 for each version from the first reference

	for ( int i=0 ; i < N ; i++ )
	{
		key.set_ui( 0 );
		fm_test( fm_nodeGet( refs[0], vers[i], key, val ) );
		fm_test( val.get_si() == vals[i] );
	}

	// check back the value at key 0 for each version from the last reference

	for ( int i=0 ; i < N ; i++ )
	{
		key.set_ui( 0 );
		fm_test( fm_nodeGet( refs[N-1], vers[i], key, val ) );
		fm_test( val.get_si() == vals[i] );
	}

	// check enumerate on head

	n = fm_nodeDeflate( refs[N-1], FM_VERSION_HEAD, headKvps, fm_sizeofArray(headKvps) );

	fm_test( n >  1 );
	fm_test( n <= fm_sizeofArray(headKvps) );

	for ( unsigned i=0 ; i < n ; i++ )
	{
		k = headKvps[i].key.get_ui();
		v = headKvps[i].value.get_si();

		fm_test( k < fm_sizeofArray(headVals) );
		fm_test( headVals[k] == v );

		headVals[k] = -1;
	}

	for ( int i=0 ; i < fm_sizeofArray(headVals) ; i++ )
	{
		fm_test( headVals[i] == -1 );
	}

	// check back the value at key 0 for each duplicated version from the last reference

	for ( int i=0 ; i < N ; i++ )
	{
		fm_data v0, v1;

		fm_test( fm_begin() );
		ref = fm_nodeDuplicate( ref0, vers[i] );
		fm_test( ref != FM_BAD_REF );
		fm_test( fm_end() );

		key.set_ui( 0 );
		fm_test( fm_nodeGet( ref,       FM_VERSION_HEAD, key, v0 ) );
		fm_test( fm_nodeGet( refs[N-1], vers[i],         key, v1 ) );

		fm_test( v0.get_si() == vals[i] );
		fm_test( v1.get_si() == vals[i] );
	}

	delete refs;
	delete vers;
	delete vals;
}


void fm_test_node_5 ( )
{
	int		N, P;
	fm_ref	ref;
	long	ut0, ut1;
	long	kt0, kt1;

	N = 32000;
	P = 16;

	fm_util_getProcessTime( &kt0, &ut0 );

	fm_test( fm_begin() );

	for ( int i=0 ; i < N ; i++ )
	{
		ref = fm_nodeAlloc();

		for ( int j=0 ; j < P ; j++ )
		{
			fm_nodeSet( ref, fm_data::as_ui(rand()%1000), fm_data::as_ui(rand()%1000) );
		}
	}

	fm_test( fm_end() );

	fm_util_getProcessTime( &kt1, &ut1 );

	OutputDebugString( fm_va("fm_test_node_5:\n") );
	OutputDebugString( fm_va("\tuser time = %ld\n", ut1-ut0) );
	OutputDebugString( fm_va("\tkernel time = %ld\n", kt1-kt0) );
}


void fm_test_brush_1 ( )
{
	fm_ref	ref;

	fm_test( fm_begin() );

	for ( int i=0 ; i < 16000 ; i++ )
	{
		ref = fm_brushAlloc();
		fm_test( fm_refGetType(ref) == FM_RT_BRUSH );
	}

	fm_test( fm_end() );
}


void fm_test_brush_2 ( )
{
	fm_ref			ref;
	fm_face			f, ff;
	fm_faceMod		faceMods[FM_BRUSH_LOAD];
	fm_face			faceValue[FM_BRUSH_LOAD];
	bool			faceDefined[FM_BRUSH_LOAD];
	int				I;
	fm_uint			N, P;

	fm_test( fm_begin() );
	ref = fm_brushAlloc();
	fm_test( fm_end() );

	fm_test( fm_brushDeflate(ref,0,faceMods,FM_BRUSH_LOAD) == 0 );
	fm_test( fm_brushGet(ref,0,0,f) == 0 );

	::memset( faceValue,   0, sizeof(faceValue) );
	::memset( faceDefined, 0, sizeof(faceDefined) );

	for ( int i=0 ; i < 16000 ; i++ )
	{
		f.plane.dir[0] = float(rand()) / float(RAND_MAX);
		f.plane.dir[1] = float(rand()) / float(RAND_MAX);
		f.plane.dir[2] = float(rand()) / float(RAND_MAX);
		f.plane.dist   = float(rand()) / float(RAND_MAX);

		I = rand() % 8;

		fm_test( fm_begin() );
		fm_test( fm_brushSet(ref,I,f) );
		fm_test( fm_end() );

		faceValue[I]   = f;
		faceDefined[I] = true;

		fm_test( fm_brushGet(ref,FM_VERSION_HEAD,I,ff) );
		fm_test( ff.plane.dir[0] == f.plane.dir[0] );
		fm_test( ff.plane.dir[1] == f.plane.dir[1] );
		fm_test( ff.plane.dir[2] == f.plane.dir[2] );
		fm_test( ff.plane.dist   == f.plane.dist   );

		N = fm_brushDeflate(ref,FM_VERSION_HEAD,faceMods,FM_BRUSH_LOAD);
		fm_test( N > 0);

		for ( unsigned j=0 ; j < N ; j++ )
		{
			fm_test( faceDefined[ faceMods[j].id ] == true );
			fm_test( faceValue[ faceMods[j].id ].plane.dir[0] == faceMods[j].face.plane.dir[0] );
			fm_test( faceValue[ faceMods[j].id ].plane.dir[1] == faceMods[j].face.plane.dir[1] );
			fm_test( faceValue[ faceMods[j].id ].plane.dir[2] == faceMods[j].face.plane.dir[2] );
			fm_test( faceValue[ faceMods[j].id ].plane.dist   == faceMods[j].face.plane.dist	);
		}
	}

	// unset faces one by one

	for ( ;; )
	{
		P = fm_brushDeflate(ref,FM_VERSION_HEAD,faceMods,FM_BRUSH_LOAD);

		if ( P == 0 )
			break;

		fm_test( fm_begin() );
		fm_test( fm_brushUnset(ref,faceMods[0].id) );
		fm_test( fm_end() );

		N = fm_brushDeflate(ref,FM_VERSION_HEAD,faceMods,FM_BRUSH_LOAD);
		fm_test( N == P-1 );

		for ( unsigned j=0 ; j < N ; j++ )
		{
			fm_test( faceDefined[ faceMods[j].id ] == true );
			fm_test( faceValue  [ faceMods[j].id ].plane.dir[0] == faceMods[j].face.plane.dir[0] );
			fm_test( faceValue  [ faceMods[j].id ].plane.dir[1] == faceMods[j].face.plane.dir[1] );
			fm_test( faceValue  [ faceMods[j].id ].plane.dir[2] == faceMods[j].face.plane.dir[2] );
			fm_test( faceValue  [ faceMods[j].id ].plane.dist   == faceMods[j].face.plane.dist	);
		}
	}
}


void fm_test_array_1 ( )
{
	fm_ref		ref;
	fm_version  v0, v1, v2;

	struct array_type
	{
		fm_arrayType	type;
		fm_uint			stride;
	};

	array_type arr_types[] = {
		{FM_ARRAY_U8,1},
		{FM_ARRAY_U32,4},
		{FM_ARRAY_FLOAT,4},
		{FM_ARRAY_LOCATION,sizeof(float)*3},
		{FM_ARRAY_TRI,sizeof(int)*3},
		{FM_ARRAY_MAPPING,sizeof(float)*2},
		{FM_ARRAY_EDGE,sizeof(int)*2},
		{FM_ARRAY_NORMAL,sizeof(float)*3},
		{FM_ARRAY_REF,sizeof(fm_ref)},
	};

	char randomBytes[256];
	char outputBytes[256];

	for( int i=0 ; i < fm_sizeofArray(randomBytes) ; i++ )
		randomBytes[i] = (rand() % 254) + 1;

	for ( int i=0 ; i < fm_sizeofArray(arr_types) ; i++ )
	{
		fm_test( fm_begin() );
		ref = fm_arrayAlloc( arr_types[i].type, 256 );
		fm_test( fm_end() );

		fm_test( fm_refGetType(ref) == FM_RT_ARRAY );
		fm_test( fm_arrayDeref(ref) != NULL );
		fm_test( fm_arrayMax(ref) == 256 );
		fm_test( fm_arrayStride(ref) == arr_types[i].stride );
		fm_test( fm_arrayRange(ref,FM_VERSION_HEAD) == 0 );

		v0 = fm_headVersion();

		fm_test( fm_begin() );
		fm_test( fm_arraySet( ref, 0, (void*)randomBytes ) );
		fm_test( fm_end() );

		v1 = fm_headVersion();

		fm_test( fm_begin() );
		fm_test( fm_arraySet( ref, 9, (void*)randomBytes ) );
		fm_test( fm_end() );

		v2 = fm_headVersion();

		fm_test( fm_begin() );
		fm_test( fm_arrayUnset( ref, 9 ) );
		fm_test( fm_end() );

		fm_test( fm_arrayRange(ref,v0) == 0 );
		fm_test( fm_arrayRange(ref,v1) == 1 );
		fm_test( fm_arrayRange(ref,v2) == 10 );
		fm_test( fm_arrayRange(ref,FM_VERSION_HEAD) == 10 );
	
		fm_test( fm_arrayGet(ref,v0,9,outputBytes,256) == false );
		fm_test( fm_arrayGet(ref,v1,9,outputBytes,256) == false );
		fm_test( fm_arrayGet(ref,v2,9,outputBytes,256) == true );
		fm_test( fm_arrayGet(ref,FM_VERSION_HEAD,9,outputBytes,256) == false );
	}
}










void fm_unit_tests ( )
{
	//
	fm_test_init();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_types();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_extends();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_ref_1();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_ref_2();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_ref_3();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_string_1();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_string_2();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_node_1();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_node_2();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_node_3();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_node_4();
	fm_test_probe("node_4");
	fm_test_shut();

	//
	fm_test_init();
	fm_test_node_5();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_brush_1();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_brush_2();
	fm_test_shut();

	//
	fm_test_init();
	fm_test_array_1();
	fm_test_shut();
}

int main( int argc, char** argv )
{
	fm_unit_tests();
}

