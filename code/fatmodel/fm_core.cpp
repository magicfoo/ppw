
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <intrin.h>

#include <ppw_fat_model.h>

#define FM_USING_ALL
#include "fm_utils.h"


namespace
{

	void*		init_address = NULL;

	fm_zone*	active_zone  = NULL;

}


#include "fm_defs.h"



// misc functions

int fm_data::compare ( const fm_data& d	) const
{
	if ( type < d.type )					return -1;
	if ( type > d.type )					return +1;

	switch ( type )
	{
	case FM_DT_FLOAT:
		if		( content.fp  < d.content.fp  )	return -1;
		else if ( content.fp  > d.content.fp  )	return +1;
		break;

	case FM_DT_INT:
		if		( content.si  < d.content.si  )	return -1;
		else if ( content.si  > d.content.si  )	return +1;
		break;

	default:
		if ( content.all  < d.content.all  )	return -1;
		if ( content.all  > d.content.all  )	return +1;
	}

	return 0;
}



// setup functions

fm_zone* fm_getActive ( )
{
	return active_zone;
}


bool fm_setActive ( fm_zone* inz )
{
	active_zone = inz;
	return true;
}


bool fm_initAndActive ( )
{
	fm_uintptr loAddr;
	fm_uintptr hiAddr;

	return_false_if( init_address );

	init_address = _aligned_malloc( FM_DEFAULT_ZONE_LOAD, 256 );
	return_false_if( !init_address );

	loAddr = fm_uintptr( init_address );
	hiAddr = loAddr + FM_DEFAULT_ZONE_LOAD;

	if ( fm_initAndActive( loAddr, hiAddr ) )
		return true;

	_aligned_free( init_address );
	init_address = NULL;

	return false;
}


bool fm_initAndActive ( fm_uintptr inloAddr, fm_uintptr inhiAddr )
{
	return_false_if( !inloAddr );
	return_false_if( !inhiAddr );

	fm_assert( inhiAddr > inloAddr );
	fm_assert( inhiAddr > inloAddr + sizeof(fm_zone) );
	fm_assert( fm_alignBits(inloAddr,8) );

	active_zone = (fm_zone*) inloAddr;
	fm_zeroPointed( active_zone );

	active_zone->magic			= FM_MAGIC;
	active_zone->revision		= FM_REVISION;
	active_zone->headVersion	= FM_VERSION_NONE;
	active_zone->currVersion	= FM_VERSION_NONE;
	active_zone->loAddr			= inloAddr;
	active_zone->hiAddr			= inhiAddr;
	active_zone->bsAddr			= inloAddr + sizeof(fm_zone);

	for ( int i=0 ; i < FM_RT_LAST ; i++ )
		active_zone->directory.lastRefs[i] = FM_BAD_REF;

	return true;
}


void fm_shutAndForget ( )
{
	return_if( !init_address );

	_aligned_free( init_address );

	init_address = NULL;
	active_zone  = NULL;
}


bool fm_openAndActive ( fm_string /* identifier */ )
{
	return false;
}


void fm_closeAndForget ( )
{
	//
}



fm_version fm_headVersion ( )
{
	return_value_if_no_zone( FM_VERSION_NONE );
	return active_zone->headVersion;
}


bool fm_begin ( )
{
	return_value_if_transactioning( false );
	
	if ( fm_isFull() )
	{
		// reject a new transaction if the zone if dangerously full
		return false;
	}

	active_zone->currVersion = active_zone->headVersion + 1;
	return true;
}


bool fm_end ( )
{
	return_value_if_not_transactioning( false );
	active_zone->headVersion = active_zone->currVersion;
	active_zone->currVersion = FM_VERSION_NONE;
	return true;
}


bool fm_extend ( fm_refType intype, fm_uint inextra )
{
	fm_uintptr	pgAddr;
	fm_uintptr	loAddr;
	fm_uintptr	hiAddr;
	fm_uint		editBSize;
	fm_page*	pg;
	fm_knot*	kn;
	fm_knot*	kn_end;

	return_value_if_no_zone( false );

	if ( active_zone->directory.pagei == FM_DIRECTORY_LOAD )
		return false;

	if ( intype == FM_RT_ARRAY )
		editBSize = fm_alignBytes( inextra, 8 );
	else
		editBSize = _editUsageBSize( FM_UT_PHYSICAL, intype, NULL );

	return_false_if( editBSize == 0 );

	pgAddr	= fm_alignBytes( active_zone->bsAddr, 64 );
	loAddr	= fm_alignBytes( pgAddr + sizeof(fm_page), 64 );
	hiAddr	= loAddr + editBSize * FM_PAGE_LOAD;

	if ( hiAddr > active_zone->hiAddr )
		return false;

	pg = (fm_page*) pgAddr;
	fm_zeroPointed( pg );

	pg->editType	= intype;
	pg->editBSize	= editBSize;
	pg->loAddr		= loAddr;
	pg->hiAddr		= hiAddr;

	kn		   = pg->knots;
	kn_end	   = pg->knots + FM_PAGE_LOAD;

	while ( kn < kn_end )
	{
		kn->version = FM_VERSION_NONE;
		kn->addr    = loAddr;
		kn		   += 1;
		loAddr     += editBSize;
	}

	active_zone->bsAddr = hiAddr;
	active_zone->directory.pages[ active_zone->directory.pagei ] = pg;
	active_zone->directory.pagei += 1;

	return true;
}


