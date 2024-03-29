#include <complex.h>
#include <pfft.h>

int main(int argc, char **argv)
{
  int np[2];
  ptrdiff_t n[4], N[4];
  ptrdiff_t alloc_local;
  ptrdiff_t local_ni[4], local_i_start[4];
  ptrdiff_t local_no[4], local_o_start[4];
  double err, *in, *out;
  pfft_plan plan_forw=NULL, plan_back=NULL;
  MPI_Comm comm_cart_2d;
  pfft_r2r_kind kinds_forw[4], kinds_back[4];
  
  /* Set size of FFT and process mesh */
  n[0] = 13; n[1] = 14; n[2] = 19; n[3] = 17;
  np[0] = 2; np[1] = 2;
  
  /* Set FFTW kinds of 1d R2R trafos */
  kinds_forw[0] = PFFT_REDFT00; kinds_back[0] = PFFT_REDFT00;
  kinds_forw[1] = PFFT_REDFT01; kinds_back[1] = PFFT_REDFT10;
  kinds_forw[2] = PFFT_RODFT00; kinds_back[2] = PFFT_RODFT00;
  kinds_forw[3] = PFFT_RODFT10; kinds_back[3] = PFFT_RODFT01;

  /* Set logical DFT sizes corresponding to FFTW manual:
   * for REDFT00 N=2*(n-1), for RODFT00 N=2*(n+1), otherwise N=2*n */
  N[0] = 2*(n[0]-1);
  N[1] = 2*n[1];
  N[2] = 2*(n[2]+1); 
  N[3] = 2*n[3];

  /* Initialize MPI and PFFT */
  MPI_Init(&argc, &argv);
  pfft_init();

  /* Create two-dimensional process grid of size np[0] x np[1], if possible */
  if( pfft_create_procmesh_2d(MPI_COMM_WORLD, np[0], np[1], &comm_cart_2d) ){
    pfft_fprintf(MPI_COMM_WORLD, stderr, "Error: This test file only works with %d processes.\n", np[0]*np[1]);
    MPI_Finalize();
    return 1;
  }
  
  /* Get parameters of data distribution */
  alloc_local = pfft_local_size_r2r(4, n, comm_cart_2d, PFFT_TRANSPOSED_OUT,
      local_ni, local_i_start, local_no, local_o_start);

  /* Allocate memory */
  in  = pfft_alloc_real(alloc_local);
  out = pfft_alloc_real(alloc_local);

  /* Plan parallel forward FFT */
  plan_forw = pfft_plan_r2r(
      4, n, in, out, comm_cart_2d, kinds_forw, PFFT_TRANSPOSED_OUT| PFFT_MEASURE| PFFT_DESTROY_INPUT);
  
  /* Plan parallel backward FFT */
  plan_back = pfft_plan_r2r(
      4, n, out, in, comm_cart_2d, kinds_back, PFFT_TRANSPOSED_IN| PFFT_MEASURE| PFFT_DESTROY_INPUT);

  /* Initialize input with random numbers */
  pfft_init_input_r2r(4, n, local_ni, local_i_start,
      in);

  /* execute parallel forward FFT */
  pfft_execute(plan_forw);
  
  /* execute parallel backward FFT */
  pfft_execute(plan_back);
  
  /* Scale data */
  for(ptrdiff_t l=0; l < local_ni[0] * local_ni[1] * local_ni[2] * local_ni[3]; l++)
    in[l] /= (N[0]*N[1]*N[2]*N[3]);
  
  /* Print error of back transformed data */
  MPI_Barrier(MPI_COMM_WORLD);
  err = pfft_check_output_r2r(4, n, local_ni, local_i_start, in, comm_cart_2d);
  pfft_printf(comm_cart_2d, "Error after one forward and backward trafo of size n=(%td, %td, %td, %td):\n", n[0], n[1], n[2], n[3]); 
  pfft_printf(comm_cart_2d, "maxerror = %6.2e;\n", err);
  
  /* free mem and finalize */
  pfft_destroy_plan(plan_forw);
  pfft_destroy_plan(plan_back);
  MPI_Comm_free(&comm_cart_2d);
  pfft_free(in); pfft_free(out);
  MPI_Finalize();
  return 0;
}
