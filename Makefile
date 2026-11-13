# Load our module (and misc) configuration from config.mk
# It also contains MOSQUITTO_SRC
include config.mk

BE_CFLAGS =
BE_LDFLAGS =
BE_LDADD =
BE_DEPS =
BUILD_DIR = build
OBJS = $(BUILD_DIR)/auth-plug.o $(BUILD_DIR)/base64.o $(BUILD_DIR)/pbkdf2-check.o $(BUILD_DIR)/log.o $(BUILD_DIR)/envs.o $(BUILD_DIR)/hash.o $(BUILD_DIR)/be-psk.o $(BUILD_DIR)/backends.o $(BUILD_DIR)/cache.o $(BUILD_DIR)/client-gc.o

BACKENDS =
BACKENDSTR =

ifneq ($(BACKEND_CDB),no)
	BACKENDS += -DBE_CDB
	BACKENDSTR += CDB

	CDBDIR = contrib/tinycdb-0.78
	CDB = $(CDBDIR)/cdb
	CDBINC = $(CDBDIR)/
	CDBLIB = $(CDBDIR)/libcdb.a
	BE_CFLAGS += -I$(CDBINC)/
	BE_LDFLAGS += -L$(CDBDIR)
	BE_LDADD += -lcdb
	BE_DEPS += $(CDBLIB)
	OBJS += $(BUILD_DIR)/be-cdb.o
endif

ifneq ($(BACKEND_MYSQL),no)
	BACKENDS += -DBE_MYSQL
	BACKENDSTR += MySQL

	BE_CFLAGS += `mysql_config --cflags`
	BE_LDADD += `mysql_config --libs`
	OBJS += $(BUILD_DIR)/be-mysql.o
endif

ifneq ($(BACKEND_SQLITE),no)
	BACKENDS += -DBE_SQLITE
	BACKENDSTR += SQLite

	BE_LDADD += -lsqlite3
	OBJS += $(BUILD_DIR)/be-sqlite.o
endif

ifneq ($(BACKEND_REDIS),no)
	BACKENDS += -DBE_REDIS
	BACKENDSTR += Redis

	BE_CFLAGS += -I/usr/local/include/hiredis
	BE_LDFLAGS += -L/usr/local/lib
	BE_LDADD += -lhiredis
	OBJS += $(BUILD_DIR)/be-redis.o
endif

ifeq ($(BACKEND_MEMCACHED),yes)
	BACKENDS += -DBE_MEMCACHED
	BACKENDSTR += Memcached

	BE_CFLAGS += -I/usr/local/include/libmemcached
	BE_LDFLAGS += -L/usr/local/lib
	BE_LDADD += -lmemcached
	OBJS += $(BUILD_DIR)/be-memcached.o
endif

ifneq ($(BACKEND_POSTGRES),no)
	BACKENDS += -DBE_POSTGRES
	BACKENDSTR += PostgreSQL

	BE_CFLAGS += -I`pg_config --includedir`
	BE_LDADD += -L`pg_config --libdir` -lpq
	OBJS += $(BUILD_DIR)/be-postgres.o
endif

ifneq ($(BACKEND_LDAP),no)
	BACKENDS += -DBE_LDAP
	BACKENDSTR += LDAP

	BE_LDADD += -lldap -llber
	OBJS += $(BUILD_DIR)/be-ldap.o
endif

ifneq ($(BACKEND_HTTP), no)
	BACKENDS+= -DBE_HTTP
	BACKENDSTR += HTTP

	BE_LDADD += -lcurl
	OBJS += $(BUILD_DIR)/be-http.o
endif

ifneq ($(BACKEND_JWT), no)
	BACKENDS+= -DBE_JWT
	BACKENDSTR += JWT

	BE_LDADD += -lcurl
	OBJS += $(BUILD_DIR)/be-jwt.o
endif

ifneq ($(BACKEND_MONGO), no)
	BACKENDS+= -DBE_MONGO
	BACKENDSTR += MongoDB

	BE_CFLAGS += -I/usr/local/include/
	BE_CFLAGS +=`pkg-config --cflags-only-I libmongoc-1.0 libbson-1.0`
	BE_LDFLAGS +=`pkg-config --libs-only-L libbson-1.0 libmongoc-1.0`
	BE_LDFLAGS += -L/usr/local/lib
	BE_LDADD += -lmongoc-1.0 -lbson-1.0
	OBJS += $(BUILD_DIR)/be-mongo.o
endif

ifneq ($(BACKEND_FILES), no)
	BACKENDS+= -DBE_FILES
	BACKENDSTR += Files

	OBJS += $(BUILD_DIR)/be-files.o
endif

