
      program test

      implicit none

      include "mpif.h"
#include "fftw3.f"
      include "pfft.f"
      
      integer np(3), myrank, ierror, comm_cart_2d
      integer(ptrdiff_t_kind) :: l, m
      integer(ptrdiff_t_kind) :: n(3)
      integer(ptrdiff_t_kind) :: alloc_local
      integer(ptrdiff_t_kind) :: local_ni(3), local_i_start(3)
      integer(ptrdiff_t_kind) :: local_no(3), local_o_start(3)
      integer(8) fftplan_forw, fftplan_back
      complex(8), allocatable ::  data_in(:)
      complex(8), allocatable ::  data_out(:)
      
      n = (/ 2,2,4 /)
      np = (/ 2,2,1 /)
      
!     Initialize MPI and PFFT
      call MPI_Init(ierror)
      call dpfft_init();
      
      call MPI_Comm_rank(MPI_COMM_WORLD, myrank, ierror)
      
!     Create two-dimensional process grid of
!     size np(1) x np(2), if possible
      call dpfft_create_procmesh_2d(ierror, MPI_COMM_WORLD, &
           & np(1), np(2), comm_cart_2d)
      if (ierror .ne. 0) then
        if(myrank .eq. 0) then
          write(*,*) "Error: This test file only works with 4 processes"
        endif
        call MPI_Finalize(ierror)
        call exit(1)
      endif
      
!     Get parameters of data distribution
      call dpfft_local_size_dft_3d( &
     &     alloc_local, n, comm_cart_2d, PFFT_TRANSPOSED_OUT, &
     &     local_ni, local_i_start, local_no, local_o_start);
      
!     Allocate memory
      allocate(data_in(alloc_local))
      allocate(data_out(alloc_local))
!       data_in  = reshape(data_in,  (/ local_ni(1), local_ni(2), local_ni(3) /))
!       data_out = reshape(data_out, (/ local_no(1), local_no(2), local_no(3) /))
  
!     Plan parallel forward FFT
      call dpfft_plan_dft_3d(fftplan_forw, n, data_in, data_out, comm_cart_2d, &
     &     PFFT_FORWARD, PFFT_TRANSPOSED_OUT + PFFT_MEASURE + PFFT_DESTROY_INPUT)
      
!     Plan parallel backward FFT
      call dpfft_plan_dft_3d(fftplan_back, n, data_out, data_in, comm_cart_2d, &
     &     PFFT_BACKWARD, PFFT_TRANSPOSED_IN + PFFT_MEASURE + PFFT_DESTROY_INPUT)

!     Initialize input with random numbers
      call dpfft_init_input_c2c_3d(n, local_ni, local_i_start, &
     &     data_in)

!     Print input data
      call dpfft_apr_complex_3d(data_in, local_ni, local_i_start, &
     &     "PFFT, g_hat"//CHAR(0), MPI_COMM_WORLD)

!     execute parallel forward FFT
      call dpfft_execute(fftplan_forw)

!     Print transformed data 
      call dpfft_apr_complex_permuted_3d( &
     &     data_out, local_no, local_o_start, 1, 2, 3, &
     &     "PFFT, g"//CHAR(0), MPI_COMM_WORLD)
      
!     execute parallel backward FFT
      call dpfft_execute(fftplan_back)
  
!     Scale data
      m=1
      do l=1,local_ni(1)*local_ni(2)*local_ni(3)
        data_in(l) = data_in(l) / (n(1)*n(2)*n(3))
        m = m+1
      enddo
      
!     Print back transformed data
      call dpfft_apr_complex_3d(data_in, local_ni, local_i_start, &
     &     "PFFT^H, g_hat"//CHAR(0), MPI_COMM_WORLD)
      
!     free mem and finalize
      call dpfft_destroy_plan(fftplan_forw)
      call dpfft_destroy_plan(fftplan_back)
      call MPI_Comm_free(comm_cart_2d, ierror)
      deallocate(data_out)
      deallocate(data_in)
  
!     Finalize MPI
      call MPI_Finalize(ierror)
      end