fm_ref fm_refAlloc ( fm_refType intype, fm_uint inextra )
{
	fm_ref		ref;
	fm_knot*	kn;
	fm_uint		editBSize;

	return_value_if_not_transactioning( FM_BAD_REF );
	return_value_if( intype == FM_RT_NONE, FM_BAD_REF );

	if ( intype == FM_RT_ARRAY )
		editBSize = inextra;
	else
		editBSize = _editUsageBSize( FM_UT_PHYSICAL, intype, NULL );

	return_value_if( editBSize == 0, FM_BAD_REF );

	ref = _findFreeRefFrom( fm_refGetLastAlloc(intype), intype, editBSize );

	if( ref == FM_BAD_REF )
	{
		if ( fm_extend(intype,editBSize) )
		{
			// free ref is the first of the new page
			fm_assert( active_zone->directory.pagei > 0 );
			ref = _ref( active_zone->directory.pagei-1, 0 );
		}
		else
		{
			if ( ref == FM_BAD_REF )
				return FM_BAD_REF;
		}
	}

	kn = _getKnot( ref );

	kn->version = active_zone->currVersion;
	kn->prev = FM_BAD_REF;
	kn->next = FM_BAD_REF;

	::memset( (void*)kn->addr, 0, editBSize );

	fm_refSetLastAlloc( ref );

	return ref;
}


fm_ref fm_refGetLastAlloc ( fm_refType intype )
{
	return_value_if_no_zone( FM_BAD_REF );
	return_value_if( intype == FM_RT_NONE, FM_BAD_REF );
	return_value_if( intype == FM_RT_LAST, FM_BAD_REF );

	return active_zone->directory.lastRefs[ intype ];
}


fm_ref fm_refSetLastAlloc ( fm_ref inref )
{
	fm_ref		lastRef;
	fm_refType	type;

	return_value_if_no_zone( FM_BAD_REF );
	return_value_if_bad_ref( inref, FM_BAD_REF );

	type = _getType( inref );

	lastRef = active_zone->directory.lastRefs[ type ];

	active_zone->directory.lastRefs[ type ] = inref;

	return lastRef;
}


bool fm_refDealloc ( fm_ref inref )
{
	fm_knot*	kn;

	return_value_if_no_zone( false );
	return_value_if_bad_ref( inref, false );

	kn = _getKnot( inref );
	kn->version = FM_VERSION_NONE;

	return true;
}


bool fm_refLink ( fm_ref fromref, fm_ref toref )
{
	fm_knot		*from;
	fm_knot		*to;

	return_value_if_not_transactioning( false );
	return_false_if( fromref == toref );
	return_value_if_bad_ref( fromref, false );
	return_value_if_bad_ref( toref, false );
	return_value_if( _getType(fromref) != _getType(toref), false );

	from = _getKnot( fromref );
	to   = _getKnot( toref );

	fm_assert( from->next == FM_BAD_REF );
	fm_assert( to->next == FM_BAD_REF );
	fm_assert( to->prev == FM_BAD_REF );
	fm_assert( from->version < to->version );

	from->next = toref;
	to->prev   = fromref;

	return true;
}


bool fm_refUnlink ( fm_ref inref )
{
	fm_knot		*kn;
	fm_knot		*prev_kn;
	fm_knot		*next_kn;

	return_value_if_not_transactioning( false );
	return_value_if_bad_ref( inref, false );

	kn		 = _getKnot( inref    );
	prev_kn  = _getKnot( kn->prev );
	next_kn  = _getKnot( kn->next );

	if ( prev_kn )
	{
		fm_assert( prev_kn->next == inref );
		prev_kn->next = kn->next;
	}

	if ( next_kn )
	{
		fm_assert( next_kn->prev == inref );
		next_kn->prev = kn->prev;
	}

	return true;
}


bool fm_refIsHead ( fm_ref inref )
{
	return_value_if_bad_ref( inref, false );

	return _getKnot(inref)->next == FM_BAD_REF;
}


fm_refType fm_refGetType ( fm_ref inref )
{
	return_value_if_bad_ref( inref, FM_RT_NONE );

	return _getType( inref );
}


fm_version fm_refGetVersion ( fm_ref inref )
{
	return_value_if_bad_ref( inref, FM_VERSION_NONE );

	return _getKnot(inref)->version;
}


fm_ref fm_refRevise ( fm_ref inref, fm_version inver )
{
	return_value_if_bad_ref( inref, FM_BAD_REF );

	return _reviseRef( inref, inver );
}


bool fm_refIsAncestorOf ( fm_ref inref, fm_ref inrefref )
{
	fm_knot*	kn;
	fm_uint		cnt;

	return_value_if_bad_ref( inref, false );
	return_value_if_bad_ref( inrefref, false );

	kn  = _getKnot( inrefref );

	for ( cnt=0 ; cnt < 1024 ; cnt++ )
	{
		if ( kn->prev == inref )
			return true;

		if ( kn->prev == FM_BAD_REF )
			return false;

		kn  = _getKnot( kn->prev );
	}

	// probably an infinite loop ...
	fm_assert( cnt );
	return false;
}


bool fm_refIsDescendantOf ( fm_ref inref, fm_ref inrefref )
{
	fm_knot*	kn;
	fm_uint		cnt;

	return_value_if_bad_ref( inref, false );
	return_value_if_bad_ref( inrefref, false );

	kn  = _getKnot( inrefref );

	for ( cnt=0 ; cnt < 1024 ; cnt++ )
	{
		if ( kn->next == inref )
			return true;

		if ( kn->next == FM_BAD_REF )
			return false;

		kn  = _getKnot( kn->next );
	}

	// probably an infinite loop ...
	fm_assert( cnt );
	return false;
}


bool fm_refIsSameLineage ( fm_ref inref0, fm_ref inref1 )
{
	return_value_if_bad_ref( inref0, false );
	return_value_if_bad_ref( inref1, false );

	if ( inref0 == inref1 )
		return true;

	if ( fm_refIsAncestorOf(inref0,inref1) )
		return true;

	if ( fm_refIsDescendantOf(inref0,inref1) )
		return true;

	return false;
}


