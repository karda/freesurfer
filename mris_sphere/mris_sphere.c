/**
 * @file  mris_sphere.c
 * @brief REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: greve $
 *    $Date: 2007/03/08 18:36:08 $
 *    $Revision: 1.47 $
 *
 * Copyright (C) 2002-2007,
 * The General Hospital Corporation (Boston, MA). 
 * All rights reserved.
 *
 * Distribution, usage and copying of this software is covered under the
 * terms found in the License Agreement file named 'COPYING' found in the
 * FreeSurfer source code root directory, and duplicated here:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferOpenSourceLicense
 *
 * General inquiries: freesurfer@nmr.mgh.harvard.edu
 * Bug reports: analysis-bugs@nmr.mgh.harvard.edu
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "macros.h"
#include "error.h"
#include "tags.h"
#include "diag.h"
#include "proto.h"
#include "mrisurf.h"
#include "mri.h"
#include "macros.h"
#include "utils.h"
#include "timer.h"
#include "version.h"

static char vcid[]=
  "$Id: mris_sphere.c,v 1.47 2007/03/08 18:36:08 greve Exp $";

int main(int argc, char *argv[]) ;

static int  get_option(int argc, char *argv[]) ;
static void usage_exit(void) ;
static void print_usage(void) ;
static void print_help(void) ;
static void print_version(void) ;
int MRISscaleUp(MRI_SURFACE *mris) ;

char *Progname ;

static INTEGRATION_PARMS  parms ;
#define BASE_DT_SCALE     1.0
static float base_dt_scale = BASE_DT_SCALE ;
static int nbrs = 2 ;
static int inflate = 0 ;
static double disturb = 0 ;
static int   max_passes = 1 ;
static int   randomly_project = 0 ;
static int   talairach = 0 ;
static float scale = 1.0 ;
static int mrisDisturbVertices(MRI_SURFACE *mris, double amount) ;
static int quick = 0 ;
static int load = 0 ;
static float inflate_area  = 0.0f ;
static float inflate_dt = 0.9 ;
static float inflate_tol  = 1.0f ;
static float inflate_nlarea  = 0.0f ;
static float inflate_spring  = 0.0f ;
static float inflate_tspring = 0 ;

static float ralpha = 0.0 ;
static float rbeta = 0.0 ;
static float rgamma = 0.0 ;

static double target_radius = DEFAULT_RADIUS ;
#if 0
static int   inflate_avgs = 64 ;
static int   inflate_iterations = 50 ;
static float l_convex = 1.0 ;
static float l_spring_norm = .1 ;
static float l_sphere = 0.25 ;
#else
static int   inflate_avgs = 0 ;
static int   inflate_iterations = 1000 ;
static float l_convex = 1.0 ;
static float l_expand = 0.0 ;
static float l_spring_norm = 1.0 ;
static float l_sphere = 0.025 ;
#endif

static char *orig_name = "smoothwm" ;
static int smooth_avgs = 0 ;

static char *xform_fname = NULL ;
static char *vol_fname = NULL ;

static int remove_negative = 1 ;

int
main(int argc, char *argv[]) {
  char         **av, *in_surf_fname, *out_fname, fname[STRLEN], *cp ;
  int          ac, nargs, msec, err ;
  MRI_SURFACE  *mris ;
  struct timeb then ;
  float        max_dim ;

  char cmdline[CMD_LINE_LEN] ;

  make_cmd_version_string
  (argc, argv,
   "$Id: mris_sphere.c,v 1.47 2007/03/08 18:36:08 greve Exp $",
   "$Name:  $", cmdline);

  /* rkt: check for and handle version tag */
  nargs = handle_version_option
          (argc, argv,
           "$Id: mris_sphere.c,v 1.47 2007/03/08 18:36:08 greve Exp $",
           "$Name:  $");
  if (nargs && argc - nargs == 1)
    exit (0);
  argc -= nargs;

  Gdiag = DIAG_SHOW ;

  TimerStart(&then) ;
  Progname = argv[0] ;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;

  memset(&parms, 0, sizeof(parms)) ;
  parms.dt = .05 ;
  parms.projection = PROJECT_ELLIPSOID ;
  parms.tol = .5 /*1e-1*/ ;
  parms.n_averages = 1024 ;
  parms.min_averages = 0 ;
  parms.l_angle = 0.0 /* L_ANGLE */ ;
  parms.l_area = 0.0 /* L_AREA */ ;
  parms.l_neg = 0.0 ;
  parms.l_dist = 1.0 ;
  parms.l_spring = 0.0 ;
  parms.l_area = 1.0 ;
  parms.l_boundary = 0.0 ;
  parms.l_curv = 0.0 ;
  parms.niterations = 25 ;
  parms.write_iterations = 1000 ;
  parms.a = parms.b = parms.c = 0.0f ;  /* ellipsoid parameters */
  parms.dt_increase = 1.01 /* DT_INCREASE */;
  parms.dt_decrease = 0.99 /* DT_DECREASE*/ ;
  parms.error_ratio = 1.03 /*ERROR_RATIO */;
  parms.integration_type = INTEGRATE_LINE_MINIMIZE ;
  parms.momentum = 0.9 ;
  parms.desired_rms_height = -1.0 ;
  parms.base_name[0] = 0 ;
  parms.Hdesired = 0.0 ;   /* a flat surface */
  parms.nbhd_size = 7 ;
  parms.max_nbrs = 8 ;

  ac = argc ;
  av = argv ;
  for ( ; argc > 1 && ISOPTION(*argv[1]) ; argc--, argv++) {
    nargs = get_option(argc, argv) ;
    argc -= nargs ;
    argv += nargs ;
  }

  parms.scale = scale ;

  if (argc < 3)
    usage_exit() ;

  parms.base_dt = base_dt_scale * parms.dt ;
  in_surf_fname = argv[1] ;
  out_fname = argv[2] ;

  printf("%s\n",vcid);
  printf("  %s\n",MRISurfSrcVersion());
  fflush(stdout);

  if (parms.base_name[0] == 0) {
    FileNameOnly(out_fname, fname) ;
    cp = strchr(fname, '.') ;
    if (cp)
      strcpy(parms.base_name, cp+1) ;
    else
      strcpy(parms.base_name, "sphere") ;
  }

  mris = MRISread(in_surf_fname) ;
  if (!mris)
    ErrorExit(ERROR_NOFILE, "%s: could not read surface file %s",
              Progname, in_surf_fname) ;

  MRISaddCommandLine(mris, cmdline) ;

  fprintf(stderr, "reading original vertex positions...\n") ;
  if (!FZERO(disturb))
    mrisDisturbVertices(mris, disturb) ;
  if (quick == 0) {
    // don't need original properties unless preserving metric
    err = MRISreadOriginalProperties(mris, orig_name) ;
    if(err) exit(1);
  }
  if (smooth_avgs > 0) {
    MRISsaveVertexPositions(mris, TMP_VERTICES) ;
    MRISrestoreVertexPositions(mris, ORIGINAL_VERTICES) ;
    MRISaverageVertexPositions(mris, smooth_avgs) ;
    MRISsaveVertexPositions(mris, ORIGINAL_VERTICES) ;
    MRISrestoreVertexPositions(mris, TMP_VERTICES) ;
  }

  if (!FZERO(ralpha) || !FZERO(rbeta) || !FZERO(rgamma)) {
    MRISrotate(mris,mris,RADIANS(ralpha),RADIANS(rbeta),RADIANS(rgamma)) ;
    //                if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
    MRISwrite(mris, "rot") ;
  }
  fprintf(stderr, "unfolding cortex into spherical form...\n");
  if (talairach) {
    MRIStalairachTransform(mris, mris) ;
    MRISwrite(mris, "tal") ;
  }

  if (xform_fname) {
    LTA *lta ;
    MRI *mri ;
    TRANSFORM transform ;

    lta = LTAread(xform_fname) ;
    if (lta == NULL)
      ErrorExit(ERROR_NOFILE, "%s: could not load %s", xform_fname) ;
    mri = MRIread(vol_fname) ;
    if (mri == NULL)
      ErrorExit(ERROR_NOFILE, "%s: could not load %s", vol_fname) ;
    transform.type = lta->type ;
    transform.xform = (void *)lta ;
    MRIStransform(mris, mri, &transform, mri) ;
    MRIfree(&mri) ;
    LTAfree(&lta) ;
    MRISwrite(mris, "xfm") ;
  }
