/**********************************************************************************/
/* kGUI - kguirandom.cpp                                                          */
/*                                                                                */
/* Programmed by Kevin Pickell ( and others )                                     */
/* the main chunk of the random number routine was taken from the linux random    */
/* number generator	                                                              */
/*                                                                                */
/* http://www.scale18.com/cgi-bin/page/kgui.html	                              */
/*                                                                                */
/*    kGUI is free software; you can redistribute it and/or modify                */
/*    it under the terms of the GNU Lesser General Public License as published by */
/*    the Free Software Foundation; version 2.                                    */
/*                                                                                */
/*    kGUI is distributed in the hope that it will be useful,                     */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of              */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               */
/*    GNU General Public License for more details.                                */
/*                                                                                */
/*    http://www.gnu.org/licenses/lgpl.txt                                        */
/*                                                                                */
/*    You should have received a copy of the GNU General Public License           */
/*    along with kGUI; if not, write to the Free Software                         */
/*    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA  */
/*                                                                                */
/**********************************************************************************/

#include "kgui.h"

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

void kGUIRandom::InitSeed(void)
{
	m_add_ptr = 0;
	m_entropy_count = 0;
	m_input_rotate=0;
	memset(&m_pool,0,sizeof(m_pool));
}

/*
 * This function adds a byte into the entropy "pool".  It does not
 * update the entropy estimate.  The caller must do this if appropriate.
 *
 * The pool is stirred with a primitive polynomial of degree 128
 * over GF(2), namely x^128 + x^99 + x^59 + x^31 + x^9 + x^7 + 1.
 * For a pool of size 64, try x^64+x^62+x^38+x^10+x^6+x+1.
 * 
 * We rotate the input word by a changing number of bits, to help
 * assure that all bits in the entropy get toggled.  Otherwise, if we
 * consistently feed the entropy pool small numbers (like jiffies and
 * scancodes, for example), the upper bits of the entropy pool don't
 * get affected. --- TYT, 10/11/95
 */

#define TAP1    99     /* The polynomial taps */
#define TAP2    59
#define TAP3    31
#define TAP4    9
#define TAP5    7

void kGUIRandom::AddEntropy(const char *buf, int nbytes)
{
	while(nbytes>4)
	{
		AddEntropy(*((const unsigned long *)buf));
		buf+=4;
		nbytes-=4;
	}
	while(nbytes>0)
	{
		AddEntropy(buf[0]);
		++buf;
		--nbytes;
	}
}

void kGUIRandom::AddEntropy(const unsigned long input)
{
	unsigned i;
	unsigned long w;

	w = (input << m_input_rotate) | (input >> (32 - m_input_rotate));
	i = m_add_ptr = (m_add_ptr - 1) & (POOLWORDS-1);
	if (i)
		m_input_rotate = (m_input_rotate + 7) & 31;
	else
		/*
		 * At the beginning of the pool, add an extra 7 bits
		 * rotation, so that successive passes spread the
		 * input bits across the pool evenly.
		 */
		m_input_rotate = (m_input_rotate + 14) & 31;

	/* XOR in the various taps */
	w ^= m_pool[(i+TAP1)&(POOLWORDS-1)];
	w ^= m_pool[(i+TAP2)&(POOLWORDS-1)];
	w ^= m_pool[(i+TAP3)&(POOLWORDS-1)];
	w ^= m_pool[(i+TAP4)&(POOLWORDS-1)];
	w ^= m_pool[(i+TAP5)&(POOLWORDS-1)];
	w ^= m_pool[i];
	/* Rotate w left 1 bit (stolen from SHA) and store */
	m_pool[i] = (w << 1) | (w >> 31);
}

/*
 * MD5 transform algorithm, taken from code written by Colin Plumb,
 * and put into the public domain
 *
 */

/* The four core functions - F1 is optimized somewhat */

#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s) ( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */

