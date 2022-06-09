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



#define FM_MAGIC					'GGFM'
#define FM_REVISION					1

#define FM_VERSION_NONE				((fm_version)0)
#define FM_VERSION_FIRST			((fm_version)1)
#define FM_VERSION_HEAD				((fm_version)~0U)

#define FM_BAD_REF					((fm_ref)~0U)

#define FM_DEFAULT_ZONE_LOAD		((fm_uint)2*1024*1024*1024)
#define FM_DEFAULT_EXTRA			(1024)

#define FM_PAGE_BITS				(12)
#define FM_PAGE_LOAD				(1<<FM_PAGE_BITS)

#define FM_NODE_LOAD				(32)
#define FM_BRUSH_LOAD				(32)
#define FM_PATCH_LOAD				(32)
#define FM_MODEL_LOAD				(32)

#define FM_DIRECTORY_LOAD			(4096)



typedef const char*					fm_string;
typedef signed						fm_int;
typedef unsigned					fm_uint;
typedef unsigned					fm_version;
typedef float						fm_fp;
typedef double						fm_dfp;
typedef unsigned					fm_ref;
typedef uintptr_t					fm_uintptr;
typedef unsigned __int32			fm_ui32;
typedef unsigned __int64			fm_ui64;



enum fm_usageType
{
	FM_UT_PHYSICAL = 0,		// the physical amount (like reserved memory footprint)
	FM_UT_LOGICAL,			// the logically initialized amount (like before to apply any costly reduction algorithm on this amount - defragmentation or recycling)
	FM_UT_PAYLOAD,			// the optimal real payload of data WITH containers
	FM_UT_DATA,				// the optimal real payload of data ONLY
};


enum fm_refType
{
	FM_RT_NONE = 0,
	//
	FM_RT_SSTRING,
	FM_RT_MSTRING,
	FM_RT_LSTRING,
	FM_RT_ARRAY,
	FM_RT_NODE,
	FM_RT_BRUSH,
	//
	FM_RT_LAST,
};


enum fm_arrayType
{
	FM_ARRAY_U8 = 0,
	FM_ARRAY_U32,
	FM_ARRAY_FLOAT,
	FM_ARRAY_LOCATION,
	FM_ARRAY_TRI,
	FM_ARRAY_MAPPING,
	FM_ARRAY_EDGE,
	FM_ARRAY_NORMAL,
	FM_ARRAY_REF,
	FM_ARRAY_CUSTOM	= 100,
};


enum fm_dataType
{
	FM_DT_NONE = 0,
	FM_DT_INT,
	FM_DT_UINT,
	FM_DT_FLOAT,
	FM_DT_REF,
};


union fm_dataContent
{
	fm_int					si;
	fm_uint					ui;
	fm_fp					fp;
	fm_ref					ref;
	fm_uint					all;
};


struct fm_data
{
	static fm_data			as_si		( fm_int  insi		)		{ fm_data d; d.set_si(insi);   return d;					}
	static fm_data			as_ui		( fm_uint inui		)		{ fm_data d; d.set_ui(inui);   return d;					}
	static fm_data			as_fp		( fm_fp	  infp		)		{ fm_data d; d.set_fp(infp);   return d;					}
	static fm_data			as_ref		( fm_ref  inref		)		{ fm_data d; d.set_ref(inref); return d;					}

	void					set_si		( fm_int  insi		)		{ type=FM_DT_INT;   content.si  = insi;						}
	void					set_ui		( fm_uint inui		)		{ type=FM_DT_UINT;  content.ui  = inui;						}
	void					set_fp		( fm_fp	  infp		)		{ type=FM_DT_FLOAT; content.fp  = infp;						}
	void					set_ref		( fm_ref  inref		)		{ type=FM_DT_REF;   content.ref = inref;					}

	fm_int					get_si		(					)		{ assert(type==FM_DT_INT);   return content.si;				}
	fm_uint					get_ui		(					)		{ assert(type==FM_DT_UINT);  return content.ui;				}
	fm_fp					get_fp		(					)		{ assert(type==FM_DT_FLOAT); return content.fp;				}
	fm_ref					get_ref		(					)		{ assert(type==FM_DT_REF);   return content.ref;			}

