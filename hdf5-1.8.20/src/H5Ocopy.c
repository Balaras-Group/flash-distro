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

/*-------------------------------------------------------------------------
 *
 * Created:		H5Ocopy.c
 *			Nov  6 2006
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		Object copying routines.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5O_PACKAGE		/*suppress error about including H5Opkg	  */

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Aprivate.h"         /* Attributes                           */
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5HGprivate.h"        /* Global Heaps                         */
#include "H5FOprivate.h"        /* File objects                         */
#include "H5Lprivate.h"         /* Links			  	*/
#include "H5MFprivate.h"	/* File memory management		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Opkg.h"             /* Object headers			*/
#include "H5Pprivate.h"         /* Property lists                       */


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/

/* Key object for skiplist of committed datatypes */
typedef struct H5O_copy_search_comm_dt_key_t {
    H5T_t               *dt;    /* Datatype */
    unsigned long       fileno; /* File number */
} H5O_copy_search_comm_dt_key_t;

/* Callback struct for building a list of committed datatypes */
typedef struct H5O_copy_search_comm_dt_ud_t {
    H5SL_t      *dst_dt_list;   /* Skip list of committed datatypes */
    H5G_loc_t   *dst_root_loc;  /* Starting location for iteration */
    H5O_loc_t   obj_oloc;       /* Object location (for attribute iteration callback) */
    hid_t       dxpl_id;        /* Dataset transfer property list id */
} H5O_copy_search_comm_dt_ud_t;


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

static herr_t H5O_copy_free_addrmap_cb(void *item, void *key, void *op_data);
static herr_t H5O_copy_header_real(const H5O_loc_t *oloc_src, H5O_loc_t *oloc_dst /*out*/,
    hid_t dxpl_id, H5O_copy_t *cpy_info, H5O_type_t *obj_type, void **udata);
static herr_t H5O_copy_header(const H5O_loc_t *oloc_src, H5O_loc_t *oloc_dst /*out*/,
    hid_t dxpl_id, hid_t ocpypl_id);
static herr_t H5O_copy_obj(H5G_loc_t *src_loc, H5G_loc_t *dst_loc,
    const char *dst_name, hid_t ocpypl_id, hid_t lcpl_id, hid_t dxpl_id);
static herr_t H5O_copy_obj_by_ref(H5O_loc_t *src_oloc, hid_t dxpl_id,
    H5O_loc_t *dst_oloc, H5G_loc_t *dst_root_loc, H5O_copy_t *cpy_info);
static herr_t H5O_copy_free_comm_dt_cb(void *item, void *key, void *op_data);
static int H5O_copy_comm_dt_cmp(const void *dt1, const void *dt2);
static herr_t H5O_copy_search_comm_dt_cb(hid_t group, const char *name,
    const H5L_info_t *linfo, void *udata);
static htri_t H5O_copy_search_comm_dt(H5F_t *file_src, H5O_t *oh_src,
    H5O_loc_t *oloc_dst/*in, out*/, hid_t dxpl_id, H5O_copy_t *cpy_info);
static herr_t H5O_copy_insert_comm_dt(H5F_t *file_src, H5O_t *oh_src,
    H5O_loc_t *oloc_dst, hid_t dxpl_id, H5O_copy_t *cpy_info);


/*********************/
/* Package Variables */
/*********************/

/* Declare a free list to manage the H5O_addr_map_t struct */
H5FL_DEFINE(H5O_addr_map_t);

/* Declare a free list to manage the H5O_copy_search_comm_dt_key_t struct */
H5FL_DEFINE(H5O_copy_search_comm_dt_key_t);