void kGUIRandom::MD5Transform(unsigned long buf[4],unsigned long const in[16])
{
	unsigned long a, b, c, d;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	MD5STEP(F1, a, b, c, d, in[ 0]+0xd76aa478,  7);
	MD5STEP(F1, d, a, b, c, in[ 1]+0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, in[ 2]+0x242070db, 17);
	MD5STEP(F1, b, c, d, a, in[ 3]+0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, in[ 4]+0xf57c0faf,  7);
	MD5STEP(F1, d, a, b, c, in[ 5]+0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, in[ 6]+0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, in[ 7]+0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, in[ 8]+0x698098d8,  7);
	MD5STEP(F1, d, a, b, c, in[ 9]+0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, in[10]+0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, in[11]+0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, in[12]+0x6b901122,  7);
	MD5STEP(F1, d, a, b, c, in[13]+0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, in[14]+0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, in[15]+0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, in[ 1]+0xf61e2562,  5);
	MD5STEP(F2, d, a, b, c, in[ 6]+0xc040b340,  9);
	MD5STEP(F2, c, d, a, b, in[11]+0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, in[ 0]+0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, in[ 5]+0xd62f105d,  5);
	MD5STEP(F2, d, a, b, c, in[10]+0x02441453,  9);
	MD5STEP(F2, c, d, a, b, in[15]+0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, in[ 4]+0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, in[ 9]+0x21e1cde6,  5);
	MD5STEP(F2, d, a, b, c, in[14]+0xc33707d6,  9);
	MD5STEP(F2, c, d, a, b, in[ 3]+0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, in[ 8]+0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, in[13]+0xa9e3e905,  5);
	MD5STEP(F2, d, a, b, c, in[ 2]+0xfcefa3f8,  9);
	MD5STEP(F2, c, d, a, b, in[ 7]+0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, in[12]+0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, in[ 5]+0xfffa3942,  4);
	MD5STEP(F3, d, a, b, c, in[ 8]+0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, in[11]+0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, in[14]+0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, in[ 1]+0xa4beea44,  4);
	MD5STEP(F3, d, a, b, c, in[ 4]+0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, in[ 7]+0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, in[10]+0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, in[13]+0x289b7ec6,  4);
	MD5STEP(F3, d, a, b, c, in[ 0]+0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, in[ 3]+0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, in[ 6]+0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, in[ 9]+0xd9d4d039,  4);
	MD5STEP(F3, d, a, b, c, in[12]+0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, in[15]+0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, in[ 2]+0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, in[ 0]+0xf4292244,  6);
	MD5STEP(F4, d, a, b, c, in[ 7]+0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, in[14]+0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, in[ 5]+0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, in[12]+0x655b59c3,  6);
	MD5STEP(F4, d, a, b, c, in[ 3]+0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, in[10]+0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, in[ 1]+0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, in[ 8]+0x6fa87e4f,  6);
	MD5STEP(F4, d, a, b, c, in[15]+0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, in[ 6]+0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, in[13]+0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, in[ 4]+0xf7537e82,  6);
	MD5STEP(F4, d, a, b, c, in[11]+0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, in[ 2]+0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, in[ 9]+0xeb86d391, 21);

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

#if POOLWORDS % 16
#error ExtractEntropy() assumes that POOLWORDS is a multiple of 16 words.
#endif

/*
 * This function extracts randomness from the "entropy pool", and
 * returns it in a buffer.  This function computes how many remaining
 * bits of entropy are left in the pool, but it does not restrict the
 * number of bytes that are actually obtained.
 */
void kGUIRandom::ExtractEntropy(char * buf,  int nbytes)
{
	int i;
	unsigned long tmp[4];
	
	/* Redundant, but just in case... */
	if (m_entropy_count > POOLBITS) 
		m_entropy_count = POOLBITS;
	/* Why is this here?  Left in from Ted Ts'o.  Perhaps to limit time. */
	if (nbytes > 32768)
		nbytes = 32768;

	if ((m_entropy_count / 8) >= (unsigned int)nbytes)
		m_entropy_count -= nbytes*8;
	else
		m_entropy_count = 0;

	while (nbytes) {
		/* Hash the pool to get the output */
		tmp[0] = 0x67452301;
		tmp[1] = 0xefcdab89;
		tmp[2] = 0x98badcfe;
		tmp[3] = 0x10325476;
		for (i = 0; i < POOLWORDS; i += 16)
			MD5Transform(tmp, m_pool+i);
		/* Modify pool so next hash will produce different results */
		AddEntropy(tmp[0]);
		AddEntropy(tmp[1]);
		AddEntropy(tmp[2]);
		AddEntropy(tmp[3]);
		/*
		 * Run the MD5 Transform one more time, since we want
		 * to add at least minimal obscuring of the inputs to
		 * add_entropy_word().  --- TYT
		 */
		MD5Transform(tmp, m_pool);
		
		/* Copy data to destination buffer */
		i = MIN(nbytes, 16);
		memcpy(buf, (unsigned char const *)tmp, i);
		nbytes -= i;
		buf += i;
	}
}
