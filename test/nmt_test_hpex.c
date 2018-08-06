#include "namaster.h"
#include "ctest.h"
#include "utils.h"
#include "nmt_test_utils.h"
#include <chealpix.h>

CTEST(nmt,he_io) {

  set_error_policy(THROW_ON_ERROR);

  printf("\nError messages expected: \n");

  int ii;
  long nside=4;
  long npix=he_nside2npix(nside);
  ASSERT_EQUAL(npix,12*nside*nside);

  double **mps0=my_malloc(1*sizeof(double *));
  double **mps2=my_malloc(1*sizeof(double *));
  for(ii=0;ii<1;ii++)
    mps0[ii]=my_calloc(npix,sizeof(double));
  for(ii=0;ii<2;ii++)
    mps2[ii]=my_calloc(npix,sizeof(double));

  //Writing
  try {he_write_healpix_map(mps0,1,nside,"!mps.fits");}
  catch(1) {}
  ASSERT_EQUAL(0,exception_status);
  try {he_write_healpix_map(mps2,1,nside,"!mps.fits");}
  catch(1) {}
  ASSERT_EQUAL(0,exception_status);
  try {he_write_healpix_map(mps2,2,nside,"!mps.fits");}
  catch(1) {}
  ASSERT_EQUAL(0,exception_status);

  for(ii=0;ii<1;ii++)
    free(mps0[ii]);
  for(ii=0;ii<2;ii++)
    free(mps2[ii]);
  
  //Reading
  long ns2;
  int nfd,isnest;
  //Correct file params
  he_get_file_params("mps.fits",&ns2,&nfd,&isnest);
  ASSERT_EQUAL(nside,ns2);
  ASSERT_EQUAL(2,nfd);
  ASSERT_EQUAL(0,isnest);
  //Read map
  mps0[0]=he_read_healpix_map("mps.fits",&ns2,0);
  ASSERT_EQUAL(nside,ns2);
  mps2[0]=he_read_healpix_map("mps.fits",&ns2,0);
  ASSERT_EQUAL(nside,ns2);
  //Go one column too far
  try {mps2[1]=he_read_healpix_map("mps.fits",&ns2,2);}
  catch(1) {}
  ASSERT_EQUAL(1,exception_status);
  mps2[1]=he_read_healpix_map("mps.fits",&ns2,1);
  ASSERT_EQUAL(nside,ns2);
  
  for(ii=0;ii<1;ii++)
    free(mps0[ii]);
  for(ii=0;ii<2;ii++)
    free(mps2[ii]);
  free(mps0); free(mps2);

  set_error_policy(EXIT_ON_ERROR);
}

CTEST_SKIP(nmt,he_qdisc) {
  int ii;
  long nside=256;
  long ntot_alloc=20000;
  int *indices=my_malloc(ntot_alloc*sizeof(int));
  int ntot_alloc_d=ntot_alloc/2;
  he_query_disc(nside,1.,0.,M_PI/180,indices,&ntot_alloc_d,1);
  ASSERT_EQUAL(84,ntot_alloc_d); ntot_alloc_d=ntot_alloc/2;
  he_query_disc(nside,-1.,0.,M_PI/180,indices,&ntot_alloc_d,1);
  ASSERT_EQUAL(84,ntot_alloc_d); ntot_alloc_d=ntot_alloc/2;
  /*
    This doesn't seem to pass - it's different than the healpy implementation
  */
  he_query_disc(nside,0.,0.,M_PI/180,indices,&ntot_alloc_d,1);
  ASSERT_EQUAL(88,ntot_alloc_d); ntot_alloc_d=ntot_alloc/2;
  free(indices);
}

CTEST(nmt,he_qstrip) {
  long nside=1024;

  set_error_policy(THROW_ON_ERROR);

  printf("\nError messages expected: \n");
    
  //queries
  long ntot_alloc=20000;
  int *indices=my_malloc(ntot_alloc*sizeof(int));
  //query strip around equator
  he_query_strip(nside,M_PI/2-M_PI/8192,M_PI/2+M_PI/8192,indices,&ntot_alloc);
  ASSERT_EQUAL(12288,ntot_alloc); ntot_alloc=20000;
  //query strip around pole
  he_query_strip(nside,M_PI-M_PI/1024,M_PI,indices,&ntot_alloc);
  ASSERT_EQUAL(40,ntot_alloc); ntot_alloc=20000;
  //query strip exceptions
  try { he_query_strip(nside,-M_PI,M_PI,indices,&ntot_alloc); }
  catch(1) {}
  ASSERT_EQUAL(1,exception_status);
  try { he_query_strip(nside,0,M_PI/2,indices,&ntot_alloc); }
  catch(1) {}
  ASSERT_EQUAL(1,exception_status);

  free(indices);
  set_error_policy(EXIT_ON_ERROR);
}

CTEST(nmt,he_ringnum) {
  //ring_num
  long nside=1024;
  gsl_rng *r=init_rng(1234);
  int ii;
  for(ii=0;ii<NNO_RNG;ii++) {
    double z=2*rng_01(r)-1;
    int rn=he_ring_num(nside,z);
    ASSERT_INTERVAL(0,4*nside-2,rn);
  }
  end_rng(r);
}

CTEST(nmt,he_r2n) {
  int ii;
  long nside=256;
  long listpix[5]={123,453,6,723475,39642};
  long iother,npix=he_nside2npix(nside);
  double *mp=my_malloc(npix*sizeof(double));
  //setup
  for(ii=0;ii<npix;ii++)
    mp[ii]=ii;
  //ring2nest
  he_ring2nest_inplace(mp,nside);
  for(ii=0;ii<5;ii++) {
    ring2nest(nside,listpix[ii],&iother);
    ASSERT_DBL_NEAR_TOL((double)(listpix[ii]),mp[iother],1E-5);
  }
  //nest2ring
  he_nest2ring_inplace(mp,nside);
  for(ii=0;ii<5;ii++) {
    ASSERT_DBL_NEAR_TOL((double)(listpix[ii]),mp[listpix[ii]],1E-5);
  }
  free(mp);
}

CTEST(nmt,he_ud) {
  //ud-grade
  int ii;
  long nside=256,npix=he_nside2npix(nside);
  double *mp=my_malloc(npix*sizeof(double));
  double *mplo=my_malloc((npix/4)*sizeof(double));
  for(ii=0;ii<npix;ii++)
    mp[ii]=ii;
  he_nest2ring_inplace(mp,nside);
  he_udgrade(mp,nside,mplo,nside/2,0);
  for(ii=0;ii<npix/4;ii++) {
    long inest;
    double predmean;
    ring2nest(nside/2,ii,&inest);
    predmean=inest*4+1.5;
    ASSERT_DBL_NEAR_TOL(predmean,mplo[ii],1E-5);
  }
  free(mplo);  
}

CTEST(nmt,he_x2y) {
  long nside=1024;
  double ip,ip0=1026;
  double vec[3],vec0[3]={0.010057788838065481, 0.015334332284729826, 0.9998318354288737};
  double th0=0.018339535684317072,ph0=0.9902846408054783;
  he_pix2vec_ring(nside,ip0,vec);
  ip=he_ang2pix(nside,cos(th0),ph0);
  ASSERT_EQUAL(ip0,ip);
  ASSERT_DBL_NEAR_TOL(vec0[0],vec[0],1E-8);
  ASSERT_DBL_NEAR_TOL(vec0[1],vec[1],1E-8);
  ASSERT_DBL_NEAR_TOL(vec0[2],vec[2],1E-8);
}