/* Declare a free list to manage haddr_t variables */
H5FL_DEFINE(haddr_t);


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:    H5Ocopy
 *
 * Purpose:     Copy an object (group or dataset) to destination location
 *              within a file or cross files. PLIST_ID is a property list
 *              which is used to pass user options and properties to the
 *              copy. The name, dst_name, must not already be taken by some
 *              other object in the destination group.
 *
 *              H5Ocopy() will fail if the name of the destination object
 *                  exists in the destination group.  For example,
 *                  H5Ocopy(fid_src, "/dset", fid_dst, "/dset", ...)
 *                  will fail if "/dset" exists in the destination file
 *
 *              OPTIONS THAT HAVE BEEN IMPLEMENTED.
 *                  H5O_COPY_SHALLOW_HIERARCHY_FLAG
 *                      If this flag is specified, only immediate members of
 *                      the group are copied. Otherwise (default), it will
 *                      recursively copy all objects below the group
 *                  H5O_COPY_EXPAND_SOFT_LINK_FLAG
 *                      If this flag is specified, it will copy the objects
 *                      pointed by the soft links. Otherwise (default), it
 *                      will copy the soft link as they are
 *                  H5O_COPY_WITHOUT_ATTR_FLAG
 *                      If this flag is specified, it will copy object without
 *                      copying attributes. Otherwise (default), it will
 *                      copy object along with all its attributes
 *                  H5O_COPY_EXPAND_REFERENCE_FLAG
 *                      1) Copy object between two different files:
 *                          When this flag is specified, it will copy objects that
 *                          are pointed by the references and update the values of
 *                          references in the destination file.  Otherwise (default)
 *                          the values of references in the destination will set to
 *                          zero
 *                          The current implementation does not handle references
 *                          inside of other datatype structure. For example, if
 *                          a member of compound datatype is reference, H5Ocopy()
 *                          will copy that field as it is. It will not set the
 *                          value to zero as default is used nor copy the object
 *                          pointed by that field the flag is set
 *                      2) Copy object within the same file:
 *                          This flag does not have any effect to the H5Ocopy().
 *                          Datasets or attributes of references are copied as they
 *                          are, i.e. values of references of the destination object
 *                          are the same as the values of the source object
 *
 *              OPTIONS THAT MAY APPLY TO COPY IN THE FUTURE.
 *                  H5O_COPY_EXPAND_EXT_LINK_FLAG
 *                      If this flag is specified, it will expand the external links
 *                      into new objects, Otherwise (default), it will keep external
 *                      links as they are (default)
 *
 *              PROPERTIES THAT MAY APPLY TO COPY IN FUTURE
 *                  Change data layout such as chunk size
 *                  Add filter such as data compression.
 *                  Add an attribute to the copied object(s) that say the  date/time
 *                      for the copy or other information about the source file.
 *
 *              The intermediate group creation property should be passed in
 *              using the lcpl instead of the ocpypl.
 *
 * Usage:      H5Ocopy(src_loc_id, src_name, dst_loc_id, dst_name, ocpypl_id, lcpl_id)
 *             hid_t src_loc_id         IN: Source file or group identifier.
 *             const char *src_name     IN: Name of the source object to be copied
 *             hid_t dst_loc_id         IN: Destination file or group identifier
 *             const char *dst_name     IN: Name of the destination object
 *             hid_t ocpypl_id          IN: Properties which apply to the copy
 *             hid_t lcpl_id            IN: Properties which apply to the new hard link
 *
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *              June 4, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ocopy(hid_t src_loc_id, const char *src_name, hid_t dst_loc_id,
        const char *dst_name, hid_t ocpypl_id, hid_t lcpl_id)
{
    H5G_loc_t	loc;                    /* Source group group location */
    H5G_loc_t	src_loc;                /* Source object group location */
    H5G_loc_t	dst_loc;                /* Destination group location */

    /* for opening the destination object */
    H5G_name_t  src_path;               /* Opened source object hier. path */
    H5O_loc_t   src_oloc;               /* Opened source object object location */
    hbool_t     loc_found = FALSE;      /* Location at 'name' found */
    hbool_t     obj_open = FALSE;       /* Entry at 'name' found */

    herr_t      ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "i*si*sii", src_loc_id, src_name, dst_loc_id, dst_name,
             ocpypl_id, lcpl_id);

    /* Check arguments */
    if(H5G_loc(src_loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(H5G_loc(dst_loc_id, &dst_loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!src_name || !*src_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no source name specified")
    if(!dst_name || !*dst_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no destination name specified")

    /* check if destination name already exists */
    {
        H5G_name_t  tmp_path;
        H5O_loc_t   tmp_oloc;
        H5G_loc_t   tmp_loc;

        /* Set up group location */
        tmp_loc.oloc = &tmp_oloc;
        tmp_loc.path = &tmp_path;
        H5G_loc_reset(&tmp_loc);

        /* Check if object already exists in destination */
        if(H5G_loc_find(&dst_loc, dst_name, &tmp_loc, H5P_DEFAULT, H5AC_ind_dxpl_id) >= 0) {
            H5G_name_free(&tmp_path);
            HGOTO_ERROR(H5E_SYM, H5E_EXISTS, FAIL, "destination object already exists")
        } /* end if */
    }

    /* Set up opened group location to fill in */
    src_loc.oloc = &src_oloc;
    src_loc.path = &src_path;
    H5G_loc_reset(&src_loc);

    /* Find the source object to copy */
    if(H5G_loc_find(&loc, src_name, &src_loc/*out*/, H5P_DEFAULT, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "source object not found")
    loc_found = TRUE;

    /* Open source object's object header */
    if(H5O_open(&src_oloc) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTOPENOBJ, FAIL, "unable to open object")
    obj_open = TRUE;

    /* Get correct property lists */
    if(H5P_DEFAULT == lcpl_id) {
        if((lcpl_id = H5L_get_default_lcpl()) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "unable to get default lcpl")
    } /* end if */
    else
        if(TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link creation property list")

    /* Get object copy property list */
    if(H5P_DEFAULT == ocpypl_id)
        ocpypl_id = H5P_OBJECT_COPY_DEFAULT;
    else
        if(TRUE != H5P_isa_class(ocpypl_id, H5P_OBJECT_COPY))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not object copy property list")

    /* Do the actual copying of the object */
    if(H5O_copy_obj(&src_loc, &dst_loc, dst_name, ocpypl_id, lcpl_id, H5AC_dxpl_id) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy object")

done:
    if(loc_found && H5G_loc_free(&src_loc) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "can't free location")
    if(obj_open && H5O_close(&src_oloc) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CLOSEERROR, FAIL, "unable to release object header")

    FUNC_LEAVE_API(ret_value)
} /* end H5Ocopy() */


/*-------------------------------------------------------------------------
 * Function:    H5O_copy_header_real
 *
 * Purpose:     Copy header object from one location to another using
 *              pre-copy, copy, and post-copy callbacks for each message
 *              type.
 *
 *              The source header object is compressed into a single chunk
 *              (since we know how big it is) and any continuation messages
 *              are converted into NULL messages.
 *
 *              By default, NULL messages are not copied.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *              May 30, 2005
 *
 * Modifications:
 *	Vailin Choi; Feb 2012
 *	Bug fix for HDFFV-7853
 *	When the object is opened, call the object's flush class action
 *	to ensure that cached data is flushed so that H5Ocopy will get
 *	the correct data.
 *	
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_copy_header_real(const H5O_loc_t *oloc_src, H5O_loc_t *oloc_dst /*out*/,
    hid_t dxpl_id, H5O_copy_t *cpy_info, H5O_type_t *obj_type,
    void **udata /*out*/)
{
    H5O_addr_map_t         *addr_map = NULL;       /* Address mapping of object copied */
    H5O_t                  *oh_src = NULL;         /* Object header for source object */
    H5O_t                  *oh_dst = NULL;         /* Object header for destination object */
    unsigned               mesgno = 0;
    haddr_t                addr_new = HADDR_UNDEF;
    hbool_t                *deleted = NULL;      /* Array of flags indicating whether messages should be copied */
    hbool_t                inserted = FALSE;        /* Whether the destination object header has been inserted into the cache */
    size_t                 null_msgs;               /* Number of NULL messages found in each loop */
    size_t                 orig_dst_msgs;           /* Original # of messages in dest. object */
    H5O_mesg_t             *mesg_src;               /* Message in source object header */
    H5O_mesg_t             *mesg_dst;               /* Message in source object header */
    const H5O_msg_class_t  *copy_type;              /* Type of message to use for copying */
    const H5O_obj_class_t  *obj_class = NULL;       /* Type of object we are copying */
    void                   *cpy_udata = NULL;       /* User data for passing to message callbacks */
    uint64_t               dst_oh_size;             /* Total size of the destination OH */
    size_t                 dst_oh_null;             /* Size of the null message to add to destination OH */
    size_t                 dst_oh_gap;              /* Size of the gap in chunk #0 of destination OH */
    uint8_t                *current_pos;            /* Current position in destination image */
    size_t                 msghdr_size;
    herr_t                 ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(oloc_src);
    HDassert(oloc_src->file);
    HDassert(H5F_addr_defined(oloc_src->addr));
    HDassert(oloc_dst->file);
    HDassert(cpy_info);

    /* Get pointer to object class for this object */
    if((obj_class = H5O_obj_class(oloc_src, dxpl_id)) == NULL)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to determine object type")

    /* Set the pointer to the shared struct for the object if opened in the file */
    cpy_info->shared_fo = H5FO_opened(oloc_src->file, oloc_src->addr);

    /* Get source object header */
    if(NULL == (oh_src = H5O_protect(oloc_src, dxpl_id, H5AC_READ)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header")

    /* Retrieve user data for particular type of object to copy */
    if(obj_class->get_copy_file_udata &&
            (NULL == (cpy_udata = (obj_class->get_copy_file_udata)())))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to retrieve copy user data")

    /* If we are merging committed datatypes, check for a match in the destination
     * file now */
    if(cpy_info->merge_comm_dt && obj_class->type == H5O_TYPE_NAMED_DATATYPE) {
        unsigned long fileno_src; /* fileno for source file */
        unsigned long fileno_dst; /* fileno for destination file */
        htri_t merge; /* Whether we found a match in the destination file */

        /* Check if the source and dest file are the same.  If so, just return
         * the source object address */
        H5F_GET_FILENO(oloc_src->file, fileno_src);
        H5F_GET_FILENO(oloc_dst->file, fileno_dst);
        if(fileno_src == fileno_dst) {
            merge = TRUE;
            oloc_dst->addr = oloc_src->addr;
        } /* end if */
        else
            /* Search for a matching committed datatype, building the list if
             * necessary */
            if((merge = H5O_copy_search_comm_dt(oloc_src->file, oh_src, oloc_dst, dxpl_id, cpy_info)) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't search for matching committed datatype")

        if(merge) {
            /* Found a match, add to skip list and exit */
            /* Allocate space for the address mapping of the object copied */
            if(NULL == (addr_map = H5FL_MALLOC(H5O_addr_map_t)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

            /* Insert the address mapping for the found object into the copied
             * list */
            addr_map->src_obj_pos.fileno = fileno_src;
            addr_map->src_obj_pos.addr = oloc_src->addr;
            addr_map->dst_addr = oloc_dst->addr;
            addr_map->is_locked = TRUE;                 /* We've locked the object currently */
            addr_map->inc_ref_count = 0;                /* Start with no additional ref counts to add */
            addr_map->obj_class = obj_class;
            addr_map->udata = cpy_udata;

            /* Insert into skip list */
            if(H5SL_insert(cpy_info->map_list, addr_map, &(addr_map->src_obj_pos)) < 0) {
                addr_map = H5FL_FREE(H5O_addr_map_t, addr_map);
                HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "can't insert object into skip list")
            } /* end if */

            HGOTO_DONE(SUCCEED)
        } /* end if */
    } /* end if */

    /* Flush any dirty messages in source object header to update the header chunks */
    if(H5O_flush_msgs(oloc_src->file, oh_src) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTFLUSH, FAIL, "unable to flush object header messages")

    /* Allocate the destination object header and fill in header fields */
    if(NULL == (oh_dst = H5FL_CALLOC(H5O_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Initialize header information */
    oh_dst->version = oh_src->version;
    oh_dst->flags = oh_src->flags;
    oh_dst->link_msgs_seen = oh_src->link_msgs_seen;
    oh_dst->attr_msgs_seen = oh_src->attr_msgs_seen;
    oh_dst->sizeof_size = H5F_SIZEOF_SIZE(oloc_dst->file);
    oh_dst->sizeof_addr = H5F_SIZEOF_ADDR(oloc_dst->file);

    /* Copy time fields */
    oh_dst->atime = oh_src->atime;
    oh_dst->mtime = oh_src->mtime;
    oh_dst->ctime = oh_src->ctime;
    oh_dst->btime = oh_src->btime;

    /* Copy attribute storage information */
    oh_dst->max_compact = oh_src->max_compact;
    oh_dst->min_dense = oh_src->min_dense;

    /* Initialize size of chunk array.  Start off with zero chunks so this field
     * is consistent with the current state of the chunk array.  This is
     * important if an error occurs.
     */
    oh_dst->alloc_nchunks = oh_dst->nchunks = 0;

    /* Allocate memory for the chunk array - always start with 1 chunk */
    if(NULL == (oh_dst->chunk = H5FL_SEQ_MALLOC(H5O_chunk_t, 1)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Update number of allocated chunks.  There are still no chunks used. */
    oh_dst->alloc_nchunks = 1;

    /* Allocate memory for "deleted" array.  This array marks the message in
     * the source that shouldn't be copied to the destination.
     */
    if(NULL == (deleted = (hbool_t *)HDmalloc(sizeof(hbool_t) * oh_src->nmesgs)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
    HDmemset(deleted, FALSE, sizeof(hbool_t) * oh_src->nmesgs);

    /* "pre copy" pass over messages, to gather information for actual message copy operation
     * (for messages which depend on information from other messages)
     * Keep track of how many NULL or deleted messages we find (or create)
     */
    null_msgs = 0;
    for(mesgno = 0; mesgno < oh_src->nmesgs; mesgno++) {
        /* Set up convenience variables */
        mesg_src = &(oh_src->mesg[mesgno]);

        /* Sanity check */
        HDassert(!mesg_src->dirty);     /* Should be cleared by earlier call to flush messages */

        /* Get message class to operate on */
        copy_type = mesg_src->type;

        /* Check for continuation message; these are converted to NULL
         * messages because the destination OH will have only one chunk
         */
        if(H5O_CONT_ID == mesg_src->type->id || H5O_NULL_ID == mesg_src->type->id) {
            deleted[mesgno] = TRUE;
            ++null_msgs;
            copy_type = H5O_MSG_NULL;
        } /* end if */
        HDassert(copy_type);

        if(copy_type->pre_copy_file) {
            /* Decode the message if necessary. */
            H5O_LOAD_NATIVE(oloc_src->file, dxpl_id, 0, oh_src, mesg_src, FAIL)

            /* Perform "pre copy" operation on message */
            if((copy_type->pre_copy_file)(oloc_src->file, mesg_src->native,
                    &(deleted[mesgno]), cpy_info, cpy_udata) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to perform 'pre copy' operation on message")

            /* Check if the message should be deleted in the destination */
            if(deleted[mesgno])
                /* Mark message as deleted */
                ++null_msgs;
        } /* end if(copy_type->pre_copy_file) */
    } /* end for */

    /* Initialize size of message list.  It may or may not include the NULL messages
     * detected above.
     */
    if(cpy_info->preserve_null)
        oh_dst->alloc_nmesgs = oh_dst->nmesgs = oh_src->nmesgs;
    else
        oh_dst->alloc_nmesgs = oh_dst->nmesgs = (oh_src->nmesgs - null_msgs);

    /* Allocate memory for destination message array */
    if(oh_dst->alloc_nmesgs > 0)
        if(NULL == (oh_dst->mesg = H5FL_SEQ_CALLOC(H5O_mesg_t, oh_dst->alloc_nmesgs)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* "copy" pass over messages, to perform main message copying */
    null_msgs = 0;
    for(mesgno = 0; mesgno < oh_dst->nmesgs; mesgno++) {
        /* Skip any deleted or NULL messages in the source unless the
         * preserve_null flag is set
         */
        if(FALSE == cpy_info->preserve_null) {
            while(deleted[mesgno + null_msgs]) {
                ++null_msgs;
                HDassert(mesgno + null_msgs < oh_src->nmesgs);
            } /* end while */
        } /* end if */

        /* Set up convenience variables */
        mesg_src = &(oh_src->mesg[mesgno + null_msgs]);
        mesg_dst = &(oh_dst->mesg[mesgno]);

        /* Initialize non-zero components of destination message */
        mesg_dst->crt_idx = mesg_src->crt_idx;
        mesg_dst->flags = mesg_src->flags;
        mesg_dst->raw_size = mesg_src->raw_size;
        mesg_dst->type = mesg_src->type;

        /* If we're preserving deleted messages, set their types to 'NULL'
         * in the destination.
         */
        if(cpy_info->preserve_null && deleted[mesgno]) {
            mesg_dst->type = H5O_MSG_NULL;
            mesg_dst->flags = 0;
            mesg_dst->dirty = TRUE;
        } /* end if */

        /* Check for message class to operate on */
        /* (Use destination message, in case the message has been removed (i.e
         *      converted to a nil message) in the destination -QAK)
         */
        copy_type = mesg_dst->type;
        HDassert(copy_type);

        /* copy this message into destination file */
        if(copy_type->copy_file) {
            hbool_t recompute_size;     /* Whether copy_file callback created a shared message */
            unsigned mesg_flags;        /* Message flags */

            /* Decode the message if necessary. */
            H5O_LOAD_NATIVE(oloc_src->file, dxpl_id, 0, oh_src, mesg_src, FAIL)

            /* Get destination message flags, and unset shared and shareable
             * flags.  mesg_dst->flags will contain the original flags for now.
             */
            mesg_flags = (unsigned)mesg_dst->flags & ~H5O_MSG_FLAG_SHARED
                    & ~H5O_MSG_FLAG_SHAREABLE;

            /* Copy the source message */
            recompute_size = FALSE;
            if((mesg_dst->native = H5O_msg_copy_file(copy_type, oloc_src->file,
                    mesg_src->native, oloc_dst->file, &recompute_size,
                    &mesg_flags, cpy_info, cpy_udata, dxpl_id)) == NULL)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy object header message")

            /* Check if the sharing state changed, and recompute the size if so
             */
            if(!(mesg_flags & H5O_MSG_FLAG_SHARED)
                    != !(mesg_dst->flags & H5O_MSG_FLAG_SHARED))
                recompute_size = TRUE;

            /* Set destination message flags */
            mesg_dst->flags = (uint8_t)mesg_flags;

            /* Recompute message's size */
            /* (its sharing status or one of its components (for attributes)
             *  could have changed)
             */
            if(recompute_size)
                mesg_dst->raw_size = H5O_ALIGN_OH(oh_dst,
                        H5O_msg_raw_size(oloc_dst->file, mesg_dst->type->id, FALSE, mesg_dst->native));

            /* Mark the message in the destination as dirty, so it'll get encoded when the object header is flushed */
            mesg_dst->dirty = TRUE;
        } /* end if (mesg_src->type->copy_file) */
    } /* end of mesgno loop */


    /* Allocate the destination header and copy any messages that didn't have
     * copy callbacks.  They get copied directly from the source image to the
     * destination image.
     */

    /* Calculate how big the destination object header will be on disk.
     * This isn't necessarily the same size as the original.
     */

    /* Compute space for messages. */
    dst_oh_size = 0;
    for(mesgno = 0; mesgno < oh_dst->nmesgs; mesgno++) {
        dst_oh_size += (uint64_t)H5O_SIZEOF_MSGHDR_OH(oh_dst);
        dst_oh_size += oh_dst->mesg[mesgno].raw_size;
    } /* end for */

    /* Check if we need to determine correct value for chunk #0 size bits */
    if(oh_dst->version > H5O_VERSION_1) {
        /* Reset destination object header's "chunk 0 size" flags */
        oh_dst->flags = (uint8_t)(oh_dst->flags & ~H5O_HDR_CHUNK0_SIZE);

        /* Determine correct value for chunk #0 size bits */
        if(dst_oh_size > 4294967295)
            oh_dst->flags |= H5O_HDR_CHUNK0_8;
        else if(dst_oh_size > 65535)
            oh_dst->flags |= H5O_HDR_CHUNK0_4;
        else if(dst_oh_size > 255)
            oh_dst->flags |= H5O_HDR_CHUNK0_2;
    } /* end if */

    /* Check if the chunk's data portion is too small */
    dst_oh_gap = dst_oh_null = 0;
    if(dst_oh_size < H5O_MIN_SIZE) {
        size_t delta = (size_t)(H5O_MIN_SIZE - dst_oh_size);    /* Delta in chunk size needed */

        /* Sanity check */
        HDassert((oh_dst->flags & H5O_HDR_CHUNK0_SIZE) == H5O_HDR_CHUNK0_1);

        /* Determine whether to create gap or NULL message */
        if(delta < H5O_SIZEOF_MSGHDR_OH(oh_dst))
            dst_oh_gap = delta;
        else
            dst_oh_null = delta;

        /* Increase destination object header size */
        dst_oh_size += delta;

        /* Sanity check */
        HDassert(dst_oh_size <= 255);
    } /* end if */

    /* Add in destination's object header size now */
    dst_oh_size += (uint64_t)H5O_SIZEOF_HDR(oh_dst);

    /* Allocate space for chunk in destination file */
    if(HADDR_UNDEF == (oh_dst->chunk[0].addr = H5MF_alloc(oloc_dst->file, H5FD_MEM_OHDR, dxpl_id, (hsize_t)dst_oh_size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "file allocation failed for object header")
    addr_new = oh_dst->chunk[0].addr;

    /* Create memory image for the new chunk */
    /* Note: we use calloc() instead of malloc() here because older versions of
     *  some messages don't initialize "unused" bytes and because we want to
     *  write out the same version of the object header and older versions of
     *  object headers aligned messages.  In both those situations, it's
     *  complex and error-prone to determine all the proper ways/places to
     *  clear to zero bytes, so we just set the buffer to zero's here.
     *  (QAK - 2010/08/17)
     */
    if(NULL == (oh_dst->chunk[0].image = H5FL_BLK_CALLOC(chunk_image, (size_t)dst_oh_size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Set dest. chunk information */
    oh_dst->chunk[0].size = (size_t)dst_oh_size;
    oh_dst->chunk[0].gap = dst_oh_gap;

    /* Update size of chunk array.  The destination now has one chunk. */
    oh_dst->nchunks = 1;

    /* Set up raw pointers and copy messages that didn't need special
     * treatment.  This has to happen after the destination header has been
     * allocated.
     */
    HDassert(H5O_SIZEOF_MSGHDR_OH(oh_src) == H5O_SIZEOF_MSGHDR_OH(oh_dst));
    msghdr_size = H5O_SIZEOF_MSGHDR_OH(oh_dst);

    current_pos = oh_dst->chunk[0].image;

    /* Write the magic number for versions > 1 and skip the rest of the
     * header.  This will be written when the header is flushed to disk.
     */
    if(oh_dst->version > H5O_VERSION_1)
        HDmemcpy(current_pos, H5O_HDR_MAGIC, (size_t)H5_SIZEOF_MAGIC);
    current_pos += H5O_SIZEOF_HDR(oh_dst) - H5O_SIZEOF_CHKSUM_OH(oh_dst);

    /* Loop through destination messages, updating their "raw" info */
    null_msgs = 0;
    for(mesgno = 0; mesgno < oh_dst->nmesgs; mesgno++) {
        /* Skip any deleted or NULL messages in the source unless the
         * preserve_null flag is set.
         */
        if(FALSE == cpy_info->preserve_null) {
            while(deleted[mesgno + null_msgs]) {
                ++null_msgs;
                HDassert(mesgno + null_msgs < oh_src->nmesgs);
            } /* end while */
        } /* end if */

        /* Set up convenience variables */
        mesg_src = &(oh_src->mesg[mesgno + null_msgs]);
        mesg_dst = &(oh_dst->mesg[mesgno]);

        /* Copy each message that wasn't dirtied above */
        if(!mesg_dst->dirty)
            /* Copy the message header plus the message's raw data. */
            HDmemcpy(current_pos, mesg_src->raw - msghdr_size, msghdr_size + mesg_src->raw_size);

        /* Set message's raw pointer to destination chunk's new "image" */
        mesg_dst->raw = current_pos + msghdr_size;

        /* Move to location where next message should go */
        current_pos += mesg_dst->raw_size + msghdr_size;
    } /* end for */

    /* Save this in case more messages are added during NULL message checking */
    orig_dst_msgs = oh_dst->nmesgs;

    /* Check if we need to add a NULL message to this header */
    if(dst_oh_null > 0) {
        size_t null_idx;                /* Index of new NULL message */

        /* Make sure we have enough space for new NULL message */
        if(oh_dst->nmesgs + 1 > oh_dst->alloc_nmesgs)
            if(H5O_alloc_msgs(oh_dst, (size_t)1) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate more space for messages")

        /* Create null message for [rest of] space in new chunk */
        /* (account for chunk's magic # & checksum) */
        null_idx = oh_dst->nmesgs++;
        oh_dst->mesg[null_idx].type = H5O_MSG_NULL;
        oh_dst->mesg[null_idx].dirty = TRUE;
        oh_dst->mesg[null_idx].native = NULL;
        oh_dst->mesg[null_idx].raw = current_pos + msghdr_size;
        oh_dst->mesg[null_idx].raw_size = dst_oh_null -  msghdr_size;
        oh_dst->mesg[null_idx].chunkno = 0;
    } /* end if */

    /* Make sure we filled the chunk, except for room at the end for a checksum */
    HDassert(current_pos + dst_oh_gap + dst_oh_null + H5O_SIZEOF_CHKSUM_OH(oh_dst) == (size_t)dst_oh_size + oh_dst->chunk[0].image);

    /* Set the dest. object location to the first chunk address */
    HDassert(H5F_addr_defined(addr_new));
    oloc_dst->addr = addr_new;


    /* If we are merging committed datatypes and this is a committed datatype, insert
     * the copied datatype into the list of committed datatypes in the target file.
     */
    if(cpy_info->merge_comm_dt && obj_class->type == H5O_TYPE_NAMED_DATATYPE)
        if(H5O_copy_insert_comm_dt(oloc_src->file, oh_src, oloc_dst, dxpl_id, cpy_info) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't insert committed datatype into destination list")

    /* Allocate space for the address mapping of the object copied */
    if(NULL == (addr_map = H5FL_MALLOC(H5O_addr_map_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Insert the address mapping for the new object into the copied list */
    /* (Do this here, because "post copy" possibly checks it) */
    H5F_GET_FILENO(oloc_src->file, addr_map->src_obj_pos.fileno);
    addr_map->src_obj_pos.addr = oloc_src->addr;
    addr_map->dst_addr = oloc_dst->addr;
    addr_map->is_locked = TRUE;                 /* We've locked the object currently */
    addr_map->inc_ref_count = 0;                /* Start with no additional ref counts to add */
    addr_map->obj_class = obj_class;
    addr_map->udata = cpy_udata;

    /* Insert into skip list */
    if(H5SL_insert(cpy_info->map_list, addr_map, &(addr_map->src_obj_pos)) < 0) {
        addr_map = H5FL_FREE(H5O_addr_map_t, addr_map);
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "can't insert object into skip list")
    } /* end if */

    /* "post copy" loop over messages, to fix up any messages which require a complete
     * object header for destination object
     */
    null_msgs = 0;
    for(mesgno = 0; mesgno < orig_dst_msgs; mesgno++) {
        /* Skip any deleted or NULL messages in the source unless the
         * preserve_null flag is set
         */
        if(FALSE == cpy_info->preserve_null) {
            while(deleted[mesgno + null_msgs]) {
                ++null_msgs;
                HDassert(mesgno + null_msgs < oh_src->nmesgs);
            } /* end while */
        } /* end if */

        /* Set up convenience variables */
        mesg_src = &(oh_src->mesg[mesgno + null_msgs]);
        mesg_dst = &(oh_dst->mesg[mesgno]);

        /* Check for message class to operate on */
        /* (Use destination message, in case the message has been removed (i.e
         *      converted to a nil message) in the destination -QAK)
         */
        copy_type = mesg_dst->type;
        HDassert(copy_type);

        if(copy_type->post_copy_file && mesg_src->native) {
            unsigned mesg_flags;        /* Message flags */

            /* Sanity check destination message */
            HDassert(mesg_dst->type == mesg_src->type);
            HDassert(mesg_dst->native);

            /* Get destination message flags.   mesg_dst->flags will contain the
             * original flags for now. */
            mesg_flags = (unsigned)mesg_dst->flags;

            /* the object header is needed in the post copy for shared message */
            cpy_info->oh_dst = oh_dst;

            /* Perform "post copy" operation on message */
            if((copy_type->post_copy_file)(oloc_src, mesg_src->native, oloc_dst,
                    mesg_dst->native, &mesg_flags, dxpl_id, cpy_info) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to perform 'post copy' operation on message")

            /* Verify that the flags did not change */
            HDassert(mesg_flags == (unsigned) mesg_dst->flags);
        } /* end if */
    } /* end for */

    /* Indicate that the destination address will no longer be locked */
    addr_map->is_locked = FALSE;

    /* Increment object header's reference count, if any descendents have created links to this object */
    if(addr_map->inc_ref_count) {
        H5_CHECK_OVERFLOW(addr_map->inc_ref_count, hsize_t, unsigned);
        oh_dst->nlink += (unsigned)addr_map->inc_ref_count;
    } /* end if */

    /* Insert destination object header in cache */
    if(H5AC_insert_entry(oloc_dst->file, dxpl_id, H5AC_OHDR, oloc_dst->addr, oh_dst, H5AC__NO_FLAGS_SET) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "unable to cache object header")
    oh_dst = NULL;
    inserted = TRUE;

    /* Set obj_type and udata, if requested */
    if(obj_type) {
        HDassert(udata);
        *obj_type = obj_class->type;
        *udata = cpy_udata;
    } /* end if */

done:
    /* Free deleted array */
    if(deleted)
        HDfree(deleted);

    /* Release pointer to source object header and its derived objects */
    if(oh_src && H5O_unprotect(oloc_src, dxpl_id, oh_src, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

    /* Free destination object header on failure */
    if(ret_value < 0 && oh_dst && !inserted) {
        if(H5O_free(oh_dst) < 0)
            HDONE_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to destroy object header data")
        if(H5O_loc_reset(oloc_dst) < 0)
            HDONE_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to destroy object header data")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_copy_header_real() */


/*-------------------------------------------------------------------------
 * Function:    H5O_copy_header_map
 *
 * Purpose:     Copy header object from one location to another, detecting
 *              already mapped objects, etc.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              November 1, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_copy_header_map(const H5O_loc_t *oloc_src, H5O_loc_t *oloc_dst /*out*/,
    hid_t dxpl_id, H5O_copy_t *cpy_info, hbool_t inc_depth,
    H5O_type_t *obj_type, void **udata /*out*/)
{
    H5O_addr_map_t      *addr_map = NULL;       /* Address mapping of object copied */
    H5_obj_t            src_obj_pos;            /* Position of source object */
    hbool_t             inc_link;               /* Whether to increment the link count for the object */
    herr_t              ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(oloc_src);
    HDassert(oloc_src->file);
    HDassert(oloc_dst);
    HDassert(oloc_dst->file);
    HDassert(cpy_info);

    /* Create object "position" struct */
    H5F_GET_FILENO(oloc_src->file, src_obj_pos.fileno);
    src_obj_pos.addr = oloc_src->addr;

    /* Search for the object in the skip list of copied objects */
    addr_map = (H5O_addr_map_t *)H5SL_search(cpy_info->map_list,
            &src_obj_pos);

    /* Check if address is already in list of objects copied */
    if(addr_map == NULL) {
        /* Copy object for the first time */

        /* Check for incrementing the depth of copy */
        /* (Can't do this for all copies, since committed datatypes should always be copied) */
        if(inc_depth)
            cpy_info->curr_depth++;

        /* Copy object referred to */
        if(H5O_copy_header_real(oloc_src, oloc_dst, dxpl_id, cpy_info, obj_type,
                udata) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy object")

        /* Check for incrementing the depth of copy */
        if(inc_depth)
            cpy_info->curr_depth--;

        /* When an object is copied for the first time, increment it's link */
        inc_link = TRUE;

        /* indicate that a new object is created */
        ret_value++;
    } /* end if */
    else {
        /* Object has already been copied, set its address in destination file */
        oloc_dst->addr = addr_map->dst_addr;

        /* Return saved obj_type and udata, if requested */
        if(obj_type) {
            HDassert(udata);
            *obj_type = addr_map->obj_class->type;
            *udata = addr_map->udata;
        } /* end if */

        /* If the object is locked currently (because we are copying a group
         * hierarchy and this is a link to a group higher in the hierarchy),
         * increment it's deferred reference count instead of incrementing the
         * reference count now.
         */
        if(addr_map->is_locked) {
            addr_map->inc_ref_count++;
            inc_link = FALSE;
        } /* end if */
        else
            inc_link = TRUE;
    } /* end else */

    /* Increment destination object's link count, if allowed */
    if(inc_link)
        if(H5O_link(oloc_dst, 1, dxpl_id) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to increment object link count")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_copy_header_map() */


/*--------------------------------------------------------------------------
 NAME
    H5O_copy_free_addrmap_cb
 PURPOSE
    Internal routine to free address maps from the skip list for copying objects
 USAGE
    herr_t H5O_copy_free_addrmap_cb(item, key, op_data)
        void *item;             IN/OUT: Pointer to addr
        void *key;              IN/OUT: (unused)
        void *op_data;          IN: (unused)
 RETURNS
    Returns zero on success, negative on failure.
 DESCRIPTION
        Releases the memory for the address.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5O_copy_free_addrmap_cb(void *_item, void H5_ATTR_UNUSED *key, void H5_ATTR_UNUSED *op_data)
{
    H5O_addr_map_t *item = (H5O_addr_map_t *)_item;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(item);

    /* Release user data for particular type of object */
    if(item->udata) {
        HDassert(item->obj_class);
        HDassert(item->obj_class->free_copy_file_udata);
        (item->obj_class->free_copy_file_udata)(item->udata);
    } /* end if */

    /* Release the item */
    item = H5FL_FREE(H5O_addr_map_t, item);

    FUNC_LEAVE_NOAPI(0)
}   /* H5O_copy_free_addrmap_cb() */


/*-------------------------------------------------------------------------
 * Function:    H5O_copy_header
 *
 * Purpose:     copy header object from one location to another.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *              May 30, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_copy_header(const H5O_loc_t *oloc_src, H5O_loc_t *oloc_dst /*out */,
    hid_t dxpl_id, hid_t ocpypl_id)
{
    H5O_copy_t  cpy_info;               /* Information for copying object */
    H5P_genplist_t  *ocpy_plist;        /* Object copy property list created */
    H5O_copy_dtype_merge_list_t *dt_list = NULL; /* List of datatype merge suggestions */
    H5O_mcdt_cb_info_t   cb_info;      	/* Callback info struct */
    unsigned    cpy_option = 0;         /* Copy options */
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(oloc_src);
    HDassert(oloc_src->file);
    HDassert(H5F_addr_defined(oloc_src->addr));
    HDassert(oloc_dst->file);

    /* Intialize copy info before errors can be thrown */
    HDmemset(&cpy_info, 0, sizeof(H5O_copy_t));

    /* Get the copy property list */
    if(NULL == (ocpy_plist = (H5P_genplist_t *)H5I_object(ocpypl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

    /* Retrieve the copy parameters */
    if(H5P_get(ocpy_plist, H5O_CPY_OPTION_NAME, &cpy_option) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get object copy flag")

    /* Retrieve the marge committed datatype list */
    if(H5P_get(ocpy_plist, H5O_CPY_MERGE_COMM_DT_LIST_NAME, &dt_list) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get merge committed datatype list")

    /* Get callback info */
    if(H5P_get(ocpy_plist, H5O_CPY_MCDT_SEARCH_CB_NAME, &cb_info) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get callback info")

    /* Convert copy flags into copy struct */
    if((cpy_option & H5O_COPY_SHALLOW_HIERARCHY_FLAG) > 0) {
        cpy_info.copy_shallow = TRUE;
        cpy_info.max_depth = 1;
    } /* end if */
    else
        cpy_info.max_depth = -1;        /* Current default is for full, recursive hier. copy */
    cpy_info.curr_depth = 0;
    if((cpy_option & H5O_COPY_EXPAND_SOFT_LINK_FLAG) > 0)
        cpy_info.expand_soft_link = TRUE;
    if((cpy_option & H5O_COPY_EXPAND_EXT_LINK_FLAG) > 0)
        cpy_info.expand_ext_link = TRUE;
    if((cpy_option & H5O_COPY_EXPAND_REFERENCE_FLAG) > 0)
        cpy_info.expand_ref = TRUE;
    if((cpy_option & H5O_COPY_WITHOUT_ATTR_FLAG) > 0)
        cpy_info.copy_without_attr = TRUE;
    if((cpy_option & H5O_COPY_PRESERVE_NULL_FLAG) > 0)
        cpy_info.preserve_null = TRUE;
    if((cpy_option & H5O_COPY_MERGE_COMMITTED_DTYPE_FLAG) > 0)
        cpy_info.merge_comm_dt = TRUE;

    /* Add dt_list to copy struct */
    cpy_info.dst_dt_suggestion_list = dt_list;

    /* Add set callback information */
    cpy_info.mcdt_cb = cb_info.func;
    cpy_info.mcdt_ud = cb_info.user_data;

    /* Create a skip list to keep track of which objects are copied */
    if(NULL == (cpy_info.map_list = H5SL_create(H5SL_TYPE_OBJ, NULL)))
        HGOTO_ERROR(H5E_SLIST, H5E_CANTCREATE, FAIL, "cannot make skip list")

    /* copy the object from the source file to the destination file */
    if(H5O_copy_header_real(oloc_src, oloc_dst, dxpl_id, &cpy_info, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy object")

done:
    if(cpy_info.map_list)
        H5SL_destroy(cpy_info.map_list, H5O_copy_free_addrmap_cb, NULL);
    if(cpy_info.dst_dt_list)
        H5SL_destroy(cpy_info.dst_dt_list, H5O_copy_free_comm_dt_cb, NULL);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_copy_header() */


/*-------------------------------------------------------------------------
 * Function:    H5O_copy_obj
 *
 * Purpose:     Copy an object to destination location
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *              June 4, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_copy_obj(H5G_loc_t *src_loc, H5G_loc_t *dst_loc, const char *dst_name,
    hid_t ocpypl_id, hid_t lcpl_id, hid_t dxpl_id)
{
    H5G_name_t      new_path;                   /* Copied object group hier. path */
    H5O_loc_t       new_oloc;                   /* Copied object object location */
    H5G_loc_t       new_loc;                    /* Group location of object copied */
    H5F_t           *cached_dst_file;           /* Cached destination file */
    hbool_t         entry_inserted = FALSE;     /* Flag to indicate that the new entry was inserted into a group */
    herr_t          ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(src_loc);
    HDassert(src_loc->oloc->file);
    HDassert(dst_loc);
    HDassert(dst_loc->oloc->file);
    HDassert(dst_name);

    /* Set up copied object location to fill in */
    new_loc.oloc = &new_oloc;
    new_loc.path = &new_path;
    H5G_loc_reset(&new_loc);
    new_oloc.file = dst_loc->oloc->file;

    /* Make a copy of the destination file, in case the original is changed by
     * H5O_copy_header.  If and when oloc's point to the shared file struct,
     * this will no longer be necessary, so this code can be removed. */
    cached_dst_file = dst_loc->oloc->file;

    /* Copy the object from the source file to the destination file */
    if(H5O_copy_header(src_loc->oloc, &new_oloc, dxpl_id, ocpypl_id) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy object")

    /* Patch dst_loc.  Again, this can be removed once oloc's point to shared
     * file structs. */
    dst_loc->oloc->file = cached_dst_file;

    /* Insert the new object in the destination file's group */
    if(H5L_link(dst_loc, dst_name, &new_loc, lcpl_id, H5P_DEFAULT, dxpl_id) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to insert link")
    entry_inserted = TRUE;

done:
    /* Free the ID to name buffers */
    if(entry_inserted)
        H5G_loc_free(&new_loc);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_copy_obj() */


/*-------------------------------------------------------------------------
 * Function:    H5O_copy_obj_by_ref
 *
 * Purpose:     Copy the object pointed by _src_ref.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *              Aug 7 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_copy_obj_by_ref(H5O_loc_t *src_oloc, hid_t dxpl_id, H5O_loc_t *dst_oloc,
    H5G_loc_t *dst_root_loc, H5O_copy_t *cpy_info)
{
    herr_t  ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(src_oloc);
    HDassert(dst_oloc);

    /* Perform the copy, or look up existing copy */
    if((ret_value = H5O_copy_header_map(src_oloc, dst_oloc, dxpl_id, cpy_info,
            FALSE, NULL, NULL)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy object")

    /* Check if a new valid object is copied to the destination */
    if(H5F_addr_defined(dst_oloc->addr) && (ret_value > SUCCEED)) {
        char    tmp_obj_name[80];
        H5G_name_t      new_path;
        H5O_loc_t       new_oloc;
        H5G_loc_t       new_loc;

        /* Set up group location for new object */
        new_loc.oloc = &new_oloc;
        new_loc.path = &new_path;
        H5G_loc_reset(&new_loc);
        new_oloc.file = dst_oloc->file;
        new_oloc.addr = dst_oloc->addr;

        /* Pick a default name for the new object */
        HDsnprintf(tmp_obj_name, sizeof(tmp_obj_name), "~obj_pointed_by_%llu", (unsigned long long)dst_oloc->addr);

        /* Create a link to the newly copied object */
        /* Note: since H5O_copy_header_map actually copied the target object, it
         * must exist either in cache or on disk, therefore it is is safe to not
         * pass the obj_type and udata fields returned by H5O_copy_header_map.
         * This could be changed in the future to slightly improve performance
         * --NAF */
        if(H5L_link(dst_root_loc, tmp_obj_name, &new_loc, H5P_DEFAULT, H5P_DEFAULT, dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to insert link")

        H5G_loc_free(&new_loc);
    } /* if (H5F_addr_defined(dst_oloc.addr)) */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_copy_obj_by_ref() */


/*-------------------------------------------------------------------------
 * Function:	H5O_copy_expand_ref
 *
 * Purpose:	Copy the object pointed by _src_ref.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *		Aug 7 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_copy_expand_ref(H5F_t *file_src, void *_src_ref, hid_t dxpl_id,
    H5F_t *file_dst, void *_dst_ref, size_t ref_count, H5R_type_t ref_type,
    H5O_copy_t *cpy_info)
{
    H5O_loc_t 	dst_oloc;         	/* Copied object object location */
    H5O_loc_t	src_oloc;          	/* Temporary object location for source object */
    H5G_loc_t   dst_root_loc;           /* The location of root group of the destination file */
    const uint8_t *q;                   /* Pointer to source OID to store */
    uint8_t     *p;                     /* Pointer to destination OID to store */
    size_t      i;                      /* Local index variable */
    herr_t	ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(file_src);
    HDassert(_src_ref);
    HDassert(file_dst);
    HDassert(_dst_ref);
    HDassert(ref_count);
    HDassert(cpy_info);

    /* Initialize object locations */
    H5O_loc_reset(&src_oloc);
    H5O_loc_reset(&dst_oloc);
    src_oloc.file = file_src;
    dst_oloc.file = file_dst;

    /* Set up the root group in the destination file */
    if(NULL == (dst_root_loc.oloc = H5G_oloc(H5G_rootof(file_dst))))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get object location for root group")
    if(NULL == (dst_root_loc.path = H5G_nameof(H5G_rootof(file_dst))))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get path for root group")

    /* Copy object references */
    if(H5R_OBJECT == ref_type) {
        hobj_ref_t *src_ref = (hobj_ref_t *)_src_ref;
        hobj_ref_t *dst_ref = (hobj_ref_t *)_dst_ref;

        /* Making equivalent references in the destination file */
        for(i = 0; i < ref_count; i++) {
            /* Set up for the object copy for the reference */
            q = (uint8_t *)(&src_ref[i]);
            H5F_addr_decode(src_oloc.file, (const uint8_t **)&q, &(src_oloc.addr));
            dst_oloc.addr = HADDR_UNDEF;

            /* Attempt to copy object from source to destination file */
            if(src_oloc.addr != (haddr_t)0) {
                if(H5O_copy_obj_by_ref(&src_oloc, dxpl_id, &dst_oloc, &dst_root_loc, cpy_info) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy object")
            } /* end if */
            else
                /* Set parameters so the reference is written as all 0's */
                HDmemset(&dst_oloc.addr, 0, sizeof(dst_oloc.addr));

            /* Set the object reference info for the destination file */
            p = (uint8_t *)(&dst_ref[i]);
            H5F_addr_encode(dst_oloc.file, &p, dst_oloc.addr);
	} /* end for */
    }  /* end if */
    /* Copy region references */
    else if(H5R_DATASET_REGION == ref_type) {
        hdset_reg_ref_t *src_ref = (hdset_reg_ref_t *)_src_ref;
        hdset_reg_ref_t *dst_ref = (hdset_reg_ref_t *)_dst_ref;
        uint8_t *buf = NULL;    /* Buffer to store serialized selection in */
        H5HG_t hobjid;          /* Heap object ID */
        size_t buf_size;        /* Length of object in heap */

        /* Making equivalent references in the destination file */
        for(i = 0; i < ref_count; i++) {
            /* Get the heap ID for the dataset region */
            q = (const uint8_t *)(&src_ref[i]);
            H5F_addr_decode(src_oloc.file, (const uint8_t **)&q, &(hobjid.addr));
            UINT32DECODE(q, hobjid.idx);

            if(hobjid.addr != (haddr_t)0) {
                /* Get the dataset region from the heap (allocate inside routine) */
                if((buf = (uint8_t *)H5HG_read(src_oloc.file, dxpl_id, &hobjid, NULL, &buf_size)) == NULL)
                    HGOTO_ERROR(H5E_REFERENCE, H5E_READERROR, FAIL, "Unable to read dataset region information")

                /* Get the object oid for the dataset */
                q = (const uint8_t *)buf;
                H5F_addr_decode(src_oloc.file, (const uint8_t **)&q, &(src_oloc.addr));
                dst_oloc.addr = HADDR_UNDEF;

                /* copy the object pointed by the ref to the destination */
                if(H5O_copy_obj_by_ref(&src_oloc, dxpl_id, &dst_oloc, &dst_root_loc, cpy_info) < 0) {
                    H5MM_xfree(buf);
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy object")
                } /* end if */

                /* Serialize object ID */
                p = (uint8_t *)buf;
                H5F_addr_encode(dst_oloc.file, &p, dst_oloc.addr);

                /* Save the serialized buffer to the destination */
                if(H5HG_insert(dst_oloc.file, dxpl_id, buf_size, buf, &hobjid) < 0) {
                    H5MM_xfree(buf);
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "Unable to write dataset region information")
                } /* end if */
            } /* end if */
            else
                /* Set parameters so the reference is written as all 0's */
                HDmemset(&hobjid, 0, sizeof(hobjid));

            /* Set the dataset region reference info for the destination file */
            p = (uint8_t *)(&dst_ref[i]);
            H5F_addr_encode(dst_oloc.file, &p, hobjid.addr);
            UINT32ENCODE(p, hobjid.idx);

            /* Free the buffer allocated in H5HG_read() */
            H5MM_xfree(buf);
        } /* end for */
    } /* end if */
    else
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference type")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_copy_expand_ref() */


/*-------------------------------------------------------------------------
 * Function:    H5O_copy_free_comm_dt_cb
 *
 * Purpose:     Frees the merge committed dt skip list key and object.
 *
 * Return:      SUCCEED (never fails)
 *
 * Programmer:  Neil Fortner
 *              Oct 6 2011
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_copy_free_comm_dt_cb(void *item, void *_key, void H5_ATTR_UNUSED *op_data)
{
    haddr_t     *addr = (haddr_t *)item;
    H5O_copy_search_comm_dt_key_t *key = (H5O_copy_search_comm_dt_key_t *)_key;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(addr);
    HDassert(key);
    HDassert(key->dt);

    key->dt = (H5T_t *)H5O_msg_free(H5O_DTYPE_ID, key->dt);
    key = H5FL_FREE(H5O_copy_search_comm_dt_key_t, key);
    addr = H5FL_FREE(haddr_t, addr);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_copy_free_comm_dt_cb */


/*-------------------------------------------------------------------------
 * Function:    H5O_copy_comm_dt_cmp
 *
 * Purpose:     Skiplist callback used to compare 2 keys for the merge
 *              committed dt list.  Mostly a wrapper for H5T_cmp.
 *
 * Return:      0 if key1 and key2 are equal.
 *              <0 if key1 is less than key2.
 *              >0 if key1 is greater than key2.
 *
 * Programmer:  Neil Fortner
 *              Oct 6 2011
 *
 *-------------------------------------------------------------------------
 */
static int
H5O_copy_comm_dt_cmp(const void *_key1, const void *_key2)
{
    const H5O_copy_search_comm_dt_key_t *key1 = (const H5O_copy_search_comm_dt_key_t *)_key1;
    const H5O_copy_search_comm_dt_key_t *key2 = (const H5O_copy_search_comm_dt_key_t *)_key2;
    int ret_value = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check fileno.  It is unlikely to be different so check if they are equal
     * first so only one comparison needs to be made. */
    if(key1->fileno != key2->fileno) {
        if(key1->fileno < key2->fileno)
            HGOTO_DONE(-1)
        if(key1->fileno > key2->fileno)
            HGOTO_DONE(1)
    } /* end if */

    ret_value = H5T_cmp(key1->dt, key2->dt, FALSE);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_copy_comm_dt_cmp */


/*-------------------------------------------------------------------------
 * Function:    H5O_copy_search_comm_dt_attr_cb
 *
 * Purpose:     Callback for H5O_attr_iterate_real from
 *              H5O_copy_search_comm_dt_check.  Checks if the attribute's
 *              datatype is committed.  If it is, adds it to the merge
 *              committed dt skiplist present in udata if it does not match
 *              any already present.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Neil Fortner
 *              Nov 3 2011
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_copy_search_comm_dt_attr_cb(const H5A_t *attr, void *_udata)
{
    H5O_copy_search_comm_dt_ud_t *udata = (H5O_copy_search_comm_dt_ud_t *)_udata;
    H5T_t       *dt = NULL;             /* Datatype */
    H5O_copy_search_comm_dt_key_t *key = NULL; /* Skiplist key */
    haddr_t     *addr = NULL;           /* Destination address */
    hbool_t     obj_inserted = FALSE;   /* Object inserted into skip list */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity checks */
    HDassert(attr);
    HDassert(udata);
    HDassert(udata->dst_dt_list);
    HDassert(H5F_addr_defined(udata->obj_oloc.addr));

    /* Get attribute datatype */
    if(NULL == (dt = H5A_type(attr)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't get attribute datatype")

    /* Check if the datatype is committed and search the skip list if so */
    if(H5T_committed(dt)) {
        /* Allocate key */
        if(NULL == (key = H5FL_MALLOC(H5O_copy_search_comm_dt_key_t)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

        /* Copy datatype into key */
        if(NULL == (key->dt = (H5T_t *)H5O_msg_copy(H5O_DTYPE_ID, dt, NULL)))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to copy datatype message")

        /* Get datatype object fileno */
        H5F_GET_FILENO(udata->obj_oloc.file, key->fileno);

        if(!H5SL_search(udata->dst_dt_list, key)) {
            /* Allocate destination address */
            if(NULL == (addr = H5FL_MALLOC(haddr_t)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

            /* Add the destination datatype to the skip list */
            *addr = ((H5O_shared_t *)(key->dt))->u.loc.oh_addr;
            if(H5SL_insert(udata->dst_dt_list, addr, key) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "can't insert object into skip list")
            obj_inserted = TRUE;
        } /* end if */
    } /* end if */

done:
    /* Release resources */
    if(!obj_inserted) {
        if(key) {
            if(key->dt)
                key->dt = (H5T_t *)H5O_msg_free(H5O_DTYPE_ID, key->dt);
            key = H5FL_FREE(H5O_copy_search_comm_dt_key_t, key);
        } /* end if */
        if(addr) {
            HDassert(ret_value < 0);
            addr = H5FL_FREE(haddr_t, addr);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_copy_search_comm_dt_attr_cb */


/*-------------------------------------------------------------------------
 * Function:    H5O_copy_search_comm_dt_check
 *
 * Purpose:     Check if the object at obj_oloc is or contains a reference
 *              to a committed datatype.  If it does, adds it to the merge
 *              committed dt skiplist present in udata if it does not match
 *              any already present.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Neil Fortner
 *              Nov 3 2011
 *
 * Modifications:
 *	Vailin Choi; August 2012
 *	Use H5O_obj_class to get object type instead of
 *	H5O_get_info(...TRUE....) saving time in traversing metadata.
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_copy_search_comm_dt_check(H5O_loc_t *obj_oloc,
    H5O_copy_search_comm_dt_ud_t *udata)
{
    H5O_copy_search_comm_dt_key_t *key = NULL; /* Skiplist key */
    haddr_t     *addr = NULL;           /* Destination address */
    hbool_t     obj_inserted = FALSE;   /* Object inserted into skip list */
    H5A_attr_iter_op_t attr_op;         /* Attribute iteration operator */
    const H5O_obj_class_t  *obj_class = NULL;       /* Type of object */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity checks */
    HDassert(obj_oloc);
    HDassert(udata);
    HDassert(udata->dst_dt_list);
    HDassert(udata->dst_root_loc);

    /* Get pointer to object class for this object */
    if((obj_class = H5O_obj_class(obj_oloc, udata->dxpl_id)) == NULL)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to determine object type")

    /* Check if the object is a datatype, a dataset using a committed
     * datatype, or contains an attribute using a committed datatype */
    if(obj_class->type == H5O_TYPE_NAMED_DATATYPE) {
        /* Allocate key */
        if(NULL == (key = H5FL_MALLOC(H5O_copy_search_comm_dt_key_t)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

        /* Read the destination datatype */
        if(NULL == (key->dt = (H5T_t *)H5O_msg_read(obj_oloc, H5O_DTYPE_ID, NULL, udata->dxpl_id)))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't read DTYPE message")

        /* Get destination object fileno */
        H5F_GET_FILENO(obj_oloc->file, key->fileno);

        /* Check if the datatype is already present in the skip list */
        if(!H5SL_search(udata->dst_dt_list, key)) {
            /* Allocate destination address */
            if(NULL == (addr = H5FL_MALLOC(haddr_t)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

            /* Add the destination datatype to the skip list */
            *addr = obj_oloc->addr;
            if(H5SL_insert(udata->dst_dt_list, addr, key) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "can't insert object into skip list")
            obj_inserted = TRUE;
        } /* end if */
    } /* end if */
    else if(obj_class->type == H5O_TYPE_DATASET) {
        /* Allocate key */
        if(NULL == (key = H5FL_MALLOC(H5O_copy_search_comm_dt_key_t)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

        /* Read the destination datatype */
        if(NULL == (key->dt = (H5T_t *)H5O_msg_read(obj_oloc, H5O_DTYPE_ID, NULL, udata->dxpl_id)))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't read DTYPE message")

        /* Check if the datatype is committed and search the skip list if so
            */
        if(H5T_committed(key->dt)) {
            /* Get datatype object fileno */
            H5F_GET_FILENO(obj_oloc->file, key->fileno);

            if(!H5SL_search(udata->dst_dt_list, key)) {
                /* Allocate destination address */
                if(NULL == (addr = H5FL_MALLOC(haddr_t)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

                /* Add the destination datatype to the skip list */
                *addr = ((H5O_shared_t *)(key->dt))->u.loc.oh_addr;
                if(H5SL_insert(udata->dst_dt_list, addr, key) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "can't insert object into skip list")
                obj_inserted = TRUE;
            } /* end if */
        } /* end if */
    } /* end else */

    /* Search within attributes */
    attr_op.op_type = H5A_ATTR_OP_LIB;
    attr_op.u.lib_op = H5O_copy_search_comm_dt_attr_cb;
    udata->obj_oloc.file = obj_oloc->file;
    udata->obj_oloc.addr = obj_oloc->addr;
    if(H5O_attr_iterate_real((hid_t)-1, obj_oloc, udata->dxpl_id, H5_INDEX_NAME, 
                             H5_ITER_NATIVE, (hsize_t)0, NULL, &attr_op, udata) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_BADITER, FAIL, "error iterating over attributes");

done:
    /* Release resources */
    if(!obj_inserted) {
        if(key) {
            if(key->dt)
                key->dt = (H5T_t *)H5O_msg_free(H5O_DTYPE_ID, key->dt);
            key = H5FL_FREE(H5O_copy_search_comm_dt_key_t, key);
        } /* end if */
        if(addr) {
            HDassert(ret_value < 0);
            addr = H5FL_FREE(haddr_t, addr);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_copy_search_comm_dt_check */


/*-------------------------------------------------------------------------
 * Function:    H5O_copy_search_comm_dt_cb
 *
 * Purpose:     H5G_visit callback to add committed datatypes to the merge
 *              committed dt skiplist.  Mostly a wrapper for
 *              H5O_copy_search_comm_dt_check.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Neil Fortner
 *              Oct 6 2011
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_copy_search_comm_dt_cb(hid_t H5_ATTR_UNUSED group, const char *name,
    const H5L_info_t *linfo, void *_udata)
{
    H5O_copy_search_comm_dt_ud_t *udata = (H5O_copy_search_comm_dt_ud_t *)_udata; /* Skip list of dtypes in dest file */
    H5G_loc_t   obj_loc;                /* Location of object */
    H5O_loc_t   obj_oloc;               /* Object's object location */
    H5G_name_t  obj_path;               /* Object's group hier. path */
    hbool_t     obj_found = FALSE;      /* Object at 'name' found */
    herr_t      ret_value = H5_ITER_CONT; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity checks */
    HDassert(name);
    HDassert(linfo);
    HDassert(udata);
    HDassert(udata->dst_dt_list);
    HDassert(udata->dst_root_loc);

    /* Check if this is a hard link */
    if(linfo->type == H5L_TYPE_HARD) {
        /* Set up opened group location to fill in */
        obj_loc.oloc = &obj_oloc;
        obj_loc.path = &obj_path;
        H5G_loc_reset(&obj_loc);

        /* Find the object */
        if(H5G_loc_find(udata->dst_root_loc, name, &obj_loc/*out*/, H5P_LINK_ACCESS_DEFAULT, udata->dxpl_id) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, H5_ITER_ERROR, "object not found")
        obj_found = TRUE;

        /* Check object and add to skip list if appropriate */
        if(H5O_copy_search_comm_dt_check(&obj_oloc, udata) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, H5_ITER_ERROR, "can't check object")
    } /* end if */

done:
    /* Release resources */
    if(obj_found && H5G_loc_free(&obj_loc) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTRELEASE, H5_ITER_ERROR, "can't free location")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_copy_search_comm_dt_cb */


/*-------------------------------------------------------------------------
 * Function:    H5O_copy_search_comm_dt
 *
 * Purpose:     Checks if the committed datatype present in oh_src matches any
 *              in the destination file, building the destination file
 *              skiplist as necessary.
 *
 * Return:      TRUE if a match is found in the destination file
 *                      - oloc_dst will contain the address
 *              FALSE if a match is not found
 *              Negative on failure
 *
 * Programmer:  Neil Fortner
 *              Sep 27 2011
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5O_copy_search_comm_dt(H5F_t *file_src, H5O_t *oh_src,
    H5O_loc_t *oloc_dst/*in, out*/, hid_t dxpl_id, H5O_copy_t *cpy_info)
{
    H5O_copy_search_comm_dt_key_t *key = NULL; /* Skiplist key */
    haddr_t     *dst_addr;      /* Destination datatype address */
    H5G_loc_t   dst_root_loc = {NULL, NULL}; /* Destination root group location */
    H5O_copy_search_comm_dt_ud_t udata; /* Group iteration user data */
    herr_t      ret_value = FALSE; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity checks */
    HDassert(oh_src);
    HDassert(oloc_dst);
    HDassert(oloc_dst->file);
    HDassert(H5F_FILE_ID(oloc_dst->file) >= 0);
    HDassert(cpy_info);

    /* Allocate key */
    if(NULL == (key = H5FL_MALLOC(H5O_copy_search_comm_dt_key_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Read the source datatype */
    if(NULL == (key->dt = (H5T_t *)H5O_msg_read_oh(file_src, dxpl_id, oh_src, H5O_DTYPE_ID, NULL)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't read DTYPE message")

    /* Get destination object fileno */
    H5F_GET_FILENO(oloc_dst->file, key->fileno);

    /* Check if the destination dtype list exists, create it if it does not */
    if(!cpy_info->dst_dt_list) {
        /* Create the skip list */
        if(NULL == (cpy_info->dst_dt_list = H5SL_create(H5SL_TYPE_GENERIC, H5O_copy_comm_dt_cmp)))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTCREATE, FAIL, "can't create skip list for committed datatypes")

        /* Add suggested types to list, if they are present */
        if(cpy_info->dst_dt_suggestion_list) {
            H5O_copy_dtype_merge_list_t *suggestion = cpy_info->dst_dt_suggestion_list;
            H5G_loc_t   obj_loc;                /* Location of object */
            H5O_loc_t   obj_oloc;               /* Object's object location */
            H5G_name_t  obj_path;               /* Object's group hier. path */

            /* Set up the root group in the destination file */
            if(NULL == (dst_root_loc.oloc = H5G_oloc(H5G_rootof(oloc_dst->file))))
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get object location for root group")
            if(NULL == (dst_root_loc.path = H5G_nameof(H5G_rootof(oloc_dst->file))))
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get path for root group")

            /* Set up opened group location to fill in */
            obj_loc.oloc = &obj_oloc;
            obj_loc.path = &obj_path;
            H5G_loc_reset(&obj_loc);

            /* Build udata */
            udata.dst_dt_list = cpy_info->dst_dt_list;
            udata.dst_root_loc = &dst_root_loc;
            udata.obj_oloc.file = NULL;
            udata.obj_oloc.addr = HADDR_UNDEF;
            udata.dxpl_id = dxpl_id;

            /* Walk through the list of datatype suggestions */
            while(suggestion) {
                /* Find the object */
                if(H5G_loc_find(&dst_root_loc, suggestion->path, &obj_loc/*out*/, H5P_LINK_ACCESS_DEFAULT, dxpl_id) < 0)
                    /* Ignore errors - i.e. suggestions not present in
                     * destination file */
                    H5E_clear_stack(NULL);
                else
                    /* Check object and add to skip list if appropriate */
                    if(H5O_copy_search_comm_dt_check(&obj_oloc, &udata) < 0) {
                        if(H5G_loc_free(&obj_loc) < 0)
                            HERROR(H5E_OHDR, H5E_CANTRELEASE, "can't free location");
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't check object")
                    } /* end if */

                /* Free location */
                if(H5G_loc_free(&obj_loc) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "can't free location");

                /* Advance the suggestion pointer */
                suggestion = suggestion->next;
            } /* end while */
        } /* end if */
    }

    if(!cpy_info->dst_dt_list_complete) {
        /* Search for the type in the destination file, and return its address
         * if found, but only if the list is populated with and only with
         * suggested types.  We will search complete lists later. */
        if(cpy_info->dst_dt_suggestion_list
                && NULL != (dst_addr = (haddr_t *)H5SL_search(
                cpy_info->dst_dt_list, key))) {
            oloc_dst->addr = *dst_addr;
            ret_value = TRUE;
        } /* end if */
        else {
            H5O_mcdt_search_ret_t search_cb_ret = H5O_MCDT_SEARCH_CONT;

            /* Make callback to see if we should search destination file */
            if(cpy_info->mcdt_cb)
                if((search_cb_ret = cpy_info->mcdt_cb(cpy_info->mcdt_ud)) == H5O_MCDT_SEARCH_ERROR)
                    HGOTO_ERROR(H5E_OHDR, H5E_CALLBACK, FAIL, "callback returned error")

            if(search_cb_ret == H5O_MCDT_SEARCH_CONT) {
                /* Build the complete dst dt list */
                /* Set up the root group in the destination file, if necessary */
                if(!dst_root_loc.oloc) {
                    HDassert(!dst_root_loc.path);
                    if(NULL == (dst_root_loc.oloc = H5G_oloc(H5G_rootof(oloc_dst->file))))
                        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get object location for root group")
                    if(NULL == (dst_root_loc.path = H5G_nameof(H5G_rootof(oloc_dst->file))))
                        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get path for root group")
                } /* end if */
                else
                    HDassert(dst_root_loc.path);

                /* Build udata.  Note that this may be done twice in some cases, but
                * it should be rare and should be cheaper on average than trying to
                * keep track of whether it was done before. */
                udata.dst_dt_list = cpy_info->dst_dt_list;
                udata.dst_root_loc = &dst_root_loc;
                udata.obj_oloc.file = NULL;
                udata.obj_oloc.addr = HADDR_UNDEF;
                udata.dxpl_id = dxpl_id;

                /* Traverse the destination file, adding committed datatypes to the skip
                * list */
                if(H5G_visit(H5F_FILE_ID(oloc_dst->file), "/", H5_INDEX_NAME, H5_ITER_NATIVE, H5O_copy_search_comm_dt_cb, &udata, H5P_LINK_ACCESS_DEFAULT, dxpl_id) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_BADITER, FAIL, "object visitation failed")
                cpy_info->dst_dt_list_complete = TRUE;
            } /* end if */
            else
                if(search_cb_ret != H5O_MCDT_SEARCH_STOP)
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unknown return value for callback")
        } /* end if */
    } /* end if */

    /* Search for the type in the destination file, and return its address if
     * found, but only if the list is complete */
    if(cpy_info->dst_dt_list_complete) {
        if(NULL != (dst_addr = (haddr_t *)H5SL_search(cpy_info->dst_dt_list, key))) {
            oloc_dst->addr = *dst_addr;
            ret_value = TRUE;
        } /* end if */
    } /* end if */

done:
    if(key) {
        if(key->dt)
            key->dt = (H5T_t *)H5O_msg_free(H5O_DTYPE_ID, key->dt);
        key = H5FL_FREE(H5O_copy_search_comm_dt_key_t, key);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_copy_search_comm_dt */


/*-------------------------------------------------------------------------
 * Function:    H5O_copy_insert_comm_dt
 *
 * Purpose:     Insert the committed datatype at oloc_dst into the merge committed
 *              dt skiplist.  The datatype must not be present already.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Neil Fortner
 *              Oct 6 2011
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_copy_insert_comm_dt(H5F_t *file_src, H5O_t *oh_src, H5O_loc_t *oloc_dst,
    hid_t dxpl_id, H5O_copy_t *cpy_info)
{
    H5O_copy_search_comm_dt_key_t *key = NULL; /* Skiplist key */
    haddr_t     *addr = NULL;   /* Destination object address */
    herr_t      ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity checks */
    HDassert(oh_src);
    HDassert(oloc_dst);
    HDassert(oloc_dst->file);
    HDassert(oloc_dst->addr != HADDR_UNDEF);
    HDassert(cpy_info);
    HDassert(cpy_info->dst_dt_list);

    /* Allocate key */
    if(NULL == (key = H5FL_MALLOC(H5O_copy_search_comm_dt_key_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Read the datatype.  Read from the source file because the destination
     * object could be changed in the post-copy. */
    if(NULL == (key->dt = (H5T_t *)H5O_msg_read_oh(file_src, dxpl_id, oh_src, H5O_DTYPE_ID, NULL)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't read DTYPE message")

    /* Get destination object fileno */
    H5F_GET_FILENO(oloc_dst->file, key->fileno);

    /* Allocate destination address */
    if(NULL == (addr = H5FL_MALLOC(haddr_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Add the destination datatype to the skip list */
    *addr = oloc_dst->addr;
    if(H5SL_insert(cpy_info->dst_dt_list, addr, key) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "can't insert object into skip list")

done:
    if(ret_value < 0) {
        if(key) {
            if(key->dt)
                key->dt = (H5T_t *)H5O_msg_free(H5O_DTYPE_ID, key->dt);
            key = H5FL_FREE(H5O_copy_search_comm_dt_key_t, key);
        } /* end if */
        if(addr)
            addr = H5FL_FREE(haddr_t, addr);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_copy_insert_comm_dt */

