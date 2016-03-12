#include <stdlib.h>
#include <stdio.h>

static unsigned int MAX_SIZES[12] = { 0x80, 0x100, 0x200, 0x400, 0x700, 0xD00, 0x1500, 0x2500, 0x5100, 0x9200, 0x10900, 0x10900, } ;

static unsigned char MODE_INIT[12][12] =
{
    { 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06 }, // mode 0
	{ 0x05, 0x06, 0x07, 0x07, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x06, 0x06 }, // mode 1
	{ 0x05, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08 }, // mode 2
	{ 0x05, 0x06, 0x07, 0x08, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09 }, // mode 3
    { 0x06, 0x07, 0x07, 0x08, 0x07, 0x08, 0x09, 0x09, 0x08, 0x09, 0x0A, 0x0A }, // mode 4
	{ 0x06, 0x07, 0x07, 0x08, 0x07, 0x09, 0x09, 0x0A, 0x08, 0x0A, 0x0B, 0x0B }, // mode 5
	{ 0x06, 0x07, 0x08, 0x08, 0x07, 0x09, 0x09, 0x0A, 0x08, 0x0A, 0x0B, 0x0C }, // mode 6
	{ 0x06, 0x07, 0x08, 0x08, 0x07, 0x09, 0x09, 0x0A, 0x09, 0x0A, 0x0C, 0x0D }, // mode 7
    { 0x06, 0x07, 0x07, 0x08, 0x07, 0x09, 0x09, 0x0C, 0x09, 0x0A, 0x0C, 0x0E }, // mode 8
	{ 0x06, 0x07, 0x08, 0x09, 0x07, 0x09, 0x0A, 0x0C, 0x09, 0x0B, 0x0D, 0x0F }, // mode 9
	{ 0x06, 0x07, 0x08, 0x08, 0x07, 0x0A, 0x0B, 0x0B, 0x09, 0x0C, 0x0D, 0x10 }, // mode A
	{ 0x06, 0x08, 0x08, 0x09, 0x07, 0x0B, 0x0C, 0x0C, 0x09, 0x0D, 0x0E, 0x11 }, // mode B
} ;

static unsigned short static_token_lens[12] = { 2, 2, 2, 2, 6, 0x0A, 0x0A, 0x12, 0x16, 0x2A, 0x8A, 0x4012 };
static unsigned char static_reps[4] = { 0, 2, 6, 14 };
static unsigned char static_rep_bits_cnts[4] = { 1, 2, 3, 4 };
static unsigned char static_token_extra_bits[12] = { 1,  1,  1,  1,  2,  3,  3,  4,  4,  5,  7, 14 };

unsigned char *src;

unsigned int read_pos, src_size, max_encoded_size, endoffset, token_run_len;
unsigned char write_byte;

unsigned short last_reps, encoded_reps, last_token_len, encoded_token_len;
unsigned char last_reps_bits_cnt, encoded_reps_bits_cnt, last_token_len_bits_cnt, encoded_token_len_bits_cnt, last_offset_bits_cnt, encoded_offset_bits_cnt;
unsigned int last_offset, encoded_offset;

char write_bits_cnt;

unsigned char counts[8];
unsigned int offsets[8];

unsigned int run_base_offs[12];
unsigned char run_extra_bits[12];

static unsigned short word_get_bits_right(unsigned short value, unsigned char count){
	return (unsigned short)(((1 << count)-1) & (value >> 0));
}

static unsigned int long_get_bits_right(unsigned int value, unsigned char count){
	return (unsigned int)(((1 << count)-1) & (value >> 0));
}

static void copy_bytes(unsigned char *dst, unsigned char *src, int count){
	for (int i = 0; i < count; i++){
		dst[i] = src[i];
	}
}

static unsigned short read_word(unsigned char *buf){
	return ((buf[0] << 8) | buf[1]);
}

static unsigned int read_dword(unsigned char *buf){
	return ((read_word(&buf[0]) << 16) | read_word(&buf[2]));
}

static void write_word(unsigned char *dst, unsigned short value){
	dst[0] = (value >> 8) & 0xFF;
	dst[1] = (value >> 0) & 0xFF;
}

static void write_dword(unsigned char *dst, unsigned int value){
	write_word(&dst[0], (value >> 16) & 0xFFFF);
	write_word(&dst[2], (value >> 0) & 0xFFFF);
}

static unsigned int checksum(unsigned char *buf, unsigned int size){
	unsigned int sum = 0;

	size >>= 1;
	for (int i = 0; i < size; i++){
		sum += read_word(&buf[i*2]);
	}

	return (sum+7);
}

