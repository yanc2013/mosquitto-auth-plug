/*
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

#ifndef MOSQ_COMPAT_H
#define MOSQ_COMPAT_H

#include <mosquitto.h>
#include <mosquitto_broker.h>
#include <mosquitto_plugin.h>


#if LIBMOSQUITTO_VERSION_NUMBER >= 1004090
# define MOSQ_DENY_AUTH	MOSQ_ERR_PLUGIN_DEFER
# define MOSQ_DENY_ACL	MOSQ_ERR_PLUGIN_DEFER
#else
# define MOSQ_DENY_AUTH	MOSQ_ERR_AUTH
# define MOSQ_DENY_ACL	MOSQ_ERR_ACL_DENIED
#endif

#if MOSQ_AUTH_PLUGIN_VERSION >= 3
# define mosquitto_auth_opt mosquitto_opt
#endif

const char *compat_client_id(struct mosquitto *client);
const char *compat_client_username(struct mosquitto *client);
void *compat_client_certificate(struct mosquitto *client);

// const char *compat_client_id(void *client);
// const char *compat_client_username(void *client);
// bool compat_client_has_certificate(void *client);
// int compat_client_kick(const char *clientid);

#endif
