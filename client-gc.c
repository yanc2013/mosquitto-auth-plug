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



#include <time.h>
#include <errno.h>
#include <mosquitto.h>
#include <mosquitto_broker.h>
#include <mosquitto_plugin.h>
// #include <unistd.h>
// #include <pthread.h>
#include "client-gc.h"
#include "log.h"
#include "userdata.h"


/*
 * Client GC Lifecycle:
 *
 * mosquitto_auth_plugin_init()
 *     |
 *     +-- client_gc_init(ud)
 *              |
 *              +-- Initialize GC manager
 *              +-- Load GC configuration
 *
 *
 * mosquitto_auth_unpwd_check()
 *     |
 *     +-- client_gc_tick(ud)
 *     |         |
 *     |         +-- SCAN -> REMOVE -> KICK -> SCAN ...
 *     |                   (one phase per interval)
 *     |
 *     +-- client_gc_touch(...)  [on successful authentication]
 *
 *
 * mosquitto_auth_acl_check()
 *     |
 *     +-- client_gc_tick(ud)
 *     |         |
 *     |         +-- SCAN -> REMOVE -> KICK -> SCAN ...
 *     |                   (one phase per interval)
 *     |
 *     +-- client_gc_touch(...)  [on successful ACL check]
 *
 *
 * GC State Transitions:
 *
 * GC_ACTIVE
 *     |
 *     +-- Duplicate Client ID --> GC_REMOVE_PENDING
 *     |
 *     +-- Timeout ------------> GC_KICK_PENDING
 *                                    |
 *                                    +-- Activity detected --> GC_ACTIVE
 *                                    |
 *                                    +-- Kick succeeded ----> GC_REMOVE_PENDING
 *                                    |
 *                                    +-- Client not found --> GC_REMOVE_PENDING
 *                                                               |
 *                                                               v
 *                                                         Free client entry
 *
 *
 * mosquitto_auth_plugin_cleanup()
 *     |
 *     +-- Release all remaining client entries
 *     +-- Release GC manager
 *     +-- free(ud)
 *
 * Notes:
 * - GC is triggered by authentication and ACL callbacks.
 * - No background threads, timers, or locks are required.
 * - Each GC phase runs synchronously at the configured interval.
 * - GC does not run when no authentication or ACL callbacks occur.
 */

#define GC_TIMER_START() \
    struct timespec gc_start, gc_end; \
    clock_gettime(CLOCK_MONOTONIC, &gc_start)

#define GC_TIMER_ELAPSED_MS() \
    (clock_gettime(CLOCK_MONOTONIC, &gc_end), \
     (gc_end.tv_sec - gc_start.tv_sec) * 1000.0 + \
     (gc_end.tv_nsec - gc_start.tv_nsec) / 1000000.0)

#define GC_TIMER_LOG(phase, count, processed) \
    _log(LOG_NOTICE, \
        "[CLIENT-GC] %s: clients=%u, processed=%u, elapsed=%.3f ms", \
        phase, (unsigned int)(count), \
        (unsigned int)(processed), GC_TIMER_ELAPSED_MS())


static bool client_gc_has_newer_client(struct userdata *ud, const struct cliententry *entry);
static void client_gc_scan(struct userdata *ud, time_t now);
static void client_gc_remove(struct userdata *ud, time_t now);
static void client_gc_kick(struct userdata *ud, time_t now);


/*
 * Initialize the client garbage collection manager.
 *
 * Configure the GC settings and initialize the state required
 * for periodic client scanning, disconnection, and cleanup.
 */
void client_gc_init(struct userdata *ud, bool enabled, unsigned int interval, unsigned int idle_timeout, unsigned int max_lifetime) {
    if (ud == NULL)
        return;

    ud->gc_mgr = calloc(1, sizeof(struct client_gc_manager));

    if (ud->gc_mgr == NULL) {
        _log(LOG_NOTICE, "[CLIENT-GC] Failed to allocate GC manager");
        return;
    }

    struct client_gc_manager *mgr = ud->gc_mgr;

    mgr->enabled = enabled;
    mgr->interval = interval > 0 ? interval : 60;
    mgr->idle_timeout = idle_timeout;
    mgr->max_lifetime = max_lifetime;

    mgr->last_gc_at = time(NULL);
    mgr->phase = GC_PHASE_SCAN;

    _log(LOG_NOTICE,
        "[CLIENT-GC] INIT: enabled=%d, interval=%u, idle=%u, lifetime=%u",
        mgr->enabled,
        mgr->interval,
        mgr->idle_timeout,
        mgr->max_lifetime);
}

