#include <string.h>

#include "sha1.h"

void hmac_sha1(unsigned char* key, int keylen, unsigned char* msg, int msglen, unsigned char digest[20])
{
#define BLOCK_SIZE (64)
	const int block_size = 64;
	const unsigned char i_pad = 0x36;
	const unsigned char o_pad = 0x5C;

	unsigned char i_pad_block[BLOCK_SIZE];
	unsigned char o_pad_block[BLOCK_SIZE];
	unsigned char key_block[BLOCK_SIZE];

	if(keylen == block_size){
		memcpy(&key_block[0], key, keylen);
	}
	else if(keylen > block_size){
		SHA1Context sha1ctx;
		SHA1Reset(&sha1ctx);
		SHA1Input(&sha1ctx, key, keylen);
		SHA1Result(&sha1ctx, &key_block[0]);
		keylen = 20;
	}
	else if(keylen < block_size){
		memcpy(key_block, key, keylen);
	}

	// !!!
	if(keylen < block_size){
		memset(&key_block[0]+keylen, 0, block_size-keylen);
	}

	for(int i=0; i<block_size; i++){
		i_pad_block[i] = i_pad ^ key_block[i];
		o_pad_block[i] = o_pad ^ key_block[i];
	}

	unsigned char dig[20];
	SHA1Context ctx;
	SHA1Reset(&ctx);
	SHA1Input(&ctx, i_pad_block, block_size);
	SHA1Input(&ctx, msg, msglen);
	SHA1Result(&ctx, &dig[0]);

	SHA1Reset(&ctx);
	SHA1Input(&ctx, o_pad_block, block_size);
	SHA1Input(&ctx, dig, 20);
	SHA1Result(&ctx, &dig[0]);

	memcpy(digest, dig, 20);
}

