#include "stdafx.h"

#include <stdio.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <ctime>

using namespace std;

extern "C" __global__ static void kernel(unsigned char* output, int outputLength, unsigned char* query, int queryLength, int N);
__device__ static unsigned char* RIPEMD160Compute(unsigned char* output, int outputLength, unsigned char* query, int queryLength, int idx, int idy, int idz);
__device__ static int RL(int x, int n);
__device__ static int F1(int x, int y, int z);
__device__ static int F2(int x, int y, int z);
__device__ static int F3(int x, int y, int z);
__device__ static int F4(int x, int y, int z);
__device__ static int F5(int x, int y, int z);
__device__ static int* ProcessBlock(int* X, int XLen0, int* H, int HLength);
extern "C" __global__ static void kernel(unsigned char* output, int outputLength, unsigned char* query, int queryLength, int N)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;

	if (idx < N)
	{
		for (int v1 = 0x73; v1 < 0x74; v1++)
		{
			RIPEMD160Compute(output, outputLength, query, queryLength, idx & 0x000000FF, idx >> 8, v1);
		}
	}
}

__device__ static unsigned char* RIPEMD160Compute(unsigned char* output, int outputLength, unsigned char* query, int queryLength, int idx, int idy, int idz)
{
	int InputLength = 8;//32;
	int InputLengthAtStart = InputLength;
	unsigned char * input = new unsigned char[InputLength];

	//password = 70 61 73 73 77 6f 72 64
	input[0] = idx;
	input[1] = idy;
	input[2] = idz;
	input[3] = 0x73;
	input[4] = 0x77;
	input[5] = 0x6f;
	input[6] = 0x72;
	input[7] = 0x64;

	int * H = new int[5];
	int HLength = 5;

	H[0] = 1732584193;
	H[1] = -271733879;
	H[2] = -1732584194;
	H[3] = 271733878;
	H[4] = -1009589776;

	int xOffset = 0;
	int byteCount = 0;
	int xBufOffset = 0;
	int offset = 0;

	int XLength = 16;
	int* X = new int[16];
	for (int i = 0; i < XLength; i++) X[i] = 0;

	unsigned char * xBuf = new unsigned char[4];
	xBuf[0] = 0;
	xBuf[1] = 0;
	xBuf[2] = 0;
	xBuf[3] = 0;
	int xBufLength2use = 4;

	while (xBufOffset != 0 && InputLength > 0)
	{
		xBuf[xBufOffset++] = input[offset];
		if (xBufOffset == xBufLength2use)
		{
			X[xOffset++] = (int)((xBuf[0] & 255) | (int)(xBuf[1] & 255) << 8 | (int)(xBuf[2] & 255) << 16 | (int)(xBuf[3] & 255) << 24);
			if (xOffset == 16)
			{
				H = ProcessBlock(X, XLength, H, HLength);
				xOffset = 0;
			}
			xBufOffset = 0;
		}
		byteCount++;
		offset++;
		InputLength--;
	}

	while (InputLength > xBufLength2use)
	{
		X[xOffset++] = (int)((input[offset] & 255) | (int)(input[offset + 1] & 255) << 8 | (int)(input[offset + 2] & 255) << 16 | (int)(input[offset + 3] & 255) << 24);
		if (xOffset == 16)
		{
			H = ProcessBlock(X, XLength, H, HLength);
			xOffset = 0;
		}
		offset += xBufLength2use;
		InputLength -= xBufLength2use;
		byteCount += xBufLength2use;
	}
	while (InputLength > 0)
	{
		xBuf[xBufOffset++] = input[offset];
		if (xBufOffset == xBufLength2use)
		{
			X[xOffset++] = (int)((xBuf[0] & 255) | (int)(xBuf[1] & 255) << 8 | (int)(xBuf[2] & 255) << 16 | (int)(xBuf[3] & 255) << 24);
			if (xOffset == 16)
			{
				H = ProcessBlock(X, XLength, H, HLength);
				xOffset = 0;
			}
			xBufOffset = 0;
		}
		byteCount++;
		offset++;
		InputLength--;
	}
	long long num6 = (long long)(byteCount << 3);
	xBuf[xBufOffset++] = 128;
	if (xBufOffset == xBufLength2use)
	{
		X[xOffset++] = (int)((xBuf[0] & 255) | (int)(xBuf[1] & 255) << 8 | (int)(xBuf[2] & 255) << 16 | (int)(xBuf[3] & 255) << 24);
		if (xOffset == 16)
		{
			H = ProcessBlock(X, XLength, H, HLength);
			xOffset = 0;
		}
		xBufOffset = 0;
	}
	byteCount++;
	while (xBufOffset != 0)
	{
		xBuf[xBufOffset++] = 0;
		if (xBufOffset == xBufLength2use)
		{
			X[xOffset++] = (int)((xBuf[0] & 255) | (int)(xBuf[1] & 255) << 8 | (int)(xBuf[2] & 255) << 16 | (int)(xBuf[3] & 255) << 24);
			xBufOffset = 0;
		}
		byteCount++;
	}
	if (xOffset > 14)
	{
		H = ProcessBlock(X, XLength, H, HLength);
		xOffset = 0;
	}

	X[14] = (int)(num6 & (long long)((unsigned long long) - 1));
	X[15] = (int)((unsigned int)num6 >> 32);

	H = ProcessBlock(X, XLength, H, HLength);
	xOffset = 0;

	unsigned char * result = new unsigned char[20];
	result[0] = (unsigned char)H[0];
	result[1] = (unsigned char)((unsigned int)H[0] >> 8);
	result[2] = (unsigned char)((unsigned int)H[0] >> 16);
	result[3] = (unsigned char)((unsigned int)H[0] >> 24);
	result[4] = (unsigned char)H[1];
	result[5] = (unsigned char)((unsigned int)H[1] >> 8);
	result[6] = (unsigned char)((unsigned int)H[1] >> 16);
	result[7] = (unsigned char)((unsigned int)H[1] >> 24);
	result[8] = (unsigned char)H[2];
	result[9] = (unsigned char)((unsigned int)H[2] >> 8);
	result[10] = (unsigned char)((unsigned int)H[2] >> 16);
	result[11] = (unsigned char)((unsigned int)H[2] >> 24);
	result[12] = (unsigned char)H[3];
	result[13] = (unsigned char)((unsigned int)H[3] >> 8);
	result[14] = (unsigned char)((unsigned int)H[3] >> 16);
	result[15] = (unsigned char)((unsigned int)H[3] >> 24);
	result[16] = (unsigned char)H[4];
	result[17] = (unsigned char)((unsigned int)H[4] >> 8);
	result[18] = (unsigned char)((unsigned int)H[4] >> 16);
	result[19] = (unsigned char)((unsigned int)H[4] >> 24);

	bool cont = true;
	int t = 0;
	while (cont)
	{
		if (result[t] == query[t])
		{
			cont = true;
			if (t == 19)
			{
				//full match found
				printf("%d : *** match found ***n", idx);
				for (int i = 0; i < 20; i++)
				{
					printf("%02X", result[i]);
					if (i < InputLengthAtStart)
					{
						output[i] = input[i];
					}
					outputLength = InputLengthAtStart;
				}
				printf("n");
				break;
			}
		}
		else
		{
			cont = false;
			break;
		}
		t++;
	}

	free(input);
	free(H);
	free(xBuf);
	free(result);
	free(output);
	free(query);

	return output;
}

