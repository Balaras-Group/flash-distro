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

/*
 * Small program to illustrate the "misalignment" of members within a compound
 * datatype, in a datatype fixed by H5Tget_native_type().
 */

#include "hdf5.h"
#include "H5private.h"
#include "h5tools.h"

const char *fname = "talign.h5";
const char *setname = "align";

/*
 * This program assumes that there is no extra space between the members 'Ok'
 * and 'Not Ok', (there shouldn't be because they are of the same atomic type
 * H5T_NATIVE_FLOAT, and they are placed within the compound next to one
 * another per construction)
 */

int main(void)
{
    hid_t fil=-1, spc=-1, set=-1;
    hid_t cs6=-1, cmp=-1, fix=-1;
    hid_t cmp1=-1, cmp2=-1, cmp3=-1;
    hid_t plist=-1;
    hid_t array_dt=-1;

    hsize_t dim[2];
    hsize_t cdim[4];

    char string5[5];
    float fok[2] = {1234.0f, 2341.0f};
    float fnok[2] = {5678.0f, 6785.0f};
    float *fptr = NULL;

    char *data = NULL;

    int result = 0;
    herr_t error = 1;

    printf("%-70s", "Testing alignment in compound datatypes");

    strcpy(string5, "Hi!");
    HDunlink(fname);
    fil = H5Fcreate(fname, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    if (fil < 0) {
        puts("*FAILED*");
        return 1;
    }

    H5E_BEGIN_TRY {
        (void)H5Ldelete(fil, setname, H5P_DEFAULT);
    } H5E_END_TRY;

    cs6 = H5Tcopy(H5T_C_S1);
    H5Tset_size(cs6, sizeof(string5));
    H5Tset_strpad(cs6, H5T_STR_NULLPAD);

    cmp = H5Tcreate(H5T_COMPOUND, sizeof(fok) + sizeof(string5) + sizeof(fnok));
    H5Tinsert(cmp, "Awkward length", 0, cs6);

    cdim[0] = sizeof(fok) / sizeof(float);
    array_dt = H5Tarray_create2(H5T_NATIVE_FLOAT, 1, cdim);
    H5Tinsert(cmp, "Ok", sizeof(string5), array_dt);
    H5Tclose(array_dt);

    cdim[0] = sizeof(fnok) / sizeof(float);
    array_dt = H5Tarray_create2(H5T_NATIVE_FLOAT, 1, cdim);
    H5Tinsert(cmp, "Not Ok", sizeof(fok) + sizeof(string5), array_dt);
    H5Tclose(array_dt);

    fix = H5Tget_native_type(cmp, H5T_DIR_DEFAULT);
    cmp1 = H5Tcreate(H5T_COMPOUND, sizeof(fok));

    cdim[0] = sizeof(fok) / sizeof(float);
    array_dt = H5Tarray_create2(H5T_NATIVE_FLOAT, 1, cdim);
    H5Tinsert(cmp1, "Ok", 0, array_dt);
    H5Tclose(array_dt);

    cmp2 = H5Tcreate(H5T_COMPOUND, sizeof(string5));
    H5Tinsert(cmp2, "Awkward length", 0, cs6);

    cmp3 = H5Tcreate(H5T_COMPOUND, sizeof(fnok));

    cdim[0] = sizeof(fnok) / sizeof(float);
    array_dt = H5Tarray_create2(H5T_NATIVE_FLOAT, 1, cdim);
    H5Tinsert(cmp3, "Not Ok", 0, array_dt);
    H5Tclose(array_dt);

    plist = H5Pcreate(H5P_DATASET_XFER);
    if((error = H5Pset_preserve(plist, 1)) < 0)
        goto out;

    /*
     * Create a small dataset, and write data into it we write each field
     * in turn so that we are avoid alignment issues at this point
     */
    dim[0] = 1;
    spc = H5Screate_simple(1, dim, NULL);
    set = H5Dcreate2(fil, setname, cmp, spc, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    H5Dwrite(set, cmp1, spc, H5S_ALL, plist, fok);
    H5Dwrite(set, cmp2, spc, H5S_ALL, plist, string5);
    H5Dwrite(set, cmp3, spc, H5S_ALL, plist, fnok);

    H5Dclose(set);

    /* Now open the set, and read it back in */
    data = (char *)HDmalloc(H5Tget_size(fix));

    if(!data) {
        perror("malloc() failed");
        abort();
    }

    set = H5Dopen2(fil, setname, H5P_DEFAULT);

    H5Dread(set, fix, spc, H5S_ALL, H5P_DEFAULT, data);
    fptr = (float *)(data + H5Tget_member_offset(fix, 1));
    H5Dclose(set);

out:
    if(error < 0) {
        result = 1;
        puts("*FAILED - HDF5 library error*");
    } else if(fok[0] != fptr[0] || fok[1] != fptr[1]
                    || fnok[0] != fptr[2] || fnok[1] != fptr[3]) {
        char *mname;

        result = 1;
        mname = H5Tget_member_name(fix, 0);
        printf("%14s (%2d) %6s = %s\n",
            mname ? mname : "(null)", (int)H5Tget_member_offset(fix,0),
            string5, (char *)(data + H5Tget_member_offset(fix, 0)));
        if(mname)
            H5free_memory(mname);

        fptr = (float *)(data + H5Tget_member_offset(fix, 1));
        mname = H5Tget_member_name(fix, 1);
        printf("Data comparison:\n"
            "%14s (%2d) %6f = %f\n"
            "                    %6f = %f\n",
            mname ? mname : "(null)", (int)H5Tget_member_offset(fix,1),
            (double)fok[0], (double)fptr[0],
            (double)fok[1], (double)fptr[1]);
        if(mname)
            H5free_memory(mname);

        fptr = (float *)(data + H5Tget_member_offset(fix, 2));
        mname = H5Tget_member_name(fix, 2);
        printf("%14s (%2d) %6f = %f\n"
            "                    %6f = %6f\n",
            mname ? mname : "(null)", (int)H5Tget_member_offset(fix,2),
            (double)fnok[0], (double)fptr[0],
            (double)fnok[1], (double)fptr[1]);
        if(mname)
            H5free_memory(mname);

        fptr = (float *)(data + H5Tget_member_offset(fix, 1));
        printf("\n"
            "Short circuit\n"
            "                    %6f = %f\n"
            "                    %6f = %f\n"
            "                    %6f = %f\n"
            "                    %6f = %f\n",
            (double)fok[0], (double)fptr[0],
            (double)fok[1], (double)fptr[1],
            (double)fnok[0], (double)fptr[2],
            (double)fnok[1], (double)fptr[3]);
        puts("*FAILED - compound type alignmnent problem*");
    } else {
        puts(" PASSED");
    }

    if(data)
        HDfree(data);
    H5Sclose(spc);
    H5Tclose(cs6);
    H5Tclose(cmp);
    H5Tclose(fix);
    H5Tclose(cmp1);
    H5Tclose(cmp2);
    H5Tclose(cmp3);
    H5Pclose(plist);
    H5Fclose(fil);
    HDunlink(fname);
    fflush(stdout);
    return result;
}

