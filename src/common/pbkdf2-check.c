/*
 * Copyright (c) 2013 Jan-Piet Mens <jp@mens.de>
 * Copyright (c) 2026 Mikro Y <yc_8301jp@yahoo.co.jp>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of mosquitto nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include "base64.h"

#define SEPARATOR       "$"
#define TRUE	(1)
#define FALSE	(0)


/*
 * Split PBKDF2$... string into their components. The caller must free()
 * the strings.
 */

static int detoken(char *pbkstr, char **sha, int *iter, char **salt, char **key)
{
	char *p, *s, *save;
	int rc = 1;

	save = s = strdup(pbkstr);

#if defined(SUPPORT_DJANGO_HASHERS)
{
	if ((p = strsep(&s, "_")) == NULL)
		goto out;
	if (strcmp(p, "pbkdf2") != 0)
		goto out;
}
#else
{
	if ((p = strsep(&s, SEPARATOR)) == NULL)
		goto out;
	if (strcmp(p, "PBKDF2") != 0)
		goto out;
}
#endif

	if ((p = strsep(&s, SEPARATOR)) == NULL)
		goto out;
	*sha = strdup(p);

	if ((p = strsep(&s, SEPARATOR)) == NULL)
		goto out;
	*iter = atoi(p);

	if ((p = strsep(&s, SEPARATOR)) == NULL)
		goto out;
	*salt = strdup(p);

	if ((p = strsep(&s, SEPARATOR)) == NULL)
		goto out;
	*key = strdup(p);

	rc = 0;

     out:
	free(save);
	return rc;
}

int pbkdf2_check(char *password, char *hash)
{
	char *sha, *salt, *h_pw;
	int iterations, saltlen, blen;
	char *b64, *keybuf;
	unsigned char *out;
	int match = FALSE;
	const EVP_MD *evpmd;
	int keylen, rc;

	if (detoken(hash, &sha, &iterations, &salt, &h_pw) != 0)
		return match;

	/* Determine key length by decoding base64 */
	if ((keybuf = malloc(strlen(h_pw) + 1)) == NULL) {
		fprintf(stderr, "Out of memory\n");
		return FALSE;
	}
	keylen = base64_decode(h_pw, keybuf);
	if (keylen < 1) {
		free(keybuf);
		return (FALSE);
	}
	free(keybuf);

	if ((out = malloc(keylen)) == NULL) {
		fprintf(stderr, "Cannot allocate out; out of memory\n");
		return (FALSE);
	}

#ifdef RAW_SALT
	char *rawSalt;

	if ((rawSalt = malloc(strlen(salt) + 1)) == NULL) {
		fprintf(stderr, "Out of memory\n");
		return FALSE;
	}

	saltlen = base64_decode(salt, rawSalt);
	if (saltlen < 1) {
		return (FALSE);
	}

	free(salt);
	salt = rawSalt;
	rawSalt = NULL;
#else
	saltlen = strlen((char *)salt);
#endif

#ifdef PWDEBUG
	fprintf(stderr, "sha        =[%s]\n", sha);
	fprintf(stderr, "iterations =%d\n", iterations);
	fprintf(stderr, "salt       =[%s]\n", salt);
	fprintf(stderr, "salt len   =[%d]\n", saltlen);
	fprintf(stderr, "h_pw       =[%s]\n", h_pw);
	fprintf(stderr, "kenlen     =[%d]\n", keylen);
#endif


	evpmd = EVP_sha256();
	if (strcmp(sha, "sha1") == 0) {
		evpmd = EVP_sha1();
	} else if (strcmp(sha, "sha512") == 0) {
		evpmd = EVP_sha512();
	}

	rc = PKCS5_PBKDF2_HMAC(password, strlen(password),
		(unsigned char *)salt, saltlen,
		iterations,
		evpmd, keylen, out);
	if (rc != 1) {
		goto out;
	}

	blen = base64_encode(out, keylen, &b64);
	if (blen > 0) {
		int i, diff = 0, hlen = strlen(h_pw);
#ifdef PWDEBUG
		fprintf(stderr, "HMAC b64   =[%s]\n", b64);
#endif

		/* "manual" strcmp() to ensure constant time */
		for (i = 0; (i < blen) && (i < hlen); i++) {
			diff |= h_pw[i] ^ b64[i];
		}

		match = diff == 0;
		if (hlen != blen)
			match = 0;

		free(b64);
	}

  out:
	free(sha);
	free(salt);
	free(h_pw);
	free(out);

	return match;
}

#include <errno.h>
#define MOSQ_SALT_LEN 12
#define MOSQ_HASH_LEN 64

/*
 * Verify a Mosquitto password hash ($7$ format).
 * Returns 1 on success, 0 on failure.
 */