/*
 * Execute periodic client garbage collection.
 *
 * Run one GC phase per interval in a cyclic sequence:
 * SCAN → REMOVE → KICK → SCAN → REMOVE → KICK ....
 *
 * Each phase is executed synchronously without background threads.
 * Actual disconnection and cleanup are deferred to their
 * respective phases to distribute the GC workload.
 *
 * Client GC State Transitions:
 *
 * GC_ACTIVE
 *     |
 *     +-- Newer connection with the same Client ID --> GC_REMOVE_PENDING
 *     |
 *     +-- Idle timeout or maximum lifetime --------> GC_KICK_PENDING
 *                                                        |
 *                                                        +-- Kick succeeded ----> GC_REMOVE_PENDING
 *                                                        |
 *                                                        +-- Client not found --> GC_REMOVE_PENDING
 *                                                                                     |
 *                                                                                     v
 *                                                                               Free client entry
 *
 * GC Execution Cycle:
 *
 * SCAN -> REMOVE -> KICK -> SCAN -> REMOVE -> KICK -> ...
 *
 * Each phase runs once per configured interval.
 * No background threads or locks are required.
 */
void client_gc_tick(struct userdata *ud)
{
    struct client_gc_manager *mgr = ud->gc_mgr;
    time_t now = time(NULL);

    if (!mgr || !mgr->enabled)
        return;

    if (now - mgr->last_gc_at < mgr->interval)
        return;

    mgr->last_gc_at = now;

    switch (mgr->phase) {
    case GC_PHASE_SCAN:
        client_gc_scan(ud, now);
        mgr->phase = GC_PHASE_REMOVE;
        break;

    case GC_PHASE_REMOVE:
        client_gc_remove(ud, now);
        mgr->phase = GC_PHASE_KICK;
        break;

    case GC_PHASE_KICK:
        client_gc_kick(ud, now);
        mgr->phase = GC_PHASE_SCAN;
        break;
    }
}

/*
 * Update the last activity time of a client entry.
 *
 * Refresh the last_seen timestamp and restore the client
 * to GC_ACTIVE if it is pending garbage collection.
 *
 * This prevents active clients from being removed by GC.
 */
void client_gc_touch(struct client_gc *gc)
{
    if (!gc)
        return;

    gc->last_seen = time(NULL);

    if (gc->state == GC_KICK_PENDING) {
        gc->state = GC_ACTIVE;
    }
}


/*
 * Check whether a newer client entry exists with the same Client ID.
 *
 * Used to identify stale entries that can be removed directly
 * without disconnecting the newer client.
 *
 * Returns true if a newer entry is found, false otherwise.
 */
static bool client_gc_has_newer_client(struct userdata *ud, const struct cliententry *entry)
{
    if (ud == NULL || entry == NULL || entry->clientid == NULL || entry->gc == NULL)
        return false;

    struct cliententry *other, *tmp;

    HASH_ITER(hh, ud->clients, other, tmp) {
        if (other == entry || other->clientid == NULL || other->gc == NULL)
            continue;

        if (strcmp(entry->clientid, other->clientid) == 0 && other->gc->created_at > entry->gc->created_at) {
            return true;
        }
    }

    return false;
}

/*
 * Scan client entries for idle timeout or maximum lifetime expiration.
 *
 * Only ACTIVE entries are checked. Expired entries are marked as
 * KICK_PENDING, or REMOVE_PENDING if a newer entry with the same
 * Client ID already exists.
 *
 * Actual client disconnection and memory cleanup are deferred to
 * the KICK and REMOVE phases.
 */
