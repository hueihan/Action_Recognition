/***********************************************************************/
/*                                                                     */
/*   svm_struct_api.c                                                  */
/*                                                                     */
/*   Definition of API for attaching implementing SVM learning of      */
/*   structures (e.g. parsing, multi-label classification, HMM)        */ 
/*                                                                     */
/*   Author: Thorsten Joachims                                         */
/*   Date: 03.07.04                                                    */
/*                                                                     */
/*   Copyright (c) 2004  Thorsten Joachims - All rights reserved       */
/*                                                                     */
/*   This software is available for non-commmercial use only. It must   */
/*   not be modified and distributed without prior permission of the   */
/*   author. The author is not responsible for implications from the   */
/*   use of this software.                                             */
/*                                                                     */
/***********************************************************************/

#include <stdio.h>
#include <string.h>
#include "svm_struct/svm_struct_common.h"
#include "svm_struct_api.h"
#include "assert.h"

#define MAX(x,y)      ((x) < (y) ? (y) : (x))
#define MIN(x,y)      ((x) > (y) ? (y) : (x))
#define SIGN(x)       ((x) > (0) ? (1) : (((x) < (0) ? (-1) : (0))))
#define INDEX_E(ii,i,x)(i+ii*x)
#define INDEX_E0(i,x)(i)
#define INDEX_E1(ii,i,x)(i+ii*x)
#define INDEX_E2(iii,ii,i,x)(i+ii*x+iii*x*x)
#define INDEX_E3(iiii,iii,ii,i,x)(i+ii*x+iii*x*x+iiii*x*x*x)
#define INDEX_T(iiii,iii,ii,i,x)(i+ii*x+iii*x*x+iiii*x*x*x)
#define INDEX_T1(ii,i,x)(i+ii*x)
#define INDEX_T2(iii,ii,i,x)(i+ii*x+iii*x*x)
#define INDEX_T3(iiii,iii,ii,i,x)(i+ii*x+iii*x*x+iiii*x*x*x)
#define INDEX_P(iii,ii,i,x)(i+ii*x+iii*x*x)
#define INDEX_P0(i,x)(i)
#define INDEX_P1(ii,i,x)(i+ii*x)
#define INDEX_P2(iii,ii,i,x)(i+ii*x+iii*x*x)
#define INDEX(iii,ii,i,x)(i+ii*x+iii*x*x)


typedef struct viterbi_path {
  double    score;	           /* viterbi score */
  long      label;                 /* label in current state */
  long      index;                 /* index that summarizes history of state */
  struct viterbi_path *backpointer;/* pointer to optimal VPATH element
				      in previous step that leads to
				      current state */
} VPATH;

typedef struct beam {
  long      beam_width;
  long      num;
  long      worst;
  VPATH     **elem;
} BEAM;


long *viterbi(double *trans, double **emit, long length, long classes, 
	      long trans_order, long emit_order, long beam_width);  
VPATH *viterbi_forward(double *trans, double **emit, 
		       long trans_order, long emit_order,
		       long start, long end,
		       long classes, VPATH **path);
VPATH *viterbi_forward_beam(double *trans, double **emit, long beam_width, 
		       long trans_order, long emit_order,
		       long start, long end,
		       long classes, VPATH **path);
VPATH *viterbi_forward_order0(double *trans, double **emit, long emit_order,
			      long start, long end,
			      long classes, VPATH **path);
VPATH *viterbi_forward_order1(double *trans, double **emit, long emit_order, 
			      long start, long end,
			      long classes, VPATH **path);
VPATH *viterbi_forward_order2(double *trans, double **emit, long emit_order,
			      long start, long end,
			      long classes, VPATH **path);
VPATH *viterbi_forward_order3(double *trans, double **emit, long emit_order,
			      long start, long end,
			      long classes, VPATH **path);
long index_max(long order, long classes);
long index_max_cum(long order, long classes);
BEAM *beam_init(long beam_width); 
void beam_add(BEAM *beam, VPATH *new); 
void beam_free(BEAM *beam);
double loss_token(long y, long ybar);

double rt_viterbi=0,rt_emit=0;
long   classify_beam_width=0;

void        svm_struct_learn_api_init(int argc, char* argv[])
{
  /* Called in learning part before anything else is done to allow
     any initializations that might be necessary. */
}

void        svm_struct_learn_api_exit()
{
  /* Called in learning part at the very end to allow any clean-up
     that might be necessary. */
}

void        svm_struct_classify_api_init(int argc, char* argv[])
{
  /* Called in prediction part before anything else is done to allow
     any initializations that might be necessary. */
}

void        svm_struct_classify_api_exit()
{
  /* Called in prediction part at the very end to allow any clean-up
     that might be necessary. */
}

SAMPLE      read_struct_examples(char *file, STRUCT_LEARN_PARM *sparm)
{
  /* Reads struct examples and returns them in sample. The number of
     examples must be written into sample.n */
  SAMPLE   sample;  /* sample */
  EXAMPLE  *examples=NULL;
  long     n;       /* number of examples */
  DOC      **examples_flat=NULL;
  double   *labels_flat=NULL;
  long     totwords=0,tottokens=0,i,queryid,maxlabel=0;

  /* Read as one set of examples using the read_documents function
     from SVM-light */
  read_documents(file,&examples_flat,&labels_flat,&totwords,&tottokens);

  /* Create examples by splitting the input into sequences of tokens. */
  queryid=-1;
  n=0;
  for(i=0;i<tottokens;i++) {
    if(queryid < (long)examples_flat[i]->queryid) {
      queryid=(long)examples_flat[i]->queryid;
      n++;
      examples=(EXAMPLE *)realloc(examples,sizeof(EXAMPLE)*(n));
      examples[n-1].x.length=1;
      examples[n-1].y.length=1;
      examples[n-1].x.tokens=(DOC **)my_malloc(sizeof(DOC **));
      examples[n-1].y.labels=(long *)my_malloc(sizeof(long *));
    }
    else if(queryid > (long)examples_flat[i]->queryid) {
      printf("ERROR (Line %ld): qid example ID's have to be in increasing order.\n",i+1);
      exit(1);
    }
    else {
      examples[n-1].x.length++;
      examples[n-1].y.length++;
      examples[n-1].x.tokens=(DOC **)realloc(examples[n-1].x.tokens,
				       sizeof(DOC **)*examples[n-1].x.length);
      examples[n-1].y.labels=(long *)realloc(examples[n-1].y.labels,
				       sizeof(long *)*examples[n-1].y.length);
    }
    examples[n-1].x.tokens[examples[n-1].x.length-1]=examples_flat[i];
    examples[n-1].y.labels[examples[n-1].y.length-1]=(long)labels_flat[i];
    if(labels_flat[i] < 0) {
      printf("ERROR (Line %ld): Token label IDs cannot be negative.\n",i+1);
      exit(1);
    }
    maxlabel=MAX(maxlabel,(long)labels_flat[i]);
  }

  if(struct_verbosity>=1)
    printf(" %ld examples, %ld tokens, %ld features, %ld classes... ",n,tottokens,totwords,maxlabel);

  free(examples_flat); 
  free(labels_flat);

  sample.n=n;
  sample.examples=examples;
  return(sample);
}

