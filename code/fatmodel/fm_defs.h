
#define fm_check( C ) \
			fm_assert( C ); \
			if( !(C) ) return false;


#define fm_assert_page( REF ) \
			fm_assert( _checkPage(REF) )

#define fm_assert_knot( REF ) \
			fm_assert( _checkKnot(REF) )

#define fm_assert_ref( REF ) \
			fm_assert( _checkAllocated(REF) )

#define fm_assert_type( REF, TYPE ) \
			fm_assert( _getType(REF) == TYPE );

#define return_if_bad_page( REF ) \
			return_if( (REF)==FM_BAD_REF ); \
			return_if( !_checkPage(REF) );

#define return_value_if_bad_page( REF, VALUE ) \
			return_value_if( (REF)==FM_BAD_REF, VALUE ); \
			return_value_if( !_checkPage(REF), VALUE );

#define return_if_bad_ref( REF ) \
			return_if( (REF)==FM_BAD_REF ); \
			return_if( !_checkAllocated(REF) );

#define return_value_if_bad_ref( REF, VALUE ) \
			return_value_if( (REF)==FM_BAD_REF, VALUE ); \
			return_value_if( !_checkAllocated(REF), VALUE );

#define return_value_if_not_string( REF, VALUE ) \
			return_value_if_bad_ref( REF, VALUE ); \
			return_value_if( _getType(REF) < FM_RT_SSTRING, NULL ); \
			return_value_if( _getType(REF) > FM_RT_LSTRING, NULL );

#define return_value_if_not_array( REF, VALUE ) \
			return_value_if_bad_ref( REF, VALUE ); \
			return_value_if( _getType(REF) > FM_RT_ARRAY, NULL );

#define return_value_if_not_node( REF, VALUE ) \
			return_value_if_bad_ref( REF, VALUE ); \
			return_value_if( _getType(REF) != FM_RT_NODE, NULL );

#define return_value_if_not_brush( REF, VALUE ) \
			return_value_if_bad_ref( REF, VALUE ); \
			return_value_if( _getType(REF) != FM_RT_BRUSH, NULL );

#define return_value_if_not_patch( REF, VALUE ) \
			return_value_if_bad_ref( REF, VALUE ); \
			return_value_if( _getType(REF) != FM_RT_PATCH, NULL );

#define return_value_if_not_model( REF, VALUE ) \
			return_value_if_bad_ref( REF, VALUE ); \
			return_value_if( _getType(REF) != FM_RT_MODEL, NULL );

#define return_value_if_no_zone( VALUE ) \
			return_value_if( !active_zone, VALUE );

#define return_value_if_transactioning( VALUE ) \
			return_value_if( !active_zone, VALUE ); \
			return_value_if( active_zone->currVersion != FM_VERSION_NONE, VALUE );

#define return_value_if_not_transactioning( VALUE ) \
			return_value_if( !active_zone, VALUE ); \
			return_value_if( active_zone->currVersion==FM_VERSION_NONE, VALUE );








namespace
{

	#define _bitFindFirstSet( m, b ) \
				_BitScanForward64( &b, m)

	#define _bitTest( m, b ) \
				_bittest64( (__int64*)&m, b )

	#define _bitClear( m, b ) \
		_bittestandreset64( (__int64*)&m, b )

	#define _bitSet( m, b ) \
		_bittestandset64( (__int64*)&m, b )

	#define _bitTestAndClear( m, b ) \
		_bittestandreset64( (__int64*)&m, b )

	#define _bitTestAndSet( m, b ) \
		_bittestandset64( (__int64*)&m, b )


	inline unsigned _ref2page ( fm_ref inref )
	{
		return inref >> FM_PAGE_BITS;
	}


	inline unsigned _ref2index ( fm_ref inref )
	{
		return inref & (FM_PAGE_LOAD - 1);
	}


	inline fm_ref _ref ( unsigned inpage, unsigned inindex  )
	{
		fm_assert( inindex < FM_PAGE_LOAD );
		return (inpage << FM_PAGE_BITS) | inindex;
	}


