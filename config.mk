# Select your backends from this list
BACKEND_CDB ?= yes
BACKEND_HTTP ?= yes
BACKEND_JWT ?= yes
BACKEND_FILES ?= yes
# sudo apt install libmariadb-dev
BACKEND_MYSQL ?= no
# sudo apt install libsqlite3-dev
BACKEND_SQLITE ?= no
# sudo apt install libhiredis-dev
BACKEND_REDIS ?= no
# sudo apt install libpq-dev
BACKEND_POSTGRES ?= no
# sudo apt install libldap2-dev
BACKEND_LDAP ?= no
# libmongoc and libbson
BACKEND_MONGO ?= no
# sudo apt install libmemcached-dev
BACKEND_MEMCACHED ?= no

# Specify the path to the Mosquitto sources here
# MOSQUITTO_SRC = /usr/local/Cellar/mosquitto/1.4.12
MOSQUITTO_SRC = ./contrib/mosquitto-2.0.11

# Specify the path the OpenSSL here
OPENSSLDIR = /usr

# Add support for django hashers algorithm name
SUPPORT_DJANGO_HASHERS ?= no

# Specify optional/additional linker/compiler flags here
# On macOS, add
#	CFG_LDFLAGS = -undefined dynamic_lookup
# as described in https://github.com/eclipse/mosquitto/issues/244
#
# CFG_LDFLAGS = -undefined dynamic_lookup  -L/usr/local/Cellar/openssl/1.0.2l/lib
# CFG_CFLAGS = -I/usr/local/Cellar/openssl/1.0.2l/include -I/usr/local/Cellar/mosquitto/1.4.12/include
CFG_LDFLAGS =
CFG_CFLAGS = -I./contrib/mosquitto-2.0.11 -I./contrib/mosquitto-2.0.11/include