void        init_struct_model(SAMPLE sample, STRUCTMODEL *sm, 
			      STRUCT_LEARN_PARM *sparm, LEARN_PARM *lparm, 
			      KERNEL_PARM *kparm)
{
  /* Initialize structmodel sm. The weight vector w does not need to be
     initialized, but you need to provide the maximum size of the
     feature space in sizePsi. This is the maximum number of different
     weights that can be learned. Later, the weight vector w will
     contain the learned weights for the model. */
  EXAMPLE  *examples=sample.examples;
  PATTERN  *x;
  LABEL    *y;
  long     maxlabel=0,maxfeat=0,i,j;
  WORD     *ai;
  double   sizePsi;

  /* find number of classes and number of features in training set */
  for(i=0;i<sample.n;i++) {
    x=&examples[i].x;
    y=&examples[i].y;
    for(j=0;j<y->length;j++) {
      maxlabel=MAX(maxlabel,y->labels[j]);
      if(y->labels[j]<1) {
	printf("ERROR: Found token label ID '%ld'. Token label IDs must be greater or equal to 1!\n",y->labels[j]);
	exit(1);
      }
      for(ai=x->tokens[j]->fvec->words;ai->wnum;ai++) {
	maxfeat=MAX(maxfeat,ai->wnum);
      }
    }
  }
  sparm->num_classes=maxlabel;
  sparm->num_features=maxfeat;

  sizePsi=index_max_cum(sparm->hmm_trans_order,sparm->num_classes)+1
          +(index_max_cum(sparm->hmm_emit_order,sparm->num_classes)+1)*sparm->num_features;

  if(struct_verbosity >= 2) 
    printf("Dimensionality of Psi: %.0lf\n",sizePsi);
  if(sizePsi > FNUM_MAX) {
    printf("ERROR: Number of features exceeds maximum (change type of FNUM in svm_common.h\n");
    exit(1);
  }
  sm->sizePsi=(long)(sizePsi+0.5);

  sm->walpha=1;

  /* Viterbi test example 
  long classes=2;
  long length=3;
  long i;
  double *trans=(double *)my_malloc(sizeof(double)*(INDEX(0,0,classes,classes)+1));
  double **emit=(double **)my_malloc(sizeof(double)*length);
  for(i=0;i<length;i++) 
    emit[i]=(double *)my_malloc(sizeof(double)*(INDEX(0,0,classes,classes)+1));

  trans[INDEX(0,1,1,classes)]=1.0;
  trans[INDEX(0,2,1,classes)]=0.5;
  trans[INDEX(0,1,2,classes)]=2.0;
  trans[INDEX(0,2,2,classes)]=5.0;

  emit[0][INDEX(0,0,1,classes)]=9;
  emit[0][INDEX(0,0,2,classes)]=0.1;
  emit[1][INDEX(0,0,1,classes)]=0.2;
  emit[1][INDEX(0,0,2,classes)]=3;
  emit[2][INDEX(0,0,1,classes)]=7;
  emit[2][INDEX(0,0,2,classes)]=0.3;

  long *labels = viterbi(trans,emit,length,classes,1);

  for(i=0;i<length;i++) {
    printf(" %ld",labels[i]);
  }
  printf("\n");
  */

}

CONSTSET    init_struct_constraints(SAMPLE sample, STRUCTMODEL *sm, 
				    STRUCT_LEARN_PARM *sparm)
{
  /* Initializes the optimization problem. Typically, you do not need
     to change this function, since you want to start with an empty
     set of constraints. However, if for example you have constraints
     that certain weights need to be positive, you might put that in
     here. The constraints are represented as lhs[i]*w >= rhs[i]. lhs
     is an array of feature vectors, rhs is an array of doubles. m is
     the number of constraints. The function returns the initial
     set of constraints. */
  CONSTSET c;
  long     sizePsi=sm->sizePsi;
  long     i;
  WORD     words[2];

  if(1) { /* normal case: start with empty set of constraints */
    c.lhs=NULL;
    c.rhs=NULL;
    c.m=0;
  }
  else { /* add constraints so that all learned weights are
            positive. WARNING: Currently, they are positive only up to
            precision epsilon set by -e. */
    c.lhs=my_malloc(sizeof(DOC *)*sizePsi);
    c.rhs=my_malloc(sizeof(double)*sizePsi);
    for(i=0; i<sizePsi; i++) {
      words[0].wnum=i+1;
      words[0].weight=1.0;
      words[1].wnum=0;
      /* the following slackid is a hack. we will run into problems,
         if we have move than 1000000 slack sets (ie examples) */
      c.lhs[i]=create_example(i,0,1000000+i,1,create_svector(words,NULL,1.0));
      c.rhs[i]=0.0;
    }
  }
  return(c);
}

LABEL       classify_struct_example(PATTERN x, STRUCTMODEL *sm, 
				    STRUCT_LEARN_PARM *sparm)
{
  /* Finds the label yhat for pattern x that scores the highest
     according to the linear evaluation function in sm, especially the
     weights sm.w. The returned label is taken as the prediction of sm
     for the pattern x. The weights correspond to the features defined
     by psi() and range from index 1 to index sm->sizePsi. If the
     function cannot find a label, it shall return an empty label as
     recognized by the function empty_label(y). */
  LABEL y;
  long length=x.length;
  long classes=sparm->num_classes;
  long fnum=sparm->num_features;
  double *trans;
  double **emit,*emiti;
  long i,j;
  long index,index_m,index_l,index_h,source,target,order;
  double *transbase=sm->w;  /* first weights in w are the transition scores */
  long emitbase;

  emitbase=index_max_cum(sparm->hmm_trans_order,classes)+1;

  emit=(double **)my_malloc(sizeof(double)*length);
  for(i=0;i<length;i++) {
    emit[i]=(double *)my_malloc(sizeof(double)
			  *(index_max_cum(sparm->hmm_emit_order,classes)+1));
    emiti=emit[i];

    /* Cut off feature numbers that are higher than the max seen in the
       training data to avoid memory out-of-bounds errors */
    for(j=0;x.tokens[i]->fvec->words[j].wnum;j++)
      if(x.tokens[i]->fvec->words[j].wnum > sparm->num_features)
	x.tokens[i]->fvec->words[j].wnum=0;

    /* compute emission scores at each order */
    index_l=1;
    for(order=0;order<=sparm->hmm_emit_order;order++) {
      index_m=index_max(order,classes)+1;
      index_h=index_l+index_m;
      for(index=index_l;index<index_h;index++) 
	emiti[index]=sprod_ns(&(sm->w[emitbase+fnum*index]),x.tokens[i]->fvec);
      index_l+=index_m;
    }
    /* add lower order emission scores to higher order scores */
    index_l=1;
    for(order=0;order<sparm->hmm_emit_order;order++) {
      index_m=index_max(order,classes)+1;
      index_h=index_l+index_m;
      for(target=0;target<classes;target++) 
	for(source=0;source<index_m;source++) 
	  emiti[index_h+index_m*target+source]+=emiti[index_l+source];
      index_l+=index_m;
    }
  }

  /* compute the table of transition scores */
  trans=(double *)my_malloc(sizeof(double)
			 *(index_max_cum(sparm->hmm_trans_order,classes)+1));
  /* copy transition weights */
  index_l=1;
  for(order=0;order<=sparm->hmm_trans_order;order++) {
    index_m=index_max(order,classes)+1;
    index_h=index_l+index_m;
    for(index=index_l;index<index_h;index++) 
      trans[index]=transbase[index];
    index_l+=index_m;
  }
  /* add lower order transition weights to higher order weights */
  index_l=1;
  for(order=0;order<sparm->hmm_trans_order;order++) {
    index_m=index_max(order,classes)+1;
    index_h=index_l+index_m;
    if(order>0)
      for(target=0;target<classes;target++) 
	for(source=0;source<index_m;source++) 
	  trans[index_h+index_m*target+source]+=trans[index_l+source];
    index_l+=index_m;
  }

  y.labels=viterbi(trans,emit,length,classes,
		   sparm->hmm_trans_order,sparm->hmm_emit_order,
		   sparm->beam_width);
  y.length=length;

  for(i=0;i<length;i++) 
    free(emit[i]);
  free(emit);
  free(trans);

  return(y);
}

LABEL       find_most_violated_constraint_slackrescaling(PATTERN x, LABEL y, 
						     STRUCTMODEL *sm, 
						     STRUCT_LEARN_PARM *sparm)
{
  /* Finds the label ybar for pattern x that that is responsible for
     the most violated constraint for the slack rescaling
     formulation. For linear slack variables, this is that label ybar
     that maximizes

            argmax_{ybar} loss(y,ybar)*(1-psi(x,y)+psi(x,ybar)) 

     Note that ybar may be equal to y (i.e. the max is 0), which is
     different from the algorithms described in
     [Tschantaridis/05]. Note that this argmax has to take into
     account the scoring function in sm, especially the weights sm.w,
     as well as the loss function, and whether linear or quadratic
     slacks are used. The weights in sm.w correspond to the features
     defined by psi() and range from index 1 to index
     sm->sizePsi. Most simple is the case of the zero/one loss
     function. For the zero/one loss, this function should return the
     highest scoring label ybar (which may be equal to the correct
     label y), or the second highest scoring label ybar, if
     Psi(x,ybar)>Psi(x,y)-1. If the function cannot find a label, it
     shall return an empty label as recognized by the function
     empty_label(y). */
  LABEL ybar;

  /* insert your code for computing the label ybar here */

  return(ybar);
}

