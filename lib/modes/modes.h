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

 function: predefined encoding modes
 last mod: $Id: modes.h,v 1.13 2001/08/13 11:30:59 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_H_
#define _V_MODES_H_

#include "masking.h"
/* stereo (coupled) */
#include "modes/mode_44c_Z.h"
#include "modes/mode_44c_Y.h"
#include "modes/mode_44c_X.h"
#include "modes/mode_44c_A.h"
#include "modes/mode_44c_B.h"
#include "modes/mode_44c_C.h"
#include "modes/mode_44c_D.h"
#include "modes/mode_44c_E.h"

/* mono/dual/multi */
#include "modes/mode_44_Z.h"
#include "modes/mode_44_Y.h"
#include "modes/mode_44_X.h"
#include "modes/mode_44_A.h"
#include "modes/mode_44_B.h"
#include "modes/mode_44_C.h"

#endif
