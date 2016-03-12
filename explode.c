#include <stdio.h>

static unsigned char explode_token_base[4] = { 6, 10, 10, 18 };
static unsigned char static_token_extra_bits[12] = { 1,  1,  1,  1,  2,  3,  3,  4,  4,  5,  7, 14 };

unsigned char *src, *cmpr_data;
unsigned int write_pos, src_size, src_end /*a4*/, token_run_len /*d2*/;
int cmpr_pos /*a3*/;

unsigned char token;

unsigned short run_base_off_tbl[8];
unsigned char run_extra_bits_tbl[12];

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

unsigned int read_bits(unsigned char count){
	unsigned int retn = 0;

	for (int i = 0; i < count; i++){
		char bit = (token >> 7);
		token <<= 1;

		if (!token){
			token = (cmpr_data[cmpr_pos-1] << 1) | bit;
			bit = (cmpr_data[--cmpr_pos] >> 7);
		}

		retn <<= 1;
		retn |= bit;
	}
	return retn;
}

int check_imp(unsigned char *input){
	unsigned int id, end_off, out_len;

	if (!input){
		return 0;
	}

	id = read_dword(&input[0]);
	out_len = read_dword(&input[4]);
	end_off = read_dword(&input[8]);

	/* check for magic ID 'IMP!', or one of the other IDs used by Imploder
	clones; ATN!, BDPI, CHFI, Dupa, EDAM, FLT!, M.H., PARA and RDC9 */
	if (id != 0x494d5021 && id != 0x41544e21 && id != 0x42445049 && id != 0x43484649 && id != 0x44757061 &&
			id != 0x4544414d && id != 0x464c5421 && id != 0x4d2e482e && id != 0x50415241 &&	id != 0x52444339){
		return 0;
	}

	/* sanity checks */
	return !((end_off & 1) || (end_off < 14) || ((end_off + 0x26) > out_len));
}

int explode(unsigned char *input){
	if (!check_imp(input)){
		return 0;
	}

	write_pos = 0, src_size = 0, src_end = 0 /*a4*/, token_run_len = 0 /*d2*/;
	cmpr_pos = 0 /*a3*/;
	token = 0;

	src = input;
	src_end = src_size = read_dword(&src[0x04]);
	cmpr_data = &src[read_dword(&src[0x08])];
	cmpr_pos = 0;

	write_dword(&src[0x08], read_dword(&cmpr_data[0x00]));
	write_dword(&src[0x04], read_dword(&cmpr_data[0x04]));
	write_dword(&src[0x00], read_dword(&cmpr_data[0x08]));

	token_run_len = read_dword(&cmpr_data[0x0C]);

	if (!(cmpr_data[0x10] & 0x80)){
		cmpr_pos--;
	}

	token /*d3*/ = cmpr_data[0x11];

	for (int i = 0; i < 8; i++){
		/*a1*/run_base_off_tbl[i] = read_word(&cmpr_data[0x12+i*2]);
	}

	copy_bytes(&run_extra_bits_tbl[0], &cmpr_data[0x22], 12);
//int cnt = 0;
	while (1){
		for (int i = 0; (i < token_run_len) && (src_end > 0); i++){
			src[--src_end] = cmpr_data[--cmpr_pos];
		}

		if (src_end == 0){
			break;
		}

		unsigned int match_len, selector;

		if (read_bits(1)){
			if (read_bits(1)){
				if (read_bits(1)){
					if (read_bits(1)){
						if (read_bits(1)){
							match_len = cmpr_data[--cmpr_pos];
							selector = 3;
						}
						else{
							match_len = read_bits(3) + 6;
							selector = 3;
						}
					}
					else{
						match_len = 5;
						selector = 3;
					}
				}
				else{
					match_len = 4;
					selector = 2;
				}
			}
			else{
				match_len = 3;
				selector = 1;
			}
		}
		else{
			match_len = 2;
			selector = 0;
		}

		if (read_bits(1)){
			if (read_bits(1)){
				token_run_len = read_bits(static_token_extra_bits[selector+8]) + explode_token_base[selector];
			}
			else{
				token_run_len = read_bits(static_token_extra_bits[selector+4]) + 2;
			}
		}
		else{
			token_run_len = read_bits(static_token_extra_bits[selector]);
		}

		unsigned char *match;

		if (read_bits(1)){
			if (read_bits(1)){
				match = &src[src_end + read_bits(run_extra_bits_tbl[8+selector]) + run_base_off_tbl[4+selector] + 1];
			}
			else{
				match = &src[src_end + read_bits(run_extra_bits_tbl[4+selector]) + run_base_off_tbl[selector] + 1];
			}
		}
		else{
			match = &src[src_end + read_bits(run_extra_bits_tbl[selector]) + 1];
		}

		//printf("%06X-%06X\n", match_len, src_end); cnt++;

		for (int i = 0; (i < match_len) && (src_end > 0); i++){
			src[--src_end] = *--match;
		}
	}

	return src_size;
}

int imploded_size(unsigned char *input){
	if(!check_imp(input)) return 0;

	return read_dword(&input[8]) + 0x32;
}

int exploded_size(unsigned char *input){
	if(!check_imp(input)) return 0;

	return read_dword(&input[4]);
}
