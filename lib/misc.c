/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************/

#define HEAD_ALIGN 32
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "vorbis/codec.h"
#define MISC_C
#include "misc.h"

static pthread_mutex_t memlock=PTHREAD_MUTEX_INITIALIZER;
void **pointers=NULL;
long *insertlist=NULL; /* We can't embed this in the pointer list;
			  a pointer can have any value... */
int ptop=0;
int palloced=0;
int pinsert=0;

typedef struct {
  char *file;
  long line;
  long ptr;
} head;

static void *_insert(void *ptr,char *file,long line){
  ((head *)ptr)->file=file;
  ((head *)ptr)->line=line;
  ((head *)ptr)->ptr=pinsert;

  pthread_mutex_lock(&memlock);
  if(pinsert>=palloced){
    palloced+=64;
    if(pointers){
      pointers=(void **)realloc(pointers,sizeof(void **)*palloced);
      insertlist=(long *)realloc(insertlist,sizeof(long *)*palloced);
    }else{
      pointers=(void **)malloc(sizeof(void **)*palloced);
      insertlist=(long *)malloc(sizeof(long *)*palloced);
    }
  }

  pointers[pinsert]=ptr;

  if(pinsert==ptop)
    pinsert=++ptop;
  else
    pinsert=insertlist[pinsert];
  
  pthread_mutex_unlock(&memlock);
  return(ptr+HEAD_ALIGN);
}

static void _ripremove(void *ptr){
  int insert;
  pthread_mutex_lock(&memlock);
  insert=((head *)ptr)->ptr;
  insertlist[insert]=pinsert;
  pinsert=insert;
  pointers[insert]=NULL;
  pthread_mutex_unlock(&memlock);
}

void _VDBG_dump(void){
  int i;
  pthread_mutex_lock(&memlock);
  for(i=0;i<ptop;i++){
    head *ptr=pointers[i];
    if(ptr)
      fprintf(stderr,"unfreed bytes from %s:%ld\n",
	      ptr->file,ptr->line);
  }

  pthread_mutex_unlock(&memlock);
}

extern void *_VDBG_malloc(void *ptr,long bytes,char *file,long line){
  bytes+=HEAD_ALIGN;
  if(ptr){
    ptr-=HEAD_ALIGN;
    _ripremove(ptr);
    ptr=realloc(ptr,bytes);
  }else{
    ptr=malloc(bytes);
    memset(ptr,0,bytes);
  }
  return _insert(ptr,file,line);
}

extern void _VDBG_free(void *ptr,char *file,long line){
  if(ptr){
    ptr-=HEAD_ALIGN;
    _ripremove(ptr);
    free(ptr);
  }
}

