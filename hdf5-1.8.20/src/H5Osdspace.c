/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define H5O_PACKAGE	/*suppress error about including H5Opkg	  */
#define H5S_PACKAGE		/*prevent warning from including H5Spkg.h */

#include "H5private.h"		/* Generic Functions			*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free lists                           */
#include "H5Gprivate.h"		/* Groups			  	*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Opkg.h"		/* Object headers		  	*/
#include "H5Spkg.h"		/* Dataspaces 				*/


/* PRIVATE PROTOTYPES */
static void *H5O_sdspace_decode(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh,
    unsigned mesg_flags, unsigned *ioflags, const uint8_t *p);
static herr_t H5O_sdspace_encode(H5F_t *f, uint8_t *p, const void *_mesg);
static void *H5O_sdspace_copy(const void *_mesg, void *_dest);
static size_t H5O_sdspace_size(const H5F_t *f, const void *_mesg);
static herr_t H5O_sdspace_reset(void *_mesg);
static herr_t H5O_sdspace_free(void *_mesg);
static herr_t H5O_sdspace_pre_copy_file(H5F_t *file_src, const void *mesg_src,
    hbool_t *deleted, const H5O_copy_t *cpy_info, void *_udata);
static herr_t H5O_sdspace_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg,
				FILE * stream, int indent, int fwidth);

/* Set up & include shared message "interface" info */
#define H5O_SHARED_TYPE			H5O_MSG_SDSPACE
#define H5O_SHARED_DECODE		H5O_sdspace_shared_decode
#define H5O_SHARED_DECODE_REAL		H5O_sdspace_decode
#define H5O_SHARED_ENCODE		H5O_sdspace_shared_encode
#define H5O_SHARED_ENCODE_REAL		H5O_sdspace_encode
#define H5O_SHARED_SIZE			H5O_sdspace_shared_size
#define H5O_SHARED_SIZE_REAL		H5O_sdspace_size
#define H5O_SHARED_DELETE		H5O_sdspace_shared_delete
#undef H5O_SHARED_DELETE_REAL
#define H5O_SHARED_LINK			H5O_sdspace_shared_link
#undef H5O_SHARED_LINK_REAL
#define H5O_SHARED_COPY_FILE		H5O_sdspace_shared_copy_file
#undef H5O_SHARED_COPY_FILE_REAL
#define H5O_SHARED_POST_COPY_FILE	H5O_sdspace_shared_post_copy_file
#undef H5O_SHARED_POST_COPY_FILE_REAL
#undef  H5O_SHARED_POST_COPY_FILE_UPD
#define H5O_SHARED_DEBUG		H5O_sdspace_shared_debug
#define H5O_SHARED_DEBUG_REAL		H5O_sdspace_debug
#include "H5Oshared.h"			/* Shared Object Header Message Callbacks */

/* This message derives from H5O message class */
const H5O_msg_class_t H5O_MSG_SDSPACE[1] = {{
    H5O_SDSPACE_ID,	    	/* message id number		    	*/
    "dataspace",	    	/* message name for debugging	   	*/
    sizeof(H5S_extent_t),   	/* native message size		    	*/
    H5O_SHARE_IS_SHARABLE|H5O_SHARE_IN_OHDR,	/* messages are sharable?       */
    H5O_sdspace_shared_decode,	/* decode message			*/
    H5O_sdspace_shared_encode,	/* encode message			*/
    H5O_sdspace_copy,	    	/* copy the native value		*/
    H5O_sdspace_shared_size,	/* size of symbol table entry	    	*/
    H5O_sdspace_reset,	    	/* default reset method		    	*/
    H5O_sdspace_free,		/* free method				*/
    H5O_sdspace_shared_delete,	/* file delete method		*/
    H5O_sdspace_shared_link,	/* link method			*/
    NULL,			/* set share method		*/
    NULL,		    	/*can share method		*/
    H5O_sdspace_pre_copy_file,	/* pre copy native value to file */
    H5O_sdspace_shared_copy_file,/* copy native value to file    */
    H5O_sdspace_shared_post_copy_file,/* post copy native value to file    */
    NULL,			/* get creation index		*/
    NULL,			/* set creation index		*/
    H5O_sdspace_shared_debug	/* debug the message		    	*/
}};

/* Declare external the free list for H5S_extent_t's */
H5FL_EXTERN(H5S_extent_t);

/* Declare external the free list for hsize_t arrays */
H5FL_ARR_EXTERN(hsize_t);


