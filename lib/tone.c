#include <stdlib.h>
#include <stdio.h>
#include <math.h>

void usage(){
  fprintf(stderr,"tone <frequency_Hz> [<amplitude>]\n");
  exit(1);
}

int main (int argc,char *argv[]){
  int i;
  double f;
  double amp=32767.;

  if(argc<2)usage();
  f=atof(argv[1]);
  if(argc>=3)amp=atof(argv[2])*32767.;

  for(i=0;i<44100*10;i++){
    long val=rint(amp*sin(i/44100.*f*2*M_PI));
    if(val>32767.)val=32767.;
    if(val<-32768.)val=-32768.;

    fprintf(stdout,"%c%c%c%c",
	    (char)(val&0xff),
	    (char)((val>>8)&0xff),
	    (char)(val&0xff),
	    (char)((val>>8)&0xff));
  }
  return(0);
}

