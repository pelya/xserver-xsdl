/*
 * Copyright © 2005 Novell, Inc.
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include "xgl.h"

#ifdef GLXEXT

/* This is just a wrapper around Mesa's hash functions. */

extern struct _mesa_HashTable *
_mesa_NewHashTable (void);

extern void
_mesa_DeleteHashTable (struct _mesa_HashTable *table);

extern void *
_mesa_HashLookup (const struct _mesa_HashTable *table,
		  unsigned int		       key);

extern void
_mesa_HashInsert (struct _mesa_HashTable *table,
		  unsigned int		 key,
		  void			 *data);

extern void
_mesa_HashRemove (struct _mesa_HashTable *table,
		  unsigned int		 key);

extern unsigned int
_mesa_HashFirstEntry (struct _mesa_HashTable *table);

extern unsigned int
_mesa_HashNextEntry (const struct _mesa_HashTable *table,
		     unsigned int		  key);

extern unsigned int
_mesa_HashFindFreeKeyBlock (struct _mesa_HashTable *table,
			    unsigned int	   numKeys);


xglHashTablePtr
xglNewHashTable (void)
{
    return (xglHashTablePtr) _mesa_NewHashTable ();
}

void
xglDeleteHashTable (xglHashTablePtr pTable)
{
    _mesa_DeleteHashTable ((struct _mesa_HashTable *) pTable);
}

void *
xglHashLookup (const xglHashTablePtr pTable,
	       unsigned int	     key)
{
    return _mesa_HashLookup ((struct _mesa_HashTable *) pTable, key);  
}

void
xglHashInsert (xglHashTablePtr pTable,
	       unsigned int    key,
	       void	       *data)
{
    _mesa_HashInsert ((struct _mesa_HashTable *) pTable, key, data);
}

void
xglHashRemove (xglHashTablePtr pTable,
	       unsigned int    key)
{
    _mesa_HashRemove ((struct _mesa_HashTable *) pTable, key);
}

unsigned int
xglHashFirstEntry (xglHashTablePtr pTable)
{
    return _mesa_HashFirstEntry ((struct _mesa_HashTable *) pTable);
}

unsigned int
xglHashNextEntry (const xglHashTablePtr pTable,
		  unsigned int		key)
{
    return _mesa_HashNextEntry ((struct _mesa_HashTable *) pTable, key);
}

unsigned int
xglHashFindFreeKeyBlock (xglHashTablePtr pTable,
			 unsigned int	 numKeys)
{
    return _mesa_HashFindFreeKeyBlock ((struct _mesa_HashTable *) pTable,
				       numKeys);
}

#endif
