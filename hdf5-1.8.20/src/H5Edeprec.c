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
 * Created:	H5Edeprec.c
 *		April 11 2007
 *		Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:	Deprecated functions from the H5E interface.  These
 *              functions are here for compatibility purposes and may be
 *              removed in the future.  Applications should switch to the
 *              newer APIs.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5E_PACKAGE		/*suppress error about including H5Epkg   */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5E__init_deprec_interface


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Iprivate.h"		/* IDs                                  */
#include "H5Epkg.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free lists                           */
#include "H5MMprivate.h"	/* Memory management			*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*--------------------------------------------------------------------------
NAME
   H5E__init_deprec_interface -- Initialize interface-specific information
USAGE
    herr_t H5E__init_deprec_interface()
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5E_init() currently).

--------------------------------------------------------------------------*/
static herr_t
H5E__init_deprec_interface(void)
{
    FUNC_ENTER_STATIC_NOERR

    FUNC_LEAVE_NOAPI(H5E_init())
} /* H5E__init_deprec_interface() */


/*--------------------------------------------------------------------------
NAME
   H5E__term_deprec_interface -- Terminate interface
USAGE
    herr_t H5E__term_deprec_interface()
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Terminates interface.  (Just resets H5_interface_initialize_g
    currently).

--------------------------------------------------------------------------*/
herr_t
H5E__term_deprec_interface(void)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Mark closed */
    H5_interface_initialize_g = 0;

    FUNC_LEAVE_NOAPI(0)
} /* H5E__term_deprec_interface() */

#ifndef H5_NO_DEPRECATED_SYMBOLS

/*-------------------------------------------------------------------------
 * Function:	H5Eget_major
 *
 * Purpose:	Retrieves a major error message.
 *
 * Return:      Returns message if succeeds.
 *              otherwise returns NULL.
 *
 * Programmer:	Raymond Lu
 *              Friday, July 14, 2003
 *
 *-------------------------------------------------------------------------
 */
char *
H5Eget_major(H5E_major_t maj)
{
    H5E_msg_t   *msg;           /* Pointer to error message */
    ssize_t      size;
    H5E_type_t  type;
    char        *msg_str = NULL;
    char        *ret_value;     /* Return value */

    FUNC_ENTER_API_NOCLEAR(NULL)
    H5TRACE1("*s", "i", maj);

    /* Get the message object */
    if(NULL == (msg = (H5E_msg_t *)H5I_object_verify(maj, H5I_ERROR_MSG)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a error message ID")

    /* Get the size & type of the message's text */
    if((size = H5E_get_msg(msg, &type, NULL, (size_t)0)) < 0)
	HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, NULL, "can't get error message text")
    if(type != H5E_MAJOR)
	HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, NULL, "Error message isn't a major one")

    /* Application will free this */
    size++;
    msg_str = (char *)H5MM_malloc((size_t)size);

    /* Get the text for the message */
    if(H5E_get_msg(msg, NULL, msg_str, (size_t)size) < 0)
	HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, NULL, "can't get error message text")

    ret_value = msg_str;

done:
    if(!ret_value)
        msg_str = (char *)H5MM_xfree(msg_str);

    FUNC_LEAVE_API(ret_value)
} /* end H5Eget_major() */


/*-------------------------------------------------------------------------
 * Function:	H5Eget_minor
 *
 * Purpose:	Retrieves a minor error message.
 *
 * Return:      Returns message if succeeds.
 *              otherwise returns NULL.
 *
 * Programmer:	Raymond Lu
 *              Friday, July 14, 2003
 *
 *-------------------------------------------------------------------------
 */
char *
H5Eget_minor(H5E_minor_t min)
{
    H5E_msg_t   *msg;           /* Pointer to error message */
    ssize_t      size;
    H5E_type_t  type;
    char        *msg_str = NULL;
    char        *ret_value;     /* Return value */

    FUNC_ENTER_API_NOCLEAR(NULL)
    H5TRACE1("*s", "i", min);

    /* Get the message object */
    if(NULL == (msg = (H5E_msg_t *)H5I_object_verify(min, H5I_ERROR_MSG)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a error message ID")

    /* Get the size & type of the message's text */
    if((size = H5E_get_msg(msg, &type, NULL, (size_t)0)) < 0)
	HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, NULL, "can't get error message text")
    if(type != H5E_MINOR)
	HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, NULL, "Error message isn't a minor one")

    /* Application will free this */
    size++;
    msg_str = (char *)H5MM_malloc((size_t)size);

    /* Get the text for the message */
    if(H5E_get_msg(msg, NULL, msg_str, (size_t)size) < 0)
	HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, NULL, "can't get error message text")

    ret_value = msg_str;

done:
    if(!ret_value)
        msg_str = (char *)H5MM_xfree(msg_str);

    FUNC_LEAVE_API(ret_value)
} /* end H5Eget_minor() */


/*-------------------------------------------------------------------------
 * Function:	H5Epush1
 *
 * Purpose:	This function definition is for backward compatibility only.
 *              It doesn't have error stack and error class as parameters.
 *              The old definition of major and minor is casted as HID_T
 *              in H5Epublic.h
 *
 * Notes: 	Basically a public API wrapper around the H5E_push2
 *              function.  For backward compatibility, it maintains the
 *              same parameter as the old function, in contrary to
 *              H5Epush2.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *		Tuesday, Sep 16, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Epush1(const char *file, const char *func, unsigned line,
        H5E_major_t maj, H5E_minor_t min, const char *str)
{
    herr_t	ret_value = SUCCEED;    /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(FAIL)
    H5TRACE6("e", "*s*sIuii*s", file, func, line, maj, min, str);

    /* Push the error on the default error stack */
    if(H5E_push_stack(NULL, file, func, line, H5E_ERR_CLS_g, maj, min, str) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTSET, FAIL, "can't push error on stack")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Epush1() */


