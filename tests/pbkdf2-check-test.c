/*
 * Copyright (c) 2013 Jan-Piet Mens <jp@mens.de>
 * Copyright (c) 2026 Mikro Y <yc_8301jp@yahoo.co.jp>
 * Copyright (c) 2026 i-Focus Co., Ltd.
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


int pbkdf2_check(char *password, char *hash);
int mosquitto_pw_check(const char *password, const char *hash);
int mosquitto_sha512_check(const char *password, const char *hash);

// ./build/tests/pbkdf2-check-test password 'PBKDF2$sha1$98$XaIs9vQgmLujKHZG4/B3dNTbeP2PyaVKySTirZznBrE=$2DX/HZDTojVbfgAIdozBi6CihjWP1+akYnh/h9uQfIVl6pLoAiwJe1ey2WW2BnT+'
int main(int argc, char *argv[])
{
	char *password;
	char *hash;
	int match;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <password> <hash>\n", argv[0]);
		return 2;
	}

	password = argv[1];
	hash = argv[2];

	printf("Checking password [%s] for %s\n", password, hash);

	if (strncmp(hash, "$7$", 3) == 0) {
		printf("Password type: Mosquitto PBKDF2-SHA512 ($7$)\n");
		match = mosquitto_pw_check(password, hash);
    } else if (strncmp(hash, "$6$", 3) == 0) {
		printf("Password type: Mosquitto SHA512 ($6$)\n");
		match = mosquitto_sha512_check(password, hash);
	} else {
		printf("Password type: Auth-plug PBKDF2\n");
		match = pbkdf2_check(password, hash);
	}

	printf("Password verification: %s\n", match ? "SUCCESS" : "FAILED");

	return match ? 0 : 1;
}