#if 0
  max_dim = MAX(abs(mris->xlo), abs(mris->xhi)) ;
  max_dim = MAX(abs(max_dim), abs(mris->ylo)) ;
  max_dim = MAX(abs(max_dim), abs(mris->yhi)) ;
  max_dim = MAX(abs(max_dim), abs(mris->zlo)) ;
  max_dim = MAX(abs(max_dim), abs(mris->zhi)) ;
#else
  max_dim = MAX(abs(mris->xhi-mris->xlo), abs(mris->yhi-mris->ylo)) ;
  max_dim = MAX(max_dim,abs(mris->zhi-mris->zlo)) ;
#endif
  if (max_dim > .75*DEFAULT_RADIUS) {
    float ratio = .75*DEFAULT_RADIUS / (max_dim) ;
    printf("scaling brain by %2.3f...\n", ratio) ;
    MRISscaleBrain(mris, mris, ratio) ;
  }

  if (target_radius < 0) {
    target_radius = sqrt(mris->total_area / (4*M_PI)) ;
    printf("setting target radius to be %2.3f to match surface areas\n", target_radius) ;
  }
  //  MRISsampleAtEachDistance(mris, parms.nbhd_size, parms.max_nbrs) ;
  if (!load && inflate) {
    INTEGRATION_PARMS inflation_parms ;

    MRIScenter(mris, mris) ;
    memset(&inflation_parms, 0, sizeof(INTEGRATION_PARMS)) ;
    strcpy(inflation_parms.base_name, parms.base_name) ;
    inflation_parms.write_iterations = parms.write_iterations ;
    inflation_parms.niterations = inflate_iterations ;
    inflation_parms.l_spring_norm = l_spring_norm ;
    inflation_parms.l_spring = inflate_spring ;
    inflation_parms.l_nlarea = inflate_nlarea ;
    inflation_parms.l_area = inflate_area ;
    inflation_parms.n_averages = inflate_avgs ;
    inflation_parms.l_expand = l_expand ;
    inflation_parms.l_tspring = inflate_tspring ;
    inflation_parms.l_sphere = l_sphere ;
    inflation_parms.l_convex = l_convex ;
#define SCALE_UP 2
    inflation_parms.a = SCALE_UP*DEFAULT_RADIUS ;
    inflation_parms.tol = inflate_tol ;
    inflation_parms.integration_type = INTEGRATE_MOMENTUM ;
    inflation_parms.momentum = 0.9 ;
    inflation_parms.dt = inflate_dt ;

    /* store the inflated positions in the v->c? field so that they can
      be used in the repulsive term.
    */
    /*    inflation_parms.l_repulse_ratio = .1 ;*/
    MRISsaveVertexPositions(mris, CANONICAL_VERTICES) ;
    //    MRISexpandSurface(mris, 3, &inflation_parms, 0) ;
    MRIScenter(mris, mris) ;
    mris->x0 = mris->xctr ;
    mris->y0 = mris->yctr ;
    mris->z0 = mris->zctr ;
    MRISinflateToSphere(mris, &inflation_parms) ;
    if (inflation_parms.l_expand > 0) {
      inflation_parms.l_expand = 0 ;
      inflation_parms.niterations += (inflate_iterations*.1) ;
      MRISinflateToSphere(mris, &inflation_parms) ;
    }
    MRISscaleBrain(mris, mris, target_radius/(DEFAULT_RADIUS*SCALE_UP)) ;
    parms.start_t = inflation_parms.start_t ;
    MRISresetNeighborhoodSize(mris, nbrs) ;
  }

  MRISwrite(mris, "before") ;
  MRISprojectOntoSphere(mris, mris, target_radius) ;
  MRISwrite(mris, "after") ;
  fprintf(stderr,"surface projected - minimizing metric distortion...\n");
  MRISsetNeighborhoodSize(mris, nbrs) ;
  if (quick) {
    if (!load) {
#if 0
      parms.n_averages = 32 ;
      parms.tol = .1 ;
      parms.l_parea = parms.l_dist = 0.0 ;
      parms.l_nlarea = 1 ;
#endif
      MRISprintTessellationStats(mris, stderr) ;
      MRISquickSphere(mris, &parms, max_passes) ;
    }
  } else
    MRISunfold(mris, &parms, max_passes) ;
  if (remove_negative) {
    parms.niterations = 1000 ;
    MRISremoveOverlapWithSmoothing(mris,&parms) ;
  }
  if (!load) {
    fprintf(stderr, "writing spherical brain to %s\n", out_fname) ;
    MRISwrite(mris, out_fname) ;
  }

  msec = TimerStop(&then) ;
  fprintf(stderr, "spherical transformation took %2.2f hours\n",
          (float)msec/(1000.0f*60.0f*60.0f));
  exit(0) ;
  return(0) ;  /* for ansi */
}

