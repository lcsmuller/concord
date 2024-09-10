# If you run into issues, change this to "cpp" instead of "cc -E"
# If you are cross-compiling, do NOT set this to the target preprocessor!
# This needs to run on the host -- that's why $(CC) and $(CPP) are
# available here to be set.
CPP = cc -E
CC = cc
HOSTCC = cc

TOP = ..

API_DIR     = api
INCLUDE_DIR = $(TOP)/include
CORE_DIR    = $(TOP)/core
DOCS_DIR    = $(TOP)/docs

# Gencodecs preprocessor (used after the CPP)
PP = gencodecs-pp

# Input file name without its extension
INPUT_NO_EXT = discord_codecs

# Input file to be preprocessed
INPUT = $(INPUT_NO_EXT).PRE.h
# Resulting single-file amalgamations after preprocessing input file
OUT_C = $(INPUT_NO_EXT).c
OUT_H = $(INPUT_NO_EXT).h
OUT_O = $(INPUT_NO_EXT).o

# Generate individual headers for doxygen's parsing
HEADERS = $(API_DIR)/application.h           \
          $(API_DIR)/application_commands.h  \
          $(API_DIR)/audit_log.h             \
          $(API_DIR)/auto_moderation.h       \
          $(API_DIR)/channel.h               \
          $(API_DIR)/custom.h                \
          $(API_DIR)/emoji.h                 \
					$(API_DIR)/entitlements.h          \
          $(API_DIR)/gateway.h               \
          $(API_DIR)/guild.h                 \
          $(API_DIR)/guild_scheduled_event.h \
          $(API_DIR)/guild_template.h        \
          $(API_DIR)/interactions.h          \
          $(API_DIR)/invite.h                \
          $(API_DIR)/message_components.h    \
          $(API_DIR)/oauth2.h                \
          $(API_DIR)/permissions.h           \
					$(API_DIR)/poll.h                  \
          $(API_DIR)/stage_instance.h        \
          $(API_DIR)/sticker.h               \
          $(API_DIR)/teams.h                 \
          $(API_DIR)/user.h                  \
          $(API_DIR)/voice.h                 \
          $(API_DIR)/voice_connections.h     \
          $(API_DIR)/webhook.h

CFLAGS   ?= -O2
CFLAGS   += -I. -I$(API_DIR) -I$(INCLUDE_DIR) -I$(CORE_DIR)
DFLAGS   += -DGENCODECS_INIT -DGENCODECS_JSON_ENCODER -DGENCODECS_JSON_DECODER
CPPFLAGS += -nostdinc -P

# Convert 'foo/bar_baz.PRE.h' -> 'FOO_BAR_BAZ_H'
HEADER_TAG   = "$$(echo '$<' | sed -e 's/\(.*\)\.PRE.h/\1/' | sed -e 's/\//_/' | tr '[:lower:]' '[:upper:]')_H"
# Doxygen file description
DOXYGEN_DESC = "/**\n @file $@\n @author Cogmasters\n @brief Generated code\n*/"

all: $(OUT_O)

$(PP): $(PP).c
	$(HOSTCC) $(CFLAGS) $< -o $@

$(OUT_O): $(OUT_C) $(OUT_H)
	$(CC) -c $(CFLAGS) $< -o $@
$(OUT_H): $(INPUT) $(PP)
	@ echo "Generating header"
	@ echo "#ifndef $(HEADER_TAG)" > $@
	@ echo "#define $(HEADER_TAG)" >> $@
	$(CPP) $(CFLAGS) -DGENCODECS_HEADER -DGENCODECS_DATA $(CPPFLAGS) $(INPUT) | ./$(PP) >> $@
	$(CPP) $(CFLAGS) $(DFLAGS) -DGENCODECS_HEADER $(CPPFLAGS) $(INPUT) | ./$(PP) >> $@
	@ echo "#endif /* $(HEADER_TAG) */" >> $@
$(OUT_C): $(INPUT) $(PP)
	@ echo "Generating forward definitions"
	@ echo "#include \"$(OUT_H)\"" > $(OUT_C)
	$(CPP) $(CFLAGS) $(DFLAGS) -DGENCODECS_FORWARD -DGENCODECS_DATA $(CPPFLAGS) $(INPUT) | ./$(PP) >> $(OUT_C)
	@ echo "Generating source"	
	$(CPP) $(CFLAGS) $(DFLAGS) -DGENCODECS_DATA $(CPPFLAGS) $(INPUT) | ./$(PP) >> $(OUT_C)

headers: $(HEADERS)

$(HEADERS): $(INPUT) $(PP)

.SUFFIXES: .PRE.h .h
.PRE.h.h:
	@ echo $(DOXYGEN_DESC) > $(DOCS_DIR)/$@
	@ echo "#ifndef $(HEADER_TAG)" >> $(DOCS_DIR)/$@
	@ echo "#define $(HEADER_TAG)" >> $(DOCS_DIR)/$@
	$(CPP) $(CFLAGS) -DGENCODECS_HEADER -DGENCODECS_DATA -DGENCODECS_READ=\"$<\" $(CPPFLAGS) -CC $(INPUT) | ./$(PP) >> $(DOCS_DIR)/$@
	$(CPP) $(CFLAGS) $(DFLAGS) -DGENCODECS_HEADER -DGENCODECS_READ=\"$<\" $(CPPFLAGS) $(INPUT) | ./$(PP) >> $(DOCS_DIR)/$@
	@ echo "#endif /* $(HEADER_TAG) */" >> $(DOCS_DIR)/$@

echo:
	@ echo 'HEADERS: $(HEADERS)'
	@ echo 'INPUT: $(INPUT)'
	@ echo 'OUT_H: $(OUT_H)'
	@ echo 'OUT_C: $(OUT_C)'
	@ echo 'OUT_O: $(OUT_O)'

clean:
	@ rm -f $(OUT_H) $(OUT_C) $(OUT_O) $(PP) $(DOCS_DIR)/$(API_DIR)/*.h

.PHONY: headers echo clean