LABEL       find_most_violated_constraint_marginrescaling(PATTERN x, LABEL y, 
						     STRUCTMODEL *sm, 
						     STRUCT_LEARN_PARM *sparm)
{
  /* Finds the label ybar for pattern x that that is responsible for
     the most violated constraint for the margin rescaling
     formulation. For linear slack variables, this is that label ybar
     that maximizes

            argmax_{ybar} loss(y,ybar)+psi(x,ybar)

     Note that ybar may be equal to y (i.e. the max is 0), which is
     different from the algorithms described in
     [Tschantaridis/05]. Note that this argmax has to take into
     account the scoring function in sm, especially the weights sm.w,
     as well as the loss function, and whether linear or quadratic
     slacks are used. The weights in sm.w correspond to the features
     defined by psi() and range from index 1 to index
     sm->sizePsi. Most simple is the case of the zero/one loss
     function. For the zero/one loss, this function should return the
     highest scoring label ybar (which may be equal to the correct
     label y), or the second highest scoring label ybar, if
     Psi(x,ybar)>Psi(x,y)-1. If the function cannot find a label, it
     shall return an empty label as recognized by the function
     empty_label(y). */
  LABEL ybar;
  long length=x.length;
  long classes=sparm->num_classes;
  long fnum=sparm->num_features;
  double *trans;
  double **emit,*emiti,*emitbuf;
  long i;
  long index,index_m,index_l,index_h,source,target,order;
  double *transbase=sm->w;  /* first weights in w are the transition scores */
  long emitbase;

  double rt1=get_runtime();

  emitbase=index_max_cum(sparm->hmm_trans_order,classes)+1;

  /* compute the table of emission scores */
  emit=(double **)my_malloc(sizeof(double)*length);
  emitbuf=(double *)my_malloc(sizeof(double)*length
		        *(index_max_cum(sparm->hmm_emit_order,classes)+1));
  for(i=0;i<length;i++) {
    emit[i]=emitbuf+i*(index_max_cum(sparm->hmm_emit_order,classes)+1);
    emiti=emit[i];
    /* compute emission scores at each order */
    index_l=1;
    for(order=0;order<=sparm->hmm_emit_order;order++) {
      index_m=index_max(order,classes)+1;
      index_h=index_l+index_m;
      for(index=index_l;index<index_h;index++) { 
	emiti[index]=sprod_ns(&(sm->w[emitbase+fnum*index]),x.tokens[i]->fvec);
	if(order==0) /* add loss to zero order emissions */
	  emiti[index]+=loss_token(y.labels[i],index)/sm->walpha;
      }
      index_l+=index_m;
    }
    /* add lower order emission scores to higher order scores */
    index_l=1;
    for(order=0;order<sparm->hmm_emit_order;order++) {
      index_m=index_max(order,classes)+1;
      index_h=index_l+index_m;
      for(target=0;target<classes;target++) 
	for(source=0;source<index_m;source++) 
	  emiti[index_h+index_m*target+source]+=emiti[index_l+source];
      index_l+=index_m;
    }
  }

  /* compute the table of transition scores */
  trans=(double *)my_malloc(sizeof(double)
			 *(index_max_cum(sparm->hmm_trans_order,classes)+1));
  /* copy transition weights */
  index_l=1;
  for(order=0;order<=sparm->hmm_trans_order;order++) {
    index_m=index_max(order,classes)+1;
    index_h=index_l+index_m;
    for(index=index_l;index<index_h;index++) 
      trans[index]=transbase[index];
    index_l+=index_m;
  }
  /* add lower order transition weights to higher order weights */
  index_l=1;
  for(order=0;order<sparm->hmm_trans_order;order++) {
    index_m=index_max(order,classes)+1;
    index_h=index_l+index_m;
    if(order>0)
      for(target=0;target<classes;target++) 
	for(source=0;source<index_m;source++) 
	  trans[index_h+index_m*target+source]+=trans[index_l+source];
    index_l+=index_m;
  }

  rt_emit+=MAX(get_runtime()-rt1,0);

  rt1=get_runtime();

  ybar.labels=viterbi(trans,emit,length,classes,
		      sparm->hmm_trans_order,sparm->hmm_emit_order,
		      sparm->beam_width);
  ybar.length=length;

  rt_viterbi+=MAX(get_runtime()-rt1,0);
/*
  for(i=0;i<length;i++) 
    free(emit[i]);
*/
  free(emitbuf);
  free(emit);
  free(trans);

  if(struct_verbosity >= 4) {
    SVECTOR *pp=psi(x,ybar,sm,sparm);
    SVECTOR *p=add_list_ss(pp);
    SVECTOR *ppy=psi(x,y,sm,sparm);
    SVECTOR *py=add_list_ss(ppy);
    double scorey=sprod_ns(sm->w,py);
    printf("  --> w*Psi(x,y) = %lf\n",scorey);
    double score=sprod_ns(sm->w,p);
    printf("  --> w*Psi(x,ybar) = %lf\n",score);
    for(i=0;i<length;i++) 
      score+=loss_token(y.labels[i],ybar.labels[i]);
    printf("  --> w*Psi(x,ybar)+delta(y,ybar) = %lf\n",score);
    printf("  --> w*(Psi(x,y)-Psi(x,ybar))+delta(y,ybar) = %lf\n",score-scorey);
    free_svector(p);
    free_svector(py);
    free_svector(pp);
    free_svector(ppy);
  }

  return(ybar);
}

int         empty_label(LABEL y)
{
  /* Returns true, if y is an empty label. An empty label might be
     returned by find_most_violated_constraint_???(x, y, sm) if there
     is no incorrect label that can be found for x, or if it is unable
     to label x at all */
  return(0);
}