/*----------------------------------------------------------------------
  Parameters:

  Description:
  ----------------------------------------------------------------------*/
static int
get_option(int argc, char *argv[]) {
  int  nargs = 0 ;
  char *option ;
  float f ;

  option = argv[1] + 1 ;            /* past '-' */
  if (!stricmp(option, "-help"))
    print_help() ;
  else if (!stricmp(option, "-version"))
    print_version() ;
  else if (!stricmp(option, "dist")) {
    sscanf(argv[2], "%f", &parms.l_dist) ;
    nargs = 1 ;
    fprintf(stderr, "l_dist = %2.3f\n", parms.l_dist) ;
  } else if (!stricmp(option, "lm")) {
    parms.integration_type = INTEGRATE_LM_SEARCH ;
    fprintf(stderr, "integrating using binary search line minimization\n") ;
  } else if (!stricmp(option, "avgs")) {
    smooth_avgs = atoi(argv[2]) ;
    fprintf
    (stderr,
     "smoothing original positions %d times before computing metrics\n",
     smooth_avgs) ;
    nargs = 1 ;
  } else if (!stricmp(option, "rotate")) {
    ralpha = atof(argv[2]) ;
    rbeta = atof(argv[3]) ;
    rgamma = atof(argv[4]) ;
    nargs = 3 ;
    fprintf(stderr, "rotating brain by (%2.1f, %2.1f, %2.1f)\n",
            ralpha, rbeta, rgamma) ;
  } else if (!stricmp(option, "talairach")) {
    talairach = 1 ;
    fprintf(stderr, "transforming surface into Talairach space.\n") ;
  } else if (!stricmp(option, "remove_negative")) {
    remove_negative = atoi(argv[2]) ;
    nargs = 1 ;
    fprintf
    (stderr,
     "%sremoving negative triangles with iterative smoothing\n",
     remove_negative ? "" : "not ") ;
  } else if (!stricmp(option, "notal")) {
    talairach = 0 ;
    fprintf(stderr, "transforming surface into Talairach space.\n") ;
  } else if (!stricmp(option, "dt")) {
    parms.dt = atof(argv[2]) ;
    parms.base_dt = base_dt_scale*parms.dt ;
    nargs = 1 ;
    fprintf(stderr, "momentum with dt = %2.2f\n", parms.dt) ;
  } else if (!stricmp(option, "curv")) {
    sscanf(argv[2], "%f", &parms.l_curv) ;
    nargs = 1 ;
    fprintf(stderr, "using l_curv = %2.3f\n", parms.l_curv) ;
  } else if (!stricmp(option, "area")) {
    sscanf(argv[2], "%f", &parms.l_area) ;
    nargs = 1 ;
    fprintf(stderr, "using l_area = %2.3f\n", parms.l_area) ;
  } else if (!stricmp(option, "iarea")) {
    inflate_area = atof(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "inflation l_area = %2.3f\n", inflate_area) ;
  } else if (!stricmp(option, "itol")) {
    inflate_tol = atof(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "inflation tol = %2.3f\n", inflate_tol) ;
  } else if (!stricmp(option, "in")) {
    inflate_iterations = atoi(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "inflation iterations = %d\n", inflate_iterations) ;
  } else if (!stricmp(option, "idt")) {
    inflate_dt = atof(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "inflation dt = %f\n", inflate_dt) ;
  } else if (!stricmp(option, "iavgs")) {
    inflate_avgs = atoi(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "inflation averages = %d\n", inflate_avgs) ;
  } else if (!stricmp(option, "inlarea")) {
    inflate_nlarea = atof(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "inflation l_nlarea = %2.3f\n", inflate_nlarea) ;
  } else if (!stricmp(option, "ispring")) {
    inflate_spring = atof(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "inflation l_spring = %2.3f\n", inflate_spring) ;
  } else if (!stricmp(option, "adaptive")) {
    parms.integration_type = INTEGRATE_ADAPTIVE ;
    fprintf(stderr, "using adaptive time step integration\n") ;
  } else if (!stricmp(option, "nbrs")) {
    nbrs = atoi(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "using neighborhood size=%d\n", nbrs) ;
  } else if (!stricmp(option, "spring")) {
    sscanf(argv[2], "%f", &parms.l_spring) ;
    nargs = 1 ;
    fprintf(stderr, "using l_spring = %2.3f\n", parms.l_spring) ;
  } else if (!stricmp(option, "tspring")) {
    sscanf(argv[2], "%f", &parms.l_tspring) ;
    nargs = 1 ;
    fprintf(stderr, "using l_tspring = %2.3f\n", parms.l_tspring) ;
  } else if (!stricmp(option, "itspring")) {
    sscanf(argv[2], "%f", &inflate_tspring) ;
    nargs = 1 ;
    fprintf(stderr, "using inflation l_tspring = %2.3f\n", inflate_tspring) ;
  } else if (!stricmp(option, "convex")) {
    sscanf(argv[2], "%f", &l_convex) ;
    nargs = 1 ;
    fprintf(stderr, "using l_convex = %2.3f\n", l_convex) ;
  } else if (!stricmp(option, "expand")) {
    sscanf(argv[2], "%f", &l_expand) ;
    nargs = 1 ;
    fprintf(stderr, "using l_expand = %2.3f\n", l_expand) ;
  } else if (!stricmp(option, "spring_norm")) {
    sscanf(argv[2], "%f", &l_spring_norm) ;
    nargs = 1 ;
    fprintf(stderr, "using l_spring_norm = %2.3f\n", l_spring_norm) ;
  } else if (!stricmp(option, "sphere")) {
    sscanf(argv[2], "%f", &l_sphere) ;
    nargs = 1 ;
    fprintf(stderr, "using l_sphere = %2.3f\n", l_sphere) ;
  } else if (!stricmp(option, "name")) {
    strcpy(parms.base_name, argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "using base name = %s\n", parms.base_name) ;
  } else if (!stricmp(option, "angle")) {
    sscanf(argv[2], "%f", &parms.l_angle) ;
    nargs = 1 ;
    fprintf(stderr, "using l_angle = %2.3f\n", parms.l_angle) ;
  } else if (!stricmp(option, "area")) {
    sscanf(argv[2], "%f", &parms.l_area) ;
    nargs = 1 ;
    fprintf(stderr, "using l_area = %2.3f\n", parms.l_area) ;
  } else if (!stricmp(option, "tol")) {
    if (sscanf(argv[2], "%e", &f) < 1)
      ErrorExit(ERROR_BADPARM, "%s: could not scan tol from %s",
                Progname, argv[2]) ;
    parms.tol = (double)f ;
    nargs = 1 ;
    fprintf(stderr, "using tol = %2.2e\n", (float)parms.tol) ;
  } else if (!stricmp(option, "error_ratio")) {
    parms.error_ratio = atof(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "error_ratio=%2.3f\n", parms.error_ratio) ;
  } else if (!stricmp(option, "dt_inc")) {
    parms.dt_increase = atof(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "dt_increase=%2.3f\n", parms.dt_increase) ;
  } else if (!stricmp(option, "NLAREA")) {
    parms.l_nlarea = atof(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "nlarea = %2.3f\n", parms.l_nlarea) ;
  } else if (!stricmp(option, "PAREA")) {
    parms.l_parea = atof(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "parea = %2.3f\n", parms.l_parea) ;
  } else if (!stricmp(option, "RADIUS")) {
    target_radius = atof(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "target radius = %2.3f\n", target_radius) ;
  } else if (!stricmp(option, "RA")) {
    target_radius = -1 ;
    fprintf(stderr, "computing area-matching target radius\n") ;
  } else if (!stricmp(option, "NLDIST")) {
    parms.l_nldist = atof(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "nldist = %2.3f\n", parms.l_nldist) ;
  } else if (!stricmp(option, "vnum") || !stricmp(option, "distances")) {
    parms.nbhd_size = atof(argv[2]) ;
    parms.max_nbrs = atof(argv[3]) ;
    nargs = 2 ;
    fprintf(stderr, "nbr size = %d, max neighbors = %d\n",
            parms.nbhd_size, parms.max_nbrs) ;
  } else if (!stricmp(option, "dt_dec")) {
    parms.dt_decrease = atof(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "dt_decrease=%2.3f\n", parms.dt_decrease) ;
  } else if (!stricmp(option, "seed")) {
    setRandomSeed(atol(argv[2])) ;
    fprintf(stderr,"setting seed for random number genererator to %d\n",
            atoi(argv[2])) ;
    nargs = 1 ;
  } else switch (toupper(*option)) {
    case 'T':
      xform_fname = argv[2] ;
      vol_fname = argv[3] ;
      nargs = 2 ;
      break ;
    case 'O':
      orig_name = argv[2] ;
      fprintf(stderr, "using %s as original surface...\n", orig_name) ;
      nargs = 1 ;
      break ;
    case 'P':
      max_passes = atoi(argv[2]) ;
      fprintf(stderr, "limitting unfolding to %d passes\n", max_passes) ;
      nargs = 1 ;
      break ;
    case 'Q':
      remove_negative = 0 ;
      quick = 1 ;
      inflate = 1 ;
      inflate_iterations = 300 ;
      max_passes = 3 ;
      fprintf(stderr, "doing quick spherical unfolding.\n") ;
      nbrs = 1 ;
      parms.l_spring = parms.l_dist = parms.l_parea = parms.l_area = 0.0 ;
      parms.l_nlarea = 1.0 ;
      parms.tol = 1e-1 ;
      parms.n_averages = 32 ;
      /*    parms.tol = 10.0f / (sqrt(33.0/1024.0)) ;*/
      break ;
    case 'L':
      load = 1 ;
      break ;
    case 'B':
      base_dt_scale = atof(argv[2]) ;
      parms.base_dt = base_dt_scale*parms.dt ;
      nargs = 1;
      break ;
    case 'S':
      scale = atof(argv[2]) ;
      fprintf(stderr, "scaling brain by = %2.3f\n", (float)scale) ;
      nargs = 1 ;
      break ;
    case 'V':
      Gdiag_no = atoi(argv[2]) ;
      nargs = 1 ;
      break ;
    case 'D':
      disturb = atof(argv[2]) ;
      nargs = 1 ;
      fprintf
      (stderr, "disturbing vertex positions by %2.3f\n",(float)disturb) ;
      break ;
    case 'R':
      randomly_project = !randomly_project ;
      fprintf(stderr, "using random placement for projection.\n") ;
      break ;
    case 'M':
      parms.integration_type = INTEGRATE_MOMENTUM ;
      parms.momentum = atof(argv[2]) ;
      nargs = 1 ;
      fprintf(stderr, "momentum = %2.2f\n", (float)parms.momentum) ;
      break ;
    case 'W':
      Gdiag |= DIAG_WRITE ;
      sscanf(argv[2], "%d", &parms.write_iterations) ;
      nargs = 1 ;
      fprintf
      (stderr, "using write iterations = %d\n", parms.write_iterations) ;
      break ;
    case 'I':
      inflate = 1 ;
      fprintf(stderr, "inflating brain...\n") ;
      break ;
    case 'A':
      sscanf(argv[2], "%d", &parms.n_averages) ;
      nargs = 1 ;
      fprintf(stderr, "using n_averages = %d\n", parms.n_averages) ;
      break ;
    case '?':
    case 'U':
      print_usage() ;
      exit(1) ;
      break ;
    case 'N':
      sscanf(argv[2], "%d", &parms.niterations) ;
      nargs = 1 ;
      fprintf(stderr, "using niterations = %d\n", parms.niterations) ;
      break ;
    default:
      fprintf(stderr, "unknown option %s\n", argv[1]) ;
      exit(1) ;
      break ;
    }

  return(nargs) ;
}