bool fm_refIsValid ( fm_ref inref )
{
	return _checkAllocated( inref );
}


fm_editBase* fm_refDeref ( fm_ref inref, fm_refType* outtype )
{
	fm_knot*	kn;

	return_value_if_bad_ref( inref, NULL );

	if ( outtype )
		*outtype = _getType( inref );

	kn = _getKnot( inref );

	return (fm_editBase*) kn->addr;
}


bool fm_refIsType ( fm_ref inref, fm_refType intype )
{
	return_value_if_bad_ref( inref, false );

	return _getType(inref) == intype;
}


bool fm_refIsString ( fm_ref inref )
{
	fm_refType	type;

	return_value_if_bad_ref( inref, false );

	type = _getType( inref );

	return (type >= FM_RT_SSTRING && type <= FM_RT_LSTRING);
}


bool fm_refIsArray ( fm_ref inref )
{
	fm_refType	type;

	return_value_if_bad_ref( inref, false );

	type = _getType( inref );

	return (type == FM_RT_ARRAY);
}


bool fm_refIsNode ( fm_ref inref )
{
	fm_refType	type;

	return_value_if_bad_ref( inref, false );

	type = _getType( inref );

	return (type == FM_RT_NODE);
}


bool fm_refIsBrush ( fm_ref inref )
{
	fm_refType	type;

	return_value_if_bad_ref( inref, false );

	type = _getType( inref );

	return (type == FM_RT_BRUSH);
}




//
// strings


fm_uint fm_stringHash ( fm_string instr )
{
	return fm_util_crc32_str( instr );
}


fm_ref fm_stringFind ( fm_string instr )
{
	fm_ref ref;
	size_t len;

	return_value_if( !instr, FM_BAD_REF );

	len = strlen( instr );
	return_value_if( len > fm_editLString::MaxLength, FM_BAD_REF );

	if ( len <= fm_editSString::MaxLength )
	{
		ref = _findString<fm_editSString>( FM_RT_SSTRING, instr );
	}
	else if ( len <= fm_editMString::MaxLength )
	{
		ref = _findString<fm_editMString>( FM_RT_MSTRING, instr );
	}
	else
	{
		ref = _findString<fm_editLString>( FM_RT_LSTRING, instr );
	}

	return ref;
}


fm_string fm_stringDeref ( fm_ref inref )
{
	fm_refType		type;
	fm_editBase*	base;
	
	return_value_if_not_string( inref, NULL );

	base = fm_refDeref( inref, &type );
	fm_assert( base );

	if ( type == FM_RT_SSTRING )
		return ((fm_editSString*)base)->text;

	else if ( type == FM_RT_MSTRING )
		return ((fm_editMString*)base)->text;

	else if ( type == FM_RT_LSTRING )
		return ((fm_editLString*)base)->text;

	return NULL;
}


fm_ref fm_stringAlloc ( fm_string instr )
{
	size_t		len;
	fm_refType	type;
	fm_ref		ref;
	fm_knot*	kn;
	char*		text;

	return_value_if_not_transactioning( FM_BAD_REF );
	return_value_if( !instr, FM_BAD_REF );

	len = strlen( instr );

	if ( len <= fm_editSString::MaxLength )
		type = FM_RT_SSTRING;
	else if ( len <= fm_editMString::MaxLength )
		type = FM_RT_MSTRING;
	else if ( len <= fm_editLString::MaxLength )
		type = FM_RT_LSTRING;
	else
		return FM_BAD_REF;

	ref = fm_refAlloc( type );

	if ( ref == FM_BAD_REF )
		return FM_BAD_REF;

	kn   = _getKnot( ref );
	text = ((fm_editSString*)kn->addr)->text;

	strcpy_s( text, len+1, instr );

	return ref;
}





// arrays

fm_editArray* fm_arrayDeref ( fm_ref inref )
{
	fm_editBase*	base;
	
	return_value_if_not_array( inref, NULL );

	base = fm_refDeref( inref );
	fm_assert( base );

	return (fm_editArray*)base;
}


fm_ref fm_arrayAlloc ( fm_arrayType intype, fm_uint inmax, fm_uint instride )
{
	fm_ref			ref;
	fm_uint			stride;
	fm_uint			editBSize;
	fm_knot*		kn;
	fm_editArray*	ar;

	return_value_if_not_transactioning( FM_BAD_REF );

	if ( intype >= FM_ARRAY_CUSTOM )
		stride = instride;
	else
		stride = _arrayTypeStride( intype );

	return_value_if( inmax == 0, FM_BAD_REF );
	return_value_if( stride == 0, FM_BAD_REF );

	editBSize = sizeof(fm_editArray) + sizeof(fm_arrayMod)*inmax + stride*inmax;

	ref = fm_refAlloc( FM_RT_ARRAY, editBSize );

	if ( ref == FM_BAD_REF )
		return FM_BAD_REF;

	kn = _getKnot( ref );
	ar = (fm_editArray*)kn->addr;

	ar->type	= intype;
	ar->stride	= stride;
	ar->max		= inmax;
	ar->cnt		= 0;
	ar->mods	= (fm_arrayMod*)(ar+1);
	ar->data	= (fm_uintptr)(ar->mods+inmax);

	return ref;
}