	bool _checkZone ( )
	{
		return_false_if( !active_zone );
		return_false_if( active_zone->magic != FM_MAGIC );
		return_false_if( active_zone->revision > FM_REVISION );

		fm_assert( (active_zone->currVersion == FM_VERSION_NONE) || (active_zone->currVersion > active_zone->headVersion) );
		fm_assert( active_zone->bsAddr >= active_zone->loAddr );
		fm_assert( active_zone->bsAddr <  active_zone->hiAddr );

		return true;
	}


	bool _checkPage ( fm_ref inref )
	{
		// This function is safe against REF

		fm_uint  pi;
		fm_page* pg;

		fm_assert( active_zone );
		return_false_if( inref == FM_BAD_REF );

		pi = _ref2page( inref );
		return_false_if( pi >= FM_DIRECTORY_LOAD );
		return_false_if( pi >= active_zone->directory.pagei );

		pg = active_zone->directory.pages[ pi ];
		return_false_if( !pg );

		fm_assert( pg->editType  > FM_RT_NONE );
		fm_assert( pg->editBSize > 0 );
		fm_assert( pg->loAddr );
		fm_assert( pg->hiAddr );

		return true;
	}


	bool _checkKnot ( fm_ref inref )
	{
		// This function is safe against REF

		fm_uint  pi;
		fm_uint	 ki;
		fm_page* pg;
		fm_knot* kn;

		fm_assert( active_zone );
		return_false_if( inref == FM_BAD_REF );

		pi = _ref2page( inref );
		ki = _ref2index( inref );
		return_false_if( pi >= FM_DIRECTORY_LOAD );
		return_false_if( pi >= active_zone->directory.pagei );

		pg = active_zone->directory.pages[ pi ];
		return_false_if( !pg );

		fm_assert( pg->editType  > FM_RT_NONE );
		fm_assert( pg->editBSize > 0 );
		fm_assert( pg->loAddr );
		fm_assert( pg->hiAddr );

		kn = pg->knots + ki;
		fm_assert( kn->addr >= pg->loAddr );
		fm_assert( kn->addr <  pg->hiAddr );

		return true;
	}


	bool _checkAllocated ( fm_ref inref )
	{
		// This function is safe against REF

		fm_uint  pi;
		fm_uint	 ki;
		fm_page* pg;
		fm_knot* kn;

		fm_assert( active_zone );
		return_false_if( inref == FM_BAD_REF );

		pi = _ref2page( inref );
		ki = _ref2index( inref );
		return_false_if( pi >= FM_DIRECTORY_LOAD );
		return_false_if( pi >= active_zone->directory.pagei );

		pg = active_zone->directory.pages[ pi ];
		return_false_if( !pg );

		fm_assert( pg->editType  > FM_RT_NONE );
		fm_assert( pg->editBSize > 0 );
		fm_assert( pg->loAddr );
		fm_assert( pg->hiAddr );

		kn = pg->knots + ki;
		fm_assert( kn->addr >= pg->loAddr );
		fm_assert( kn->addr <  pg->hiAddr );

		return (kn->version != FM_VERSION_NONE);
	}


	fm_refType _getType ( fm_ref inref )
	{
		fm_uint  pi;
		fm_page* pg;

		fm_assert_page( inref );

		pi = _ref2page( inref );
		pg = active_zone->directory.pages[ pi ];

		return pg->editType;
	}


	fm_page* _getPage ( fm_ref inref )
	{
		fm_uint		pi;
		fm_page*	pg;

		fm_assert_page( inref );
	
		pi = _ref2page( inref );
		pg = active_zone->directory.pages[ pi ];

		return pg;
	}


	fm_knot* _getKnot ( fm_ref inref )
	{
		fm_uint		pi;
		fm_uint		ki;
		fm_page*	pg;

		fm_assert_knot( inref );
	
		pi = _ref2page( inref );
		ki = _ref2index( inref );
		pg = active_zone->directory.pages[ pi ];

		return pg->knots + ki;
	}


