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
 last mod: $Id: latticepare.c,v 1.2 2000/05/08 20:49:50 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "vorbis/codebook.h"
#include "../lib/sharedbook.h"
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

/* search neighboring [non-culled] cells for lowest distance.  Spiral
   out in the event culling is deep */
static int secondbest(codebook *b,double *vec,int best){
  encode_aux_threshmatch *tt=b->c->thresh_tree;
  int dim=b->dim;
  int i,j,spiral=1;
  int *index=alloca(dim*sizeof(int)*2);
  int *mod=index+dim;
  int entry=best;

  double bestmetric=0;
  int bestentry=-1;

  /* decompose index */
  for(i=0;i<dim;i++){
    index[i]=best%tt->quantvals;
    best/=tt->quantvals;
  }

  /* hit one off on all sides of it; most likely we'll find a possible
     match */
  /* suboptimal for unaligned entries */
#if 0
  for(i=0;i<dim;i++){
    /* one up */
    if(index[i]+1<tt->quantvals){
      int newentry=entry+rint(pow(tt->quantvals,i));
      if(b->c->lengthlist[newentry]>0){
	double thismetric=_dist(dim, vec, b->valuelist+newentry*dim);

	if(bestentry==-1 || bestmetric>thismetric){
	  bestmetric=thismetric;
	  bestentry=newentry;
	}
      }
    }
      
    /* one down */
    if(index[i]-1>=0){
      int newentry=entry-rint(pow(tt->quantvals,i));
      if(b->c->lengthlist[newentry]>0){
	double thismetric=_dist(dim, vec, b->valuelist+newentry*dim);

	if(bestentry==-1 || bestmetric>thismetric){
	  bestmetric=thismetric;
	  bestentry=newentry;
	}
      }
    }
  }
     
#endif 
  /* no match?  search all cells, binary count, that are one away on
     one or more axes.  Then continue out until there's a match.
     We'll find one eventually, it's relatively OK to be inefficient
     as the attempt above will almost always succeed */
  while(bestentry==-1){
    for(i=0;i<dim;i++)mod[i]= -spiral;
    while(1){
      int newentry=entry;

      /* update the mod */
      for(j=0;j<dim;j++){
	if(mod[j]<=spiral)
	  break;
	else{
	  if(j+1<dim){
	    mod[j]= -spiral;
	    mod[j+1]++;
	  }
	}
      }
      if(j==dim)break;

      /* reconstitute the entry */
      for(j=0;j<dim;j++){
	if(index[j]+mod[j]<0 || index[j]+mod[j]>=tt->quantvals){
	  newentry=-1;
	  break;
	}
	newentry+=mod[j]*rint(pow(tt->quantvals,j));
      }

      if(newentry!=-1 && newentry!=entry)
	if(b->c->lengthlist[newentry]>0){
	  double thismetric=_dist(dim, vec, b->valuelist+newentry*dim);

	  if(bestentry==-1 || bestmetric>thismetric){
	    bestmetric=thismetric;
	    bestentry=newentry;
	  }
	}
      mod[0]++;
    }
    spiral++;
  }

  return(bestentry);
}

void usage(void){
  fprintf(stderr,"Ogg/Vorbis lattice codebook paring utility\n\n"
	  "usage: latticepare book.vqh data.vqd <target_cells>\n"
	  "                   -<n_0,n_1,...> [-<n_0,n_1,...>]\n\n"
	  "where <target_cells> is the desired number of final cells (or -1\n"
	  "for no change) and n,n,n,n...n  are explicit entries to cull\n\n"
	  "produces new book on stdout\n\n");
  exit(1);
}