fm_ref fm_arrayDuplicate ( fm_ref inref, fm_version inver )
{
	fm_page*		pg;
	fm_editArray*	ar;
	fm_editArray*	new_ar;
	fm_ref			new_ref;

	inref = fm_refRevise( inref, inver );

	return_value_if_not_transactioning( FM_BAD_REF );
	return_value_if_not_array( inref, FM_BAD_REF );
	return_value_if( inver == FM_VERSION_NONE, FM_BAD_REF );

	if ( inver == FM_VERSION_HEAD )
		inver = active_zone->headVersion;

	ar = _arrayDeref( inref );
	pg = _getPage( inref );

	new_ref = fm_refAlloc( FM_RT_ARRAY, pg->editBSize );
	return_value_if( new_ref == FM_BAD_REF, FM_BAD_REF );

	new_ar = _arrayDeref( new_ref );

	_arrayDeflate( ar, new_ar, inver );
 
	return new_ref;
}


fm_uint fm_arrayDeflate ( fm_ref inref, fm_version inver, fm_arrayMod* outmods, void* outdata, fm_uint incapacity )
{
	inref = fm_refRevise( inref, inver );

	return_value_if_not_array( inref, 0 );
	return_value_if( inver == FM_VERSION_NONE, 0 );

	if ( inver == FM_VERSION_HEAD )
		inver = active_zone->headVersion;

	return _arrayDeflate( _arrayDeref(inref), inver, outmods, outdata, incapacity );
}


fm_uint fm_arrayMax ( fm_ref inref )
{
	fm_editArray*	ar;

	return_value_if_not_array( inref, 0 );

	ar = _arrayDeref( inref );

	return ar->max;
}


fm_uint fm_arrayStride ( fm_ref inref )
{
	fm_editArray*	ar;

	return_value_if_not_array( inref, 0 );

	ar = _arrayDeref( inref );

	return ar->stride;
}


fm_uint fm_arrayRange ( fm_ref inref, fm_version inver )
{
	fm_editArray*	ar;
	int				modi;

	inref = fm_refRevise( inref, inver );

	return_value_if_not_array( inref, 0 );
	return_value_if( inver == FM_VERSION_NONE, 0 );

	if ( inver == FM_VERSION_HEAD )
		inver = active_zone->headVersion;

	ar = _arrayDeref( inref );

	return_value_if( ar->cnt==0, 0 );

	modi = _modFind( ar->mods, ar->cnt, inver );

	if ( modi < 0 )
		return 0;

	return ar->mods[modi].last+1;
}


bool fm_arrayGet ( fm_ref inref, fm_version inver, fm_uint inindex, void* outdata, fm_uint inbsize )
{
	fm_editArray*	ar;
	int				modi;
	char*			base;
	char*			data;
	unsigned		stride;
	int				set_index;
	int				unset_index;

	inref = fm_refRevise( inref, inver );

	return_value_if_not_array( inref, false );
	return_false_if( inver == FM_VERSION_NONE );
	return_false_if( !outdata );
	return_false_if( !inbsize );

	if ( inver == FM_VERSION_HEAD )
		inver = active_zone->headVersion;

	ar = _arrayDeref( inref );

	return_false_if( inbsize < ar->stride );
	return_false_if( ar->cnt == 0 );

	modi = _modFind( ar->mods, ar->cnt, inver );

	if ( modi < 0 )
		return false;

	if ( inindex > ar->mods[modi].last )
		return false;

	base		= (char*)ar->data;
	stride		= ar->stride;
	data		= NULL;
	set_index	=   int(inindex);
	unset_index	= - int(inindex) - 1;

	for ( signed i=0 ; i <= modi ; i++ )
	{
		// set?
		if ( ar->mods[i].index == set_index )
			data = base + i*stride;
		// unset?
		else if ( ar->mods[i].index == unset_index )
			data = NULL;
	}

	if ( data )
		::memcpy( outdata, data, stride );

	return (data != NULL);
}


bool fm_arraySet ( fm_ref inref, fm_uint inindex, void* indata )
{
	fm_editArray*	ar;
	fm_editArray*	new_ar;
	fm_ref			new_ref;
	fm_uint			last;

	inref = fm_refRevise( inref, FM_VERSION_HEAD );

	return_value_if_not_transactioning( false );
	return_value_if_not_array( inref, false );
	return_false_if( !indata );

	ar		= _arrayDeref( inref );
	new_ar  = NULL;
	new_ref = FM_BAD_REF;

	if ( ar->cnt == ar->max )
	{
		new_ref = fm_arrayDuplicate( inref, FM_VERSION_HEAD );
		return_false_if( new_ref == FM_BAD_REF );

		new_ar = _arrayDeref( new_ref );

		if ( new_ar->cnt == new_ar->max )
		{
			// can't reduce the amount of mods?
			fm_refDealloc( new_ref );
			return false;
		}

		ar = new_ar;
	}

	fm_assert( ar->cnt < ar->max );

	::memcpy( ((char*)ar->data)+(ar->cnt*ar->stride), indata, ar->stride );

	last = ar->cnt ? ar->mods[ ar->cnt-1 ].last : 0;

	ar->mods[ ar->cnt ].version = active_zone->currVersion;
	ar->mods[ ar->cnt ].index   = inindex;
	ar->mods[ ar->cnt ].last    = fm_max( last, inindex );

	_WriteBarrier();

	ar->cnt += 1;

	return true;
}


bool fm_arrayUnset ( fm_ref inref, fm_uint inindex )
{
	fm_editArray*	ar;
	fm_editArray*	new_ar;
	fm_ref			new_ref;
	fm_uint			last;

	inref = fm_refRevise( inref, FM_VERSION_HEAD );

	return_value_if_not_transactioning( false );
	return_value_if_not_array( inref, false );

	ar		= _arrayDeref( inref );
	new_ar  = NULL;
	new_ref = FM_BAD_REF;

	return_false_if( ar->cnt == 0 );

	last = ar->mods[ ar->cnt-1 ].last;

	if ( inindex > last )
		return false;

	if ( ar->cnt == ar->max )
	{
		new_ref = fm_arrayDuplicate( inref, FM_VERSION_HEAD );
		return_false_if( new_ref == FM_BAD_REF );

		new_ar = _arrayDeref( new_ref );

		if ( new_ar->cnt == new_ar->max )
		{
			// can't reduce the amount of mods?
			fm_refDealloc( new_ref );
			return false;
		}

		ar = new_ar;
	}

	fm_assert( ar->cnt < ar->max );

	ar->mods[ ar->cnt ].version = active_zone->currVersion;
	ar->mods[ ar->cnt ].index   = - int(inindex) - 1;
	ar->mods[ ar->cnt ].last    = last;

	_WriteBarrier();

	ar->cnt += 1;

	return true;
}