__device__ static int RL(int x, int n)
{
	return x << (n & 31) | (unsigned int)x >> (32 - n & 31);
}

__device__ static int F1(int x, int y, int z)
{
	return x ^ y ^ z;
}

__device__ static int F2(int x, int y, int z)
{
	return (x & y) | (~x & z);
}

__device__ static int F3(int x, int y, int z)
{
	return (x | ~y) ^ z;
}

__device__ static int F4(int x, int y, int z)
{
	return (x & z) | (y & ~z);
}

__device__ static int F5(int x, int y, int z)
{
	return x ^ (y | ~z);
}

__device__ static int* ProcessBlock(int* X, int XLen0, int* H, int HLength)
{
	int a = H[0];
	int aa = H[0];
	int b = H[1];
	int bb = H[1];
	int c = H[2];
	int cc = H[2];
	int d = H[3];
	int dd = H[3];
	int e = H[4];
	int ee = H[4];
	a = RL(a + F1(b, c, d) + X[0], 11) + e; c = RL(c, 10);
	e = RL(e + F1(a, b, c) + X[1], 14) + d; b = RL(b, 10);
	d = RL(d + F1(e, a, b) + X[2], 15) + c; a = RL(a, 10);
	c = RL(c + F1(d, e, a) + X[3], 12) + b; e = RL(e, 10);
	b = RL(b + F1(c, d, e) + X[4], 5) + a; d = RL(d, 10);
	a = RL(a + F1(b, c, d) + X[5], 8) + e; c = RL(c, 10);
	e = RL(e + F1(a, b, c) + X[6], 7) + d; b = RL(b, 10);
	d = RL(d + F1(e, a, b) + X[7], 9) + c; a = RL(a, 10);
	c = RL(c + F1(d, e, a) + X[8], 11) + b; e = RL(e, 10);
	b = RL(b + F1(c, d, e) + X[9], 13) + a; d = RL(d, 10);
	a = RL(a + F1(b, c, d) + X[10], 14) + e; c = RL(c, 10);
	e = RL(e + F1(a, b, c) + X[11], 15) + d; b = RL(b, 10);
	d = RL(d + F1(e, a, b) + X[12], 6) + c; a = RL(a, 10);
	c = RL(c + F1(d, e, a) + X[13], 7) + b; e = RL(e, 10);
	b = RL(b + F1(c, d, e) + X[14], 9) + a; d = RL(d, 10);
	a = RL(a + F1(b, c, d) + X[15], 8) + e; c = RL(c, 10);
	aa = RL(aa + F5(bb, cc, dd) + X[5] + 1352829926, 8) + ee; cc = RL(cc, 10);
	ee = RL(ee + F5(aa, bb, cc) + X[14] + 1352829926, 9) + dd; bb = RL(bb, 10);
	dd = RL(dd + F5(ee, aa, bb) + X[7] + 1352829926, 9) + cc; aa = RL(aa, 10);
	cc = RL(cc + F5(dd, ee, aa) + X[0] + 1352829926, 11) + bb; ee = RL(ee, 10);
	bb = RL(bb + F5(cc, dd, ee) + X[9] + 1352829926, 13) + aa; dd = RL(dd, 10);
	aa = RL(aa + F5(bb, cc, dd) + X[2] + 1352829926, 15) + ee; cc = RL(cc, 10);
	ee = RL(ee + F5(aa, bb, cc) + X[11] + 1352829926, 15) + dd; bb = RL(bb, 10);
	dd = RL(dd + F5(ee, aa, bb) + X[4] + 1352829926, 5) + cc; aa = RL(aa, 10);
	cc = RL(cc + F5(dd, ee, aa) + X[13] + 1352829926, 7) + bb; ee = RL(ee, 10);
	bb = RL(bb + F5(cc, dd, ee) + X[6] + 1352829926, 7) + aa; dd = RL(dd, 10);
	aa = RL(aa + F5(bb, cc, dd) + X[15] + 1352829926, 8) + ee; cc = RL(cc, 10);
	ee = RL(ee + F5(aa, bb, cc) + X[8] + 1352829926, 11) + dd; bb = RL(bb, 10);
	dd = RL(dd + F5(ee, aa, bb) + X[1] + 1352829926, 14) + cc; aa = RL(aa, 10);
	cc = RL(cc + F5(dd, ee, aa) + X[10] + 1352829926, 14) + bb; ee = RL(ee, 10);
	bb = RL(bb + F5(cc, dd, ee) + X[3] + 1352829926, 12) + aa; dd = RL(dd, 10);
	aa = RL(aa + F5(bb, cc, dd) + X[12] + 1352829926, 6) + ee; cc = RL(cc, 10);
	e = RL(e + F2(a, b, c) + X[7] + 1518500249, 7) + d; b = RL(b, 10);
	d = RL(d + F2(e, a, b) + X[4] + 1518500249, 6) + c; a = RL(a, 10);
	c = RL(c + F2(d, e, a) + X[13] + 1518500249, 8) + b; e = RL(e, 10);
	b = RL(b + F2(c, d, e) + X[1] + 1518500249, 13) + a; d = RL(d, 10);
	a = RL(a + F2(b, c, d) + X[10] + 1518500249, 11) + e; c = RL(c, 10);
	e = RL(e + F2(a, b, c) + X[6] + 1518500249, 9) + d; b = RL(b, 10);
	d = RL(d + F2(e, a, b) + X[15] + 1518500249, 7) + c; a = RL(a, 10);
	c = RL(c + F2(d, e, a) + X[3] + 1518500249, 15) + b; e = RL(e, 10);
	b = RL(b + F2(c, d, e) + X[12] + 1518500249, 7) + a; d = RL(d, 10);
	a = RL(a + F2(b, c, d) + X[0] + 1518500249, 12) + e; c = RL(c, 10);
	e = RL(e + F2(a, b, c) + X[9] + 1518500249, 15) + d; b = RL(b, 10);
	d = RL(d + F2(e, a, b) + X[5] + 1518500249, 9) + c; a = RL(a, 10);
	c = RL(c + F2(d, e, a) + X[2] + 1518500249, 11) + b; e = RL(e, 10);
	b = RL(b + F2(c, d, e) + X[14] + 1518500249, 7) + a; d = RL(d, 10);
	a = RL(a + F2(b, c, d) + X[11] + 1518500249, 13) + e; c = RL(c, 10);
	e = RL(e + F2(a, b, c) + X[8] + 1518500249, 12) + d; b = RL(b, 10);
	ee = RL(ee + F4(aa, bb, cc) + X[6] + 1548603684, 9) + dd; bb = RL(bb, 10);
	dd = RL(dd + F4(ee, aa, bb) + X[11] + 1548603684, 13) + cc; aa = RL(aa, 10);
	cc = RL(cc + F4(dd, ee, aa) + X[3] + 1548603684, 15) + bb; ee = RL(ee, 10);
	bb = RL(bb + F4(cc, dd, ee) + X[7] + 1548603684, 7) + aa; dd = RL(dd, 10);
	aa = RL(aa + F4(bb, cc, dd) + X[0] + 1548603684, 12) + ee; cc = RL(cc, 10);
	ee = RL(ee + F4(aa, bb, cc) + X[13] + 1548603684, 8) + dd; bb = RL(bb, 10);
	dd = RL(dd + F4(ee, aa, bb) + X[5] + 1548603684, 9) + cc; aa = RL(aa, 10);
	cc = RL(cc + F4(dd, ee, aa) + X[10] + 1548603684, 11) + bb; ee = RL(ee, 10);
	bb = RL(bb + F4(cc, dd, ee) + X[14] + 1548603684, 7) + aa; dd = RL(dd, 10);
	aa = RL(aa + F4(bb, cc, dd) + X[15] + 1548603684, 7) + ee; cc = RL(cc, 10);
	ee = RL(ee + F4(aa, bb, cc) + X[8] + 1548603684, 12) + dd; bb = RL(bb, 10);
	dd = RL(dd + F4(ee, aa, bb) + X[12] + 1548603684, 7) + cc; aa = RL(aa, 10);
	cc = RL(cc + F4(dd, ee, aa) + X[4] + 1548603684, 6) + bb; ee = RL(ee, 10);
	bb = RL(bb + F4(cc, dd, ee) + X[9] + 1548603684, 15) + aa; dd = RL(dd, 10);
	aa = RL(aa + F4(bb, cc, dd) + X[1] + 1548603684, 13) + ee; cc = RL(cc, 10);
	ee = RL(ee + F4(aa, bb, cc) + X[2] + 1548603684, 11) + dd; bb = RL(bb, 10);
	d = RL(d + F3(e, a, b) + X[3] + 1859775393, 11) + c; a = RL(a, 10);
	c = RL(c + F3(d, e, a) + X[10] + 1859775393, 13) + b; e = RL(e, 10);
	b = RL(b + F3(c, d, e) + X[14] + 1859775393, 6) + a; d = RL(d, 10);
	a = RL(a + F3(b, c, d) + X[4] + 1859775393, 7) + e; c = RL(c, 10);
	e = RL(e + F3(a, b, c) + X[9] + 1859775393, 14) + d; b = RL(b, 10);
	d = RL(d + F3(e, a, b) + X[15] + 1859775393, 9) + c; a = RL(a, 10);
	c = RL(c + F3(d, e, a) + X[8] + 1859775393, 13) + b; e = RL(e, 10);
	b = RL(b + F3(c, d, e) + X[1] + 1859775393, 15) + a; d = RL(d, 10);
	a = RL(a + F3(b, c, d) + X[2] + 1859775393, 14) + e; c = RL(c, 10);
	e = RL(e + F3(a, b, c) + X[7] + 1859775393, 8) + d; b = RL(b, 10);
	d = RL(d + F3(e, a, b) + X[0] + 1859775393, 13) + c; a = RL(a, 10);
	c = RL(c + F3(d, e, a) + X[6] + 1859775393, 6) + b; e = RL(e, 10);
	b = RL(b + F3(c, d, e) + X[13] + 1859775393, 5) + a; d = RL(d, 10);
	a = RL(a + F3(b, c, d) + X[11] + 1859775393, 12) + e; c = RL(c, 10);
	e = RL(e + F3(a, b, c) + X[5] + 1859775393, 7) + d; b = RL(b, 10);
	d = RL(d + F3(e, a, b) + X[12] + 1859775393, 5) + c; a = RL(a, 10);
	dd = RL(dd + F3(ee, aa, bb) + X[15] + 1836072691, 9) + cc; aa = RL(aa, 10);
	cc = RL(cc + F3(dd, ee, aa) + X[5] + 1836072691, 7) + bb; ee = RL(ee, 10);
	bb = RL(bb + F3(cc, dd, ee) + X[1] + 1836072691, 15) + aa; dd = RL(dd, 10);
	aa = RL(aa + F3(bb, cc, dd) + X[3] + 1836072691, 11) + ee; cc = RL(cc, 10);
	ee = RL(ee + F3(aa, bb, cc) + X[7] + 1836072691, 8) + dd; bb = RL(bb, 10);
	dd = RL(dd + F3(ee, aa, bb) + X[14] + 1836072691, 6) + cc; aa = RL(aa, 10);
	cc = RL(cc + F3(dd, ee, aa) + X[6] + 1836072691, 6) + bb; ee = RL(ee, 10);
	bb = RL(bb + F3(cc, dd, ee) + X[9] + 1836072691, 14) + aa; dd = RL(dd, 10);
	aa = RL(aa + F3(bb, cc, dd) + X[11] + 1836072691, 12) + ee; cc = RL(cc, 10);
	ee = RL(ee + F3(aa, bb, cc) + X[8] + 1836072691, 13) + dd; bb = RL(bb, 10);
	dd = RL(dd + F3(ee, aa, bb) + X[12] + 1836072691, 5) + cc; aa = RL(aa, 10);
	cc = RL(cc + F3(dd, ee, aa) + X[2] + 1836072691, 14) + bb; ee = RL(ee, 10);
	bb = RL(bb + F3(cc, dd, ee) + X[10] + 1836072691, 13) + aa; dd = RL(dd, 10);
	aa = RL(aa + F3(bb, cc, dd) + X[0] + 1836072691, 13) + ee; cc = RL(cc, 10);
	ee = RL(ee + F3(aa, bb, cc) + X[4] + 1836072691, 7) + dd; bb = RL(bb, 10);
	dd = RL(dd + F3(ee, aa, bb) + X[13] + 1836072691, 5) + cc; aa = RL(aa, 10);
	c = RL(c + F4(d, e, a) + X[1] + -1894007588, 11) + b; e = RL(e, 10);
	b = RL(b + F4(c, d, e) + X[9] + -1894007588, 12) + a; d = RL(d, 10);
	a = RL(a + F4(b, c, d) + X[11] + -1894007588, 14) + e; c = RL(c, 10);
	e = RL(e + F4(a, b, c) + X[10] + -1894007588, 15) + d; b = RL(b, 10);
	d = RL(d + F4(e, a, b) + X[0] + -1894007588, 14) + c; a = RL(a, 10);
	c = RL(c + F4(d, e, a) + X[8] + -1894007588, 15) + b; e = RL(e, 10);
	b = RL(b + F4(c, d, e) + X[12] + -1894007588, 9) + a; d = RL(d, 10);
	a = RL(a + F4(b, c, d) + X[4] + -1894007588, 8) + e; c = RL(c, 10);
	e = RL(e + F4(a, b, c) + X[13] + -1894007588, 9) + d; b = RL(b, 10);
	d = RL(d + F4(e, a, b) + X[3] + -1894007588, 14) + c; a = RL(a, 10);
	c = RL(c + F4(d, e, a) + X[7] + -1894007588, 5) + b; e = RL(e, 10);
	b = RL(b + F4(c, d, e) + X[15] + -1894007588, 6) + a; d = RL(d, 10);
	a = RL(a + F4(b, c, d) + X[14] + -1894007588, 8) + e; c = RL(c, 10);
	e = RL(e + F4(a, b, c) + X[5] + -1894007588, 6) + d; b = RL(b, 10);
	d = RL(d + F4(e, a, b) + X[6] + -1894007588, 5) + c; a = RL(a, 10);
	c = RL(c + F4(d, e, a) + X[2] + -1894007588, 12) + b; e = RL(e, 10);
	cc = RL(cc + F2(dd, ee, aa) + X[8] + 2053994217, 15) + bb; ee = RL(ee, 10);
	bb = RL(bb + F2(cc, dd, ee) + X[6] + 2053994217, 5) + aa; dd = RL(dd, 10);
	aa = RL(aa + F2(bb, cc, dd) + X[4] + 2053994217, 8) + ee; cc = RL(cc, 10);
	ee = RL(ee + F2(aa, bb, cc) + X[1] + 2053994217, 11) + dd; bb = RL(bb, 10);
	dd = RL(dd + F2(ee, aa, bb) + X[3] + 2053994217, 14) + cc; aa = RL(aa, 10);
	cc = RL(cc + F2(dd, ee, aa) + X[11] + 2053994217, 14) + bb; ee = RL(ee, 10);
	bb = RL(bb + F2(cc, dd, ee) + X[15] + 2053994217, 6) + aa; dd = RL(dd, 10);
	aa = RL(aa + F2(bb, cc, dd) + X[0] + 2053994217, 14) + ee; cc = RL(cc, 10);
	ee = RL(ee + F2(aa, bb, cc) + X[5] + 2053994217, 6) + dd; bb = RL(bb, 10);
	dd = RL(dd + F2(ee, aa, bb) + X[12] + 2053994217, 9) + cc; aa = RL(aa, 10);
	cc = RL(cc + F2(dd, ee, aa) + X[2] + 2053994217, 12) + bb; ee = RL(ee, 10);
	bb = RL(bb + F2(cc, dd, ee) + X[13] + 2053994217, 9) + aa; dd = RL(dd, 10);
	aa = RL(aa + F2(bb, cc, dd) + X[9] + 2053994217, 12) + ee; cc = RL(cc, 10);
	ee = RL(ee + F2(aa, bb, cc) + X[7] + 2053994217, 5) + dd; bb = RL(bb, 10);
	dd = RL(dd + F2(ee, aa, bb) + X[10] + 2053994217, 15) + cc; aa = RL(aa, 10);
	cc = RL(cc + F2(dd, ee, aa) + X[14] + 2053994217, 8) + bb; ee = RL(ee, 10);
	b = RL(b + F5(c, d, e) + X[4] + -1454113458, 9) + a; d = RL(d, 10);
	a = RL(a + F5(b, c, d) + X[0] + -1454113458, 15) + e; c = RL(c, 10);
	e = RL(e + F5(a, b, c) + X[5] + -1454113458, 5) + d; b = RL(b, 10);
	d = RL(d + F5(e, a, b) + X[9] + -1454113458, 11) + c; a = RL(a, 10);
	c = RL(c + F5(d, e, a) + X[7] + -1454113458, 6) + b; e = RL(e, 10);
	b = RL(b + F5(c, d, e) + X[12] + -1454113458, 8) + a; d = RL(d, 10);
	a = RL(a + F5(b, c, d) + X[2] + -1454113458, 13) + e; c = RL(c, 10);
	e = RL(e + F5(a, b, c) + X[10] + -1454113458, 12) + d; b = RL(b, 10);
	d = RL(d + F5(e, a, b) + X[14] + -1454113458, 5) + c; a = RL(a, 10);
	c = RL(c + F5(d, e, a) + X[1] + -1454113458, 12) + b; e = RL(e, 10);
	b = RL(b + F5(c, d, e) + X[3] + -1454113458, 13) + a; d = RL(d, 10);
	a = RL(a + F5(b, c, d) + X[8] + -1454113458, 14) + e; c = RL(c, 10);
	e = RL(e + F5(a, b, c) + X[11] + -1454113458, 11) + d; b = RL(b, 10);
	d = RL(d + F5(e, a, b) + X[6] + -1454113458, 8) + c; a = RL(a, 10);
	c = RL(c + F5(d, e, a) + X[15] + -1454113458, 5) + b; e = RL(e, 10);
	b = RL(b + F5(c, d, e) + X[13] + -1454113458, 6) + a; d = RL(d, 10);
	bb = RL(bb + F1(cc, dd, ee) + X[12], 8) + aa; dd = RL(dd, 10);
	aa = RL(aa + F1(bb, cc, dd) + X[15], 5) + ee; cc = RL(cc, 10);
	ee = RL(ee + F1(aa, bb, cc) + X[10], 12) + dd; bb = RL(bb, 10);
	dd = RL(dd + F1(ee, aa, bb) + X[4], 9) + cc; aa = RL(aa, 10);
	cc = RL(cc + F1(dd, ee, aa) + X[1], 12) + bb; ee = RL(ee, 10);
	bb = RL(bb + F1(cc, dd, ee) + X[5], 5) + aa; dd = RL(dd, 10);
	aa = RL(aa + F1(bb, cc, dd) + X[8], 14) + ee; cc = RL(cc, 10);
	ee = RL(ee + F1(aa, bb, cc) + X[7], 6) + dd; bb = RL(bb, 10);
	dd = RL(dd + F1(ee, aa, bb) + X[6], 8) + cc; aa = RL(aa, 10);
	cc = RL(cc + F1(dd, ee, aa) + X[2], 13) + bb; ee = RL(ee, 10);
	bb = RL(bb + F1(cc, dd, ee) + X[13], 6) + aa; dd = RL(dd, 10);
	aa = RL(aa + F1(bb, cc, dd) + X[14], 5) + ee; cc = RL(cc, 10);
	ee = RL(ee + F1(aa, bb, cc) + X[0], 15) + dd; bb = RL(bb, 10);
	dd = RL(dd + F1(ee, aa, bb) + X[3], 13) + cc; aa = RL(aa, 10);
	cc = RL(cc + F1(dd, ee, aa) + X[9], 11) + bb; ee = RL(ee, 10);
	bb = RL(bb + F1(cc, dd, ee) + X[11], 11) + aa; dd = RL(dd, 10);
	dd += c + H[1];

	H[1] = H[2] + d + ee;
	H[2] = H[3] + e + aa;
	H[3] = H[4] + a + bb;
	H[4] = H[0] + b + cc;
	H[0] = dd;

	for (int a11 = 0; a11 != XLen0; a11++)
	{
		X[a11] = 0;
	}

	return H;
}

