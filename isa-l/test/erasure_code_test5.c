/**********************************************************************
  Copyright(c) 2011-2015 Intel Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions 
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>		// for memset, memcmp
#include "erasure_code.h"
#include "erasure_code_test.h"
#include "types.h"

//typedef unsigned char u8;

//static unsigned char *Enc_matrix;
//static int TB; //total block
//static int DB; //data block
void dump(unsigned char *buf, int len)
{
	int i;
	for (i = 0; i < len;) {
		printf(" %2x", 0xff & buf[i++]);
		if (i % 32 == 0)
			printf("\n");
	}
	printf("\n");
}

void dump_matrix(unsigned char **s, int k, int m)
{
	int i, j;
	for (i = 0; i < k; i++) {
		for (j = 0; j < m; j++) {
			printf(" %2x", s[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

void dump_u8xu8(unsigned char *s, int k, int m)
{
	int i, j;
	for (i = 0; i < k; i++) {
		for (j = 0; j < m; j++) {
			printf(" %2x", 0xff & s[j + (i * m)]);
		}
		printf("\n");
	}
	printf("\n");
}

// Generate Random errors
static void gen_err_list(unsigned char *src_err_list,
			 unsigned char *src_in_err, int *pnerrs, int *pnsrcerrs, int k, int m)
{
	int i, err;
	int nerrs = 0, nsrcerrs = 0;

	for (i = 0, nerrs = 0, nsrcerrs = 0; i < m && nerrs < m - k; i++) {
		err = 1 & rand();
		src_in_err[i] = err;
		if (err) {
			src_err_list[nerrs++] = i;
			if (i < k) {
				nsrcerrs++;
			}
		}
	}
	if (nerrs == 0) {	// should have at least one error
		while ((err = (rand() % KMAX)) >= m) ;
		src_err_list[nerrs++] = err;
		src_in_err[err] = 1;
		if (err < k)
			nsrcerrs = 1;
	}
	*pnerrs = nerrs;
	*pnsrcerrs = nsrcerrs;
	return;
}

//#define NO_INVERT_MATRIX -2
// Generate decode matrix from encode matrix
static int gf_gen_decode_matrix(unsigned char *encode_matrix,
				unsigned char *decode_matrix,
				unsigned char *invert_matrix,
				unsigned int *decode_index,
				unsigned char *src_err_list,
				unsigned char *src_in_err,
				int nerrs, int nsrcerrs, int k, int m)
{
	int i, j, p;
	int r;
	unsigned char *backup, *b, s;
	int incr = 0;

	b = malloc(MMAX * KMAX);
	backup = malloc(MMAX * KMAX);

	if (b == NULL || backup == NULL) {
		printf("Test failure! Error with malloc\n");
		free(b);
		free(backup);
		return -1;
	}
	// Construct matrix b by removing error rows
	for (i = 0, r = 0; i < k; i++, r++) {
		while (src_in_err[r])
			r++;
		for (j = 0; j < k; j++) {
			b[k * i + j] = encode_matrix[k * r + j];
			backup[k * i + j] = encode_matrix[k * r + j];
		}
		decode_index[i] = r;
	}
	incr = 0;
	while (gf_invert_matrix(b, invert_matrix, k) < 0) {
		if (nerrs == (m - k)) {
			free(b);
			free(backup);
			printf("BAD MATRIX\n");
			return NO_INVERT_MATRIX;
		}
		incr++;
		memcpy(b, backup, MMAX * KMAX);
		for (i = nsrcerrs; i < nerrs - nsrcerrs; i++) {
			if (src_err_list[i] == (decode_index[k - 1] + incr)) {
				// skip the erased parity line
				incr++;
				continue;
			}
		}
		if (decode_index[k - 1] + incr >= m) {
			free(b);
			free(backup);
			printf("BAD MATRIX\n");
			return NO_INVERT_MATRIX;
		}
		decode_index[k - 1] += incr;
		for (j = 0; j < k; j++)
			b[k * (k - 1) + j] = encode_matrix[k * decode_index[k - 1] + j];

	};

	for (i = 0; i < nsrcerrs; i++) {
		for (j = 0; j < k; j++) {
			decode_matrix[k * i + j] = invert_matrix[k * src_err_list[i] + j];
		}
	}
	/* src_err_list from encode_matrix * invert of b for parity decoding */
	for (p = nsrcerrs; p < nerrs; p++) {
		for (i = 0; i < k; i++) {
			s = 0;
			for (j = 0; j < k; j++)
				s ^= gf_mul(invert_matrix[j * k + i],
					    encode_matrix[k * src_err_list[p] + j]);

			decode_matrix[k * p + i] = s;
		}
	}
	free(b);
	free(backup);
	return 0;
}

unsigned char* signedtounsigned(char* str, int n){
	unsigned char* result = (unsigned char *)malloc(n*sizeof(unsigned char));
	for(int i = 0; i < n; i++){
		if(str[i] == '\0') break;
		result[i] = str[i];
	}
	return result;
}