	fm_ref _findFreeRefFrom ( fm_ref infromref, fm_refType intype, fm_uint ineditbsize )
	{
		fm_uint  pi;
		fm_uint  ki;
		fm_page* pg;

		if ( infromref == FM_BAD_REF )
			return FM_BAD_REF;

		fm_assert( active_zone );

		pi = _ref2page( infromref );
		ki = _ref2index( infromref );

		for ( pi ; pi < active_zone->directory.pagei ; pi++, ki=0 )
		{
			pg = active_zone->directory.pages[ pi ];
			fm_assert( pg );

			if ( pg->editType != intype )
				continue;

			if ( pg->editBSize < ineditbsize )
				continue;

			for ( ; ki < FM_PAGE_LOAD ; ki++ )
			{
				if ( pg->knots[ki].version == FM_VERSION_NONE )
				{
					return _ref( pi, ki );
				}
			}
		}

		return FM_BAD_REF;
	}


	template < typename T >
	fm_ref _findString ( fm_refType intype, fm_string instr )
	{
		fm_page*	pg;
		fm_uint		pi;
		fm_uint		ki;

		fm_assert( active_zone );

		for ( pi = 0 ; pi < active_zone->directory.pagei ; pi++ )
		{
			pg = active_zone->directory.pages[ pi ];
			fm_assert( pg );

			if ( pg->editType != intype )
				continue;

			T* kstr = (T*) pg->loAddr;

			for ( ki = 0 ; ki < FM_PAGE_LOAD ; ki++, kstr++ )
			{
				if ( pg->knots[ki].version != FM_VERSION_NONE )
				{
					if ( _stricmp(instr,kstr->text) == 0 )
					{
						return _ref( pi, ki );
					}
				}
			}
		}

		return FM_BAD_REF;
	}


	fm_ref _reviseRef ( fm_ref inref, fm_version inver )
	{
		fm_knot*	kn;
		fm_knot*	kn_next;
		fm_ref		ref;

		fm_assert_ref( inref );

		ref = inref;
		kn  = _getKnot( inref );

		if ( kn->version > inver )
		{
			// downgrade the reference with previous versions
			for ( ;; )
			{
				ref = kn->prev;

				if ( ref == FM_BAD_REF )
					break;

				kn  = _getKnot( ref );

				if ( kn->version <= inver )
					break;
			}
		}

		else if ( kn->version < inver )
		{
			// upgrade the reference
			for ( ;; )
			{
				if ( kn->next == FM_BAD_REF )
					break;

				kn_next = _getKnot( kn->next );

				if ( kn_next->version > inver )
					break;

				ref = kn->next;
				kn  = kn_next;
			}
		}

		return ref;
	}


	template< typename T >
	int _modFind ( T* inarr, fm_int incnt, fm_version inver )
	{
		// find the highest mod of version inver or less

		int i, j, k;

		return_value_if( incnt == 0, -1 );
		return_value_if( inver == FM_VERSION_NONE, -1 );
		return_value_if( inver == FM_VERSION_HEAD, incnt-1 );
		return_value_if( inarr[0].version > inver, -1 );

		i = 0;
		j = incnt - 1;

		while( j > i )
		{
			k = ((i+j) >> 1) + 1;

			if ( inarr[k].version <= inver )
				i = k;
			else
				j = k-1;
		}

		fm_assert( i < incnt );
		fm_assert( inarr[i].version <= inver );
		fm_assert( (i+1 == incnt) || (inarr[i+1].version > inver) );

		return i;
	}


	int _modCompare ( const void* inm1, const void* inm2 )
	{
		fm_assert( inm1 );
		fm_assert( inm2 );

		fm_mod* m1 = (fm_mod*) inm1;
		fm_mod* m2 = (fm_mod*) inm2;

		if ( m1 == m2 )						return 0;
		if ( m1->version < m2->version )	return -1;
		if ( m1->version > m2->version )	return +1;
		return 0;
	}


	template< typename T >
	void _modSort ( T* inarr, fm_int incnt )
	{
		qsort( inarr, incnt, sizeof(T), _modCompare );
	}


	bool _nodeCheck ( fm_editNode* nd )
	{
		for ( unsigned i=0 ; i < nd->cnt ; i++ )
		{
			fm_check( (i==0) || (nd->mods[i].version >= nd->mods[i-1].version) );
			fm_check( nd->mods[i].key.type != FM_DT_NONE );
			fm_check( nd->mods[i].key.type != FM_DT_REF || fm_refIsString(nd->mods[i].key.content.ref) );
			fm_check( nd->mods[i].value.type != FM_DT_REF || fm_refIsValid(nd->mods[i].value.content.ref) );
		}

		return true;
	}


