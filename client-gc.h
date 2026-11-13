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

#ifndef CLIENT_GC_H
#define CLIENT_GC_H



struct userdata;

typedef enum {
    GC_ACTIVE = 0,
    GC_REMOVE_PENDING,
    GC_KICK_PENDING
} gc_state_t;

struct client_gc {
    time_t created_at;
    time_t last_seen;
    time_t state_changed_at;

    gc_state_t state;
};

typedef enum {
    GC_PHASE_SCAN = 0,
    GC_PHASE_REMOVE,
    GC_PHASE_KICK
} gc_phase_t;

struct client_gc_manager {
    bool enabled;
    unsigned int interval;
    unsigned int idle_timeout;
    unsigned int max_lifetime;

    time_t last_gc_at;
    gc_phase_t phase;
};


void client_gc_init(struct userdata *ud, bool enabled, unsigned int interval, unsigned int idle_timeout, unsigned int max_lifetime);
void client_gc_tick(struct userdata *ud);
void client_gc_touch(struct client_gc *gc);

#endif /* CLIENT_GC_H */