unsigned char * rearrange(unsigned char* recov[], unsigned char* temp_buffs[], int ind, unsigned int decode_index[]){
	int i = 0,j = 0,l = 0;
	unsigned char * result;
	result = (unsigned char *) calloc (4096,sizeof(unsigned char *));
	for( i = 0 ; i < DB ; i ++) {
		if(decode_index[j] != i ){
			//result[i] = (unsigned char*) malloc(sizeof(strlen((char *)temp_buffs[l])));
			strcat((char *)result, (char *)temp_buffs[l+DB]);
			//strcpy( result[i], temp_buffs[l]);
			l++;
		}
		else{
			//result[i] = (unsigned char*) malloc(sizeof(strlen((char *)recov[j])));
			//strcpy( result[i], recov[j]);
			strcat((char *)result, (char *)recov[j]);
			j++;
		}
	}
	if( result != NULL)
		return result;
	else
		return NULL;
}
/*
 	if user wants to change # of data block and parity block,
	then the uesr uses arguments that represent "--m 4 --p 3".
	In that case, we supply init2 function.
	other cases, we supply init0 function.
 */
int init0(void){
	Enc_matrix = malloc(MMAX*KMAX);
	TB = 6;
	DB = 4;
	if(Enc_matrix != NULL)
		return 0;
	else
		return -1;
}

int init2(int data_block_num, int parity_block_num){
	Enc_matrix = malloc(MMAX*KMAX);
	if( data_block_num + parity_block_num > MMAX || data_block_num > KMAX){
		printf("too larger # of data blocks!");
		return -1;
	}
	TB = data_block_num + parity_block_num;
	DB = data_block_num;	
	if(Enc_matrix != NULL)
		return 0;
	else
		return -1;
}
unsigned char** encode(char * origin_data){
	int len = strlen(origin_data);
	int n = len / DB;
	int md = len % DB;
	int i = 0;
	
	char *strip_data[DB];
	for(i = 0; i < md; i++){
		strip_data[i] = (char *)malloc((n+1)*sizeof(char));
		strncpy(strip_data[i], origin_data + (n +1) * i, n + 1);
	}

	for(i = md; i< DB; i++){
		strip_data[i] = (char *)malloc(n*sizeof(char));
		strncpy(strip_data[i], origin_data + (n+1) * md + n *(i - md),n);
	}

	void *buf;
	unsigned char *buffs[TEST_SOURCES];
	unsigned char *g_tbls=malloc(KMAX * TEST_SOURCES * 32);
	for ( i = 0 ; i < TEST_SOURCES; i++){
		if(posix_memalign(&buf, 64, TEST_LEN)) {
			printf("alloc error: FAIL");
			return -1;
		}
		buffs[i]=buf;
	}
	for( i = 0 ; i < DB ; i++ )
		buffs[i] = signedtounsigned(strip_data[i], TEST_SOURCES); 
	gf_gen_rs_matrix(Enc_matrix, TB , DB);
	ec_init_tables(DB, TB-DB, &Enc_matrix[DB*DB], g_tbls);
	ec_encode_data(TEST_LEN, DB , TB-DB, g_tbls, buffs, &buffs[DB]);
	return buffs;
}
/*
 * 	buffs : integrated data block from each server
 *  src_in_err : boolean table?  length is TEST_SOURCES
 		switch (value of element)
			case : 0 => alive server
			case : 1 => failure server
		e.g.
			[ 0 | 0 | 1 | 0 | 1 | 0 | ... ] => server0, server1, server3, server5 is alive and server2, server4 is dead.
 *	src_err_list : this array saves failure sever indexes, length is TEST_SOURCES
 		e.g.
			[ 2 | 4 | ?? | ...] => in above case, this is appropriate src_err_list.
 *
 * nerrs : # of total failure server.
 * nsrcerrs : # of total failure server that saves data block not parity block.
 */
 
unsigned char** decode(unsigned char* buffs[], unsigned char src_in_err[], unsigned char src_err_list[], int nerrs, int nsrcerrs){
	int i,re = 0;
	void *buf;
	unsigned int decode_index[MMAX];
	unsigned char *temp_buffs[TEST_SOURCES];
	unsigned char *decode_matrix, *invert_matrix, *g_tbls;
	unsigned char *recov[TEST_SOURCES];
	
	for( i = 0 ; i < TEST_SOURCES; i++) {
		if (posix_memalign(&buf, 64, TEST_LEN)){
			printf("alloc error: FAIl\n");
			return -1;
		}
		temp_buffs[i]=buf;
	}
	decode_matrix = malloc (MMAX*KMAX);
	invert_matrix = malloc (MMAX*KMAX);
	g_tbls = malloc (KMAX * TEST_SOURCES * 32);
	re = gf_gen_decode_matrix(Enc_matrix, decode_matrix, invert_matrix, decode_index, src_err_list, src_in_err, nerrs, nsrcerrs, DB, TB);
	if( re != 0){
		printf("Fail to gf_gen_decode_matrix\n");
		return -1;
	}
	for ( i = 0 ; i < DB ; i++ )
		recov[i]=buffs[decode_index[i]];
	ec_init_tables(DB, nerrs, decode_matrix, g_tbls);
	ec_encode_data(TEST_LEN, DB, nerrs, g_tbls, recov, &temp_buffs[DB]);
	return rearrange(recov, temp_buffs, DB, decode_index);
}
void allign (char* output[TEST_SOURCES], char **input){
	int i;
	for (i = 0 ;  i < TB; i++){
		output[i] = input[i];
	}
}

int main(void)
{
    unsigned char buffer[256];
    scanf("%s", buffer);

    init2(4,2);

    unsigned char *real[TEST_SOURCES];
    allign(real, encode(buffer));

    for (int i = 0; i < 6; i++)
    {
	printf("%s\n", real[i]);
    }
    
    return 0;
}