//
// nodes


fm_ref fm_nodeAlloc ( )
{
	fm_ref			ref;
	fm_editNode*	nd;

	return_value_if_not_transactioning( FM_BAD_REF );

	ref = fm_refAlloc( FM_RT_NODE );
	return_value_if( ref == FM_BAD_REF, FM_BAD_REF );

	nd = (fm_editNode*) _getKnot(ref)->addr;

	nd->cnt = 0;

	return ref;
}


fm_editNode* fm_nodeDeref ( fm_ref inref )
{
	return_value_if_not_node( inref, NULL );

	return (fm_editNode*) _getKnot(inref)->addr;
}


fm_uint fm_nodeDeflate ( fm_ref inref, fm_version inver, fm_keyValueMod* outarr, fm_uint incapacity )
{
	inref = fm_refRevise( inref, inver );

	return_value_if_not_node( inref, 0 );
	return_value_if( inver == FM_VERSION_NONE, 0 );

	if ( inver == FM_VERSION_HEAD )
		inver = active_zone->headVersion;

	return _nodeDeflate( _nodeDeref(inref), inver, outarr, incapacity );
}




bool fm_nodeGet ( fm_ref inref, fm_version inver, const fm_data& inkey, fm_data& outvalue )
{
	fm_editNode*		nd;
	fm_keyValueMod*		kvp;

	inref = fm_refRevise( inref, inver );

	return_value_if_not_node( inref, false );
	return_value_if( inver == FM_VERSION_NONE, false );

	if ( inver == FM_VERSION_HEAD )
		inver = active_zone->headVersion;

	nd	= _nodeDeref( inref );
	kvp	= _nodeFindMod( nd, inver, &inkey );

	// unset value are undefined
	if ( !kvp || kvp->value.type == FM_DT_NONE )
		return false;

	outvalue = kvp->value;

	// resolve result reference
	if ( kvp->value.type == FM_DT_REF )
		outvalue.content.ref = _reviseRef( outvalue.content.ref, inver );

	return true;
}


bool fm_nodeUnset ( fm_ref inref, const fm_data& inkey )
{
	static fm_data noneData = { FM_DT_NONE };

	return fm_nodeSet( inref, inkey, noneData );
}



bool fm_nodeSet ( fm_ref inref, const fm_data& inkey, const fm_data& invalue )
{
	fm_editNode*	nd;

	inref = fm_refRevise( inref, FM_VERSION_HEAD );

	return_value_if_not_transactioning( false );
	return_value_if_not_node( inref, false );

	//
	// check the key

	if ( inkey.type == FM_DT_NONE )
	{
		// key can't be none
		return false;
	}
	else if ( inkey.type == FM_DT_REF )
	{
		// key can only be a ref on a string

		if ( !fm_refIsString(inkey.content.ref) )
			return false;
	}

	//
	// check the value

	if ( invalue.type == FM_DT_REF )
	{
		fm_ref rev_value_ref;

		rev_value_ref = fm_refRevise( invalue.content.ref, FM_VERSION_HEAD );

		// value ref is valid

		if ( !fm_refIsValid(rev_value_ref) )
			return false;

		// value can't ref itself
		
		if ( rev_value_ref == inref )
			return false;
	}

	//
	// can't modify a non-terminal node!

	if ( !fm_refIsHead(inref) )
		return false;


	nd = _nodeDeref( inref );

	// can append a modifier in the node?

	if ( nd->cnt < FM_NODE_LOAD )
	{
		nd->mods[ nd->cnt ].version	= active_zone->currVersion;
		nd->mods[ nd->cnt ].key		= inkey;
		nd->mods[ nd->cnt ].value	= invalue;

		// flush store operations before to alter the original node
		_WriteBarrier();

		nd->cnt += 1;
	}
	else
	{
		// the node is full => one needs to reallocate it
		//
		// steps are:
		// 1- create a new node
		// 2- flatten the original node into the new one
		// 3- if key is existing => overwrite the value in the new node
		//    if not => append the new kvp
		// 4- sort the kvps (by version mainly)
		// 5- link the nodes

		fm_editNode*	new_nd;
		fm_ref			new_ref;
		fm_keyValueMod*	kvp;

		new_ref = fm_refAlloc( FM_RT_NODE );
		return_false_if( new_ref == FM_BAD_REF );

		new_nd = _nodeDeref( new_ref );

		_nodeDeflate( nd, new_nd, active_zone->currVersion );

		_modSort( new_nd->mods, new_nd->cnt );

		if ( new_nd->cnt == FM_NODE_LOAD )
		{
			// can't reduce the amount of mods => too many unique keys!
			fm_refDealloc( new_ref );
			return false;
		}

		// existing kvp?

		kvp	= _nodeFindMod( new_nd, active_zone->currVersion, &inkey );

		if ( kvp )
		{
			// it's safe here to replace the kvp in the duplicated and not used node
			fm_assert( kvp >= new_nd->mods );
			fm_assert( kvp->key == inkey );
			kvp->version = active_zone->currVersion;
			kvp->value   = invalue;

			_modSort( new_nd->mods, new_nd->cnt );
		}
		else
		{
			new_nd->mods[ new_nd->cnt ].version	= active_zone->currVersion;
			new_nd->mods[ new_nd->cnt ].key		= inkey;
			new_nd->mods[ new_nd->cnt ].value	= invalue;
			nd->cnt += 1;
		}

		fm_refLink( inref, new_ref );
	}

	return true;
}




