/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and The XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: utility for paring low hit count cells from lattice codebook
 last mod: $Id: latticepare.c,v 1.3 2000/06/14 01:38:32 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "vorbis/codebook.h"
#include "../lib/sharedbook.h"
#include "../lib/scales.h"
#include "bookutil.h"
#include "vqgen.h"
#include "vqsplit.h"

/* Lattice codebooks have two strengths: important fetaures that are
   poorly modelled by global error minimization training (eg, strong
   peaks) are not neglected 2) compact quantized representation.

   A fully populated lattice codebook, however, swings point 1 too far
   in the opposite direction; rare features need not be modelled quite
   so religiously and as such, we waste bits unless we eliminate the
   least common cells.  The codebook rep supports unused cells, so we
   need to tag such cells and build an auxiliary (non-thresh) search
   mechanism to find the proper match quickly */

/* two basic steps; first is pare the cell for which dispersal creates
   the least additional error.  This will naturally choose
   low-population cells and cells that have not taken on points from
   neighboring paring (but does not result in the lattice collapsing
   inward and leaving low population ares totally unmodelled).  After
   paring has removed the desired number of cells, we need to build an
   auxiliary search for each culled point */

/* Although lattice books (due to threshhold-based matching) do not
   actually use error to make cell selections (in fact, it need not
   bear any relation), the 'secondbest' entry finder here is in fact
   error/distance based, so latticepare is only useful on such books */

/* command line:
   latticepare latticebook.vqh input_data.vqd <target_cells>

   produces a new output book on stdout 
*/

static double _dist(int el,double *a, double *b){
  int i;
  double acc=0.;
  for(i=0;i<el;i++){
    double val=(a[i]-b[i]);
    acc+=val*val;
  }
  return(acc);
}

static double *pointlist;
static long points=0;

void add_vector(codebook *b,double *vec,long n){
  int dim=b->dim,i,j;
  int step=n/dim;
  for(i=0;i<step;i++){
    for(j=i;j<n;j+=step){
      pointlist[points++]=vec[j];
    }
  }
}

static int bestm(codebook *b,double *vec){
  encode_aux_threshmatch *tt=b->c->thresh_tree;
  int dim=b->dim;
  int i,k,o;
  int best=0;
  
  /* what would be the closest match if the codebook was fully
     populated? */
  
  for(k=0,o=dim-1;k<dim;k++,o--){
    int i;
    for(i=0;i<tt->threshvals-1;i++)
      if(vec[o]<tt->quantthresh[i])break;
    best=(best*tt->quantvals)+tt->quantmap[i];
  }
  return(best);
}

static int closest(codebook *b,double *vec,int current){
  encode_aux_threshmatch *tt=b->c->thresh_tree;
  int dim=b->dim;
  int i,k,o;

  double bestmetric=0;
  int bestentry=-1;
  int best=bestm(b,vec);

  if(current<0 && b->c->lengthlist[best]>0)return best;

  for(i=0;i<b->entries;i++){
    if(b->c->lengthlist[i]>0 && i!=best && i!=current){
      double thismetric=_dist(dim, vec, b->valuelist+i*dim);
      if(bestentry==-1 || thismetric<bestmetric){
	bestentry=i;
	bestmetric=thismetric;
      }
    }
  }

  return(bestentry);
}

static double _heuristic(codebook *b,double *ppt,int secondbest){
  double *secondcell=b->valuelist+secondbest*b->dim;
  int best=bestm(b,ppt);
  double *firstcell=b->valuelist+best*b->dim;
  double error=_dist(b->dim,firstcell,secondcell);
  double *zero=alloca(b->dim*sizeof(double));
  double fromzero;
  
  memset(zero,0,b->dim*sizeof(double));
  fromzero=sqrt(_dist(b->dim,firstcell,zero));

  return(error/fromzero);
}

static int longsort(const void *a, const void *b){
  return **(long **)b-**(long **)a;
}

void usage(void){
  fprintf(stderr,"Ogg/Vorbis lattice codebook paring utility\n\n"
	  "usage: latticepare book.vqh data.vqd <target_cells> <protected_cells> base\n"
	  "where <target_cells> is the desired number of final cells (or -1\n"
	  "                     for no change)\n"
	  "      <protected_cells> is the number of highest-hit count cells\n"
	  "                     to protect from dispersal\n"
	  "      base is the base name (not including .vqh) of the new\n"
	  "                     book\n\n");
  exit(1);
}