SVECTOR     *psi(PATTERN x, LABEL y, STRUCTMODEL *sm,
		 STRUCT_LEARN_PARM *sparm)
{
  /* Returns a feature vector describing the match between pattern x
     and label y. The feature vector is returned as a list of
     SVECTOR's. Each SVECTOR is in a sparse representation of pairs
     <featurenumber:featurevalue>, where the last pair has
     featurenumber 0 as a terminator. Featurenumbers start with 1 and
     end with sizePsi. Featuresnumbers that are not specified default
     to value 0. As mentioned before, psi() actually returns a list of
     SVECTOR's. Each SVECTOR has a field 'factor' and 'next'. 'next'
     specifies the next element in the list, terminated by a NULL
     pointer. The list can be though of as a linear combination of
     vectors, where each vector is weighted by its 'factor'. This
     linear combination of feature vectors is multiplied with the
     learned (kernelized) weight vector to score label y for pattern
     x. Without kernels, there will be one weight in sm.w for each
     feature. Note that psi has to match
     find_most_violated_constraint_???(x, y, sm) and vice versa. In
     particular, find_most_violated_constraint_???(x, y, sm) finds
     that ybar!=y that maximizes psi(x,ybar,sm)*sm.w (where * is the
     inner vector product) and the appropriate function of the
     loss + margin/slack rescaling method. See that paper for details. */
  SVECTOR *fvec=NULL,*fshift=NULL;
  long length=x.length;
  long classes=sparm->num_classes;
  long fnum=sparm->num_features;
  long i,back,order;
  double val;
  long emitbase;
  double *fvec_n;
  WORD tfeat[2];

  emitbase=index_max_cum(sparm->hmm_trans_order,classes)+1;

  /* Create transition features */
  tfeat[1].wnum=0;
  tfeat[0].weight=1;
  fvec=NULL;
  if(0) {
    fvec_n=create_nvector(emitbase);
    clear_nvector(fvec_n,emitbase);
    if(sparm->hmm_trans_order>=1)
      for(i=1;i<length;i++) 
	fvec_n[INDEX_T(0,0,y.labels[i-1],y.labels[i],classes)]++;
    if(sparm->hmm_trans_order>=2)
      for(i=2;i<length;i++)
	fvec_n[INDEX_T(0,y.labels[i-2],y.labels[i-1],y.labels[i],classes)]++;
    if(sparm->hmm_trans_order>=3)
      for(i=3;i<length;i++) 
	fvec_n[INDEX_T(y.labels[i-3],y.labels[i-2],y.labels[i-1],y.labels[i],classes)]++;
    fvec=create_svector_n(fvec_n,emitbase,NULL,1.0);
    free(fvec_n);
  }
  if(1) {
    if(sparm->hmm_trans_order>=1) {
      for(i=1;i<length;i++) {
	tfeat[0].wnum=INDEX_T(0,0,y.labels[i-1],y.labels[i],classes);
	fshift=create_svector(tfeat,NULL,1.0);
	append_svector_list(fshift,fvec);
	fvec=fshift;
      }
    }
    if(sparm->hmm_trans_order>=2) {
      for(i=2;i<length;i++) {
	tfeat[0].wnum=INDEX_T(0,y.labels[i-2],y.labels[i-1],y.labels[i],classes);
	fshift=create_svector(tfeat,NULL,1.0);
	append_svector_list(fshift,fvec);
	fvec=fshift;
      }
    }
    if(sparm->hmm_trans_order>=3) {
      for(i=3;i<length;i++) {
	tfeat[0].wnum=INDEX_T(y.labels[i-3],y.labels[i-2],y.labels[i-1],y.labels[i],classes);
	fshift=create_svector(tfeat,NULL,1.0);
	append_svector_list(fshift,fvec);
	fvec=fshift;
      }
    }
  }
  if(0) {
    for(order=1;order<=sparm->hmm_trans_order;order++) 
    for(i=order;i<length;i++) {
      val=0;
      for(back=order;back>=0;back--) {
	val=y.labels[i-back]+classes*val;;
      }
      tfeat[0].wnum=val;
      fshift=create_svector(tfeat,NULL,1.0);
      append_svector_list(fshift,fvec);
      fvec=fshift;
    }
  }

  /* Create emission features */
  for(i=0;i<length;i++) {
    fshift=shift_s(x.tokens[i]->fvec,emitbase+fnum*INDEX_E(0,y.labels[i],classes));
    append_svector_list(fshift,fvec);
    fvec=fshift;
  }
  if(sparm->hmm_emit_order>=1) {
    for(i=1;i<length;i++) {
      fshift=shift_s(x.tokens[i]->fvec,emitbase+fnum*INDEX_E(y.labels[i-1],y.labels[i],classes));
      append_svector_list(fshift,fvec);
      fvec=fshift;
    }
  }

  return(fvec);
}

double      loss(LABEL y, LABEL ybar, STRUCT_LEARN_PARM *sparm)
{
  /* loss for correct label y and predicted label ybar. The loss for
     y==ybar has to be zero. sparm->loss_function is set with the -l option. */
  long i;
  double l;
  if(sparm->loss_function == 0) { /* type 0 loss: 0/1 loss */
                                  /* return 0, if y==ybar. return 1 else */
    printf("ERROR: 0/1 loss function is not supported!\n");
    exit(1);
  }
  else if(sparm->loss_function == 1) {
    l=0;
    for(i=0;i<y.length;i++) 
      l+=loss_token(y.labels[i],ybar.labels[i]);
    return(l);
  }
  else {
    printf("ERROR: Unknown loss function!\n");
    exit(1);
  }
}

int         finalize_iteration(double ceps, int cached_constraint,
			       SAMPLE sample, STRUCTMODEL *sm,
			       CONSTSET cset, double *alpha, 
			       STRUCT_LEARN_PARM *sparm)
{
  /* This function is called just before the end of each cutting plane iteration. ceps is the amount by which the most violated constraint found in the current iteration was violated. cached_constraint is true if the added constraint was constructed from the cache. If the return value is FALSE, then the algorithm is allowed to terminate. If it is TRUE, the algorithm will keep iterating even if the desired precision sparm->epsilon is already reached. */
  return(0);
}

void        print_struct_learning_stats(SAMPLE sample, STRUCTMODEL *sm,
					CONSTSET cset, double *alpha, 
					STRUCT_LEARN_PARM *sparm)
{
  /* This function is called after training and allows final touches to
     the model sm. But primarly it allows computing and printing any
     kind of statistic (e.g. training error) you might want. */
  MODEL *model;

  printf("Viterbi timing: CreateTransEmitMatrix=%lf Viterbi=%lf\n",rt_emit,rt_viterbi);

  /* Replace SV with single weight vector */
  model=sm->svm_model;
  if(model->kernel_parm.kernel_type == LINEAR) {
    if(struct_verbosity>=1) {
      printf("Compacting linear model..."); fflush(stdout);
    }
    sm->svm_model=compact_linear_model(model);
    sm->w=sm->svm_model->lin_weights; /* short cut to weight vector */
    free_model(model,1);
    if(struct_verbosity>=1) {
      printf("done\n"); fflush(stdout);
    }
  }  
}

void        print_struct_testing_stats(SAMPLE sample, STRUCTMODEL *sm,
				       STRUCT_LEARN_PARM *sparm, 
				       STRUCT_TEST_STATS *teststats)
{
  /* This function is called after making all test predictions in
     svm_struct_classify and allows computing and printing any kind of
     evaluation (e.g. precision/recall) you might want. You can use
     the function eval_prediction to accumulate the necessary
     statistics for each prediction. */

  double avg_token_loss = teststats->sum_loss/teststats->num_tokens;
  printf("Average loss per token: %.4lf\n", avg_token_loss);
}

void        eval_prediction(long exnum, EXAMPLE ex, LABEL ypred, 
			    STRUCTMODEL *sm, STRUCT_LEARN_PARM *sparm, 
			    STRUCT_TEST_STATS *teststats)
{
  /* This function allows you to accumlate statistic for how well the
     predicition matches the labeled example. It is called from
     svm_struct_classify. See also the function
     print_struct_testing_stats. */
  long i;

  if(exnum == 0) { /* this is the first time the function is
		      called. So initialize the teststats */
    teststats->num_tokens = 0;
    teststats->sum_loss = 0;
  }
  teststats->num_tokens+=ex.x.length;
  for(i=0;i<ex.x.length;i++)
    teststats->sum_loss+=loss_token(ex.y.labels[i],ypred.labels[i]);
}

