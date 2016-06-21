#include "erasure_code.h"
#include "useisa.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static unsigned char *encode_matrix;
static unsigned char *decode_matrix;
static unsigned int *decode_index;
static unsigned char *invert_matrix;
static unsigned char *gf_tbls[2];

static int k;
static int m;
static int rows;

static int gf_gen_decode_matrix(unsigned char *encode_matrix,
				unsigned char *decode_matrix,
				unsigned char *invert_matrix,
				unsigned int *decode_index,
				unsigned char *src_err_list,
				unsigned char *src_in_err,
				int nerrs, int nsrcerrs)
{
	int i, j, p;
	int r;
	unsigned char *backup, *b, s;
	int incr = 0;

	b = malloc(m * k);
	backup = malloc(m * k);

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
		memcpy(b, backup, m * k);
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


void init_erasure(int mv, int kv)
{
    k = kv;
    m = mv;
    rows = m - k;

    encode_matrix = (unsigned char *) malloc(m*k*sizeof(unsigned char));
    decode_matrix = (unsigned char *) malloc(m*k*sizeof(unsigned char));
    decode_index = (unsigned int *) malloc(m*sizeof(unsigned int));
    invert_matrix = (unsigned char *) malloc(m*k*sizeof(unsigned char));
    gf_tbls[0] = (unsigned char *) malloc(32*k*rows*sizeof(unsigned char));
    gf_tbls[1] = (unsigned char *) malloc(32*k*rows*sizeof(unsigned char));

    gf_gen_rs_matrix(encode_matrix, m, k);
    ec_init_tables(k, rows, encode_matrix+k*k, gf_tbls[0]);
}

void encode_erasure(unsigned char *original, size_t original_len, unsigned char **result, size_t *result_frg_len)
{
    int original_len_digit = 0;
    
    // Calculate digit of original length
    int temp_len = original_len;
    while (temp_len != 0)
    {
	original_len_digit++;
	temp_len /= 10;
    }
    
    int save_len = k*((original_len + original_len_digit + k)/k);
    unsigned char *save = (unsigned char *) malloc(save_len * sizeof(unsigned char));
    memset(save, 2, save_len);
    memcpy(save, original, original_len);

    temp_len = original_len;
    for (int i = 0; i < original_len_digit; i++)
    {
	save[save_len - 1 - i] = '0' + temp_len % 10;
	temp_len /= 10;
    }

    //printf("after length = %d\n", save_len);

    int frg_len = save_len/k;

    for (int i = 0; i < m; i++)
    {
	result[i] = (unsigned char *) malloc((frg_len + 1) * sizeof(unsigned char));
	result[i][frg_len] = 0;
    }

    //printf("after frg malloc\n");

    for (int i = 0; i < k; i++)
    {
	memcpy(result[i], save+i*frg_len, frg_len);
    }

    ec_encode_data(frg_len, k, rows, gf_tbls[0], result, result+k);

    //printf("after encode\n");
    /*
    for (int i = 0; i < k; i++)
    {
        printf("%s\n", result[i]);
    }
    */
    
    free(save);
}

void after_encode_erasure(unsigned char **result)
{
    for (int i = 0; i < m; i++)
    {
	free(result[i]);
    }
}

void decode_erasure(unsigned char **data, size_t frg_len, unsigned char **result)
{
    unsigned char *src_err_lst = (unsigned char *) malloc(m*sizeof(unsigned char));
    for (int i = 0, r = 0; i < m; i++, r++)
    {
	while (r < m && data[r] != NULL)
	{
	    r++;
	}

	if (r >= m)
	{
	    src_err_lst[i] = 0;
	}
	else
	{
	    src_err_lst[i] = r;
	}
    }


    unsigned char *src_in_err = (unsigned char *) malloc(m*sizeof(unsigned char));
    int nerrs = 0, nsrcerrs = 0;
    for (int i = 0; i < m; i++)
    {
	src_in_err[i] = (data[i] == NULL)?1:0;

	if (src_in_err[i])
	{
	    nerrs++;
	    if (i < k)
	    {
		nsrcerrs++;
	    }
	}
    }


    
    int decode_err = gf_gen_decode_matrix(encode_matrix, decode_matrix,
					  invert_matrix, decode_index,
					  src_err_lst, src_in_err,
					  nerrs, nsrcerrs);

    if (decode_err)
    {
	printf("Decoding Error.\n");

	exit(-1);
    }



    ec_init_tables(k, nerrs, decode_matrix, gf_tbls[1]);

    unsigned char **recov = (unsigned char **) malloc(k*sizeof(unsigned char *));
    for (int i = 0; i < k; i++)
    {
	recov[i] = data[decode_index[i]];
    }
    
    unsigned char **temp_buffer = (unsigned char **) malloc(nerrs*sizeof(unsigned char *));
    for (int i = 0; i < nerrs; i++)
    {
	temp_buffer[i] = (unsigned char *) malloc(frg_len*sizeof(unsigned char));
	temp_buffer[i][frg_len] = 0;
    }

    ec_encode_data(frg_len, k, nerrs, gf_tbls[1], recov, temp_buffer);




    size_t save_len = frg_len * k;
    unsigned char *save = (unsigned char *) malloc(save_len*sizeof(unsigned char));

    for (int i = 0; i < nerrs; i++)
    {
	memcpy(save+frg_len*src_err_lst[i], temp_buffer[i], frg_len);
	free(temp_buffer[i]);
    }
    for (int i = 0; i < k; i++)
    {
	if (decode_index[i] < k)
	{
	    memcpy(save+frg_len*decode_index[i], recov[i], frg_len);
	}
    }
    free(recov);

    size_t result_len = 0;
    int multiplyer = 1;
    for (int i = save_len-1; i > 0 && save[i] != 2; i--)
    {
	result_len += (save[i] - '0') * multiplyer;
	multiplyer *= 10;
    }
    *result = (unsigned char *) malloc((result_len + 1) * sizeof(unsigned char));



    memcpy(*result, save, result_len);
    (*result)[result_len] = 0;
    free(src_in_err);
    free(src_err_lst);
    free(save);
}

void after_decode_erasure(unsigned char *result)
{
    free(result);
}

void deinit(void)
{
    free(encode_matrix);
    free(decode_matrix);
    free(decode_index);
    free(invert_matrix);
    free(gf_tbls[0]);
    free(gf_tbls[1]);
}

/* MainFunction For Test */
/*
int main(void)
{
    char buffer[256];
    
    scanf("%s", buffer);

    init_erasure(6, 4);

    unsigned char *result[6];
    encode_erasure(buffer, strlen(buffer), result, NULL);

    for (int i = 0; i < m; i++)
    {
	printf("%s\n", result[i]);
    }

    unsigned char *decoded;
    
    decode_erasure(result, strlen(result[0]), &decoded);

    printf("%s\n", decoded);
}
*/