void find_repeats(){
	for (int i = 0; i < 8; i++){
		counts[i] = 0;
	}

	unsigned int limit = read_pos + max_encoded_size;

	if (limit > src_size){
		limit = src_size;
	}

	int min_reps = 1;
	unsigned int offset = read_pos;

	while (offset < limit){
		while (src[++offset] != src[read_pos] && offset < limit);

		int reps = 1;

		while (src[read_pos+reps] == src[offset+reps] && reps < 0xFF && (offset+reps) < limit){
			reps++;
		}

		if (reps > min_reps){
			min_reps = reps;

			if (reps <= 8){
				if (counts[reps-2] == 0){
					counts[reps-2] = reps;

					offsets[reps-2] = offset - read_pos - 1;
				}
			}
			else{
				counts[7] = reps;
				offsets[7] = offset - read_pos - 1;

				if (reps == 0xFF){
					break;
				}
			}
		}
	}
}

int encode_match(unsigned char count, unsigned int offset){
	count -= 2;
	if (count <= 3){
		last_reps = static_reps[count];
		last_reps_bits_cnt = static_rep_bits_cnts[count];
	}
	else{
		if (count <= 11){
			last_reps = 0xF0 | (count-4);
			last_reps_bits_cnt = 8;
		}
		else{
			last_reps = 0x1F00 | (count+2);
			last_reps_bits_cnt = 13;
		}
		count = 3;
	}

	if (static_token_lens[count] > token_run_len){
		last_token_len_bits_cnt = static_token_extra_bits[count]+1;
		last_token_len = /*00+*/ word_get_bits_right(token_run_len, static_token_extra_bits[count]);
	}
	else if (static_token_lens[count+4] > token_run_len){
		last_token_len_bits_cnt = static_token_extra_bits[count+4]+2;
		last_token_len = /*10+*/ (2 << static_token_extra_bits[count+4]) | word_get_bits_right(token_run_len - static_token_lens[count], static_token_extra_bits[count+4]);
	}
	else if (static_token_lens[count+8] > token_run_len){
		last_token_len_bits_cnt = static_token_extra_bits[count+8]+2;
		last_token_len = /*11+*/ (3 << static_token_extra_bits[count+8]) | word_get_bits_right(token_run_len - static_token_lens[count+4], static_token_extra_bits[count+8]);
	}
	else{
		return 0;
	}

	if (run_base_offs[count] > offset){
		last_offset_bits_cnt = run_extra_bits[count]+1;
		last_offset = /*00+*/ long_get_bits_right(offset, run_extra_bits[count]);
	}
	else if (run_base_offs[count+4] > offset){
		last_offset_bits_cnt = run_extra_bits[count+4]+2;
		last_offset = /*10+*/ (2 << run_extra_bits[count+4]) | long_get_bits_right(offset - run_base_offs[count], run_extra_bits[count+4]);
	}
	else if (run_base_offs[count+8] > offset){
		last_offset_bits_cnt = run_extra_bits[count+8]+2;
		last_offset = /*11+*/ (3 << run_extra_bits[count+8]) | long_get_bits_right(offset - run_base_offs[count+4], run_extra_bits[count+8]);
	}
	else{
		return 0;
	}

	//printf("%02X-%02X-%02X-%02X-%04X-%04X-%02X\n", cnt_, encoded_token_len_bits_cnt, encoded_token_len & 0xFF, encoded_offset_bits_cnt, encoded_offset, encoded_reps, encoded_reps_bits_cnt);

	return -1;
}

unsigned short find_best_match_and_encode(){
	short max = 0;
	unsigned short retn = 0;

	for (int i = 0; i < 8; i++){
		if (counts[i] && encode_match(counts[i], offsets[i])){
			short encoded = (counts[i] << 3) - (last_reps_bits_cnt + last_offset_bits_cnt + last_token_len_bits_cnt);
			if (encoded >= max){
				encoded_reps = last_reps;
				encoded_reps_bits_cnt = last_reps_bits_cnt;
				encoded_token_len = last_token_len;
				encoded_token_len_bits_cnt = last_token_len_bits_cnt;
				encoded_offset = last_offset;
				encoded_offset_bits_cnt = last_offset_bits_cnt;

				max = encoded;
				retn = counts[i];
			}
		}
	}
	return retn;
}

int nn = 0;
void write_bits(unsigned char count, unsigned int value){
	//printf("%02X-%06X\n", count, value); nn++;
	for (int i = 0; i < count; i++){
		write_byte >>= 1;
		write_byte |= ((value & 1) << 7);
		value >>= 1;

		write_bits_cnt--;
		if (write_bits_cnt < 0){
			write_bits_cnt = 7;
			src[endoffset++] = write_byte;
			//printf("%02X\n", write_byte);
			write_byte = 0;
		}
	}
}

