/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *

 ********************************************************************

 function: 
           User DllMain to allow standalone vorbisenc.dll to access
           backend mapping structures in defined in registery.c 
           and built into vorbis.dll, by copying them into shared 
           memory upon DLL loading.

 created:  06-Sep-2001, Chris Wolf 
                        Adopted from "dllskel" MSDN COM tutorial
 last mod: $Id: dllmain.c,v 1.1 2001/09/11 20:16:11 cwolf Exp $

 This module is used in both vorbis.dll and vorbisenc.dll.  If vorbisenc.c
 is built into a separate DLL, then define STANDALONE_VORBISENC_DLL so
 that the memory mapping takes place.  If this is not defined (as would
 be the case when building one single DLL, the the pointers are accessed
 normally.
 
 When this module is compiled into vorbis.dll, with VORBIS_DLL defined, 
 it makes DllMain initialize the shared memory segment.

 When this module is compiled into vorbisenc.dll, with VORBIS_DLL
 not defined, it makes DllMain retrieve a pointer to the already
 initialized shared memory segment.

 If STANDALONE_VORBISENC_DLL is not defined, then DllMain is a no-op
 for either DLL.
 ********************************************************************/
#ifdef _MSC_VER // This file is meaningless outside MSC
#include <windows.h>
#include <memory.h>

#include "shmmap.h"

#define DLLSHARED_STR "vorbis_dll" // Name for file-mapped segement

BOOL initializeProcess(HINSTANCE hDll);
BOOL cleanupProcess(HINSTANCE hDll);

SHARED_MAP* 	g_shared_map;
static void* 	g_pvShared;
static HANDLE hShm;

BOOL WINAPI DllMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved)
{
  switch (dwReason)
  {
  case DLL_PROCESS_ATTACH:
#ifdef STANDALONE_VORBISENC_DLL
    return (initializeProcess(hDll));
#endif
    break;

  case DLL_PROCESS_DETACH:
#ifdef STANDALONE_VORBISENC_DLL
    cleanupProcess (hDll);
#endif
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  default:
    break;
  }
        
  return TRUE;
}

BOOL initializeProcess(HINSTANCE hDll)
{
  BOOL isFirstMapping = TRUE;
  int iSharedSize=1;

#ifdef VORBIS_DLL
  SHARED_MAP *map = table_map2mem(&iSharedSize);
#endif
  
              // Create a named file mapping object.
  hShm = CreateFileMapping(
                           (HANDLE) 0xFFFFFFFF,   // Use paging file
                            NULL,                 // No security attributes
                            PAGE_READWRITE,       // Read/Write access
                            0,                    // Mem Size: high 32 bits
                            iSharedSize,          // Mem Size: low 32 bits
                            DLLSHARED_STR);       // Name of map object

  if (NULL == hShm)
    return FALSE;

     // Determine if this is the first create of the file mapping.
    isFirstMapping = (ERROR_ALREADY_EXISTS != GetLastError());

     // Now get a pointer to the file-mapped shared memory.
    g_pvShared = MapViewOfFile(
                            hShm,                 // File Map obj to view
                            FILE_MAP_WRITE,       // Read/Write access
                            0,                    // high: map from beginning
                            0,                    // low:
                            0);                   // default: map entire file


    if (NULL != g_pvShared)
    {
#ifdef VORBIS_DLL
      if (isFirstMapping)
      {
          // If this is the first process attaching vorbis.dll, 
          // initialize the shared memory.
        (void)memcpy(g_pvShared, (void *)map, iSharedSize);
      }
#else
          // If this is a process  attaching to vorbisenc.dll
          // get the pointer tables from shared memory
      g_shared_map = (SHARED_MAP *)g_pvShared;
#endif
    }

  return TRUE;
}

BOOL cleanupProcess(HINSTANCE hDll)
{
    // Unmap any shared memory from the process's address space.
  UnmapViewOfFile(g_pvShared);
    // Close the process's handle to the file-mapping object.
  CloseHandle(hShm);

  return TRUE;
}
#endif /* _MSC_VER */
