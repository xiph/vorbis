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

 function: utility main for training codebooks
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Dec 15 1999

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "vqgen.h"
#include "vqext.h"

static char *linebuffer=NULL;
static int  lbufsize=0;
static char *rline(FILE *in,FILE *out,int pass){
  long sofar=0;
  if(feof(in))return NULL;

  while(1){
    int gotline=0;

    while(!gotline){
      if(sofar+1>=lbufsize){
	if(!lbufsize){	
	  lbufsize=16;
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
      if(pass)fprintf(out,"%s",linebuffer);
      sofar=0;
    }else{
      return(linebuffer);
    }
  }
}

/* command line:
   trainvq  vqfile [options] trainfile [trainfile]

   options: -params     entries,dim,quant
            -subvector  start[,num]
	    -error      desired_error
	    -iterations iterations
*/

static void usage(void){
  fprintf(stderr, "\nOggVorbis %s VQ codebook trainer\n\n"
	  "<foo>vqtrain vqfile [options] [datasetfile] [datasetfile]\n"
	  "options: -p[arams]     <entries,dim,quant>\n"
	  "         -s[ubvector]  <start[,num]>\n"
	  "         -e[rror]      <desired_error>\n"
	  "         -i[terations] <maxiterations>\n\n"
	  "examples:\n"
	  "   train a new codebook to 1%% tolerance on datafile 'foo':\n"
	  "      xxxvqtrain book -p 256,6,8 -e .01 foo\n"
	  "      (produces a trained set in book-0.vqi)\n\n"
	  "   continue training 'book-0.vqi' (produces book-1.vqi):\n"
	  "      xxxvqtrain book-0.vqi\n\n"
	  "   add subvector from element 1 to <dimension> from files\n"
	  "      data*.m to the training in progress, prodicing book-1.vqi:\n"
	  "      xxxvqtrain book-0.vqi -s 1,1 data*.m\n\n",vqext_booktype);
}

int exiting=0;
void setexit(int dummy){
  fprintf(stderr,"\nexiting... please wait to finish this iteration\n");
  exiting=1;
}

int main(int argc,char *argv[]){
  vqgen v;
  quant_return q;

  int entries=-1,dim=-1,quant=-1;
  int start=0,num=-1;
  double desired=.05;
  int iter=1000;

  FILE *out=NULL;
  char *line;
  long i,j,k;
  int init=0;

  argv++;
  if(!*argv){
    usage();
    exit(0);
  }

  /* get the book name, a preexisting book to continue training */
  {
    FILE *in=NULL;
    char *filename=alloca(strlen(*argv)+30),*ptr;

    strcpy(filename,*argv);
    in=fopen(filename,"r");
    ptr=strrchr(filename,'-');
    if(ptr){
      int num;
      ptr++;
      num=atoi(ptr);
      sprintf(ptr,"%d.vqi",num+1);
    }else
      strcat(filename,"-0.vqi");
    
    out=fopen(filename,"w");
    if(out==NULL){
      fprintf(stderr,"Unable to open %s for writing\n",filename);
      exit(1);
    }
    
    if(in){
      /* we wish to suck in a preexisting book and continue to train it */
      double a;
      
      line=rline(in,out,1);
      if(strcmp(line,vqext_booktype)){
	fprintf(stderr,"wrong book type; %s!=%s\n",line,vqext_booktype);
	exit(1);
      } 
      
      line=rline(in,out,1);
      if(sscanf(line,"%d %d",&entries,&dim)!=2){
	fprintf(stderr,"Syntax error reading book file\n");
	exit(1);
      }
      
      vqgen_init(&v,dim,entries,vqext_metric);
      init=1;
      
      /* quant setup */
      line=rline(in,out,1);
      if(sscanf(line,"%lf %lf %d %d",&q.minval,&q.delt,
		&q.addtoquant,&quant)!=4){
	fprintf(stderr,"Syntax error reading book file\n");
	exit(1);
      }
      
      /* quantized entries */
      for(j=0;j<entries;j++)
	for(k=0;k<dim;k++)
	  line=rline(in,out,0);
      
      /* unquantized entries */
      i=0;
      for(j=0;j<entries;j++){
	for(k=0;k<dim;k++){
	  line=rline(in,out,0);
	  sscanf(line,"%lf",&a);
	  v.entrylist[i++]=a;
	}
      }
      
	/* bias, points */
      i=0;
      for(j=0;j<entries;j++){
	line=rline(in,out,0);
	sscanf(line,"%lf",&a);
	v.bias[i++]=a;
      }
      
      {
	double b[80];
	i=0;
	v.entries=0; /* hack to avoid reseeding */
	while(1){
	  for(k=0;k<dim && k<80;k++){
	    line=rline(in,out,0);
	    if(!line)break;
	    sscanf(line,"%lf",b+k);
	  }
	  if(feof(in))break;
	  vqgen_addpoint(&v,b);
	}
	v.entries=entries;
      }
      
      fclose(in);
    }
  }
  
  /* get the rest... */
  argv=argv++;
  while(*argv){
    if(argv[0][0]=='-'){
      /* it's an option */
      if(!argv[1]){
	fprintf(stderr,"Option %s missing argument.\n",argv[0]);
	exit(1);
      }
      switch(argv[0][1]){
      case 'p':
	if(sscanf(argv[1],"%d,%d,%d",&entries,&dim,&quant)!=3)
	  goto syner;
	break;
      case 's':
	if(sscanf(argv[1],"%d,%d",&start,&num)!=2){
	  num= -1;
	  if(sscanf(argv[1],"%d",&start)!=1)
	    goto syner;
	}
	break;
      case 'e':
	if(sscanf(argv[1],"%lf",&desired)!=1)
	  goto syner;
	break;
      case 'i':
	if(sscanf(argv[1],"%d",&iter)!=1)
	  goto syner;
	break;
      default:
	fprintf(stderr,"Unknown option %s\n",argv[0]);
	exit(1);
      }
      argv+=2;
    }else{
      /* it's an input file */
      char *file=strdup(*argv++);
      FILE *in;
      int cols=-1;

      if(!init){
	if(dim==-1 || entries==-1 || quant==-1){
	  fprintf(stderr,"-p required when training a new set\n");
	  exit(1);
	}
	vqgen_init(&v,dim,entries,vqext_metric);
	init=1;
      }

      in=fopen(file,"r");
      if(in==NULL){
	fprintf(stderr,"Could not open input file %s\n",file);
	exit(1);
      }
      fprintf(out,"# training file entry: %s\n",file);

      while((line=rline(in,out,0))){
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
	  double *b=malloc(cols*sizeof(double));
	  if(start*num+dim>cols){
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
	    b[i]=atof(line);
	    temp[0]=old;
	    
	    while(*line>32)line++;
	    while(*line==' ')line++;
	  }
	  if(num<=0)num=(cols-start)/dim;
	  for(i=0;i<num;i++){
	    vqext_adjdata(b,start+i*dim,dim);
	    vqgen_addpoint(&v,b+start+i*dim);
	  }
	  free(b);
	}
      }
      fclose(in);
    }
  }

  if(!init){
    fprintf(stderr,"No input files!\n");
    exit(1);
  }

  /* train the book */
  signal(SIGTERM,setexit);
  signal(SIGINT,setexit);

  for(i=0;i<iter && !exiting;i++){
    double result;
    if(i!=0)vqext_unquantize(&v,&q);
    result=vqgen_iterate(&v);
    q=vqext_quantize(&v,quant);
    if(result<desired)break;
  }

  /* save the book */

  fprintf(out,"# OggVorbis VQ codebook trainer, intermediate file\n");
  fprintf(out,"%s\n",vqext_booktype);
  fprintf(out,"%d %d\n",entries,dim);
  fprintf(out,"%g %g %d %d\n",q.minval,q.delt,q.addtoquant,quant);

  /* quantized entries */
  fprintf(out,"# quantized entries---\n");
  i=0;
  for(j=0;j<entries;j++)
    for(k=0;k<dim;k++)
      fprintf(out,"%d\n",(int)(rint(v.entrylist[i++])));

  /* dequantize */
  vqext_unquantize(&v,&q);

  fprintf(out,"# unquantized entries---\n");
  i=0;
  for(j=0;j<entries;j++)
    for(k=0;k<dim;k++)
      fprintf(out,"%f\n",v.entrylist[i++]);

  /* unquantized entries */
  
  fprintf(out,"# biases---\n");
  i=0;
  for(j=0;j<entries;j++)
    fprintf(out,"%f\n",v.bias[i++]);

  fprintf(out,"# points---\n");
  i=0;
  for(j=0;j<v.points;j++)
    for(k=0;k<dim && k<80;k++)
      fprintf(out,"%f\n",v.pointlist[i++]);

  fclose(out);
  exit(0);

  syner:
    fprintf(stderr,"Syntax error in argument '%s'\n",*argv);
    exit(1);
}