int main(int argc,char *argv[]){
  char *basename;
  codebook *b=NULL;
  int entries=0;
  int dim=0;
  long i,j,target=-1,protect=-1;
  FILE *out=NULL;

  int argnum=0;

  argv++;
  if(*argv==NULL){
    usage();
    exit(1);
  }

  while(*argv){
    if(*argv[0]=='-'){

      argv++;
	
    }else{
      switch (argnum++){
      case 0:case 1:
	{
	  /* yes, this is evil.  However, it's very convenient to parse file
	     extentions */
	  
	  /* input file.  What kind? */
	  char *dot;
	  char *ext=NULL;
	  char *name=strdup(*argv++);
	  dot=strrchr(name,'.');
	  if(dot)
	    ext=dot+1;
	  else{
	    ext="";
	    
	  }
	  
	  
	  /* codebook */
	  if(!strcmp(ext,"vqh")){
	    
	    basename=strrchr(name,'/');
	    if(basename)
	      basename=strdup(basename)+1;
	    else
	      basename=strdup(name);
	    dot=strrchr(basename,'.');
	    if(dot)*dot='\0';
	    
	    b=codebook_load(name);
	    dim=b->dim;
	    entries=b->entries;
	  }
	  
	  /* data file; we do actually need to suck it into memory */
	  /* we're dealing with just one book, so we can de-interleave */ 
	  if(!strcmp(ext,"vqd") && !points){
	    int cols;
	    long lines=0;
	    char *line;
	    double *vec;
	    FILE *in=fopen(name,"r");
	    if(!in){
	      fprintf(stderr,"Could not open input file %s\n",name);
	      exit(1);
	    }
	    
	    reset_next_value();
	    line=setup_line(in);
	    /* count cols before we start reading */
	    {
	      char *temp=line;
	      while(*temp==' ')temp++;
	      for(cols=0;*temp;cols++){
		while(*temp>32)temp++;
		while(*temp==' ')temp++;
	      }
	    }
	    vec=alloca(cols*sizeof(double));
	    /* count, then load, to avoid fragmenting the hell out of
	       memory */
	    while(line){
	      lines++;
	      for(j=0;j<cols;j++)
		if(get_line_value(in,vec+j)){
		  fprintf(stderr,"Too few columns on line %ld in data file\n",lines);
		  exit(1);
		}
	      if((lines&0xff)==0)spinnit("counting samples...",lines*cols);
	      line=setup_line(in);
	    }
	    pointlist=malloc((cols*lines+entries*dim)*sizeof(double));
	    
	    rewind(in);
	    line=setup_line(in);
	    while(line){
	      lines--;
	      for(j=0;j<cols;j++)
		if(get_line_value(in,vec+j)){
		  fprintf(stderr,"Too few columns on line %ld in data file\n",lines);
		  exit(1);
		}
	      /* deinterleave, add to heap */
	      add_vector(b,vec,cols);
	      if((lines&0xff)==0)spinnit("loading samples...",lines*cols);
	      
	      line=setup_line(in);
	    }
	    fclose(in);
	  }
	}
	break;
      case 2:
	target=atol(*argv++);
	if(target==0)target=entries;
	break;
      case 3:
	protect=atol(*argv++);
	break;
      case 4:
	{
	  char *buff=alloca(strlen(*argv)+5);
	  sprintf(buff,"%s.vqh",*argv);
	  basename=*argv++;

	  out=fopen(buff,"w");
	  if(!out){
	    fprintf(stderr,"unable ot open %s for output",buff);
	    exit(1);
	  }
	}
	break;
      default:
	usage();
      }
    }
  }
  if(!entries || !points || !out)usage();
  if(target==-1)usage();

  /* add guard points */
  for(i=0;i<entries;i++)
    for(j=0;j<dim;j++)
      pointlist[points++]=b->valuelist[i*dim+j];
  
  points/=dim;

  /* set up auxiliary vectors for error tracking */
  {
    encode_aux_nearestmatch *nt=NULL;
    long pointssofar=0;
    long *pointindex;
    long indexedpoints=0;
    long *entryindex;
    long *reventry;
    long *membership=malloc(points*sizeof(long));
    long *firsthead=malloc(entries*sizeof(long));
    long *secondary=malloc(points*sizeof(long));
    long *secondhead=malloc(entries*sizeof(long));

    long *cellcount=calloc(entries,sizeof(long));
    long *cellcount2=calloc(entries,sizeof(long));
    double *cellerror=calloc(entries,sizeof(double));
    double *cellerrormax=calloc(entries,sizeof(double));
    long cellsleft=entries;
    for(i=0;i<points;i++)membership[i]=-1;
    for(i=0;i<entries;i++)firsthead[i]=-1;
    for(i=0;i<points;i++)secondary[i]=-1;
    for(i=0;i<entries;i++)secondhead[i]=-1;

    for(i=0;i<points;i++){
      /* assign vectors to the nearest cell.  Also keep track of second
	 nearest for error statistics */
      double *ppt=pointlist+i*dim;
      int    firstentry=closest(b,ppt,-1);
      int    secondentry=closest(b,ppt,firstentry);
      double firstmetric=_dist(dim,b->valuelist+dim*firstentry,ppt);
      double secondmetric=_dist(dim,b->valuelist+dim*secondentry,ppt);
      
      if(!(i&0xff))spinnit("initializing... ",points-i);
    
      membership[i]=firsthead[firstentry];
      firsthead[firstentry]=i;
      secondary[i]=secondhead[secondentry];
      secondhead[secondentry]=i;

      if(i<points-entries){
	cellerror[firstentry]+=secondmetric-firstmetric;
	cellerrormax[firstentry]=max(cellerrormax[firstentry],
				     _heuristic(b,ppt,secondentry));
	cellcount[firstentry]++;
	cellcount2[secondentry]++;
      }
    }

    /* which cells are most heavily populated?  Protect as many from
       dispersal as the user has requested */
    {
      long **countindex=calloc(entries,sizeof(long *));
      for(i=0;i<entries;i++)countindex[i]=cellcount+i;
      qsort(countindex,entries,sizeof(long *),longsort);
      for(i=0;i<protect;i++){
	int ptr=countindex[i]-cellcount;
	cellerrormax[ptr]=9e50;
      }
    }

    {
      fprintf(stderr,"\r");
      for(i=0;i<entries;i++){
	/* decompose index */
	int entry=i;
	for(j=0;j<dim;j++){
	  fprintf(stderr,"%d:",entry%b->c->thresh_tree->quantvals);
	  entry/=b->c->thresh_tree->quantvals;
	}
	
	fprintf(stderr,":%ld/%ld, ",cellcount[i],cellcount2[i]);
      }
      fprintf(stderr,"\n");
    }

    /* do the automatic cull request */
    while(cellsleft>target){
      int bestcell=-1;
      double besterror=0;
      double besterror2=0;
      long head=-1;
      char spinbuf[80];
      sprintf(spinbuf,"cells left to eliminate: %ld : ",cellsleft-target);

      /* find the cell with lowest removal impact */
      for(i=0;i<entries;i++){
	if(b->c->lengthlist[i]>0){
	  if(bestcell==-1 || cellerrormax[i]<=besterror2){
	    if(bestcell==-1 || cellerrormax[i]<besterror2 || 
	       besterror>cellerror[i]){
	      besterror=cellerror[i];
	      besterror2=cellerrormax[i];
	      bestcell=i;
	    }
	  }
	}
      }

      fprintf(stderr,"\reliminating cell %d                              \n"
	      "     dispersal error of %g max/%g total (%ld hits)\n",
	      bestcell,besterror2,besterror,cellcount[bestcell]);

      /* disperse it.  move each point out, adding it (properly) to
         the second best */
      b->c->lengthlist[bestcell]=0;
      head=firsthead[bestcell];
      firsthead[bestcell]=-1;
      while(head!=-1){
	/* head is a point number */
	double *ppt=pointlist+head*dim;
	int firstentry=closest(b,ppt,-1);
	int secondentry=closest(b,ppt,firstentry);
	double firstmetric=_dist(dim,b->valuelist+dim*firstentry,ppt);
	double secondmetric=_dist(dim,b->valuelist+dim*secondentry,ppt);
	long next=membership[head];

	if(head<points-entries){
	  cellcount[firstentry]++;
	  cellcount[bestcell]--;
	  cellerror[firstentry]+=secondmetric-firstmetric;
	  cellerrormax[firstentry]=max(cellerrormax[firstentry],
				       _heuristic(b,ppt,secondentry));
	}

	membership[head]=firsthead[firstentry];
	firsthead[firstentry]=head;
	head=next;
	if(cellcount[bestcell]%128==0)
	  spinnit(spinbuf,cellcount[bestcell]+cellcount2[bestcell]);

      }

      /* now see that all points that had the dispersed cell as second
         choice have second choice reassigned */
      head=secondhead[bestcell];
      secondhead[bestcell]=-1;
      while(head!=-1){
	double *ppt=pointlist+head*dim;
	/* who are we assigned to now? */
	int firstentry=closest(b,ppt,-1);
	/* what is the new second closest match? */
	int secondentry=closest(b,ppt,firstentry);
	/* old second closest is the cell being disbanded */
	double oldsecondmetric=_dist(dim,b->valuelist+dim*bestcell,ppt);
	/* new second closest error */
	double secondmetric=_dist(dim,b->valuelist+dim*secondentry,ppt);
	long next=secondary[head];

	if(head<points-entries){
	  cellcount2[secondentry]++;
	  cellcount2[bestcell]--;
	  cellerror[firstentry]+=secondmetric-oldsecondmetric;
	  cellerrormax[firstentry]=max(cellerrormax[firstentry],
				       _heuristic(b,ppt,secondentry));
	}
	
	secondary[head]=secondhead[secondentry];
	secondhead[secondentry]=head;
	head=next;

	if(cellcount2[bestcell]%128==0)
	  spinnit(spinbuf,cellcount2[bestcell]);
      }

      cellsleft--;
    }

    /* paring is over.  Build decision trees using points that now fall
       through the thresh matcher. */
    /* we don't free membership; we flatten it in order to use in lp_split */

    for(i=0;i<entries;i++){
      long head=firsthead[i];
      spinnit("rearranging membership cache... ",entries-i);
      while(head!=-1){
	long next=membership[head];
	membership[head]=i;
	head=next;
      }
    }

    free(secondhead);
    free(firsthead);
    free(cellerror);
    free(cellerrormax);
    free(secondary);

    pointindex=malloc(points*sizeof(long));
    /* make a point index of fall-through points */
    for(i=0;i<points;i++){
      int best=_best(b,pointlist+i*dim,1);
      if(best==-1)
	pointindex[indexedpoints++]=i;
      spinnit("finding orphaned points... ",points-i);
    }

    /* make an entry index */
    entryindex=malloc(entries*sizeof(long));
    target=0;
    for(i=0;i<entries;i++){
      if(b->c->lengthlist[i]>0)
	entryindex[target++]=i;
    }

    /* make working space for a reverse entry index */
    reventry=malloc(entries*sizeof(long));

    /* do the split */
    nt=b->c->nearest_tree=
      calloc(1,sizeof(encode_aux_nearestmatch));

    nt->alloc=4096;
    nt->ptr0=malloc(sizeof(long)*nt->alloc);
    nt->ptr1=malloc(sizeof(long)*nt->alloc);
    nt->p=malloc(sizeof(long)*nt->alloc);
    nt->q=malloc(sizeof(long)*nt->alloc);
    nt->aux=0;

    fprintf(stderr,"Leaves added: %d              \n",
            lp_split(pointlist,points,
                     b,entryindex,target,
                     pointindex,indexedpoints,
                     membership,reventry,
                     0,&pointssofar));
    free(membership);
    free(reventry);
    free(pointindex);

    /* hack alert.  I should just change the damned splitter and
       codebook writer */
    for(i=0;i<nt->aux;i++)nt->p[i]*=dim;
    for(i=0;i<nt->aux;i++)nt->q[i]*=dim;
    
    /* recount hits.  Build new lengthlist. reuse entryindex storage */
    for(i=0;i<entries;i++)entryindex[i]=1;
    for(i=0;i<points-entries;i++){
      int best=_best(b,pointlist+i*dim,1);
      double *a=pointlist+i*dim;
      if(!(i&0xff))spinnit("counting hits...",i);
      if(best==-1){
	fprintf(stderr,"\nINTERNAL ERROR; a point count not be matched to a\n"
		"codebook entry.  The new decision tree is broken.\n");
	exit(1);
      }
      entryindex[best]++;
    }
    for(i=0;i<nt->aux;i++)nt->p[i]/=dim;
    for(i=0;i<nt->aux;i++)nt->q[i]/=dim;
    
    /* the lengthlist builder doesn't actually deal with 0 hit entries.
       So, we pack the 'sparse' hit list into a dense list, then unpack
       the lengths after the build */
    {
      int upper=0;
      long *lengthlist=calloc(entries,sizeof(long));
      for(i=0;i<entries;i++){
	if(b->c->lengthlist[i]>0)
	  entryindex[upper++]=entryindex[i];
	else{
	  if(entryindex[i]>1){
	    fprintf(stderr,"\nINTERNAL ERROR; _best matched to unused entry\n");
	    exit(1);
	  }
	}
      }
      
      /* sanity check */
      if(upper != target){
	fprintf(stderr,"\nINTERNAL ERROR; packed the wrong number of entries\n");
	exit(1);
      }
    
      build_tree_from_lengths(upper,entryindex,lengthlist);
      
      upper=0;
      for(i=0;i<entries;i++){
	if(b->c->lengthlist[i]>0)
	  b->c->lengthlist[i]=lengthlist[upper++];
      }

    }
  }
  /* we're done.  write it out. */
  write_codebook(out,basename,b->c);

  fprintf(stderr,"\r                                        \nDone.\n");
  return(0);
}




