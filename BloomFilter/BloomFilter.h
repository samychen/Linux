#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define __MGAIC_CODE__		(0x01464C42)

#define BYTE_BITS           (8)
#define MIX_UINT64(v)		((uint32_t)((v>>32)^(v)))
#define SETBIT(filter, n)   (filter->pstFilter[n / BYTE_BITS] |= (1 << (n % BYTE_BITS)))
#define GETBIT(filter, n)   (filter->pstFilter[n / BYTE_BITS] & (1 << (n % BYTE_BITS)))


typedef struct BaseBloomFilter
{
	uint8_t			cInitFlag;		// ��ʼ����־��Ϊ0ʱ�ĵ�һ��Add()���stFilter[]����ʼ��
	uint8_t			cResv[3];

	uint32_t		dwMaxItems;		// n - BloomFilter�����Ԫ�ظ��� (������)
	double			dProbFalse;		// p - �������ʣ��������֮һ:0.00001 (������)
	uint32_t		dwFilterBits;	// m - BloomFilter�ı���λ��
	uint32_t		dwHashFuncs;	// k - BloomFilter�Ĺ�ϣ��������

	uint32_t		dwSeed;			// MurmurHash������ƫ����
	uint32_t		dwCount;		// Add()�ļ���������MAX_BLOOMFILTER_N�򷵻�ʧ��

	uint32_t		dwFilterSize;	// BloomFilter���ܴ�С dwFilterBits / 8
	unsigned char*	pstFilter;		// �洢BloomFilter��ָ��
	uint32_t*		pdwHashPos;		// ��ϣ�������
} BaseBloomFilter;

typedef struct BloomFileHead
{
	uint32_t	dwMagicCode;
	uint32_t	dwSeed;
	uint32_t	dwCount;

	uint32_t	dwMaxItems;
	double		dProbFalse;
	uint32_t	dwFilterBits;
	uint32_t	dwHashFuncs;

	uint32_t	dwResv[6];
	uint32_t	dwFileCrc;
	uint32_t	dwFilterSize;
} BloomFileHead;


// ����BloomFilter�Ĳ���m,k
static inline void CalcBloomFilterParam(uint32_t n, double p, uint32_t* pm, uint32_t* pk);

// ����Ŀ�꾫�Ⱥ����ݸ�������ʼ��BloomFilter�ṹ
inline int InitBloomFilter(BaseBloomFilter* pstBloomFilter, uint32_t dwSeed, uint32_t dwMaxItems, double dProbFalse);

// �ͷ�BloomFilter
inline int FreeBloomFilter(BaseBloomFilter* pstBloomFilter);

// ����BloomFilter
// ע��: Reset()��������������ʼ��stFilter�����ǵ�һ��Add()ʱȥmemset
inline int ResetBloomFilter(BaseBloomFilter* pstBloomFilter);

// ��ResetBloomFilter��ͬ�����ú�����memset�ڴ�
inline int RealResetBloomFilter(BaseBloomFilter* pstBloomfilter);


// MurmurHash2, 64-bit versions, by Austin Appleby
// https://sites.google.com/site/murmurhash/
uint64_t MurmurHash2_x64(const void * key, int len, uint32_t seed);


// ˫��ɢ�з�װ
void BloomHash(BaseBloomFilter* pstBloomFilter, const void* key, int len);

// ��BloomFilter������һ��Ԫ��
// �ɹ�����0����������ݳ�������ֵʱ����1��ʾ�û�
int BloomFilter_Add(BaseBloomFilter* pstBloomFilter, const void* key, int len);

// ���һ��Ԫ���Ƿ���bloomfilter��
// ���أ�0-���ڣ�1-�����ڣ�������ʾʧ��
int BloomFilter_Check(BaseBloomFilter* pstBloomFilter, const void* key, int len);


/* �ļ���ط�װ */
// �����ɺõ�BloomFilterд���ļ�
inline int SaveBloomFilterToFile(BaseBloomFilter* pstBloomFilter, char* szFileName);

// ���ļ���ȡ���ɺõ�BloomFilter
inline int LoadBloomFilterFromFile(BaseBloomFilter* pstBloomFilter, char *szFileName);