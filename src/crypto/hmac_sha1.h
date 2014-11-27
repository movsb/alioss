/*---------------------------------------------------------------
File	: hmac_sha1
Author	: twofei (anhbk@qq.com)
Date	: Nov 6 2014
Desc	: hmac_sha1: c implementation
---------------------------------------------------------------*/

#ifndef __hmac_sha1_h__
#define __hmac_sha1_h__

#ifdef __cplusplus
extern "C" {
#endif

	void hmac_sha1(unsigned char* key, int keylen, unsigned char* msg, int msglen, unsigned char digest[20]);

#ifdef __cplusplus
}
#endif

#endif //__hmac_sha1_h__