unsigned int implode(unsigned char *input, unsigned int size, unsigned char mode){
	if (!input || size < 0x40){
		return 0;
	}

	read_pos = src_size = max_encoded_size = endoffset = token_run_len = 0;
	write_byte = 0;
	last_reps = encoded_reps = last_token_len = encoded_token_len = 0;
	last_reps_bits_cnt = encoded_reps_bits_cnt = last_token_len_bits_cnt = 0;
	encoded_token_len_bits_cnt = last_offset_bits_cnt = encoded_offset_bits_cnt = 0;
	last_offset = encoded_offset = 0;
	write_bits_cnt = 0;

	src_size = size;
	read_pos = 0;
	src = input;
	endoffset = 0;

	write_byte = 0;
	write_bits_cnt = 0;

	if (mode >= 0x0C){
		mode = 0;
	}

	max_encoded_size = MAX_SIZES[mode]+1;

	if (max_encoded_size > src_size){
		max_encoded_size = src_size-1;
	}

	unsigned char size_mode = 0;
	while (max_encoded_size-1 > MAX_SIZES[size_mode]){
		size_mode++;
	}

	for (int i = 0; i < 0x0C; i++){
		run_extra_bits[i] = MODE_INIT[size_mode][i];
		run_base_offs[i] = (1 << MODE_INIT[size_mode][i]);
	}

	for (int i = 0; i < 8; i++){
		run_base_offs[4+i] += run_base_offs[i];
	}

	write_bits_cnt = 7;

	while (read_pos < src_size-2){
		find_repeats();

		unsigned short len = find_best_match_and_encode();
		//printf("%02X\n", len);
		if (!len){
			src[endoffset++] = src[read_pos++];
			token_run_len++;

			if (token_run_len >= 0x4012){
				break;
			}
		}
		else{
			token_run_len = 0;

			read_pos += len;
			write_bits(encoded_offset_bits_cnt, encoded_offset);
			write_bits(encoded_token_len_bits_cnt, encoded_token_len);

			if (encoded_reps_bits_cnt == 13){
				src[endoffset++] = encoded_reps & 0xFF;
				write_bits(5, 0x1F);
			}
			else{
				write_bits(encoded_reps_bits_cnt, encoded_reps);
			}
		}
	}

	while (read_pos != src_size){
		src[endoffset++] = src[read_pos++];
		token_run_len++;
	}

	if (endoffset >= 0x0C && (src_size - endoffset > 54)){
		if (endoffset & 1){
			src[endoffset++] = 0;
			/*+0x10, SMART WORD*/ write_word(&src[endoffset+0x10], (write_byte & 0xFE) | (1 << write_bits_cnt));
		}
		else{
			/*+0x10, SMART WORD*/ write_word(&src[endoffset+0x10], 0xFF00 | (write_byte & 0xFE) | (1 << write_bits_cnt));
		}

		unsigned int cmp_dword_1 = read_dword(&src[0x00]);
		unsigned int cmp_dword_2 = read_dword(&src[0x04]);
		unsigned int cmp_dword_3 = read_dword(&src[0x08]);
		/*HEADER*/
		/*+0x00, MAGIC DWORD       */ write_dword(&src[0x00], 0x494d5021); // "IMP!"
		/*+0x04, UNCOMPRESSED SIZE */ write_dword(&src[0x04], src_size);
		/*+0x08, CMPR DATA OFFSET  */ write_dword(&src[0x08], endoffset);
		/*+0x0C, COMPRESSED DWORD AS IS: SRC[0x0C], NO NEED TO REPLACE */

		/*CMPR DATA (+0x10)*/
		/*+0x00, CMPR DATA DWORD 3 */ write_dword(&src[endoffset+0x00], cmp_dword_3);
		/*+0x04, CMPR DATA DWORD 2 */ write_dword(&src[endoffset+0x04], cmp_dword_2);
		/*+0x08, CMPR DATA DWORD 1 */ write_dword(&src[endoffset+0x08], cmp_dword_1);
		/*+0x0C, LITERAL RUN LEN   */ write_dword(&src[endoffset+0x0C], token_run_len);

		/*+0x12, BASE OFFSETS TABLE*/
		for (int i = 0; i < 8; i++){
			write_word(&src[endoffset+0x12+i*2], run_base_offs[i] & 0xFFFF);
		}

		/*+0x22, EXTRA BITS TABLE  */
		copy_bytes(&src[endoffset+0x22], &run_extra_bits[0], 12);

		/*+0x2E, CHECKSUM OF DATA  */ write_dword(&src[endoffset+0x2E], checksum(&src[0], endoffset+0x2E));
		return (endoffset + 0x2E + 4);
	}
	return 0;
}