void        write_struct_model(char *file, STRUCTMODEL *sm, 
			       STRUCT_LEARN_PARM *sparm)
  /* Writes structural model sm to file file. */
{
  FILE *modelfl;
  long j,i,sv_num;
  SVECTOR *v;
  MODEL *model=sm->svm_model;

  if(verbosity>=1) {
    printf("Writing model file..."); fflush(stdout);
  }
  if ((modelfl = fopen (file, "w")) == NULL)
  { perror (file); exit (1); }
  fprintf(modelfl,"SVM-HMM Version %s\n",INST_VERSION);
  fprintf(modelfl,"%ld # kernel type\n",
	  model->kernel_parm.kernel_type);
  fprintf(modelfl,"%ld # kernel parameter -d \n",
	  model->kernel_parm.poly_degree);
  fprintf(modelfl,"%.8g # kernel parameter -g \n",
	  model->kernel_parm.rbf_gamma);
  fprintf(modelfl,"%.8g # kernel parameter -s \n",
	  model->kernel_parm.coef_lin);
  fprintf(modelfl,"%.8g # kernel parameter -r \n",
	  model->kernel_parm.coef_const);
  fprintf(modelfl,"%s# kernel parameter -u \n",model->kernel_parm.custom);
  fprintf(modelfl,"%ld # highest feature index \n",model->totwords);
  fprintf(modelfl,"%ld # number of emission features \n",sparm->num_features);
  fprintf(modelfl,"%ld # number of classes \n",sparm->num_classes);
  fprintf(modelfl,"%ld # HMM order of transitions \n",sparm->hmm_trans_order);
  fprintf(modelfl,"%ld # HMM order of emissions \n",sparm->hmm_emit_order);
  fprintf(modelfl,"%d # loss function \n",sparm->loss_function);
 
  sv_num=1;
  for(i=1;i<model->sv_num;i++) {
    for(v=model->supvec[i]->fvec;v;v=v->next) 
      sv_num++;
  }
  fprintf(modelfl,"%ld # number of support vectors plus 1 \n",sv_num);
  fprintf(modelfl,"%.8g # threshold b, each following line is a SV (starting with alpha*y)\n",model->b);

  for(i=1;i<model->sv_num;i++) {
    for(v=model->supvec[i]->fvec;v;v=v->next) {
      fprintf(modelfl,"%.32g ",model->alpha[i]*v->factor);
      for (j=0; (v->words[j]).wnum; j++) {
	fprintf(modelfl,"%ld:%.8g ",
		(long)(v->words[j]).wnum,
		(double)(v->words[j]).weight);
      }
      if(v->userdefined)
	fprintf(modelfl,"#%s\n",v->userdefined);
      else
	fprintf(modelfl,"#\n");
    /* NOTE: this could be made more efficient by summing the
       alpha's of identical vectors before writing them to the
       file. */
    }
  }
  fclose(modelfl);
  if(verbosity>=1) {
    printf("done\n");
  }
}

STRUCTMODEL read_struct_model(char *file, STRUCT_LEARN_PARM *sparm)
  /* Reads structural model sm from file file. This function is used
     only in the prediction module, not in the learning module. */
{
  FILE *modelfl;
  long i,queryid,slackid;
  double costfactor;
  long max_sv,max_words,ll,wpos;
  char *line,*comment;
  WORD *words;
  char version_buffer[100];
  MODEL *model;
  STRUCTMODEL sm;

  nol_ll(file,&max_sv,&max_words,&ll); /* scan size of model file */
  max_words+=2;
  ll+=2;

  words = (WORD *)my_malloc(sizeof(WORD)*(max_words+10));
  line = (char *)my_malloc(sizeof(char)*ll);
  model = (MODEL *)my_malloc(sizeof(MODEL));
  sm.svm_model=model;

  if ((modelfl = fopen (file, "r")) == NULL)
  { perror (file); exit (1); }

  fscanf(modelfl,"SVM-HMM Version %s\n",version_buffer);
  if(strcmp(version_buffer,INST_VERSION)) {
    perror ("Version of model-file does not match version of svm_hmm_classify!"); 
    exit (1); 
  }
  fscanf(modelfl,"%ld%*[^\n]\n", &model->kernel_parm.kernel_type);  
  fscanf(modelfl,"%ld%*[^\n]\n", &model->kernel_parm.poly_degree);
  fscanf(modelfl,"%lf%*[^\n]\n", &model->kernel_parm.rbf_gamma);
  fscanf(modelfl,"%lf%*[^\n]\n", &model->kernel_parm.coef_lin);
  fscanf(modelfl,"%lf%*[^\n]\n", &model->kernel_parm.coef_const);
  fscanf(modelfl,"%[^#]%*[^\n]\n", model->kernel_parm.custom);

  fscanf(modelfl,"%ld%*[^\n]\n", &model->totwords);
  sm.sizePsi=model->totwords;
  fscanf(modelfl,"%ld%*[^\n]\n", &sparm->num_features);
  fscanf(modelfl,"%ld%*[^\n]\n", &sparm->num_classes);
  fscanf(modelfl,"%ld%*[^\n]\n", &sparm->hmm_trans_order);
  fscanf(modelfl,"%ld%*[^\n]\n", &sparm->hmm_emit_order);
  fscanf(modelfl,"%d%*[^\n]\n", &sparm->loss_function);
  fscanf(modelfl,"%ld%*[^\n]\n", &model->sv_num);
  fscanf(modelfl,"%lf%*[^\n]\n", &model->b);

  model->supvec = (DOC **)my_malloc(sizeof(DOC *)*model->sv_num);
  model->alpha = (double *)my_malloc(sizeof(double)*model->sv_num);
  model->index=NULL;
  model->lin_weights=NULL;

  for(i=1;i<model->sv_num;i++) {
    fgets(line,(int)ll,modelfl);
    if(!parse_document(line,words,&(model->alpha[i]),&queryid,&slackid,
		       &costfactor,&wpos,max_words,&comment)) {
      printf("\nParsing error while reading model file in SV %ld!\n%s",
	     i,line);
      exit(1);
    }
    model->supvec[i] = create_example(-1,
				      0,0,
				      0.0,
				      create_svector(words,comment,1.0));
  }
  fclose(modelfl);
  free(line);
  free(words);
  if(verbosity>=1) {
    fprintf(stdout, "(%d support vectors read)..",(int)(model->sv_num-1));
  }
  sm.w=model->lin_weights;

  return(sm);
}

void        write_label(FILE *fp, LABEL y)
{
  /* Writes label y to file handle fp. */
  int i;
  for(i=0;i<y.length;i++)
    fprintf(fp,"%ld\n",y.labels[i]);
} 

void        free_pattern(PATTERN x) {
  /* Frees the memory of x. */
  long i;
  for(i=0;i<x.length;i++)
    free_example(x.tokens[i],1);
  free(x.tokens);
}

void        free_label(LABEL y) {
  /* Frees the memory of y. */
  free(y.labels);
}

void        free_struct_model(STRUCTMODEL sm) 
{
  /* Frees the memory of model. */
  /* if(sm.w) free(sm.w); */ /* this is free'd in free_model */
  if(sm.svm_model) free_model(sm.svm_model,1);
  /* add free calls for user defined data here */
}

void        free_struct_sample(SAMPLE s)
{
  /* Frees the memory of sample s. */
  int i;
  for(i=0;i<s.n;i++) { 
    free_pattern(s.examples[i].x);
    free_label(s.examples[i].y);
  }
  free(s.examples);
}

void        print_struct_help()
{
  /* Prints a help text that is appended to the common help text of
     svm_struct_learn. */
  printf("         --t [0..3]  -> Order of dependencies of transitions in HMM (default 1)\n");
  printf("         --e [0..1]  -> Order of dependencies of emissions in HMM (default 0)\n");
  printf("         --b [0..]   -> Width of beam in approximate Viterbi search (0 means\n");
  printf("                        that exact Viterbi is used) (default 0)\n");
  printf("\n");
  printf("Additional Loss Functions:\n");
  printf("         -l [0..]    -> 0: zero/one loss per sequence\n");
  printf("                        1: zero/one loss per token (i.e. Hamming loss)\n");
  printf("                        (default %d)\n",DEFAULT_LOSS_FCT);
}

void         parse_struct_parameters(STRUCT_LEARN_PARM *sparm)
{
  /* Parses the command line parameters that start with -- for the
     learning module */
  int i;

  sparm->hmm_trans_order=1;
  sparm->hmm_emit_order=0;
  sparm->beam_width=0;
  sparm->num_features=0;

  for(i=0;(i<sparm->custom_argc) && ((sparm->custom_argv[i])[0] == '-');i++) {
    switch ((sparm->custom_argv[i])[2]) 
      { 
      case 'e': i++; sparm->hmm_emit_order=atol(sparm->custom_argv[i]); break;
      case 't': i++; sparm->hmm_trans_order=atol(sparm->custom_argv[i]); break;
      case 'b': i++; sparm->beam_width=atol(sparm->custom_argv[i]); break;
      default: printf("\nUnrecognized option %s!\n\n",sparm->custom_argv[i]);
	       exit(0);
      }
  }

  if((sparm->hmm_trans_order<0) || (sparm->hmm_trans_order>3)) {
    printf("ERROR: Invalid order '%ld' of transition depencencies in HMM!\n",sparm->hmm_trans_order);
    exit(1);
  } 
  if((sparm->hmm_emit_order<0) || (sparm->hmm_emit_order>1)) {
    printf("ERROR: Invalid order '%ld' of emission depencencies in HMM!\n",sparm->hmm_trans_order);
    exit(1);
  } 
  if((sparm->hmm_emit_order>sparm->hmm_trans_order) || (sparm->hmm_emit_order<0)) {
    printf("ERROR: Invalid order '%ld' of emission dependencies in HMM! Order must be\n",sparm->hmm_emit_order);
    printf("       between 0 and the order of transition dependencies.\n");
    exit(0);
  } 
  if(sparm->beam_width<0) {
    printf("ERROR: Invalid width '%ld' of beam for approximate Viterbi search!\n",sparm->beam_width);
    exit(1);
  } 
}