	fm_editNode* _nodeDeref ( fm_ref inref )
	{
		fm_assert_ref( inref );
		fm_assert_type( inref, FM_RT_NODE );
		fm_assert( _getKnot(inref) );
		fm_assert( _getKnot(inref)->addr );

		return (fm_editNode*) _getKnot(inref)->addr;
	}


	fm_keyValueMod* _nodeFindMod ( fm_editNode* nd, fm_version inver, const fm_data* inkey, fm_ui64& invisited )
	{
		fm_keyValueMod* kvp;
		fm_keyValueMod* kvp_ver;
		fm_keyValueMod* kvp_last;
		unsigned long	ki;

		fm_assert( _nodeCheck(nd) );

		if ( !_bitFindFirstSet(invisited,ki) )
			return NULL;

		kvp			= nd->mods + ki;
		kvp_last	= nd->mods + nd->cnt;
		kvp_ver		= NULL;

		for ( ; kvp < kvp_last ; ki++, kvp++ )
		{
			if ( _bitTestAndClear(invisited,ki) == 0 )
				continue;

			if ( kvp->version > inver )
				return NULL;

			if ( kvp->value.type == FM_DT_NONE )
				continue;

			if ( inkey && *inkey != kvp->key )
				continue;

			kvp_ver = kvp;

			fm_assert( kvp->key.type != FM_DT_NONE );
				
			ki++;
			kvp++;

			for ( ; kvp < kvp_last ; ki++, kvp++ )
			{
				fm_assert( kvp->version >= kvp_ver->version );

				if ( _bitTest(invisited,ki) == 0 )
					continue;

				if ( kvp->key != kvp_ver->key )
					continue;

				_bitClear(invisited,ki);

				if ( kvp->version <= inver )
				{
					kvp_ver = kvp;
					continue;
				}

				return kvp_ver;
			}

			return kvp_ver;
		}

		return kvp_ver;
	}


	fm_keyValueMod* _nodeFindMod ( fm_editNode* nd, fm_version inver, const fm_data* inkey )
	{
		fm_ui64 visited = fm_ui64(-1);
		return _nodeFindMod( nd, inver, inkey, visited );
	}


	fm_keyValueMod* _nodeFirstMod ( fm_editNode* nd, fm_version inver, fm_ui64& invisited )
	{
		invisited = fm_ui64(-1);
		return _nodeFindMod( nd, inver, NULL, invisited );
	}


	fm_keyValueMod* _nodeNextMod ( fm_editNode* nd, fm_version inver, fm_ui64& invisited )
	{
		return _nodeFindMod( nd, inver, NULL, invisited );
	}


	fm_uint _nodeDeflate ( fm_editNode* nd, fm_version inver, fm_keyValueMod* outarr, fm_uint incapacity )
	{
		fm_uint			cnt;
		fm_keyValueMod*	kvp;
		fm_ui64			visited;

		fm_assert( nd );
		return_value_if( inver == FM_VERSION_NONE, 0 );
		return_value_if( !outarr, 0 );
		return_value_if( incapacity == 0, 0 );
		static_assert( sizeof(visited)*8 >= FM_NODE_LOAD, "too short" );

		if ( inver == FM_VERSION_HEAD )
			inver = active_zone->headVersion;

		cnt	= 0;
		kvp	= _nodeFirstMod( nd, inver, visited );

		while ( kvp )
		{
			// skip unset values
			if ( kvp->value.type != FM_DT_NONE )
			{
				outarr[cnt++] = *kvp;

				if ( cnt == incapacity )
					break;
			}

			kvp = _nodeNextMod( nd, inver, visited );
		}

		return cnt;
	}


	void _nodeDeflate ( fm_editNode* from, fm_editNode* to, fm_version inver )
	{
		to->cnt = _nodeDeflate( from, inver, to->mods, FM_NODE_LOAD );
		fm_assert( to->cnt <= FM_NODE_LOAD );
	}





	fm_editBrush* _brushDeref ( fm_ref inref )
	{
		fm_assert_ref( inref );
		fm_assert_type( inref, FM_RT_BRUSH );
		fm_assert( _getKnot(inref) );
		fm_assert( _getKnot(inref)->addr );

		return (fm_editBrush*) _getKnot(inref)->addr;
	}