int mosquitto_pw_check(const char *password, char *hash)
{
    char *endptr;
    long iterations;
    const char *salt_start;
    const char *hash_start;
    const char *sep;

    char salt_b64[17];
    char hash_b64[89];

    unsigned char salt[MOSQ_SALT_LEN];
    unsigned char expected[MOSQ_HASH_LEN];
    unsigned char actual[MOSQ_HASH_LEN];

    int len;

    if (!password || !hash)
        return 0;

    /* Only support Mosquitto PBKDF2-SHA512 ($7$). */
    if (strncmp(hash, "$7$", 3) != 0)
        return 0;

    /* Parse iteration count. */
    errno = 0;
    iterations = strtol(hash + 3, &endptr, 10);

    if (errno == ERANGE ||
        endptr == hash + 3 ||
        *endptr != '$' ||
        iterations < 1 ||
        iterations > INT_MAX)
        return 0;

    /* Extract Base64 salt. */
    salt_start = endptr + 1;
    sep = strchr(salt_start, '$');

    if (!sep || sep - salt_start != 16)
        return 0;

    memcpy(salt_b64, salt_start, 16);
    salt_b64[16] = '\0';

    /* Extract Base64 password hash. */
    hash_start = sep + 1;

    if (strlen(hash_start) != 88)
        return 0;

    memcpy(hash_b64, hash_start, 89);

    /* Decode salt: 16 Base64 chars -> 12 bytes. */
    len = EVP_DecodeBlock(
        salt,
        (const unsigned char *)salt_b64,
        16
    );

    if (len != MOSQ_SALT_LEN)
        return 0;

    /* Decode hash: 88 Base64 chars -> 64 bytes. */
    if (hash_b64[86] != '=' || hash_b64[87] != '=')
        return 0;

    len = EVP_DecodeBlock(
        expected,
        (const unsigned char *)hash_b64,
        88
    );

    if (len != 66)
        return 0;

    /* EVP_DecodeBlock includes padding in the length. */
    len -= 2;

    if (len != MOSQ_HASH_LEN)
        return 0;

    /* Calculate PBKDF2-HMAC-SHA512. */
    if (PKCS5_PBKDF2_HMAC(
            password,
            -1,
            salt,
            MOSQ_SALT_LEN,
            (int)iterations,
            EVP_sha512(),
            MOSQ_HASH_LEN,
            actual) != 1)
        return 0;

    /* Constant-time comparison. */
    return CRYPTO_memcmp(actual, expected, MOSQ_HASH_LEN) == 0;
}

/*
 * Verify Mosquitto legacy SHA512 password hash ($6$).
 * Returns 1 on success, 0 on failure.
 */
int mosquitto_sha512_check(const char *password, char *hash)
{
    const char *salt_start;
    const char *hash_start;
    const char *sep;

    unsigned char salt[12];
    unsigned char expected[66];
    unsigned char actual[EVP_MAX_MD_SIZE];

    unsigned int actual_len = 0;
    int len;
    EVP_MD_CTX *ctx = NULL;
    int match = 0;

    if (!password || !hash || strncmp(hash, "$6$", 3) != 0)
        return 0;

    salt_start = hash + 3;
    sep = strchr(salt_start, '$');

    /* 12 bytes salt = 16 Base64 characters. */
    if (!sep || sep - salt_start != 16)
        return 0;

    hash_start = sep + 1;

    /* 64 bytes SHA512 = 88 Base64 characters. */
    if (strlen(hash_start) != 88)
        return 0;

    if (hash_start[86] != '=' || hash_start[87] != '=')
        return 0;

    len = EVP_DecodeBlock(
        salt,
        (const unsigned char *)salt_start,
        16
    );

    if (len != 12)
        return 0;

    len = EVP_DecodeBlock(
        expected,
        (const unsigned char *)hash_start,
        88
    );

    if (len != 66)
        return 0;

    ctx = EVP_MD_CTX_new();
    if (!ctx)
        return 0;

    if (EVP_DigestInit_ex(ctx, EVP_sha512(), NULL) != 1)
        goto cleanup;

    if (EVP_DigestUpdate(ctx, password, strlen(password)) != 1)
        goto cleanup;

    if (EVP_DigestUpdate(ctx, salt, sizeof(salt)) != 1)
        goto cleanup;

    if (EVP_DigestFinal_ex(ctx, actual, &actual_len) != 1)
        goto cleanup;

    if (actual_len == 64 &&
        CRYPTO_memcmp(actual, expected, 64) == 0) {
        match = 1;
    }

cleanup:
    EVP_MD_CTX_free(ctx);
    return match;
}

int password_check(const char *password, char *hash)
{
    if (!password || !hash)
        return 0;

    if (strncmp(hash, "$7$", 3) == 0)
        return mosquitto_pw_check(password, hash);

    if (strncmp(hash, "$6$", 3) == 0)
        return mosquitto_sha512_check(password, hash);

    return pbkdf2_check((char *)password, hash);
}
// match = password_check(password, phash);



#if TEST
// gcc -DTEST=1 -o pbkdf2-test pbkdf2-check.c base64.c -lcrypto
int main()
{
	char password[] = "password";
	char pbkstr[] = "PBKDF2$sha1$98$XaIs9vQgmLujKHZG4/B3dNTbeP2PyaVKySTirZznBrE=$2DX/HZDTojVbfgAIdozBi6CihjWP1+akYnh/h9uQfIVl6pLoAiwJe1ey2WW2BnT+";
	int match;

	printf("Checking password [%s] for %s\n", password, pbkstr);

	match = pbkdf2_check(password, pbkstr);
	printf("match == %d\n", match);
	return match;
}
#endif
