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

#include "ppw_fat_types.h"



// setup functions

fm_zone*			fm_getActive			(																					);
bool				fm_setActive			( fm_zone*																			);

bool				fm_initAndActive		(																					);
bool				fm_initAndActive		( fm_uintptr loAddr, fm_uintptr hiAddr												);
void				fm_shutAndForget		(																					);

bool				fm_openAndActive		( fm_string identifier																);
void				fm_closeAndForget		(																					);

bool				fm_extend				( fm_refType, fm_uint = FM_DEFAULT_EXTRA											);

fm_version			fm_headVersion			(																					);
bool				fm_begin				(																					);
bool				fm_end					(																					);

// refs

fm_ref				fm_refAlloc				( fm_refType, fm_uint = FM_DEFAULT_EXTRA											);
fm_ref				fm_refGetLastAlloc		( fm_refType																		);
fm_ref				fm_refSetLastAlloc		( fm_ref																			);
bool				fm_refDealloc			( fm_ref																			);
bool				fm_refLink				( fm_ref, fm_ref																	);
bool				fm_refUnlink			( fm_ref																			);
fm_refType			fm_refGetType			( fm_ref																			);
fm_version			fm_refGetVersion		( fm_ref																			);
bool				fm_refIsValid			( fm_ref																			);
bool				fm_refIsHead			( fm_ref																			);
bool				fm_refIsAncestorOf		( fm_ref, fm_ref																	);
bool				fm_refIsDescendantOf	( fm_ref, fm_ref																	);
bool				fm_refIsSameLineage		( fm_ref, fm_ref																	);
fm_ref				fm_refRevise			( fm_ref, fm_version																);
fm_editBase*		fm_refDeref				( fm_ref, fm_refType* = NULL														);
bool				fm_refIsType			( fm_ref, fm_refType																);
bool				fm_refIsString			( fm_ref																			);
bool				fm_refIsNode			( fm_ref																			);
bool				fm_refIsArray			( fm_ref																			);
bool				fm_refIsBrush			( fm_ref																			);

// strings

fm_uint				fm_stringHash			( fm_string																			);
fm_string			fm_stringDeref			( fm_ref																			);
fm_ref				fm_stringFind			( fm_string																			);
fm_ref				fm_stringAlloc			( fm_string																			);

// arrays

fm_editArray*		fm_arrayDeref			( fm_ref																			);
fm_ref				fm_arrayAlloc			( fm_arrayType intype, fm_uint inmax, fm_uint instride=0							);
fm_ref				fm_arrayDuplicate		( fm_ref, fm_version																);
fm_uint				fm_arrayDeflate			( fm_ref, fm_version, fm_arrayMod* outmods, void* outdata, fm_uint incapacity		);
fm_uint				fm_arrayMax				( fm_ref																			);
fm_uint				fm_arrayStride			( fm_ref																			);
fm_uint				fm_arrayRange			( fm_ref, fm_version																);
bool				fm_arrayGet				( fm_ref, fm_version, fm_uint inindex, void* outdata, fm_uint inbsize				);
bool				fm_arraySet				( fm_ref, fm_uint inindex, void* indata												);
bool				fm_arrayUnset			( fm_ref, fm_uint inindex															);

// nodes

fm_editNode*		fm_nodeDeref			( fm_ref																			);
fm_ref				fm_nodeAlloc			(																					);
fm_ref				fm_nodeDuplicate		( fm_ref, fm_version																);
fm_uint				fm_nodeDeflate			( fm_ref, fm_version, fm_keyValueMod* out, fm_uint incapacity						);
bool				fm_nodeGet				( fm_ref, fm_version, const fm_data&, fm_data&										);
bool				fm_nodeSet				( fm_ref, const fm_data&, const fm_data&											);
bool				fm_nodeUnset			( fm_ref, const fm_data&															);

// brushes

fm_editBrush*		fm_brushDeref			( fm_ref																			);
fm_ref				fm_brushAlloc			(																					);
fm_ref				fm_brushDuplicate		( fm_ref, fm_version																);
fm_uint				fm_brushDeflate			( fm_ref, fm_version, fm_faceMod* out, fm_uint incapacity							);
bool				fm_brushGet				( fm_ref, fm_version, fm_uint inid, fm_face&										);
bool				fm_brushSet				( fm_ref, fm_uint inid, const fm_face&												);
bool				fm_brushUnset			( fm_ref, fm_uint inid																);

// utilities

size_t				fm_countPage			(																					);
size_t				fm_countElementOfType	( fm_refType																		);
bool				fm_isFull				(																					);
float				fm_getFillingRate		(																					);
bool				fm_checkIntegrity		( 																					);
size_t				fm_probeMemoryUsage		( fm_usageType																		);
size_t				fm_probeEditableUsage	( fm_usageType																		);
void				fm_trimTail				( fm_version trimToVersion															);		// trim versions [ FM_VERSION_FIRST, trimToVersion ]
void				fm_trimHead				( fm_version trimFromVersion														);		// trim versions [ trimFromVersion, FM_VERSION_HEAD ]
void				fm_optimize				( /* what ... */																	);
bool				fm_dumpToMemory			( fm_buffer*																		);
bool				fm_loadFromMemory		( fm_buffer*																		);
bool				fm_dumpLog				( fm_log&, fm_version, fm_version													);
bool				fm_applyLog				( fm_log&																			);
bool				fm_dumpReplication		( fm_buffer&																		);		// store a list of in-memory write operations
bool				fm_applyReplication		( fm_buffer&																		);		// apply a replication
bool				fm_buildSignature		( fm_signature&, fm_version															);