void        print_struct_help_classify()
{
  /* Prints a help text that is appended to the common help text of
     svm_struct_classify. */
  printf("         --b [0..]   -> Width of beam in approximate Viterbi search (0 means\n");
  printf("                        that exact Viterbi is used) (default 0)\n");
}

void         parse_struct_parameters_classify(STRUCT_LEARN_PARM *sparm)
{
  /* Parses the command line parameters that start with -- for the
     classification module */
  int i;

  sparm->beam_width=0;

  for(i=0;(i<sparm->custom_argc) && ((sparm->custom_argv[i])[0] == '-');i++) {
    switch ((sparm->custom_argv[i])[2]) 
      { 
      /* case 'x': i++; strcpy(xvalue,sparm->custom_argv[i]); break; */
      case 'b': i++; sparm->beam_width=atol(sparm->custom_argv[i]); break;
      default: printf("\nUnrecognized option %s!\n\n",sparm->custom_argv[i]);
	       exit(0);
      }
  }
  if(sparm->beam_width<0) {
    printf("ERROR: Invalid width '%ld' of beam for approximate Viterbi search!\n",sparm->beam_width);
    exit(1);
  } 
}

/***************************************************************************/

long *viterbi(double *trans, double **emit, long length, long classes, 
	      long trans_order, long emit_order, long beam_width)  
     /* Compute viterbi path for given transition and emission scores */
{
  VPATH  **path,*best,*pathbuf;
  long   *bestpath;
  long   location;
  long   i;

  if(length==0) {
    return(NULL);
  }

  path=(VPATH **)my_malloc(sizeof(VPATH *)*length);
  pathbuf=(VPATH *)my_malloc(sizeof(VPATH)*length
			     *(index_max(MAX(0,trans_order-1),classes)+2));
  for(i=0;i<length;i++) 
    path[i]=pathbuf+i*(index_max(MAX(0,trans_order-1),classes)+2);

  /* Do forward pass of Viterbi. */
  if((beam_width<=0) || (trans_order==0)) {
    if(0) 
      best=viterbi_forward(trans,emit,trans_order,emit_order,0,length-1,classes,path);
    else if((length==1) || (trans_order==0)) 
      best=viterbi_forward_order0(trans,emit,emit_order,0,length-1,classes,path);
    else if((length==2) || (trans_order==1)) 
      best=viterbi_forward_order1(trans,emit,emit_order,0,length-1,classes,path);
    else if((length==3) || (trans_order==2))
      best=viterbi_forward_order2(trans,emit,emit_order,0,length-1,classes,path);
    else if((length==4) || (trans_order==3))
      best=viterbi_forward_order3(trans,emit,emit_order,0,length-1,classes,path);
    else /* use generic viterbi */
      best=viterbi_forward(trans,emit,trans_order,emit_order,0,length-1,classes,path);
  }
  else { /* use beam search viterbi */
    best=viterbi_forward_beam(trans,emit,beam_width,trans_order,emit_order,0,length-1,classes,path);
  }

  if(struct_verbosity >= 4) 
    printf(" -> Viterbi %lf\n",best->score);

  /* Trace back the highest scoring path. */
  bestpath=(long *)my_malloc(sizeof(long)*length);
  for(location=length-1;location>=0;location--) {
    bestpath[location]=best->label;
    best=best->backpointer;
  }
  /*
  for(i=0;i<length;i++) {
    free(path[i]);
  }
  */
  free(pathbuf);
  free(path);

  return(bestpath);
}

long index_max(long order, long classes) 
     /* computes the number of states in each location of viterbi path
	(minus 1) */
{
  if(order>0) 
    return((classes-1)+(classes)*index_max(order-1,classes));
  else if(order==0)
    return(classes-1);
  else 
    return(0);
}

long index_max_cum(long order, long classes) 
     /* computes the largest index for emission and transition features */
{
  if(order>0) 
    return((classes)+(classes)*index_max_cum(order-1,classes));
  else if(order==0)
    return(classes);
  else 
    return(0);
}

VPATH *viterbi_forward_order0(double *trans, double **emit, long emit_order,
			      long start, long end, long classes, VPATH **path)
     /* Compute order 0 forward pass of viterbi for given transition 
	and emission scores */
{
  double smax;
  long   location,ci;
  VPATH  *current,*backpointer,*new_backpointer;

  backpointer=NULL;
  new_backpointer=NULL;
  smax=0;
  for(ci=1;ci<=classes;ci++) {
    current=&path[start][INDEX_P0((ci-1),classes)];
    current->label=ci;
    current->score=emit[start][INDEX_E0(ci,classes)];
    current->backpointer=backpointer;
    if((smax<current->score) || (!new_backpointer)) {
      new_backpointer=current;
      smax=current->score;
    }
  }
  for(location=start+1;location<=end;location++) {
    backpointer=new_backpointer;
    new_backpointer=NULL;
    smax=0;
    for(ci=1;ci<=classes;ci++) {
      current=&path[location][INDEX_P0((ci-1),classes)];
      current->label=ci;
      current->score=emit[location][INDEX_E0(ci,classes)];
      current->backpointer=backpointer;
      if((smax<current->score) || (!new_backpointer)) {
	new_backpointer=current;
	smax=current->score;
      }
    }
  }
  return(new_backpointer);
}

VPATH *viterbi_forward_order1(double *trans, double **emit, long emit_order,
			      long start, long end, long classes, VPATH **path)
     /* Compute order 1 forward pass of viterbi for given transition
	and emission scores. Assumes that the sequence is at least 2
	tokens long. */
{
  double smax,s;
  long   p;
  int    lastpos;
  long   location,ci;
  double *emit_location,*emit_ptr,*trans_ptr;;
  VPATH  *current,*backpointer,*path_end=NULL,*path_location;

  for(ci=1;ci<=classes;ci++) {
    current=&path[start][INDEX_P0((ci-1),classes)];
    current->label=ci;
    current->score=emit[start][INDEX_E0(ci,classes)];
    current->backpointer=NULL;
  }

  smax=0;
  for(location=start+1;location<=end;location++) {
    emit_location=emit[location];
    path_location=path[location];
    backpointer=path[location-1];
    lastpos=(location==end);
    for(p=1;p<=classes;p++) {
      /* backpointer=&path[location-1][INDEX_P0((p-1),classes)]; */
      current=path_location;
      if(emit_order==0)
	emit_ptr=emit_location;
      else 
	emit_ptr=&emit_location[INDEX_E1(p,0,classes)];
      trans_ptr=&trans[INDEX_T1(p,0,classes)];
      for(ci=1;ci<=classes;ci++) {
	s=backpointer->score+(*(++trans_ptr))+(*(++emit_ptr));
	/* equivalent code that is easier to read
	s=backpointer->score
	  +trans[INDEX_T1(p,ci,classes)];
	if(emit_order==0)
	  s+=emit_location[INDEX_E0(ci,classes)];
	else
	  s+=emit_location[INDEX_E1(p,ci,classes)];
	current=&path_location[INDEX_P0((ci-1),classes)];
	*/
	if((p==1) || (current->score<s)) {
	  current->label=ci;
	  current->backpointer=backpointer;
	  current->score=s;
	  if((lastpos) && ((smax<s) || (!path_end))) {
	    path_end=current;
	    smax=current->score;
	  }
	}
	current++;
      }
      backpointer++;
    }
  }
  return(path_end);
}