/*-------------------------------------------------------------------------
 * Function:	H5Eclear1
 *
 * Purpose:	This function is for backward compatbility.
 *              Clears the error stack for the specified error stack.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              Wednesday, July 16, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eclear1(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(FAIL)
    H5TRACE0("e","");

    /* Clear the default error stack */
    if(H5E_clear_stack(NULL) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTSET, FAIL, "can't clear error stack")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eclear1() */


/*-------------------------------------------------------------------------
 * Function:	H5Eprint1
 *
 * Purpose:	This function is for backward compatbility.
 *              Prints the error stack in some default way.  This is just a
 *		convenience function for H5Ewalk() with a function that
 *		prints error messages.  Users are encouraged to write there
 *		own more specific error handlers.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              Sep 16, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eprint1(FILE *stream)
{
    H5E_t   *estack;            /* Error stack to operate on */
    herr_t ret_value = SUCCEED; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(FAIL)
    /*NO TRACE*/

    if(NULL == (estack = H5E_get_my_stack())) /*lint !e506 !e774 Make lint 'constant value Boolean' in non-threaded case */
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack")

    /* Print error stack */
    if(H5E_print(estack, stream, TRUE) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTLIST, FAIL, "can't display error stack")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eprint1() */


/*-------------------------------------------------------------------------
 * Function:	H5Ewalk1
 *
 * Purpose:	This function is for backward compatbility.
 *              Walks the error stack for the current thread and calls some
 *		function for each error along the way.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              Sep 16, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ewalk1(H5E_direction_t direction, H5E_walk1_t func, void *client_data)
{
    H5E_t   *estack;            /* Error stack to operate on */
    H5E_walk_op_t walk_op;      /* Error stack walking callback */
    herr_t ret_value = SUCCEED; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(FAIL)
    /*NO TRACE*/

    if(NULL == (estack = H5E_get_my_stack())) /*lint !e506 !e774 Make lint 'constant value Boolean' in non-threaded case */
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack")

    /* Walk the error stack */
    walk_op.vers = 1;
    walk_op.u.func1 = func;
    if(H5E_walk(estack, direction, &walk_op, client_data) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTLIST, FAIL, "can't walk error stack")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ewalk1() */


/*-------------------------------------------------------------------------
 * Function:	H5Eget_auto1
 *
 * Purpose:	This function is for backward compatbility.
 *              Returns the current settings for the automatic error stack
 *		traversal function and its data for specific error stack.
 *		Either (or both) arguments may be null in which case the
 *		value is not returned.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              Sep 16, 2003
 *
 * Modification:Raymond Lu
 *              4 October 2010
 *              If the printing function isn't the default H5Eprint1 or 2, 
 *              and H5Eset_auto2 has been called to set the new style 
 *              printing function, a call to H5Eget_auto1 should fail.
 *-------------------------------------------------------------------------
 */
herr_t
H5Eget_auto1(H5E_auto1_t *func, void **client_data)
{
    H5E_t   *estack;            /* Error stack to operate on */
    H5E_auto_op_t auto_op;      /* Error stack operator */
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "*x**x", func, client_data);

    /* Retrieve default error stack */
    if(NULL == (estack = H5E_get_my_stack())) /*lint !e506 !e774 Make lint 'constant value Boolean' in non-threaded case */
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack")

    /* Get the automatic error reporting information */
    if(H5E_get_auto(estack, &auto_op, client_data) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get automatic error info")

    /* Fail if the printing function isn't the default(user-set) and set through H5Eset_auto2 */
    if(!auto_op.is_default && auto_op.vers == 2)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "wrong API function, H5Eset_auto2 has been called")

    if(func)
        *func = auto_op.func1;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eget_auto1() */


/*-------------------------------------------------------------------------
 * Function:	H5Eset_auto1
 *
 * Purpose:	This function is for backward compatbility.
 *              Turns on or off automatic printing of errors for certain
 *              error stack.  When turned on (non-null FUNC pointer) any
 *              API function which returns an error indication will first
 *              call FUNC passing it CLIENT_DATA as an argument.
 *
 *		The default values before this function is called are
 *		H5Eprint1() with client data being the standard error stream,
 *		stderr.
 *
 *		Automatic stack traversal is always in the H5E_WALK_DOWNWARD
 *		direction.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              Sep 16, 2003
 *
 * Modification:Raymond Lu
 *              4 October 2010
 *              If the FUNC is H5Eprint2, put the IS_DEFAULT flag on.
 *-------------------------------------------------------------------------
 */
herr_t
H5Eset_auto1(H5E_auto1_t func, void *client_data)
{
    H5E_t   *estack;            /* Error stack to operate on */
    H5E_auto_op_t auto_op;      /* Error stack operator */
    herr_t ret_value = SUCCEED; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(FAIL)
    H5TRACE2("e", "x*x", func, client_data);

    if(NULL == (estack = H5E_get_my_stack())) /*lint !e506 !e774 Make lint 'constant value Boolean' in non-threaded case */
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack")

    /* Get the automatic error reporting information */
    if(H5E_get_auto(estack, &auto_op, NULL) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get automatic error info")

    /* Set the automatic error reporting information */
    auto_op.vers = 1;
    if(func != auto_op.func1_default)
        auto_op.is_default = FALSE;
    else
        auto_op.is_default = TRUE;
    auto_op.func1 = func;

    if(H5E_set_auto(estack, &auto_op, client_data) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTSET, FAIL, "can't set automatic error info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eset_auto1() */
#endif /* H5_NO_DEPRECATED_SYMBOLS */