int main(int argc,char *argv[]){
  char *basename;
  codebook *b=NULL;
  int entries=0;
  int dim=0;
  long i,j,target=-1;
  int *cvec=NULL;

  long *cullist=malloc(sizeof(int));
  long culls=0;

  argv++;


  if(*argv==NULL){
    usage();
    exit(1);
  }

  /* yes, this is evil.  However, it's very convenient to parse file
     extentions */

  while(*argv){
    if(*argv[0]=='-'){
      char *ptr=argv[0];
      long index=0;
      /* explicit cull */
      if(!b)usage();
      if(!cvec)cvec=malloc(dim*sizeof(int)); /* lazy ;-) */
      
      for(i=0;i<dim;i++){
	if(!ptr){
	  fprintf(stderr,"too few values in cull argument %s\n",argv[0]);
	  exit(1);
	}
	cvec[i]=atoi(ptr+1);
	if(cvec[i]<0 || cvec[i]>=b->c->thresh_tree->quantvals){
	  fprintf(stderr,"value too large in cull argument %s\n",argv[0]);
	  exit(1);
	}

	ptr=strchr(ptr+1,',');
      }
      if(ptr){
	fprintf(stderr,"too many values in cull argument %s\n",argv[0]);
	exit(1);
      }
      for(i=dim;i>0;i--)
	index=index*b->c->thresh_tree->quantvals+cvec[i-1];
	
      cullist=realloc(cullist,++culls*sizeof(long));
      cullist[culls-1]=index;
      fprintf(stderr,"\rExplicitly culling index %ld\n",index);
      argv++;
	
    }else{
      /* input file.  What kind? */
      char *dot;
      char *ext=NULL;
      char *name=strdup(*argv++);
      dot=strrchr(name,'.');
      if(dot)
        ext=dot+1;
      else{
	ext="";
	target=atol(name);
	if(target==0)target=entries;
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
	pointlist=malloc(cols*lines*sizeof(double));

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
  }
  if(!entries || !points)usage();
  if(target==-1)usage();

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
    long *cellhead=malloc(entries*sizeof(long));
    long *cellcount=calloc(entries,sizeof(long));
    double *cellerror1=calloc(entries,sizeof(double)); /* error for
                                                          firstentries */
    double *cellerror2=calloc(entries,sizeof(double)); /* error for
                                                          secondentry */
    double globalerror=0.;
    long cellsleft=entries;
    for(i=0;i<points;i++)membership[i]=-1;
    for(i=0;i<entries;i++)cellhead[i]=-1;

    for(i=0;i<points;i++){
      /* assign vectors to the nearest cell.  Also keep track of second
	 nearest for error statistics */
      double *ppt=pointlist+i*dim;
      int    firstentry=_best(b,ppt,1);
      int    secondentry=secondbest(b,ppt,firstentry);

      double firstmetric=_dist(dim,b->valuelist+dim*firstentry,ppt);
      double secondmetric=_dist(dim,b->valuelist+dim*secondentry,ppt);
      
      if(!(i&0xff))spinnit("initializing... ",points-i);
    
      membership[i]=cellhead[firstentry];
      cellhead[firstentry]=i;

      cellerror1[firstentry]+=firstmetric;
      cellcount[firstentry]++;
      globalerror+=firstmetric;
      cellerror2[firstentry]+=secondmetric;

    }

    /* handle the explicit cull list */
    for(i=0;i<culls;i++){
      long bestcell=cullist[i];
      char buf[80];
      sprintf(buf,"explicit culls (%d left)... ",(int)culls-i);
      
      /* disperse cell.  move each point out, adding it (properly) to
         the second best */
      if(b->c->lengthlist[bestcell]>0){
	long head=cellhead[bestcell];
	b->c->lengthlist[bestcell]=0;
	cellhead[bestcell]=-1;
	while(head!=-1){
	  /* head is a point number */
	  double *ppt=pointlist+head*dim;
	  int newentry=secondbest(b,ppt,bestcell);
	  int secondentry=secondbest(b,pointlist+head*dim,newentry);
	  double firstmetric=_dist(dim,b->valuelist+dim*newentry,ppt);
	  double secondmetric=_dist(dim,b->valuelist+dim*secondentry,ppt);
	  long next=membership[head];
	  cellcount[newentry]++;
	  cellcount[bestcell]--;
	  cellerror1[newentry]+=firstmetric;
	  cellerror2[newentry]+=secondmetric;
	  spinnit(buf,cellcount[bestcell]);
	  
	  membership[head]=cellhead[newentry];
	  cellhead[newentry]=head;
	  head=next;
	}
	cellsleft--;
      }
    }
    
    /* do the automatic cull request */
    while(cellsleft>target){
      int bestcell=-1;
      double besterror=0;
      long head=-1;
      spinnit("cells left to eliminate... ",cellsleft-target);

      /* find the cell with lowest removal impact */
      for(i=0;i<entries;i++){
	if(b->c->lengthlist[i]>0){
	  double thiserror=globalerror-cellerror1[i]+cellerror2[i];
	  if(bestcell==-1 || besterror>thiserror){
	    besterror=thiserror;
	    bestcell=i;
	  }
	}
      }

      /* disperse it.  move each point out, adding it (properly) to
         the second best */
      b->c->lengthlist[bestcell]=0;
      head=cellhead[bestcell];
      cellhead[bestcell]=-1;
      while(head!=-1){
	/* head is a point number */
	double *ppt=pointlist+head*dim;
	int newentry=secondbest(b,ppt,bestcell);
	int secondentry=secondbest(b,pointlist+head*dim,newentry);
	double firstmetric=_dist(dim,b->valuelist+dim*newentry,ppt);
	double secondmetric=_dist(dim,b->valuelist+dim*secondentry,ppt);
	long next=membership[head];
	cellerror1[newentry]+=firstmetric;
	cellerror2[newentry]+=secondmetric;

	membership[head]=cellhead[newentry];
	cellhead[newentry]=head;
	head=next;
      }

      cellsleft--;
    }

    /* paring is over.  Build decision trees using points that now fall
       through the thresh matcher. */
    /* we don't free membership; we flatten it in order to use in lp_split */

    for(i=0;i<entries;i++){
      long head=cellhead[i];
      spinnit("rearranging membership cache... ",entries-i);
      while(head!=-1){
	long next=membership[head];
	membership[head]=i;
	head=next;
      }
    }

    free(cellhead);
    free(cellerror1);
    free(cellerror2);

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
    
    /* recount hits.  Build new lengthlist. reuse entryindex storage */
    for(i=0;i<entries;i++)entryindex[i]=1;
    for(i=0;i<points;i++){
      int best=_best(b,pointlist+i*dim,1);
      if(!(i&0xff))spinnit("counting hits...",i);
      if(best==-1){
	fprintf(stderr,"\nINTERNAL ERROR; a point count not be matched to a\n"
		"codebook entry.  The new decision tree is broken.\n");
	exit(1);
      }
      entryindex[best]++;
    }
    
    /* the lengthlist builder doesn't actually deal with 0 hit entries.
       So, we pack the 'sparse' hit list into a dense list, then unpack
       the lengths after the build */
    {
      int upper=0;
      long *lengthlist=calloc(entries,sizeof(long));
      for(i=0;i<entries;i++)
	if(b->c->lengthlist[i]>0)
	  entryindex[upper++]=entryindex[i];
      
      /* sanity check */
      if(upper != target){
	fprintf(stderr,"\nINTERNAL ERROR; packed the wrong number of entries\n");
	exit(1);
      }
    

      build_tree_from_lengths(upper,entryindex,lengthlist);
      
      upper=0;
      for(i=0;i<entries;i++)
	if(b->c->lengthlist[i]>0)
	  b->c->lengthlist[i]=lengthlist[upper++];
    }
  }
  /* we're done.  write it out. */
  write_codebook(stdout,"foo",b->c);

  fprintf(stderr,"\r                                        \nDone.\n");
  return(0);
}