VPATH *viterbi_forward_order2(double *trans, double **emit, long emit_order,
			      long start, long end, long classes, VPATH **path)  
     /* Compute order 2 forward pass of viterbi for given transition 
	and emission scores. Assumes that the sequence is at least 3
	tokens long. */
{
  double smax,s;
  long   p;
  int    lastpos;
  long   location,ci,cii;
  double emitii;
  double *emit_location,*emit_ptr,*trans_ptr;
  VPATH  *current,*backpointer,*path_end=NULL,*path_location;

  for(cii=1;cii<=classes;cii++) {
    emitii=emit[start][INDEX_E0(cii,classes)];
    current=&path[start][INDEX_P0((cii-1),classes)];
    current->label=cii;
    current->score=emitii;
    current->backpointer=NULL;
    backpointer=current;
    for(ci=1;ci<=classes;ci++) {
      current=&path[start+1][INDEX_P1((cii-1),(ci-1),classes)];
      current->label=ci;
      current->score=trans[INDEX_T1(cii,ci,classes)]+emitii;
      if(emit_order==0)
	current->score+=emit[start+1][INDEX_E0(ci,classes)];
      else
	current->score+=emit[start+1][INDEX_E1(cii,ci,classes)];
      current->backpointer=backpointer;      
    }
  }

  smax=0;
  for(location=start+2;location<=end;location++) {
    emit_location=emit[location];
    path_location=path[location];
    backpointer=path[location-1];
    lastpos=(location==end);
    for(p=1;p<=classes;p++) {
      current=path_location;
      for(cii=1;cii<=classes;cii++) {
	/* backpointer=&path[location-1][INDEX_P1((p-1),(cii-1),classes)]; */
	if(emit_order==0)
	  emit_ptr=emit_location;
	else if(emit_order==1)
	  emit_ptr=&emit_location[INDEX_E1(cii,0,classes)];
	else
	  emit_ptr=&emit_location[INDEX_E2(p,cii,0,classes)];
	trans_ptr=&trans[INDEX_T2(p,cii,0,classes)];
	for(ci=1;ci<=classes;ci++) {
	  s=backpointer->score+(*(++trans_ptr))+(*(++emit_ptr));
	  /* equivalent code that is easier to read
	  s=backpointer->score
	    +trans[INDEX_T2(p,cii,ci,classes)];
	  if(emit_order==0)
	    s+=emit_location[INDEX_E0(ci,classes)];
	  else if(emit_order==1)
	    s+=emit_location[INDEX_E1(cii,ci,classes)];
	  else 
	    s+=emit_location[INDEX_E2(p,cii,ci,classes)];
	  current=&path[location][INDEX_P1((cii-1),(ci-1),classes)]; 
	  */
	  if((p==1) || (current->score<s)) {
	    current->label=ci;
	    current->backpointer=backpointer;
	    current->score=s;
	    if((lastpos) && ((smax<s) || (!path_end))) {
	      path_end=current;
	      smax=current->score;
	    }
	  }
	  current++;
	}
        backpointer++;
      }
    }
  }
  return(path_end);
}

VPATH *viterbi_forward_order3(double *trans, double **emit, long emit_order,
			      long start, long end, long classes, VPATH **path)  
     /* Compute order 3 forward pass of viterbi for given transition 
	and emission scores. Assumes that the sequence is at least 4
	tokens long. */
{
  double smax,s;
  long   p;
  int    lastpos;
  long   location,ci,cii,ciii;
  double *emit_location,*emit_ptr,*trans_ptr;
  VPATH  *backpointer,*backpointer_ii,*backpointer_iii;
  VPATH  *current,*path_end=NULL,*path_location;

  for(ciii=1;ciii<=classes;ciii++) {
    current=&path[start][INDEX_P0((ciii-1),classes)];
    current->label=ciii;
    current->score=emit[start][INDEX_E0(ciii,classes)];
    current->backpointer=NULL;
    backpointer_iii=current;
    for(cii=1;cii<=classes;cii++) {
      current=&path[start+1][INDEX_P1((ciii-1),(cii-1),classes)];
      current->label=cii;
      current->score=backpointer_iii->score
	             +trans[INDEX_T1(ciii,cii,classes)];
      if(emit_order==0)
	current->score+=emit[start+1][INDEX_E0(cii,classes)];
      else
	current->score+=emit[start+1][INDEX_E1(ciii,cii,classes)];
      current->backpointer=backpointer_iii;
      backpointer_ii=current;
      for(ci=1;ci<=classes;ci++) {
	current=&path[start+2][INDEX_P2((ciii-1),(cii-1),(ci-1),classes)];
	current->label=ci;
	current->score=backpointer_ii->score
                       +trans[INDEX_T2(ciii,cii,ci,classes)];
	if(emit_order==0)
	  current->score+=emit[start+2][INDEX_E0(ci,classes)];
	else if(emit_order==1)
	  current->score+=emit[start+2][INDEX_E1(cii,ci,classes)];
	else
	  current->score+=emit[start+2][INDEX_E2(ciii,cii,ci,classes)];
	current->backpointer=backpointer_ii;      
      }
    }
  }

  smax=0;
  for(location=start+3;location<=end;location++) {
    emit_location=emit[location];
    path_location=path[location];
    backpointer=path[location-1];
    lastpos=(location==end);
    for(p=1;p<=classes;p++) {
      current=path_location;
      for(ciii=1;ciii<=classes;ciii++) {
	for(cii=1;cii<=classes;cii++) {
	  /* backpointer=&path[location-1][INDEX_P2((p-1),(ciii-1),(cii-1),classes)]; */
	  if(emit_order==0)
	    emit_ptr=emit_location;
	  else if(emit_order==1)
	    emit_ptr=&emit_location[INDEX_E1(cii,0,classes)];
	  else if(emit_order==2)
	    emit_ptr=&emit_location[INDEX_E2(ciii,cii,0,classes)];
	  else
	    emit_ptr=&emit_location[INDEX_E3(p,ciii,cii,0,classes)];
	  trans_ptr=&trans[INDEX_T3(p,ciii,cii,0,classes)];
	  for(ci=1;ci<=classes;ci++) {
	    s=backpointer->score+(*(++trans_ptr))+(*(++emit_ptr));
	    /* equivalent code that is easier to read
	    s=backpointer->score
	      +trans[INDEX_T3(p,ciii,cii,ci,classes)];
	    if(emit_order==0)
	      s+=emit_location[INDEX_E0(ci,classes)];
	    else if(emit_order==1)
	      s+=emit_location[INDEX_E1(cii,ci,classes)];
	    else if(emit_order==2)
	      s+=emit_location[INDEX_E2(ciii,cii,ci,classes)];
	    else 
	      s+=emit_location[INDEX_E3(p,ciii,cii,ci,classes)];
	    current=&path_location[INDEX_P2((ciii-1),(cii-1),(ci-1),classes)];
	    */
	    if((p==1) || (current->score<s)) {
	      current->label=ci;
	      current->backpointer=backpointer;
	      current->score=s;
	      if((lastpos) && ((smax<s) || (!path_end))) {
		path_end=current;
		smax=current->score;
	      }
	    }
	    current++;
	  }
	  backpointer++;
	}
      }
    }
  }
  return(path_end);
}