	bool _brushCheck ( fm_editBrush* br )
	{
		fm_uint			id;
		int				indexOfId[ FM_BRUSH_LOAD ];
		
		for ( int i=0 ; i < FM_BRUSH_LOAD ; i++ )
		{
			indexOfId[i] = -1;
		}

		for ( unsigned i=0 ; i < br->cnt ; i++ )
		{
			// check order on versions
			fm_check( i==0 || (br->mods[i].version >= br->mods[i-1].version) );

			id = br->mods[i].id;
			fm_assert( id < FM_BRUSH_LOAD );

			// check the previous plane id is not flagged in the effective mask

			if ( indexOfId[id] >= 0 )
			{
				fm_check( _bitTest(br->mods[i].effective,indexOfId[id]) == 0 );
			}

			// plane has been added or removed ?
			if( _bitTest(br->mods[i].effective,i) )
				indexOfId[id] = int(i);
			else
				indexOfId[id] = -1;
		}

		return true;
	}


	fm_uint _brushDeflate ( fm_editBrush* br, fm_version inver, fm_faceMod* outarr, fm_uint incapacity )
	{
		int				hi;
		unsigned long	bi;
		fm_uint			cnt;
		fm_ui64			effective;

		fm_assert( br );
		return_value_if( inver == FM_VERSION_NONE, 0 );
		return_value_if( !outarr, 0 );
		return_value_if( incapacity == 0, 0 );

		if ( inver == FM_VERSION_HEAD )
			inver = active_zone->headVersion;

		hi  = _modFind( br->mods, br->cnt, inver );

		if ( hi < 0 )
			return 0;

		effective = br->mods[hi].effective;

		cnt = 0;

		while( _bitFindFirstSet(effective,bi) )
		{
			outarr[ cnt++ ] = br->mods[bi];

			if ( cnt == incapacity )
				break;

			_bitClear(effective,bi);
		}

		fm_assert( _brushCheck(br) );

		return cnt;
	}


	void _brushDeflate ( fm_editBrush* from, fm_editBrush* to, fm_version inver )
	{
		fm_ui64		effective;

		fm_assert( _brushCheck(from) );

		to->cnt = _brushDeflate( from, inver, to->mods, FM_BRUSH_LOAD );
		fm_assert( to->cnt <= FM_BRUSH_LOAD );

		// all the mods are unique and used at this point
		effective = (1 << to->cnt) - 1;

		for ( fm_uint i=0 ; i < to->cnt ; i++ )
			to->mods[i].effective = effective;

		fm_assert( _brushCheck(to) );
	}


	int _brushFindFace ( fm_editBrush* br, fm_uint inid, fm_version inver )
	{
		int				hi;
		unsigned long	bi;
		fm_ui64			effective;

		hi = _modFind( br->mods, br->cnt, inver );

		if ( hi < 0 )
			return -1;

		effective = br->mods[hi].effective;

		while( _bitFindFirstSet(effective,bi) )
		{
			if ( br->mods[bi].id == inid )
				return bi;

			_bitClear(effective,bi);
		}

		return -1;
	}






	fm_ref _arrayTypeStride ( fm_arrayType intype )
	{
		if( intype == FM_ARRAY_U8 )				return 1;
		if( intype == FM_ARRAY_U32 )			return sizeof(fm_uint);
		if( intype == FM_ARRAY_FLOAT )			return sizeof(float);
		if( intype == FM_ARRAY_LOCATION )		return sizeof(float) * 3;
		if( intype == FM_ARRAY_TRI )			return sizeof(int) * 3;
		if( intype == FM_ARRAY_MAPPING )		return sizeof(float) * 2;
		if( intype == FM_ARRAY_EDGE )			return sizeof(int) * 2;
		if( intype == FM_ARRAY_NORMAL )			return sizeof(float) * 3;
		if( intype == FM_ARRAY_REF )			return sizeof(fm_ref);
		return 0;
	}


	fm_editArray* _arrayDeref ( fm_ref inref )
	{
		fm_assert_ref( inref );
		fm_assert_type( inref, FM_RT_ARRAY );
		fm_assert( _getKnot(inref) );
		fm_assert( _getKnot(inref)->addr );

		return (fm_editArray*) _getKnot(inref)->addr;
	}


