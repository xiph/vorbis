#include <math.h>
#include <stdio.h>
#include "codec.h"
#include "envelope.h"
#include "mdct.h"

#define READ 4096

int main(){
   vorbis_dsp_state encode,decode;
   vorbis_info vi;
   vorbis_block vb;
   long counterin=0;
   long counterout=0;
   int done=0;
   char *temp[]={ "Test" ,"the Test band", "test records",NULL };
   int mtemp[]={0,1};
   int frame=0;

   signed char buffer[READ*4+44];
   
  
   vi.channels=2;
   vi.rate=44100;
   vi.version=0;
   vi.mode=0;
   vi.user_comments=temp;
   vi.vendor="Xiphophorus";
   vi.smallblock=512;
   vi.largeblock=2048;
   vi.envelopesa=64;
   vi.envelopech=2;
   vi.envelopemap=mtemp;
   vi.channelmap=NULL;
   vi.preecho_thresh=10.;
   vi.preecho_clamp=4.;

   vorbis_analysis_init(&encode,&vi);
   vorbis_synthesis_init(&decode,&vi);
   vorbis_block_init(&encode,&vb);
 
   fread(buffer,1,44,stdin);
   fwrite(buffer,1,44,stdout);
   
   
   while(!done){
     long bread=fread(buffer,1,READ*4,stdin);
     double **buf=vorbis_analysis_buffer(&encode,READ);
     long i;
     
     /* uninterleave samples */
     
     for(i=0;i<bread/4;i++){
       buf[0][i]=((buffer[i*4+1]<<8)|(0x00ff&(int)buffer[i*4]))/32768.;
       buf[1][i]=((buffer[i*4+3]<<8)|(0x00ff&(int)buffer[i*4+2]))/32768.;
     }
  
     vorbis_analysis_wrote(&encode,i);
 
     while(vorbis_analysis_blockout(&encode,&vb)){
       double **pcm;
       int avail;
       double *window=encode.window[vb.W][vb.lW][vb.nW];
 
       /* analysis */

       /* apply envelope */
       _ve_envelope_sparsify(&vb);
       _ve_envelope_apply(&vb,0);
 
       for(i=0;i<vb.pcm_channels;i++)
	 mdct_forward(&vb.vd->vm[vb.W],vb.pcm[i],vb.pcm[i],window);
 

       /* synthesis */

       for(i=0;i<vb.pcm_channels;i++)
	 mdct_backward(&vb.vd->vm[vb.W],vb.pcm[i],vb.pcm[i],window);
 
 
       /*{
	 FILE *out;
	 char path[80];
	 int i;
	 
	 int avail=encode.block_size[vb.W];
	 int beginW=countermid-avail/2;
	 
	 sprintf(path,"ana%d",vb.frameno);
	 out=fopen(path,"w");
	 
	 for(i=0;i<avail;i++)
	   fprintf(out,"%d %g\n",i+beginW,vb.pcm[0][i]);
	 fprintf(out,"\n");
	 for(i=0;i<avail;i++)
	   fprintf(out,"%d %g\n",i+beginW,window[i]);
	 
	 fclose(out);
       }*/
 
       _ve_envelope_apply(&vb,1);

       counterin+=bread/4;
       vorbis_synthesis_blockin(&decode,&vb);
        
       while((avail=vorbis_synthesis_pcmout(&decode,&pcm))){
	 if(avail>READ)avail=READ;

	 for(i=0;i<avail;i++){
	   int l=rint(pcm[0][i]*32768.);
	   int r=rint(pcm[1][i]*32768.);
	   if(l>32767)l=32767;
	   if(r>32767)r=32767;
	   if(l<-32768)l=-32768;
	   if(r<-32768)r=-32768;
	   buffer[i*4]=l&0xff;
	   buffer[i*4+1]=(l>>8)&0xff;
	   buffer[i*4+2]=r&0xff;
	   buffer[i*4+3]=(r>>8)&0xff;
	 }    
	 
	 fwrite(buffer,1,avail*4,stdout);
	 
	 /*{
	   FILE *out;
	   char path[80];
	   int i;
	   
	   sprintf(path,"syn%d",frame);
	   out=fopen(path,"w");
	   
	   for(i=0;i<avail;i++)
	     fprintf(out,"%ld %g\n",i+counterout,pcm[0][i]);
	   fprintf(out,"\n");
	   for(i=0;i<avail;i++)
	     fprintf(out,"%ld %g\n",i+counterout,pcm[1][i]);
	   
	   fclose(out);
	   }*/

	 vorbis_synthesis_read(&decode,avail);
	 
	 counterout+=avail;
	 frame++;
       }
       
 
       if(vb.eofflag){
	 done=1;
	 break;
       }
     }
   }
   return 0;
}
