#include "lsp16.vqh"
#define CODEBOOK _vq_book_lsp16

/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-1999             *
 * by 1999 Monty <monty@xiph.org> and The XIPHOPHORUS Company       *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: utility main for loading/testing/running finished codebooks
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Dec 28 1999

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>

/* this is a bit silly; it's a C stub used by a Perl script to build a
   quick executable against the chosen codebook and run tests */

static char *linebuffer=NULL;
static int  lbufsize=0;
static char *rline(FILE *in){
  long sofar=0;
  if(feof(in))return NULL;

  while(1){
    int gotline=0;

    while(!gotline){
      if(sofar>=lbufsize){
	if(!lbufsize){	
	  lbufsize=1024;
	  linebuffer=malloc(lbufsize);
	}else{
	  lbufsize*=2;
	  linebuffer=realloc(linebuffer,lbufsize);
	}
      }
      {
	long c=fgetc(in);
	switch(c){
	case '\n':
	case EOF:
	  gotline=1;
	  break;
	default:
	  linebuffer[sofar++]=c;
	  linebuffer[sofar]='\0';
	  break;
	}
      }
    }
    
    if(linebuffer[0]=='#'){
      sofar=0;
    }else{
      return(linebuffer);
    }
  }
}

void vqbook_unquantize(vqbook *b){
  long j,k;
  double mindel=float24_unpack(b->min);
  double delta=float24_unpack(b->delta);
  if(!b->valuelist)b->valuelist=malloc(sizeof(double)*b->entries*b->dim);
  
  for(j=0;j<b->entries;j++){
    double last=0.;
    for(k=0;k<b->dim;k++){
      double val=b->quantlist[j*b->dim+k]*delta+last+mindel;
      b->valuelist[j*b->dim+k]=val;
      if(b->sequencep)last=val;

    }
  }
}


/* command line:
   run outbase [-m] [-s <start>,<n>] datafile [-s <start>,<n>] [datafile...]

   produces: outbase-residue.m (error between input data and chosen codewords;
                                can be used to cascade)
             outbase-cells.m   (2d gnuplot file of cells if -m)

   currently assumes a 'sequenced' codebook wants its test data to be
   normalized to begin at zero... */

int main(int argc,char *argv[]){
  vqbook *b=&CODEBOOK;
  FILE *cells=NULL;
  FILE *residue=NULL;
  FILE *in=NULL;
  char *line,*name;
  long i,j,k;
  int start=0,num=-1;

  double mean=0.,meansquare=0.,mean_count=0.;
  
  argv++;

  if(*argv==NULL){
    fprintf(stderr,"Need a basename.\n");
    exit(1);
  }

  name=strdup(*argv);
  vqbook_unquantize(b);

  {
    char buf[80];
    snprintf(buf,80,"%s-residue.vqd",name);
    residue=fopen(buf,"w");
    if(!residue){
      fprintf(stderr,"Unable to open output file %s\n",buf);
      exit(1);
    }
  }   

  /* parse the command line; handle things as the come */
  argv=argv++;
  while(*argv){
    if(argv[0][0]=='-'){
      /* it's an option */
      switch(argv[0][1]){
      case 'm':
	{
	  char buf[80];
	  snprintf(buf,80,"%s-cells.m",name);
	  cells=fopen(buf,"w");
	  if(!cells){
	    fprintf(stderr,"Unable to open output file %s\n",buf);
	    exit(1);
	  }
	}   
	argv++;
	break;
      case 's':
	if(!argv[1]){
	  fprintf(stderr,"Option %s missing argument.\n",argv[0]);
	  exit(1);
	}      
	if(sscanf(argv[1],"%d,%d",&start,&num)!=2){
          num= -1;
          if(sscanf(argv[1],"%d",&start)!=1){
	    fprintf(stderr,"Option %s syntax error.\n",argv[0]);
            exit(1);
	  }
        }
	argv+=2;
        break;
      }
    }else{
      /* it's an input file */
      char *file=strdup(*argv++);
      FILE *in;
      int cols=-1;

      in=fopen(file,"r");
      if(in==NULL){
        fprintf(stderr,"Could not open input file %s\n",file);
        exit(1);
      }

      while((line=rline(in))){
        if(cols==-1){
          char *temp=line;
          while(*temp==' ')temp++;
          for(cols=0;*temp;cols++){
            while(*temp>32)temp++;
            while(*temp==' ')temp++;
          }
        }
        {
          int i;
          double *p=malloc(cols*sizeof(double));
          if(start*num+b->dim>cols){
            fprintf(stderr,"ran out of columns reading %s\n",file);
            exit(1);
          }
          while(*line==' ')line++;
          for(i=0;i<cols;i++){

            /* static length buffer bug workaround */
            char *temp=line;
            char old;
            while(*temp>32)temp++;

            old=temp[0];
            temp[0]='\0';
            p[i]=atof(line);
            temp[0]=old;
            
            while(*line>32)line++;
            while(*line==' ')line++;
          }
          if(num<=0)num=(cols-start)/b->dim;
          for(i=num-1;i>=0;i--){
	    int entry;
	    double *base=p+start+i*b->dim;

	    /* if this is a sequenced book and i||start,
	       normalize the beginning to zero */
	    if(b->sequencep && (i>0 || start>0)){
	      for(k=0;k<b->dim;k++)
		base[k]-= *(base-1);
	    }

	    /* assign the point */
	    entry=vqenc_entry(b,base);

	    /* accumulate metrics */
	    for(k=0;k<b->dim;k++){
	      double err=base[k]-b->valuelist[k+entry*b->dim];
	      mean+=fabs(err);
	      meansquare+=err*err;
	      mean_count++;
	    }

	    /* brute force it... did that work better? */


	    /* paint the cell if -m */
	    if(cells){
	      fprintf(cells,"%g %g\n%g %g\n\n",
		      base[0],base[1],
		      b->valuelist[0+entry*b->dim],
		      b->valuelist[1+entry*b->dim]);
	    }

	    /* write cascading data */



	  }
          free(b);
        }
      }
      fclose(in);
    }
  }
  
  
  fclose(residue);
  if(cells)fclose(cells);

  /* print accumulated error statistics */
  fprintf(stderr,"results:\n\tmean squared error:%g\n\tmean error:%g\n\n",
	  sqrt(meansquare/mean_count),mean/mean_count);

  return 0;
}