	bool _arrayCheck ( fm_editArray* ar )
	{
		fm_int idx;

		for ( int i=0 ; i < (int)ar->cnt ; i++ )
		{
			idx = ar->mods[i].index;

			fm_check( i==0 || (ar->mods[i].version >= ar->mods[i-1].version) );
			
			if ( idx >= 0 )
			{
				fm_check( idx <= int(ar->mods[i].last) );
			}
			else
			{
				fm_check( -idx-1 <= int(ar->mods[i].last) );
			}
		}

		return true;
	}


	fm_uint _arrayDeflate ( fm_editArray* ar, fm_version inver, fm_arrayMod* outmods, void* outdata, fm_uint incapacity )
	{
		fm_uint			cnt;
		fm_uint			stride;
		fm_int			modi;
		fm_int			idx;
		fm_int			last;
		int*			deflate_array;
		char*			outbase;
		char*			inbase;

		fm_assert( ar );
		return_value_if( inver == FM_VERSION_NONE, 0 );
		return_value_if( !outmods, 0 );
		return_value_if( !outdata, 0 );
		return_value_if( incapacity == 0, 0 );
		return_value_if( ar->cnt == 0, 0 );

		if ( inver == FM_VERSION_HEAD )
			inver = active_zone->headVersion;

		modi = _modFind( ar->mods, ar->cnt, inver );

		if ( modi < 0 )
			return 0;

		last = ar->mods[ modi ].last;
		fm_assert( last >= 0 );

		// allocate on the heap for MT purpose ... I don't like it too much.
		deflate_array = new int[ last+1 ];
		fm_assert( deflate_array );

		for ( signed i=0 ; i < last ; i++ )
		{
			deflate_array[i] = -1;
		}

		for ( signed i=0 ; i < modi ; i++ )
		{
			if ( ar->mods[i].version > inver )
				break;

			idx = ar->mods[i].index;

			if ( idx >= 0 )
			{
				fm_assert( idx <= last );
				deflate_array[idx] = i;
			}
			else
			{
				idx = -idx -1;
				fm_assert( idx <= last );
				deflate_array[idx] = -1;
			}
		}

		outbase = (char*)outdata;
		inbase  = (char*)ar->data;
		stride  = ar->stride;
		cnt     = 0;
		last	= 0;

		for ( idx=0 ; idx < int(last) ; idx++ )
		{
			signed i = deflate_array[idx];

			if ( i >= 0 )
			{
				outmods[cnt].version = ar->mods[i].version;
				outmods[cnt].index   = idx;
				outmods[cnt].last    = idx;
				::memcpy( outbase+cnt*stride, inbase+i*stride, stride );

				cnt += 1;

				if ( cnt == incapacity )
					break;
			}
		}

		delete deflate_array;
		deflate_array = NULL;

		return cnt;
	}


