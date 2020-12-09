FLASH4 - Compilation steps

Install/Copy TecioLib from existing machine to the new ( Makefile.h requires an additional flag for intel compilers: -lstdc++)

Install hdf5

	~~~
	./configure --enable-fortran --enable-parallel --prefix=/your/path/
        ./configure --enable-fortran --enable-parallel CC=mpicc CXX=mpicxx FF=mpif90 F90=mpif90 F77=mpif90 gfortran=mpif90 FC=mpif90 --prefix=/your/path
	make -j 12
	make install
	~~~	

Install hypre
        ~~~
        ./configure --enable-fortran --prefix=/your/path
        ./configure --enable-fortran --prefix=/your/path CC=mpicc CXX=mpicxx FF=mpif90 F77=mpif90

        make -j 12
        make install
        ~~~

Create new Makefile.h under FLASH4/sites