static void client_gc_scan(struct userdata *ud, time_t now)
{
    GC_TIMER_START();

    if (ud == NULL || ud->gc_mgr == NULL || ud->clients == NULL)
        return;

    struct client_gc_manager *mgr = ud->gc_mgr;
    struct cliententry *e, *tmp;
    unsigned int processed = 0;

    HASH_ITER(hh, ud->clients, e, tmp) {
        struct client_gc *gc = e->gc;

        if (gc == NULL)
            continue;

        /* Only ACTIVE entries are checked */
        if (gc->state != GC_ACTIVE)
            continue;

        /* idle timeout */
        bool idle_expired = mgr->idle_timeout > 0 && now >= gc->last_seen && (unsigned long)(now - gc->last_seen) >= mgr->idle_timeout;

        /* maximum lifetime expiration */
        bool lifetime_expired = mgr->max_lifetime > 0 && now >= gc->created_at && (unsigned long)(now - gc->created_at) >= mgr->max_lifetime;

        if (!idle_expired && !lifetime_expired)
            continue;

        /* Client ID already exists */
        bool duplicate = client_gc_has_newer_client(ud, e);

        gc->state = duplicate ? GC_REMOVE_PENDING : GC_KICK_PENDING;
        gc->state_changed_at = now;

        processed++;

        _log(LOG_NOTICE, "[CLIENT-GC] SCAN: clientid=%s, state=%s, idle=%lu, lifetime=%lu",
            e->clientid ? e->clientid : "(null)",
            duplicate ? "REMOVE_PENDING" : "KICK_PENDING",
            (unsigned long)(now - gc->last_seen),
            (unsigned long)(now - gc->created_at));
    }

    GC_TIMER_LOG("SCAN", HASH_COUNT(ud->clients), processed);
}

/*
 * Remove client entries marked as REMOVE_PENDING.
 *
 * Release the associated resources and remove the entries
 * from the client hash table.
 *
 * Only entries in REMOVE_PENDING state are removed.
 */
static void client_gc_remove(struct userdata *ud, time_t now)
{
    GC_TIMER_START();

    if (ud == NULL || ud->gc_mgr == NULL || ud->clients == NULL)
        return;

    struct cliententry *e, *tmp;
    unsigned int removed = 0;

    HASH_ITER(hh, ud->clients, e, tmp) {
        if (e->gc == NULL)
            continue;

        if (e->gc->state != GC_REMOVE_PENDING)
            continue;

        _log(LOG_NOTICE, "[CLIENT-GC] REMOVE: clientid=%s, age=%lu",
            e->clientid ? e->clientid : "(null)",
            (unsigned long)(now >= e->gc->created_at ? now - e->gc->created_at : 0));

        HASH_DEL(ud->clients, e);

        free(e->username);
        free(e->clientid);
        free(e->backend_name);
        free(e->gc);
        free(e);

        removed++;
    }

    GC_TIMER_LOG("REMOVE", HASH_COUNT(ud->clients), removed);
}

/*
 * Disconnect client entries marked as KICK_PENDING.
 *
 * Request the Mosquitto broker to disconnect each client by its Client ID.
 * Entries are marked as REMOVE_PENDING if the kick succeeds or the
 * client no longer exists.
 *
 * Actual memory cleanup is deferred to the REMOVE phase.
 */
static void client_gc_kick(struct userdata *ud, time_t now)
{
    GC_TIMER_START();

    if (ud == NULL || ud->gc_mgr == NULL || ud->clients == NULL)
        return;

    struct cliententry *e, *tmp;
    unsigned int kicked = 0;

    HASH_ITER(hh, ud->clients, e, tmp) {
        struct client_gc *gc = e->gc;

        if (gc == NULL || gc->state != GC_KICK_PENDING)
            continue;

        /* Cannot kick client: Client ID is missing */
        if (e->clientid == NULL || e->clientid[0] == '\0') {
            continue;
        }

        /* A newer connection with the same Client ID exists; only remove the stale entry */
        if (client_gc_has_newer_client(ud, e)) {
            gc->state = GC_REMOVE_PENDING;
            gc->state_changed_at = now;

            _log(LOG_NOTICE, "[CLIENT-GC] KICK: duplicate clientid=%s, marked REMOVE_PENDING", e->clientid);
            continue;
        }

        /* Execute the client kick operation */
        int rc = mosquitto_kick_client_by_clientid(e->clientid, false);

        /* Kick succeeded or client no longer exists; mark for removal. */
        if (rc == MOSQ_ERR_SUCCESS || rc == MOSQ_ERR_NOT_FOUND) {
            gc->state = GC_REMOVE_PENDING;
            gc->state_changed_at = now;
            kicked++;

        // if (rc == MOSQ_ERR_SUCCESS)
        //     kicked++;

            _log(LOG_NOTICE, "[CLIENT-GC] KICK: clientid=%s, rc=%d, marked REMOVE_PENDING", e->clientid, rc);
        } else {
            _log(LOG_NOTICE, "[CLIENT-GC] KICK: clientid=%s, rc=%d (%s)", e->clientid, rc, mosquitto_strerror(rc));
        }
    }

    GC_TIMER_LOG("KICK", HASH_COUNT(ud->clients), kicked);
}