ifeq ($(origin SUPPORT_DJANGO_HASHERS), undefined)
	SUPPORT_DJANGO_HASHERS = no
endif

ifneq ($(SUPPORT_DJANGO_HASHERS), no)
	CFG_CFLAGS += -DSUPPORT_DJANGO_HASHERS
endif

OSSLINC = -I$(OPENSSLDIR)/include
OSSLIBS = -L$(OPENSSLDIR)/lib -lcrypto

CFLAGS := $(CFG_CFLAGS)
CFLAGS += -I$(MOSQUITTO_SRC)/src/
CFLAGS += -I$(MOSQUITTO_SRC)/lib/
ifneq ($(OS),Windows_NT)
	CFLAGS += -fPIC -Wall -Werror
endif
CFLAGS += $(BACKENDS) $(BE_CFLAGS) -I$(MOSQ)/src -DDEBUG=1 $(OSSLINC)

LDFLAGS := $(CFG_LDFLAGS)
LDFLAGS += $(BE_LDFLAGS) -L$(MOSQUITTO_SRC)/lib/
# LDFLAGS += -Wl,-rpath,$(../../../../pubgit/MQTT/mosquitto/lib) -lc
# LDFLAGS += -export-dynamic
LDADD = $(BE_LDADD) $(OSSLIBS) -lmosquitto

.PHONY: all printconfig tests clean

all: printconfig $(BUILD_DIR) $(BUILD_DIR)/auth-plug.so $(BUILD_DIR)/np tests

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

printconfig:
	@echo "Selected backends:          $(BACKENDSTR)"
	@echo "Using mosquitto source dir: $(MOSQUITTO_SRC)"
	@echo "OpenSSL install dir:        $(OPENSSLDIR)"
	@echo "Build output dir:           $(BUILD_DIR)"
	@echo
	@echo "If you changed the backend selection, you might need to 'make clean' first"
	@echo
	@echo "CFLAGS:  $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "LDADD:   $(LDADD)"
	@echo



$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/auth-plug.so : $(OBJS) $(BE_DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -fPIC -shared -o $@ $(OBJS) $(BE_DEPS) $(LDADD)

$(BUILD_DIR)/be-redis.o: be-redis.c be-redis.h log.h hash.h envs.h Makefile
$(BUILD_DIR)/be-memcached.o: be-memcached.c be-memcached.h log.h hash.h envs.h Makefile
$(BUILD_DIR)/be-sqlite.o: be-sqlite.c be-sqlite.h Makefile
$(BUILD_DIR)/auth-plug.o: auth-plug.c be-cdb.h be-mysql.h be-sqlite.h Makefile cache.h
$(BUILD_DIR)/be-psk.o: be-psk.c be-psk.h Makefile
$(BUILD_DIR)/be-cdb.o: be-cdb.c be-cdb.h Makefile
$(BUILD_DIR)/be-mysql.o: be-mysql.c be-mysql.h Makefile
$(BUILD_DIR)/be-ldap.o: be-ldap.c be-ldap.h Makefile
$(BUILD_DIR)/be-sqlite.o: be-sqlite.c be-sqlite.h Makefile
$(BUILD_DIR)/pbkdf2-check.o: pbkdf2-check.c base64.h Makefile
$(BUILD_DIR)/base64.o: base64.c base64.h Makefile
$(BUILD_DIR)/log.o: log.c log.h Makefile
$(BUILD_DIR)/envs.o: envs.c envs.h Makefile
$(BUILD_DIR)/hash.o: hash.c hash.h uthash.h Makefile
$(BUILD_DIR)/be-postgres.o: be-postgres.c be-postgres.h Makefile
$(BUILD_DIR)/cache.o: cache.c cache.h uthash.h Makefile
$(BUILD_DIR)/be-http.o: be-http.c be-http.h Makefile backends.h
$(BUILD_DIR)/be-jwt.o: be-jwt.c be-jwt.h Makefile backends.h
$(BUILD_DIR)/be-mongo.o: be-mongo.c be-mongo.h Makefile
$(BUILD_DIR)/be-files.o: be-files.c be-files.h Makefile
$(BUILD_DIR)/client-gc.o: client-gc.c client-gc.h Makefile

$(BUILD_DIR)/np: np.c base64.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(OSSLIBS)

tests:
	$(MAKE) -C tests

$(CDBLIB):
	(cd $(CDBDIR); make libcdb.a cdb )

pwdb.cdb: pwdb.in
	$(CDB) -c -m  pwdb.cdb pwdb.in
clean :
	rm -rf $(BUILD_DIR)
	rm -f *.o *.so np
	(cd contrib/tinycdb-0.78; make realclean )

config.mk:
	@echo "Please create your own config.mk file"
	@echo "You can use config.mk.in as base"
	@false
