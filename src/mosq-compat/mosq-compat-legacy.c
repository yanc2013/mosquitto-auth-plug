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

#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <mosquitto_broker.h>
#include <mosquitto_plugin.h>

#include "auth-plug.h"
#include "log.h"


/*
 * Mosquitto Authentication Plugin Architecture
 *
 *  +----------------+  +----------------+  +----------------+
 *  | Mosquitto      |  | Mosquitto      |  | Mosquitto      |
 *  | Auth API V2    |  | Auth API V3    |  | Auth API V4    |
 *  +-------+--------+  +-------+--------+  +-------+--------+
 *          |                   |                   |
 *          +-------------------+-------------------+
 *                              |
 *                              v
 *  +-------------------------------------------------------+
 *  | Compatibility Layer (mosq-compat-legacy.c)            |
 *  |                                                       |
 *  |  mosquitto_auth_plugin_version()                      |
 *  |  mosquitto_auth_plugin_init()                         |
 *  |  mosquitto_auth_plugin_cleanup()                      |
 *  |  mosquitto_auth_security_init()                       |
 *  |  mosquitto_auth_security_cleanup()                    |
 *  |  mosquitto_auth_unpwd_check()                         |
 *  |  mosquitto_auth_acl_check()                           |
 *  |  mosquitto_auth_psk_key_get()                         |
 *  |                                                       |
 *  |  - Plugin API entry points                            |
 *  |  - Version-specific parameter adaptation             |
 *  |  - Broker client API wrappers                         |
 *  +---------------------------+---------------------------+
 *                              |
 *                              v
 *  +-------------------------------------------------------+
 *  | Core Layer (auth-plug.c)                              |
 *  |                                                       |
 *  |  auth_plugin_init()                                   |
 *  |  auth_plugin_cleanup()                                |
 *  |  auth_unpwd_check()                                    |
 *  |  auth_acl_check()                                      |
 *  |  auth_psk_key_get()                                    |
 *  |                                                       |
 *  |  - Authentication                                     |
 *  |  - Authorization                                      |
 *  |  - Client management                                   |
 *  +---------------------------+---------------------------+
 *                              |
 *          +-------------------+-------------------+
 *          |                   |                   |
 *          v                   v                   v
 *  +----------------+  +----------------+  +----------------+
 *  | Backends       |  | Cache          |  | Client GC      |
 *  +----------------+  +----------------+  +----------------+
 *
 * The compatibility layer isolates version-specific Mosquitto
 * plugin interfaces from the core authentication and authorization
 * logic, allowing the core implementation to be shared across
 * different plugin API versions.
 */


int mosquitto_auth_plugin_version(void)
{
	log_init();
	_log(LOG_NOTICE, "[PLUG] VER *** auth-plug: startup, auth plugin API version: %d", MOSQ_AUTH_PLUGIN_VERSION);

	/* The generic plugin interface starts at version 5 (MOSQ_PLUGIN_VERSION) */

	// mosquitto 2.0.11 auth plugin API version: 4
	return MOSQ_AUTH_PLUGIN_VERSION;
}

int mosquitto_auth_security_init(void *userdata, struct mosquitto_auth_opt *auth_opts, int auth_opt_count, bool reload)
{
	return MOSQ_ERR_SUCCESS;
}

int mosquitto_auth_security_cleanup(void *userdata, struct mosquitto_auth_opt *auth_opts, int auth_opt_count, bool reload)
{
	return MOSQ_ERR_SUCCESS;
}

int mosquitto_auth_plugin_init(void **userdata, struct mosquitto_auth_opt *auth_opts, int auth_opt_count)
{
	return auth_plugin_init(userdata, auth_opts, auth_opt_count);
}

int mosquitto_auth_plugin_cleanup(void *userdata, struct mosquitto_auth_opt *auth_opts, int auth_opt_count)
{
	return auth_plugin_cleanup(userdata, auth_opts, auth_opt_count);
}

/*
 * +---------------------------------------+
 * | Common API                            |
 * |                                       |
 * | mosquitto_auth_plugin_version()       |
 * | mosquitto_auth_plugin_init()          |
 * | mosquitto_auth_plugin_cleanup()       |
 * | mosquitto_auth_security_init()        |
 * | mosquitto_auth_security_cleanup()     |
 * +---------------------------------------+
 * | Version-specific API                  |
 * |                                       |
 * | #if MOSQ_AUTH_PLUGIN_VERSION >= 3     |
 * |                                       |
 * |     Auth Plugin API V3/V4             |
 * |                                       |
 * | #else                                 |
 * |                                       |
 * |     Auth Plugin API V2                |
 * |                                       |
 * | #endif                                |
 * +---------------------------------------+
 */
#if MOSQ_AUTH_PLUGIN_VERSION >= 3

int mosquitto_auth_unpwd_check(void *userdata, struct mosquitto *client, const char *username, const char *password)
{
	return auth_unpwd_check(userdata, client, username, password);
}

int mosquitto_auth_acl_check(void *userdata, int access, struct mosquitto *client, const struct mosquitto_acl_msg *msg)
{
	return auth_acl_check(userdata, access, client,
						NULL, NULL, msg->topic);
}

int mosquitto_auth_psk_key_get(void *userdata, struct mosquitto *client, const char *hint, const char *identity, char *key, int max_key_len)
{
	return auth_psk_key_get(userdata, client, hint, identity, key, max_key_len);
}

const char *compat_client_id(struct mosquitto *client)
{
	return client ? mosquitto_client_id(client) : NULL;
}

const char *compat_client_username(struct mosquitto *client)
{
    return client ? mosquitto_client_username(client) : NULL;
}

void *compat_client_certificate(struct mosquitto *client)
{
    return client ? mosquitto_client_certificate(client) : NULL;
}

#else

int mosquitto_auth_unpwd_check(void *userdata, const char *username, const char *password)
{
	return auth_unpwd_check(userdata, NULL, username, password);
}

int mosquitto_auth_acl_check(void *userdata, const char *clientid, const char *username, const char *topic, int access)
{
	return auth_acl_check(userdata, access, NULL,
					clientid, username, topic);
}

int mosquitto_auth_psk_key_get(void *userdata, const char *hint, const char *identity, char *key, int max_key_len)
{
	return auth_psk_key_get(userdata, NULL, hint, identity, key, max_key_len);
}

const char *compat_client_id(struct mosquitto *client)
{
	(void) client;
	return NULL;
}

const char *compat_client_username(struct mosquitto *client)
{
    (void)client;
    return NULL;
}

void *compat_client_certificate(struct mosquitto *client)
{
    (void)client;
    return NULL;
}

#endif