int main(void)
{
	printf("Copyright (C) 2014 Sean Bradley\n\n");
	printf("Permission is hereby granted, free of charge, to any person obtaining a copy of this software and\nassociated documentation files (the 'Software'),\nto deal in the Software without restriction,\nincluding without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,\nand/or sell copies of the Software,\nand to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n");
	printf("The above copyright notice and this permission notice shall be included in all copies or substantialnportions of the Software.\n");
	printf("\nTHE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT\nLIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\nIN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\nWHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THEnSOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n");
	printf("\nSean Bradley\n\n");

	int N = 65536;
	int block_size = 256;
	int n_blocks = N / block_size + (N%block_size == 0 ? 0 : 1);

	printf("%d %d\n", block_size, n_blocks);

	int host_responseLength, dev_responseLength;
	host_responseLength = 20;

	unsigned char *host_response, *dev_response;
	size_t size = 20 * sizeof(unsigned char);
	host_response = (unsigned char *)malloc(size);
	for (int i = 0; i < size; i++)
	{
		host_response[i] = 0;
	}
	cudaMalloc((void **)&dev_response, size);
	cudaMemcpy(dev_response, host_response, size, cudaMemcpyHostToDevice);

	printf("response buffer size=%d\n", size);
	//password = 2c 08 e8 f5 88 47 50 a7 b9 9f 6f 2f 34 2f c6 38 db 25 ff 31
	//the RIPEMD160 to find
	unsigned char *host_query, *dev_query;
	size = 20 * sizeof(unsigned char);
	host_query = (unsigned char *)malloc(size);
	host_query[0] = 0x2C;
	host_query[1] = 0x08;
	host_query[2] = 0xE8;
	host_query[3] = 0xF5;
	host_query[4] = 0x88;
	host_query[5] = 0x47;
	host_query[6] = 0x50;
	host_query[7] = 0xA7;
	host_query[8] = 0xB9;
	host_query[9] = 0x9F;
	host_query[10] = 0x6F;
	host_query[11] = 0x2F;
	host_query[12] = 0x34;
	host_query[13] = 0x2F;
	host_query[14] = 0xC6;
	host_query[15] = 0x38;
	host_query[16] = 0xDB;
	host_query[17] = 0x25;
	host_query[18] = 0xFF;
	host_query[19] = 0x31;
	cudaMalloc((void **)&dev_query, size);
	cudaMemcpy(dev_query, host_query, size, cudaMemcpyHostToDevice);

	std::clock_t start;

	start = std::clock();
	kernel <<<n_blocks, block_size >>> (dev_response, 20, dev_query, 20, N);
	cudaMemcpy(host_response, dev_response, sizeof(unsigned char) * 20, cudaMemcpyDeviceToHost);
	printf("ms = %lu.nn", (std::clock() - start));

	cudaFree(dev_response); cudaFree(dev_query);

	for (int j = 0; j < host_responseLength; j++)
	{
		//printf("%02X", host_response[j + (i * 20)]);
		printf("%02X", host_response[j]);
	}
	printf("n");

	cudaError_t err = cudaGetLastError();
	if (err != cudaSuccess) printf("%sn", cudaGetErrorString(err));

	free(host_response);

	printf("\nCopyright Sean Bradley 2014\n\n");

	system("pause");

}
