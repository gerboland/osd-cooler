/*
 *  Copyright(C) 2006-2007 Neuros Technology International LLC. 
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
 *  
 *
 *  This program is distributed in the hope that, in addition to its 
 *  original purpose to support Neuros hardware, it will be useful 
 *  otherwise, but WITHOUT ANY WARRANTY; without even the implied 
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 *
 * Internal definitions used by framebuffer routines
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ------------------------------- 2007-03-31 NEROCHIARO
 *
 */

/* bitmap header, used to read information from bitmap files */
typedef struct{
     short bfType; //__attribute__((packed));
     long  bfSize; //__attribute__((packed));
     short bfReserved1;
     short bfReserved2;
     long bfOffBits;
     long biSize;
     long biWidth;
     long biHeight;
     short biPlanes;
     short biBitCount;
     long biCompression;
     long biSizeImage;
     long biXpp;
     long biYpp;
     long biClrUsed;
     long biClrImportant;
} BMPHEAD;