	void _arrayDeflate ( fm_editArray* from, fm_editArray* to, fm_version inver )
	{
		fm_assert( _arrayCheck(from) );

		to->cnt = _arrayDeflate( from, inver, to->mods, (void*)to->data, to->max );
		fm_assert( to->cnt <= FM_BRUSH_LOAD );

		fm_assert( _arrayCheck(to) );
	}

	
	fm_uint _editUsageBSize ( fm_usageType inusage, fm_refType intype, fm_editBase* inedit )
	{
		if( intype == FM_RT_SSTRING )
		{
			if ( inusage == FM_UT_PHYSICAL )
			{
				return sizeof(fm_editSString);
			}
			else
			{
				fm_assert( inedit );
				fm_editSString* s = (fm_editSString*) inedit;
				return (fm_uint) ::strlen( s->text ) + 1;
			}
		}

		else if( intype == FM_RT_MSTRING )
		{
			if ( inusage == FM_UT_PHYSICAL )
			{
				return sizeof(fm_editMString);
			}
			else
			{
				fm_assert( inedit );
				fm_editMString* s = (fm_editMString*) inedit;
				return (fm_uint) ::strlen( s->text ) + 1;
			}
		}

		else if( intype == FM_RT_LSTRING )
		{
			if ( inusage == FM_UT_PHYSICAL )
			{
				return sizeof(fm_editLString);
			}
			else
			{
				fm_assert( inedit );
				fm_editLString* s = (fm_editLString*) inedit;
				return (fm_uint) ::strlen( s->text ) + 1;
			}
		}

		else if ( intype == FM_RT_ARRAY )
		{
			if ( inusage == FM_UT_PHYSICAL )
			{
				fm_assert( inedit );
				fm_editArray* ar = (fm_editArray*) inedit;
				return sizeof(fm_editArray) + (ar->stride * ar->max);
			}
			else if ( inusage == FM_UT_LOGICAL )
			{
				fm_assert( inedit );
				fm_editArray* ar = (fm_editArray*) inedit;
				return sizeof(fm_editArray) + (ar->stride * ar->cnt);
			}
			else
			{
				// todo
				fm_assert( inedit );
				fm_editArray* ar = (fm_editArray*) inedit;
				return (ar->stride * ar->cnt);
			}
		}

		else if( intype == FM_RT_NODE )
		{
			if ( inusage == FM_UT_PHYSICAL )
			{
				return sizeof(fm_editNode);
			}
			else if ( inusage == FM_UT_LOGICAL )
			{
				fm_assert( inedit );
				fm_editNode* nd = (fm_editNode*) inedit;
				return nd->cnt * sizeof(fm_keyValueMod);
			}
			else
			{
				// todo
				fm_assert( inedit );
				fm_editNode* nd = (fm_editNode*) inedit;
				return nd->cnt * sizeof(fm_keyValue);
			}
		}

		else if ( intype == FM_RT_BRUSH )
		{
			if ( inusage == FM_UT_PHYSICAL )
			{
				return sizeof(fm_editBrush);
			}
			else if ( inusage == FM_UT_LOGICAL )
			{
				fm_assert( inedit );
				fm_editBrush* br = (fm_editBrush*) inedit;
				return br->cnt * sizeof(fm_faceMod);
			}
			else
			{
				// todo
				fm_assert( inedit );
				fm_editBrush* br = (fm_editBrush*) inedit;
				return br->cnt * sizeof(fm_face);
			}
		}

		else
		{
			return 0;
		}
	}


	fm_uint _editUsageAmount ( fm_usageType inusage, fm_refType intype, fm_editBase* inedit )
	{
		if ( intype >= FM_RT_SSTRING && intype <= FM_RT_LSTRING )
		{
			return 1;
		}

		else if ( intype == FM_RT_ARRAY )
		{
			// an array stores 1 (itself) + # elements
			fm_assert( inedit );
			fm_editArray* ar = (fm_editArray*) inedit;
			fm_assert( ar->cnt <= ar->max );
			
			if ( inusage == FM_UT_PAYLOAD )
			{
				return 1 + ar->cnt;
			}
			else if ( inusage == FM_UT_DATA )
			{
				// todo
				return 1 + ar->cnt;
			}
			else
			{
				return 1;
			}
		}

		else if ( intype == FM_RT_NODE )
		{
			// a node stores 1 (itself) + #kvp elements
			fm_assert( inedit );
			fm_editNode* nd = (fm_editNode*) inedit;
			fm_assert( nd->cnt <= FM_NODE_LOAD );
			
			if ( inusage == FM_UT_PAYLOAD )
			{
				return 1 + nd->cnt;
			}
			else if ( inusage == FM_UT_DATA )
			{
				fm_keyValueMod kvps[FM_NODE_LOAD];

				return 1 + _nodeDeflate( nd, active_zone->headVersion, kvps, FM_NODE_LOAD );
			}
			else
			{
				return 1;
			}
		}

		else if ( intype == FM_RT_BRUSH )
		{
			// a brush stores 1 (itself) + # faces
			fm_assert( inedit );
			fm_editBrush* br = (fm_editBrush*) inedit;
			fm_assert( br->cnt <= FM_BRUSH_LOAD );
			
			if ( inusage == FM_UT_PAYLOAD )
			{
				return 1 + br->cnt;
			}
			else if ( inusage == FM_UT_DATA )
			{
				// todo
				return 1 + br->cnt;
			}
			else
			{
				return 1;
			}
		}

		else
		{
			return 1;
		}
	}


}