fm_ref fm_nodeDuplicate ( fm_ref inref, fm_version inver )
{
	fm_editNode*	nd;
	fm_editNode*	new_nd;
	fm_ref			new_ref;

	inref = fm_refRevise( inref, inver );

	return_value_if_not_transactioning( FM_BAD_REF );
	return_value_if_not_node( inref, FM_BAD_REF );
	return_value_if( inver == FM_VERSION_NONE, FM_BAD_REF );

	if ( inver == FM_VERSION_HEAD )
		inver = active_zone->headVersion;

	nd = _nodeDeref( inref );

	new_ref = fm_refAlloc( FM_RT_NODE );
	return_value_if( new_ref == FM_BAD_REF, FM_BAD_REF );

	new_nd = _nodeDeref( new_ref );

	_nodeDeflate( nd, new_nd, inver );

	_modSort( new_nd->mods, new_nd->cnt );
 
	return new_ref;
}




// brushes

fm_editBrush* fm_brushDeref ( fm_ref inref )
{
	return_value_if_not_brush( inref, NULL );

	return (fm_editBrush*) _getKnot(inref)->addr;
}


fm_ref fm_brushAlloc ( )
{
	fm_ref			ref;
	fm_editBrush*	br;

	return_value_if_not_transactioning( FM_BAD_REF );

	ref = fm_refAlloc( FM_RT_BRUSH );
	return_value_if( ref == FM_BAD_REF, FM_BAD_REF );

	br = (fm_editBrush*) _getKnot(ref)->addr;

	br->cnt = 0;

	return ref;
}


fm_ref fm_brushDuplicate ( fm_ref inref, fm_version inver )
{
	fm_editBrush*	br;
	fm_editBrush*	new_br;
	fm_ref			new_ref;

	inref = fm_refRevise( inref, inver );

	return_value_if_not_transactioning( FM_BAD_REF );
	return_value_if_not_brush( inref, FM_BAD_REF );
	return_value_if( inver == FM_VERSION_NONE, FM_BAD_REF );

	if ( inver == FM_VERSION_HEAD )
		inver = active_zone->headVersion;

	br = _brushDeref( inref );

	new_ref = fm_refAlloc( FM_RT_BRUSH );
	return_value_if( new_ref == FM_BAD_REF, FM_BAD_REF );

	new_br = _brushDeref( new_ref );

	_brushDeflate( br, new_br, inver );

	return new_ref;
}


fm_uint fm_brushDeflate ( fm_ref inref, fm_version inver, fm_faceMod* outarr, fm_uint incapacity )
{
	inref = fm_refRevise( inref, inver );

	return_value_if_not_brush( inref, 0 );
	return_value_if( inver == FM_VERSION_NONE, 0 );

	if ( inver == FM_VERSION_HEAD )
		inver = active_zone->headVersion;

	return _brushDeflate( _brushDeref(inref), inver, outarr, incapacity );
}


bool fm_brushGet ( fm_ref inref, fm_version inver, fm_uint inid, fm_face& outFace )
{
	fm_editBrush*	br;
	int				idx;

	inref = fm_refRevise( inref, inver );

	return_value_if_not_brush( inref, false );
	return_value_if( inver == FM_VERSION_NONE, false );

	if ( inver == FM_VERSION_HEAD )
		inver = active_zone->headVersion;

	br = _brushDeref( inref );

	idx = _brushFindFace( br, inid, inver );

	if ( idx < 0 )
		return false;

	outFace = br->mods[ idx ].face;

	return true;
}


bool fm_brushSet ( fm_ref inref, fm_uint inid, const fm_face& inface )
{
	fm_editBrush*	br;
	fm_editBrush*	new_br;
	fm_ref			new_ref;
	fm_ui64			effective;
	int				idx;
	bool			do_unset;

	inref = fm_refRevise( inref, FM_VERSION_HEAD );

	return_value_if_not_transactioning( false );
	return_value_if_not_brush( inref, false );
	return_value_if( inid > FM_BRUSH_LOAD, false );

	do_unset = (inface.plane.dir[0]==0.0f && inface.plane.dir[1]==0.0f && inface.plane.dir[2]==0.0f);

	br = _brushDeref( inref );

	if ( br->cnt == FM_BRUSH_LOAD )
	{
		new_ref = fm_refAlloc( FM_RT_BRUSH );
		return_false_if( new_ref == FM_BAD_REF );

		new_br = _brushDeref( new_ref );

		_brushDeflate( br, new_br, active_zone->currVersion );

		if ( new_br->cnt == FM_BRUSH_LOAD )
		{
			// can't reduce the amount of mods !
			fm_refDealloc( new_ref );
			return false;
		}

		br = new_br;
	}
	else
	{
		new_ref = FM_BAD_REF;
		new_br  = NULL;
	}

	// find the previous version of the face

	idx = _brushFindFace( br, inid, FM_VERSION_HEAD );

	if ( idx >= 0 && new_br )
	{
		// it's safe to replace the flattened face
		fm_assert( br->mods[ idx ].id == inid );
		br->mods[ idx ].version = active_zone->currVersion;
		br->mods[ idx ].face	= inface;

		if( do_unset )
			_bitClear( br->mods[ idx ].effective, idx );

		_modSort( br->mods, br->cnt );
	}
	else
	{
		effective = (br->cnt ? br->mods[ br->cnt-1 ].effective : 0);

		// unset the previous version
		if ( idx >= 0 )
			_bitClear( effective, idx );

		// set this new plane if not null
		if ( !do_unset )
			_bitSet( effective, br->cnt );

		// no need to unset a not existing face!
		if ( idx < 0 && do_unset )
		{
			// pass
		}
		else
		{
			br->mods[ br->cnt ].version	  = active_zone->currVersion;
			br->mods[ br->cnt ].id		  = inid;
			br->mods[ br->cnt ].face	  = inface;
			br->mods[ br->cnt ].effective = effective;

			// flush store operations before to alter the original brush
			_WriteBarrier();

			br->cnt += 1;
		}
	}

	fm_refLink( inref, new_ref );

	return true;
}