VPATH *viterbi_forward(double *trans, double **emit, 
		       long trans_order, long emit_order,
		       long start, long end, long classes, VPATH **path)  
     /* Compute order trans_order forward pass of viterbi for given transition 
	and emission scores. Works only for orders >=1. */
{
  double smax,s;
  long   o,order_current,order_curemit,order_curtrans;
  double *emit_location;
  long   location,ci;
  VPATH  *backpointer,*path_location;
  VPATH  *current,*path_end=NULL;
  long   index,index_m,index_current,index_trans;
  long   *power,*powersum;

  if(start>end) return(NULL); /* empty sequence */

  power=(long *)my_malloc(sizeof(long)*(trans_order+1));
  powersum=(long *)my_malloc(sizeof(long)*(trans_order+1));
  power[0]=classes;
  powersum[0]=1;
  for(o=1;o<=trans_order;o++) {
    powersum[o]=powersum[o-1]+power[o-1];
    power[o]=power[o-1]*classes;
  }

  smax=0;
  for(ci=1;ci<=classes;ci++) {
    current=&path[start][ci-1];
    current->label=ci;
    current->index=ci-1;
    current->backpointer=NULL;
    s=emit[start][ci];
    current->score=s;
    if((start==end) && ((smax<s) || (!path_end))) {
      path_end=current;
      smax=current->score;
    }
  }
  current=&path[start][classes];
  current->label=0;

  for(location=start+1;location<=end;location++) {
    order_current=MIN(location,trans_order-1);
    order_curtrans=MIN(location,trans_order);
    order_curemit=MIN(location,emit_order);
    path_location=path[location];
    emit_location=emit[location];

    index_m=index_max(trans_order-1,classes);
    current=path_location;
    for(index=0;index<=index_m+1;index++) {
      current->backpointer=NULL;
      current->label=0;
      current++;
    }

    for(backpointer=&path[location-1][0];backpointer->label;backpointer++) {
      index_trans=(backpointer->index*classes);
      index_current=index_trans % power[order_current];
      current=&path_location[index_current];
      for(ci=1;ci<=classes;ci++) {
	s=backpointer->score;
	if(emit_order==0)
	  s+=emit_location[ci];
	else
	  s+=emit_location[index_trans % power[order_curemit]+powersum[order_curemit]];
        s+=trans[index_trans+powersum[order_curtrans]]; 
	index_trans++;
	if((!current->backpointer) || (current->score<s)) {
	  current->backpointer=backpointer;
	  current->score=s;
	  current->label=ci;
	  current->index=index_current;
	  if((location==end) && ((smax<s) || (!path_end))) {
	    path_end=current;
	    smax=current->score;
	  }
	}
	index_current++;
	current++;
      }
    }
  }
  free(powersum);
  free(power);
  return(path_end);
}

BEAM *beam_init(long beam_width) 
{
  BEAM *beam=(BEAM *)my_malloc(sizeof(BEAM));
  beam->beam_width=beam_width;
  beam->num=0;
  beam->elem=(VPATH **)my_malloc(sizeof(VPATH *)*beam_width);
  return(beam);
}

void beam_add(BEAM *beam, VPATH *new) 
{
  long i;
  double worst_score;

  if(beam->num==0) {
    beam->elem[beam->num]=new;
    beam->num++;
    beam->worst=0;
  }
  else if(beam->num<beam->beam_width) {
    for(i=0;(i<beam->num) && (beam->elem[i]->index != new->index);i++);
    if(i==beam->num) {
      beam->elem[beam->num]=new;
      beam->num++;
    }
    worst_score=beam->elem[beam->worst]->score;
    for(i=0;i<beam->num;i++)
      if(worst_score>beam->elem[i]->score) {
	beam->worst=i;
	worst_score=beam->elem[i]->score;
      }
  }
  else if(new->score > beam->elem[beam->worst]->score) {
    for(i=0;(i<beam->num) && (beam->elem[i]->index != new->index);i++);
    if(i==beam->num) {
      beam->elem[beam->worst]=new;
    }
    worst_score=beam->elem[beam->worst]->score;
    for(i=0;i<beam->num;i++)
      if(worst_score>beam->elem[i]->score) {
	beam->worst=i;
	worst_score=beam->elem[i]->score;
      }
  }
}

void beam_free(BEAM *beam) 
{
  free(beam->elem);
  free(beam);
}

VPATH *viterbi_forward_beam(double *trans, double **emit, long beam_width,
		       long trans_order, long emit_order,
		       long start, long end, long classes, VPATH **path)  
     /* Compute order emit_order forward pass of viterbi with beam
	search of width beam_width for given transition and emission
	scores. */
{
  double smax,s;
  long   o,order_current,order_curemit,order_curtrans;
  double *emit_location;
  long   location,ci;
  VPATH  *backpointer,*path_location;
  VPATH  *current,*path_end=NULL;
  long   index,index_current,index_trans;
  long   *power,*powersum;
  BEAM   *beam,*beam_new;

  if(start>end) return(NULL); /* empty sequence */

  power=(long *)my_malloc(sizeof(long)*(trans_order+1));
  powersum=(long *)my_malloc(sizeof(long)*(trans_order+1));
  power[0]=classes;
  powersum[0]=1;
  for(o=1;o<=trans_order;o++) {
    powersum[o]=powersum[o-1]+power[o-1];
    power[o]=power[o-1]*classes;
  }

  beam=beam_init(beam_width);
  smax=0;
  for(ci=1;ci<=classes;ci++) {
    current=&path[start][ci-1];
    current->label=ci;
    current->index=ci-1;
    current->backpointer=NULL;
    s=emit[start][ci];
    current->score=s;
    beam_add(beam,current);
    if((start==end) && ((smax<s) || (!path_end))) {
      path_end=current;
      smax=current->score;
    }
  }

  for(location=start+1;location<=end;location++) {
    order_current=MIN(location,trans_order-1);
    order_curtrans=MIN(location,trans_order);
    order_curemit=MIN(location,emit_order);
    path_location=path[location];
    emit_location=emit[location];

    /* initialize all fields that are reachable with current beam */
    for(index=0;index<beam->num;index++) {
      index_current=(beam->elem[index]->index*classes) % power[order_current];
      for(ci=1;ci<=classes;ci++) {
	path_location[index_current].backpointer=NULL;
	index_current++;
      }
    }

    beam_new=beam_init(beam_width);

    for(index=0;index<beam->num;index++) {
      backpointer=beam->elem[index];
      index_trans=(backpointer->index*classes);
      index_current=index_trans % power[order_current];
      current=&path_location[index_current];
      for(ci=1;ci<=classes;ci++) {
	s=backpointer->score;
	if(emit_order==0)
	  s+=emit_location[ci];
	else
	  s+=emit_location[index_trans % power[order_curemit]+powersum[order_curemit]];
        s+=trans[index_trans+powersum[order_curtrans]]; 
	index_trans++;
	if((!current->backpointer) || (current->score<s)) {
	  current->backpointer=backpointer;
	  current->score=s;
	  current->label=ci;
	  current->index=index_current;
	  beam_add(beam_new,current);
	  if((location==end) && ((smax<s) || (!path_end))) {
	    path_end=current;
	    smax=current->score;
	  }
	}
	index_current++;
	current++;
      }
    }
    beam_free(beam);
    beam=beam_new;
  }
  beam_free(beam);
  free(powersum);
  free(power);
  return(path_end);
}

double loss_token(long y, long ybar)
     /* Computes the per label loss */
{
  if(y==ybar)
    return(0);
  else
    return(1);
}


/****************************** old stuff ****************************/

long index_min(long order, long classes);
long index_inc(long index, long classes);
long index_suborder(long index, long suborder, long classes);
long index_shift_left(long index, long shiftorder, long classes);
long index_shift_right(long index, long shiftorder, long classes);
long index_print(long index, long classes);

long index_min(long order, long classes) 
{
  return(0);
}

long index_inc(long index, long classes) 
{
  return(index+1);
}

long index_suborder(long index, long suborder, long classes) 
{
  return((index) % (long)(pow(classes,suborder+1))); 
}

long index_shift_left(long index, long shiftorder, long classes)
{
  if(shiftorder==1) 
    return((index)*(classes));
  else if(shiftorder==0) 
    return(index);
  else
    return(index_shift_left(index,shiftorder-1,classes)*(classes)); 
}

long index_shift_right(long index, long shiftorder, long classes)
{
  if(shiftorder==1) 
    return((index)/(classes));
  else if(shiftorder==0) 
    return(index);
  else
    exit(1);

    return(index_shift_left(index,shiftorder-1,classes)/(classes)); 
}

long index_print(long index, long classes)
{
  long digit;
  long verify=0;
  if(index>0) {
    digit=((index) % classes);
    verify=digit+classes*index_print((index)/classes,classes);
    printf("%ld:",digit);
  }
  return(verify);
}

long index_setorder(long index, long order, long classes) 
{
  if(order>=0)
    return(pow(classes,order)+index_setorder(index,order-1,classes));
  return(index);
}