	bool					isnone		(					) const	{ return type == FM_DT_NONE;								}
	bool					equals		( const fm_data& d	) const	{ return (type==d.type) && (content.all==d.content.all);	}
	bool					nequals		( const fm_data& d	) const	{ return !equals(d);										}
	int						compare		( const fm_data& d	) const;

	bool					operator ==	( const fm_data& d	) const	{ return equals(d);											}
	bool					operator !=	( const fm_data& d	) const	{ return !equals(d);										}
	bool					operator <	( const fm_data& d	) const	{ return compare(d) <  0;									}
	bool					operator <=	( const fm_data& d	) const	{ return compare(d) <= 0;									}
	bool					operator >	( const fm_data& d	) const	{ return compare(d) >  0;									}
	bool					operator >= ( const fm_data& d	) const	{ return compare(d) >= 0;									}

	fm_dataType				type;
	fm_dataContent			content;
};




struct fm_bounds
{
	float					midPoint[3];
	float					halfSize[3];
};


struct fm_keyValue
{
	fm_data					key;
	fm_data					value;
};


struct fm_mod
{
	fm_version				version;
};


struct fm_plane
{
	float					dir[3];
	float					dist;
};


struct fm_planarMapping
{
	float					scale[2];
	float					shift[2];
	float					rotate;
	float					skew;
	int						materialId;
};


struct fm_face
{
	fm_plane				plane;
	fm_planarMapping		mapping;
};


struct fm_keyValueMod : public fm_mod
{
	fm_data					key;
	fm_data					value;
};


struct fm_faceMod : public fm_mod
{
	fm_face					face;
	fm_uint					id;
	fm_ui64					effective;

	static_assert( sizeof(fm_faceMod::effective)*8 >= FM_BRUSH_LOAD, "effective is too short" );
};


struct fm_arrayMod : public fm_mod
{
	fm_int					index;		// >=0: set   <0:unset
	fm_uint					last;		// last index
};




struct fm_editBase
{
	//
};


struct fm_editSString : public fm_editBase
{
	static const unsigned	MaxLength = 15;
	char					text		[ MaxLength+1 ];
};


struct fm_editMString : public fm_editBase
{
	static const unsigned	MaxLength = 63;
	char					text		[ MaxLength+1 ];
};


struct fm_editLString : public fm_editBase
{
	static const unsigned	MaxLength = 255;
	char					text		[ MaxLength+1 ];
};


struct fm_editArray : public fm_editBase
{
	fm_uint					type;		// fm_arrayType
	fm_uint					stride;
	fm_uint					max;
	fm_uint					cnt;
	fm_arrayMod*			mods;		// [max]
	fm_uintptr				data;		// [max]
};


struct fm_editNode : public fm_editBase
{
	fm_uint					cnt;
	fm_keyValueMod			mods		[ FM_NODE_LOAD ];
};


struct fm_editBrush : public fm_editBase
{
	fm_uint					cnt;
	fm_faceMod				mods		[ FM_BRUSH_LOAD ];
};






struct fm_knot
{
	fm_version				version;
	fm_ref					prev;
	fm_ref					next;
	fm_uintptr				addr;
};


struct fm_page
{
	fm_refType				editType;
	fm_uint					editBSize;
	fm_uintptr				loAddr;
	fm_uintptr				hiAddr;
	fm_knot					knots		[ FM_PAGE_LOAD ];
};


struct fm_directory
{
	fm_uint					pagei;
	fm_page*				pages		[ FM_DIRECTORY_LOAD ];
	fm_ref					lastRefs	[ FM_RT_LAST ];
};




struct fm_buffer
{
	// to define ...
};


struct fm_log
{
	// to define ...
};


struct fm_signature
{
	// to define ...
};



// Zone

struct fm_zone
{
	fm_uint					magic;
	fm_uint					revision;
	char					identifier	[ 256 ];
	char					name		[ 256 ];
	fm_version				headVersion;
	fm_version				currVersion;
	fm_uintptr				loAddr;
	fm_uintptr				hiAddr;
	fm_uintptr				bsAddr;
	fm_directory			directory;
};