bool fm_brushUnset ( fm_ref inref, fm_uint inid )
{
	static fm_plane null_plane = { 0, 0, 0, 0 };
	static fm_face  null_face  = { null_plane };

	return fm_brushSet( inref, inid, null_face );
}







size_t fm_countPage ( )
{
	return_value_if_no_zone( 0 );

	return active_zone->directory.pagei;
}


size_t fm_countElementOfType ( fm_refType intype )
{
	fm_page* pg;
	fm_uint  pi;
	fm_uint  ki;
	size_t	 cnt;

	return_value_if_no_zone( 0 );

	cnt = 0;

	for ( pi = 0 ; pi < active_zone->directory.pagei ; pi++ )
	{
		pg = active_zone->directory.pages[pi];
		
		if ( pg->editType != intype )
			continue;

		for ( ki = 0 ; ki < FM_PAGE_LOAD ; ki++ )
		{
			if ( pg->knots[ki].version )
			{
				cnt += 1;
			}
		}
	}

	return cnt;
}




static size_t _getZoneMemoryMinimalSize ( )
{
	// let's say the minimal size holds 1 page of each type

	size_t sz = 0;

	for ( int t=FM_RT_NONE+1 ; t < FM_RT_LAST ; t++ )
	{
		if ( t == FM_RT_ARRAY )
			sz += sizeof(fm_page) + 4096 * FM_PAGE_LOAD;
		else
			sz += sizeof(fm_page) + _editUsageBSize( FM_UT_PHYSICAL, fm_refType(t), NULL ) * FM_PAGE_LOAD;
	}

	return sz;
}


static float _getZoneMemoryFillingRate ( bool fast )
{
	size_t		min_size;
	size_t		max_size;
	size_t		left_size;
	size_t		used_size;

	return_value_if_no_zone( 0.0f );

	min_size  = _getZoneMemoryMinimalSize();
	fm_assert( min_size );

	max_size  = active_zone->hiAddr - active_zone->loAddr;
	used_size = active_zone->bsAddr - active_zone->loAddr;
	left_size = active_zone->hiAddr - active_zone->bsAddr;
	fm_assert( left_size );

	if ( max_size <= min_size || left_size <= min_size )
		return 1.0f;
	else
		return fast ? 0.0f : float( double(used_size) / double(max_size-min_size) );
}


static float _getZonePageFillingRate ( bool fast )
{
	unsigned	page_max;

	return_value_if_no_zone( 0.0f );

	page_max = FM_DIRECTORY_LOAD - (FM_RT_LAST-1);

	if ( active_zone->directory.pagei > page_max )
		return 1.0f;
	else
		return fast ? 0.0f : float(active_zone->directory.pagei) / float(page_max);
}


static float _getZoneFillingRate ( bool fast )
{
	float memRate;
	float pageRate;

	memRate  = _getZoneMemoryFillingRate( fast );
	pageRate = _getZonePageFillingRate( fast );

	return (memRate > pageRate) ? memRate : pageRate;
}


bool fm_isFull ( )
{
	return _getZoneFillingRate(true) >= 1.0f;
}



float fm_getFillingRate ( )
{
	return _getZoneFillingRate(false);
}



static bool _check_ref_revision_integrity ( fm_ref inref, fm_ref inrefmax, bool* invisited )
{
	fm_knot* kn;
	fm_knot *kn0, *kn1;

	if ( inref >= inrefmax )
	{
		return false;
	}

	if ( !_checkAllocated(inref) )
	{
		return false;
	}

	if ( invisited[inref] )
	{
		return true;
	}

	kn = _getKnot( inref );

	// check the previous revisions

	{
		kn0 = kn;

		for ( ;; )
		{
			if ( kn0->prev == FM_BAD_REF )
				break;

			if ( !_checkAllocated(kn0->prev) )
			{
				return false;
			}

			kn1 = _getKnot( kn0->prev );

			if( kn1->version >= kn0->version )
			{
				return false;
			}

			invisited[ kn0->prev ] = true;

			kn0 = kn1;
		}
	}

	// check the next revisions

	{
		kn0 = kn;

		for ( ;; )
		{
			if ( kn0->next == FM_BAD_REF )
				break;

			if ( !_checkAllocated(kn0->next) )
			{
				return false;
			}

			kn1 = _getKnot( kn0->next );

			if( kn1->version <= kn0->version )
			{
				return false;
			}

			invisited[ kn0->next ] = true;

			kn0 = kn1;
		}
	}

	return true;
}


static bool _check_all_ref_revision_integrity ( )
{
	fm_page* pg;
	fm_uint  pi;
	fm_uint  ki;

	unsigned ref_max;
	unsigned ref_i;
	bool*	 ref_visited;
	
	bool	 res;

	ref_max = _ref( active_zone->directory.pagei, 0 );

	if ( ref_max == 0 )
		return true;

	ref_visited = new bool[ ref_max ];
	fm_assert( ref_visited );

	for ( unsigned i=0 ; i < ref_max ; i++ )
		ref_visited[i] = false;

	res = true;

	for ( pi = 0 ; pi < active_zone->directory.pagei ; pi++ )
	{
		pg = active_zone->directory.pages[ pi ];
		fm_assert( pg );
		fm_assert( pg->editType != FM_RT_NONE );

		for ( ki = 0 ; ki < FM_PAGE_LOAD ; ki++ )
		{
			if ( pg->knots[ki].version )
			{
				ref_i = _ref(pi,ki);

				if ( !_check_ref_revision_integrity(ref_i,ref_max,ref_visited) )
				{
					res = false;
					break;
				}
			}
		}

		if ( !res )
			break;
	}

	delete ref_visited;
	ref_visited = NULL;

	return res;
}


