/*------------------------------------------------------
	base64_encode
	http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c

---------------------------------------------------------*/

#ifndef __base64_h__
#define __base64_h__

char* base64_encode(
		const unsigned char* data,
		size_t input_length,
		size_t* output_length
		);

#endif