/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_decode
 PURPOSE
    Decode a simple dimensionality message and return a pointer to a memory
	struct with the decoded information
 USAGE
    void *H5O_sdspace_decode(f, dxpl_id, mesg_flags, p)
	H5F_t *f;	        IN: pointer to the HDF5 file struct
        hid_t dxpl_id;          IN: DXPL for any I/O
        unsigned mesg_flags;    IN: Message flags to influence decoding
	const uint8 *p;		IN: the raw information buffer
 RETURNS
    Pointer to the new message in native order on success, NULL on failure
 DESCRIPTION
	This function decodes the "raw" disk form of a simple dimensionality
    message into a struct in memory native format.  The struct is allocated
    within this function using malloc() and is returned to the caller.
--------------------------------------------------------------------------*/
static void *
H5O_sdspace_decode(H5F_t *f, hid_t H5_ATTR_UNUSED dxpl_id, H5O_t H5_ATTR_UNUSED *open_oh,
    unsigned H5_ATTR_UNUSED mesg_flags, unsigned H5_ATTR_UNUSED *ioflags, const uint8_t *p)
{
    H5S_extent_t	*sdim = NULL;/* New extent dimensionality structure */
    void		*ret_value;
    unsigned		i;		/* local counting variable */
    unsigned		flags, version;

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(p);

    /* decode */
    if(NULL == (sdim = H5FL_CALLOC(H5S_extent_t)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_NOSPACE, NULL, "dataspace structure allocation failed")

    /* Check version */
    version = *p++;
    if(version < H5O_SDSPACE_VERSION_1 || version > H5O_SDSPACE_VERSION_2)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "wrong version number in dataspace message")
    sdim->version = version;

    /* Get rank */
    sdim->rank = *p++;
    if(sdim->rank > H5S_MAX_RANK)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "simple dataspace dimensionality is too large")

    /* Get dataspace flags for later */
    flags = *p++;

    /* Get or determine the type of the extent */
    if(version >= H5O_SDSPACE_VERSION_2)
        sdim->type = (H5S_class_t)*p++;
    else {
        /* Set the dataspace type to be simple or scalar as appropriate */
        if(sdim->rank > 0)
            sdim->type = H5S_SIMPLE;
        else
            sdim->type = H5S_SCALAR;

        /* Increment past reserved byte */
        p++;
    } /* end else */
    HDassert(sdim->type != H5S_NULL || sdim->version >= H5O_SDSPACE_VERSION_2);

    /* Only Version 1 has these reserved bytes */
    if(version == H5O_SDSPACE_VERSION_1)
        p += 4; /*reserved*/

    /* Decode dimension sizes */
    if(sdim->rank > 0) {
        if(NULL == (sdim->size = (hsize_t *)H5FL_ARR_MALLOC(hsize_t, (size_t)sdim->rank)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

        for(i = 0; i < sdim->rank; i++)
            H5F_DECODE_LENGTH(f, p, sdim->size[i]);

        if(flags & H5S_VALID_MAX) {
            if(NULL == (sdim->max = (hsize_t *)H5FL_ARR_MALLOC(hsize_t, (size_t)sdim->rank)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
            for(i = 0; i < sdim->rank; i++)
                H5F_DECODE_LENGTH (f, p, sdim->max[i]);
        } /* end if */
    } /* end if */

    /* Compute the number of elements in the extent */
    if(sdim->type == H5S_NULL)
        sdim->nelem = 0;
    else {
        for(i = 0, sdim->nelem = 1; i < sdim->rank; i++)
            sdim->nelem *= sdim->size[i];
    } /* end else */

    /* Set return value */
    ret_value = (void*)sdim;	/*success*/

done:
    if(!ret_value && sdim) {
        H5S_extent_release(sdim);
        sdim = H5FL_FREE(H5S_extent_t, sdim);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_sdspace_decode() */


/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_encode
 PURPOSE
    Encode a simple dimensionality message
 USAGE
    herr_t H5O_sdspace_encode(f, raw_size, p, mesg)
	H5F_t *f;	        IN: pointer to the HDF5 file struct
	size_t raw_size;	IN: size of the raw information buffer
	const uint8 *p;		IN: the raw information buffer
	const void *mesg;	IN: Pointer to the extent dimensionality struct
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
	This function encodes the native memory form of the simple
    dimensionality message in the "raw" disk form.

 MODIFICATIONS
	Robb Matzke, 1998-04-09
	The current and maximum dimensions are now H5F_SIZEOF_SIZET bytes
	instead of just four bytes.

  	Robb Matzke, 1998-07-20
        Added a version number and reformatted the message for aligment.

        Raymond Lu
        April 8, 2004
        Added the type of dataspace into this header message using a reserved
        byte.

--------------------------------------------------------------------------*/
static herr_t
H5O_sdspace_encode(H5F_t *f, uint8_t *p, const void *_mesg)
{
    const H5S_extent_t	*sdim = (const H5S_extent_t *)_mesg;
    unsigned		flags = 0;
    unsigned		u;  /* Local counting variable */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(f);
    HDassert(p);
    HDassert(sdim);

    /* Version */
    HDassert(sdim->version > 0);
    HDassert(sdim->type != H5S_NULL || sdim->version >= H5O_SDSPACE_VERSION_2);
    *p++ = (uint8_t)sdim->version;

    /* Rank */
    *p++ = (uint8_t)sdim->rank;

    /* Flags */
    if(sdim->max)
        flags |= H5S_VALID_MAX;
    *p++ = (uint8_t)flags;

    /* Dataspace type */
    if(sdim->version > H5O_SDSPACE_VERSION_1)
        *p++ = sdim->type;
    else {
        *p++ = 0; /*reserved*/
        *p++ = 0; /*reserved*/
        *p++ = 0; /*reserved*/
        *p++ = 0; /*reserved*/
        *p++ = 0; /*reserved*/
    } /* end else */

    /* Current & maximum dimensions */
    if(sdim->rank > 0) {
        for(u = 0; u < sdim->rank; u++)
            H5F_ENCODE_LENGTH(f, p, sdim->size[u]);
        if(flags & H5S_VALID_MAX) {
            for(u = 0; u < sdim->rank; u++)
                H5F_ENCODE_LENGTH(f, p, sdim->max[u]);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_sdspace_encode() */


/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_copy
 PURPOSE
    Copies a message from MESG to DEST, allocating DEST if necessary.
 USAGE
    void *H5O_sdspace_copy(_mesg, _dest)
	const void *_mesg;	IN: Pointer to the source extent dimensionality struct
	const void *_dest;	IN: Pointer to the destination extent dimensionality struct
 RETURNS
    Pointer to DEST on success, NULL on failure
 DESCRIPTION
	This function copies a native (memory) simple dimensionality message,
    allocating the destination structure if necessary.
--------------------------------------------------------------------------*/
static void *
H5O_sdspace_copy(const void *_mesg, void *_dest)
{
    const H5S_extent_t	   *mesg = (const H5S_extent_t *)_mesg;
    H5S_extent_t	   *dest = (H5S_extent_t *)_dest;
    void                   *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(mesg);
    if(!dest && NULL == (dest = H5FL_CALLOC(H5S_extent_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Copy extent information */
    if(H5S_extent_copy(dest, mesg, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, NULL, "can't copy extent")

    /* Set return value */
    ret_value = dest;

done:
    if(NULL == ret_value)
        if(dest && NULL == _dest)
            dest = H5FL_FREE(H5S_extent_t, dest);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_sdspace_copy() */


/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_size
 PURPOSE
    Return the raw message size in bytes
 USAGE
    void *H5O_sdspace_size(f, mesg)
	H5F_t *f;	  IN: pointer to the HDF5 file struct
	const void *mesg;	IN: Pointer to the source extent dimensionality struct
 RETURNS
    Size of message on success, zero on failure
 DESCRIPTION
	This function returns the size of the raw simple dimensionality message on
    success.  (Not counting the message type or size fields, only the data
    portion of the message).  It doesn't take into account alignment.

 MODIFICATIONS
	Robb Matzke, 1998-04-09
	The current and maximum dimensions are now H5F_SIZEOF_SIZET bytes
	instead of just four bytes.
--------------------------------------------------------------------------*/
static size_t
H5O_sdspace_size(const H5F_t *f, const void *_mesg)
{
    const H5S_extent_t	*space = (const H5S_extent_t *)_mesg;
    size_t		ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Basic information for all dataspace messages */
    ret_value = 1 +             /* Version */
            1 +                 /* Rank */
            1 +                 /* Flags */
            1 +                 /* Dataspace type/reserved */
            ((space->version > H5O_SDSPACE_VERSION_1) ? 0 : 4); /* Eliminated/reserved */

    /* Add in the dimension sizes */
    ret_value += space->rank * H5F_SIZEOF_SIZE(f);

    /* Add in the space for the maximum dimensions, if they are present */
    ret_value += space->max ? (space->rank * H5F_SIZEOF_SIZE(f)) : 0;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_sdspace_size() */


/*-------------------------------------------------------------------------
 * Function:	H5O_sdspace_reset
 *
 * Purpose:	Frees the inside of a dataspace message and resets it to some
 *		initial value.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, April 30, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_sdspace_reset(void *_mesg)
{
    H5S_extent_t	*mesg = (H5S_extent_t*)_mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    H5S_extent_release(mesg);

    FUNC_LEAVE_NOAPI(SUCCEED)
}


/*-------------------------------------------------------------------------
 * Function:	H5O_sdsdpace_free
 *
 * Purpose:	Free's the message
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, March 30, 2000
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_sdspace_free(void *mesg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(mesg);

    mesg = H5FL_FREE(H5S_extent_t, mesg);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_sdspace_free() */


/*-------------------------------------------------------------------------
 * Function:    H5O_sdspace_pre_copy_file
 *
 * Purpose:     Perform any necessary actions before copying message between
 *              files
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              November 30, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_sdspace_pre_copy_file(H5F_t H5_ATTR_UNUSED *file_src, const void *mesg_src,
    hbool_t H5_ATTR_UNUSED *deleted, const H5O_copy_t H5_ATTR_UNUSED *cpy_info, void *_udata)
{
    const H5S_extent_t *src_space_extent = (const H5S_extent_t *)mesg_src;  /* Source dataspace extent */
    H5D_copy_file_ud_t *udata = (H5D_copy_file_ud_t *)_udata;   /* Dataset copying user data */
    herr_t         ret_value = SUCCEED;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(file_src);
    HDassert(src_space_extent);

    /* If the user data is non-NULL, assume we are copying a dataset
     * and make a copy of the dataspace extent for later in the object copying
     * process.  (We currently only need to make a copy of the dataspace extent
     * if the layout is an early version, but that information isn't
     * available here, so we just make a copy of it in all cases)
     */
    if(udata) {
        /* Allocate copy of dataspace extent */
        if(NULL == (udata->src_space_extent = H5FL_CALLOC(H5S_extent_t)))
            HGOTO_ERROR(H5E_DATASPACE, H5E_NOSPACE, FAIL, "dataspace extent allocation failed")

        /* Create a copy of the dataspace extent */
        if(H5S_extent_copy(udata->src_space_extent, src_space_extent, TRUE) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOPY, FAIL, "can't copy extent")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dspace_pre_copy_file() */


/*--------------------------------------------------------------------------
 NAME
    H5O_sdspace_debug
 PURPOSE
    Prints debugging information for a simple dimensionality message
 USAGE
    void *H5O_sdspace_debug(f, mesg, stream, indent, fwidth)
	H5F_t *f;	        IN: pointer to the HDF5 file struct
	const void *mesg;	IN: Pointer to the source extent dimensionality struct
	FILE *stream;		IN: Pointer to the stream for output data
	int indent;		IN: Amount to indent information by
	int fwidth;		IN: Field width (?)
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
	This function prints debugging output to the stream passed as a
    parameter.
--------------------------------------------------------------------------*/
static herr_t
H5O_sdspace_debug(H5F_t H5_ATTR_UNUSED *f, hid_t H5_ATTR_UNUSED dxpl_id, const void *mesg,
		  FILE * stream, int indent, int fwidth)
{
    const H5S_extent_t	   *sdim = (const H5S_extent_t *)mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(f);
    HDassert(sdim);
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
	    "Rank:",
	    (unsigned long) (sdim->rank));

    if(sdim->rank > 0) {
        unsigned		    u;	/* local counting variable */

        HDfprintf(stream, "%*s%-*s {", indent, "", fwidth, "Dim Size:");
        for(u = 0; u < sdim->rank; u++)
            HDfprintf (stream, "%s%Hu", u?", ":"", sdim->size[u]);
        HDfprintf (stream, "}\n");

        HDfprintf(stream, "%*s%-*s ", indent, "", fwidth, "Dim Max:");
        if(sdim->max) {
            HDfprintf (stream, "{");
            for(u = 0; u < sdim->rank; u++) {
                if(H5S_UNLIMITED==sdim->max[u])
                    HDfprintf (stream, "%sUNLIM", u?", ":"");
                else
                    HDfprintf (stream, "%s%Hu", u?", ":"", sdim->max[u]);
            } /* end for */
            HDfprintf (stream, "}\n");
        } /* end if */
        else
            HDfprintf (stream, "CONSTANT\n");
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_sdspace_debug() */