static void
usage_exit(void) {
  print_usage() ;
  exit(1) ;
}

static void
print_usage(void) {
  fprintf(stderr,
          "usage: %s [options] <inflated surface> <output spherical surface>"
          "\n", Progname) ;
}

static void
print_help(void) {
  print_usage() ;
  fprintf(stderr,
          "\nThis program will add a template into an average surface.\n");
  //  fprintf(stderr, "\nvalid options are:\n\n") ;
  exit(1) ;
}

static void
print_version(void) {
  fprintf(stderr, "%s\n", vcid) ;
  exit(1) ;
}

static int
mrisDisturbVertices(MRI_SURFACE *mris, double amount) {
  int    vno ;
  VERTEX *v ;

  for (vno = 0 ; vno < mris->nvertices ; vno++) {
    v = &mris->vertices[vno] ;
    if (v->ripflag)
      continue ;
    v->x += randomNumber(-amount, amount) ;
    v->y += randomNumber(-amount, amount) ;
  }

  MRIScomputeMetricProperties(mris) ;
  return(NO_ERROR) ;
}

int
MRISscaleUp(MRI_SURFACE *mris) {
  int     vno, n, max_v, max_n ;
  VERTEX  *v ;
  float   ratio, max_ratio ;

  max_ratio = 0.0f ;
  max_v = max_n = 0 ;
  for (vno = 0 ; vno < mris->nvertices ; vno++) {
    v = &mris->vertices[vno] ;
    if (v->ripflag)
      continue ;
    if (vno == Gdiag_no)
      DiagBreak() ;
    for (n = 0 ; n < v->vnum ; n++) {
      if (FZERO(v->dist[n]))   /* would require infinite scaling */
        continue ;
      ratio = v->dist_orig[n] / v->dist[n] ;
      if (ratio > max_ratio) {
        max_v = vno ;
        max_n = n ;
        max_ratio = ratio ;
      }
    }
  }

  fprintf(stderr, "max @ (%d, %d), scaling brain by %2.3f\n",
          max_v, max_n, max_ratio) ;
#if 0
  MRISscaleBrain(mris, mris, max_ratio) ;
#else
  for (vno = 0 ; vno < mris->nvertices ; vno++) {
    v = &mris->vertices[vno] ;
    if (v->ripflag)
      continue ;
    if (vno == Gdiag_no)
      DiagBreak() ;
    for (n = 0 ; n < v->vnum ; n++)
      v->dist_orig[n] /= max_ratio ;
  }
#endif
  return(NO_ERROR) ;
}