static bool _check_all_editables ( )
{
	fm_page* pg;
	fm_uint  pi;
	fm_uint  ki;

	for ( pi = 0 ; pi < active_zone->directory.pagei ; pi++ )
	{
		pg = active_zone->directory.pages[ pi ];
		fm_assert( pg );
		fm_assert( pg->editType != FM_RT_NONE );

		for ( ki = 0 ; ki < FM_PAGE_LOAD ; ki++ )
		{
			if ( pg->knots[ki].version )
			{
				fm_assert( pg->knots[ki].addr );
				if ( !pg->knots[ki].addr )
					return false;

				if ( pg->editType == FM_RT_NODE )
				{
					fm_editNode* nd = (fm_editNode*) pg->knots[ki].addr;
					fm_assert( _nodeCheck(nd) );
					if ( !_nodeCheck(nd) )
						return false;
				}
				else if ( pg->editType == FM_RT_BRUSH )
				{
					fm_editBrush* br = (fm_editBrush*) pg->knots[ki].addr;
					fm_assert( _brushCheck(br) );
					if ( !_brushCheck(br) )
						return false;
				}
				else if ( pg->editType == FM_RT_ARRAY )
				{
					fm_editArray* ar = (fm_editArray*) pg->knots[ki].addr;
					fm_assert( _arrayCheck(ar) );
					if ( !_arrayCheck(ar) )
						return false;
				}
			}
		}
	}

	return true;
}


bool fm_checkIntegrity ( )
{
	return_value_if_no_zone( false );

	if ( !_checkZone() )
		return false;

	if ( !_check_all_ref_revision_integrity() )
		return false;

	if ( !_check_all_editables() )
		return false;

	return true;
}




size_t fm_probeMemoryUsage ( fm_usageType inusage )
{
	size_t	 sz;

	return_value_if_no_zone( 0 );
	fm_assert( _checkZone() );

	if ( inusage == FM_UT_PHYSICAL )
	{
		sz = active_zone->hiAddr - active_zone->loAddr;
	}

	else if ( inusage == FM_UT_LOGICAL )
	{
		sz = active_zone->bsAddr - active_zone->loAddr;
	}

	else if ( inusage == FM_UT_PAYLOAD )
	{
		// for each page, cumulate the size of allocated editables and the related containers
		// D$ line alignment padding is not considered here.

		fm_page* pg;
		fm_uint  pi;
		fm_uint  ki;

		sz = sizeof(fm_zone);

		for ( pi = 0 ; pi < active_zone->directory.pagei ; pi++ )
		{
			pg = active_zone->directory.pages[ pi ];
			fm_assert( pg );
			fm_assert( pg->editType != FM_RT_NONE );

			sz += sizeof(fm_page);

			for ( ki = 0 ; ki < FM_PAGE_LOAD ; ki++ )
			{
				if ( pg->knots[ki].version )
				{
					sz += sizeof(fm_knot);
					sz += _editUsageBSize( inusage, pg->editType, (fm_editBase*)pg->knots[ki].addr );
				}
			}
		}
	}

	else if ( inusage == FM_UT_DATA )
	{
		// for each page, cumulate the size of allocated editables only

		fm_page* pg;
		fm_uint  pi;
		fm_uint  ki;

		sz = 0;

		for ( pi = 0 ; pi < active_zone->directory.pagei ; pi++ )
		{
			pg = active_zone->directory.pages[ pi ];
			fm_assert( pg );
			fm_assert( pg->editType != FM_RT_NONE );

			for ( ki = 0 ; ki < FM_PAGE_LOAD ; ki++ )
			{
				if ( pg->knots[ki].version )
				{
					sz += _editUsageBSize( inusage, pg->editType, (fm_editBase*)pg->knots[ki].addr );
				}
			}
		}
	}

	else
	{
		sz = 0;
	}

	return sz;
}


size_t fm_probeEditableUsage ( fm_usageType inusage )
{
	size_t	 sz;

	return_value_if_no_zone( 0 );
	fm_assert( _checkZone() );

	if ( inusage == FM_UT_PHYSICAL )
	{
		sz = FM_DIRECTORY_LOAD * FM_PAGE_LOAD;
	}

	else if ( inusage == FM_UT_LOGICAL )
	{
		sz = active_zone->directory.pagei * FM_PAGE_LOAD;
	}

	else if ( inusage == FM_UT_PAYLOAD || inusage == FM_UT_DATA )
	{
		// for each page, cumulate the amount of allocated editables only

		fm_page* pg;
		fm_uint  pi;
		fm_uint  ki;

		sz = 0;

		for ( pi = 0 ; pi < active_zone->directory.pagei ; pi++ )
		{
			pg = active_zone->directory.pages[ pi ];
			fm_assert( pg );
			fm_assert( pg->editType != FM_RT_NONE );

			for ( ki = 0 ; ki < FM_PAGE_LOAD ; ki++ )
			{
				if ( pg->knots[ki].version )
				{
					sz += _editUsageAmount( inusage, pg->editType, (fm_editBase*)pg->knots[ki].addr );
				}
			}
		}
	}

	else
	{
		sz = 0;
	}

	return sz;
}


