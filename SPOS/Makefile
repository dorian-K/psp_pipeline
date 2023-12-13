PROJ=SPOS

# Microprocessor
MCU=atmega644

SRC := $(wildcard $(PROJ)/*.c) $(wildcard $(PROJ)/*/*.c)
SRC := $(subst ',,$(subst :,,$(SRC))) # rudimentary escaping

# Include Subdirectories with depth 1
SRC_SUBDIRS := $(wildcard $(PROJ)/*/.)
SRC_SUBDIRS := $(subst /.,,$(SRC_SUBDIRS))

# standard linux include
INC = '$(PROJ)'
# subdirectories include
INC += $(SRC_SUBDIRS)

CFLAGS = \
  -x c \
  -mmcu=$(MCU) \
  -c \
  -std=c99 \
  $(addprefix -I,$(INC)) \
  -DF_CPU=20000000UL \
  -Wall \
  -Werror \
  -Wno-error=cpp \
  -Wno-format \
  -funsigned-char \
  -funsigned-bitfields \
  -ffunction-sections \
  -fdata-sections \
  -fpack-struct \
  -fshort-enums

ifeq ($(OS),Windows_NT)
  # unlike cmd.exe powershell can handle UNC paths and single quotes
  SHELL=powershell.exe
endif

ifneq (,$(filter $(MAKECMDGOALS),debug))
  # minimal optimization and all debugging symbols
  OUT := ./bin/debug
  CFLAGS += -Og -g3
else
  # good optimization and some debugging symbols
  OUT := ./bin/release
  CFLAGS += -O2 -g2
endif

# add content of ADDITIONAL_CFLAGS to CFLAGS
# this enables us to add flags to avr-gcc from CLI
CFLAGS += $(ADDITIONAL_CFLAGS)

LDFLAGS = \
  -Wl,--gc-sections \
  -Wl,-u,vfprintf -lprintf_flt -lm

############

all: elf size
	@echo ""
	@echo "COMPILE SUCCESSFUL"
	@echo ""

debug: elf size
	@echo ""
	@echo "COMPILE SUCCESSFUL (DEBUG)"
	@echo ""

elf: $(PROJ).elf

OBJ=$(patsubst %.c, $(OUT)/%.o, $(SRC))

$(PROJ).elf: $(OBJ) FORCEEXEC
	avr-gcc $(LDFLAGS) -mmcu=$(MCU) $(foreach obj,$(OBJ),'$(obj)') -o '$(PROJ).elf'

# Force executable creation since we place debug and release executables at the same spot:
FORCEEXEC:

$(OUT)/:
	mkdir -p $(OUT)
	mkdir -p $(OUT)/$(PROJ)
	$(foreach dir, $(SRC_SUBDIRS), mkdir -p $(OUT)/$(dir))
	@echo $(SRC_SUBDIRS)

$(OUT)/%.o: %.c $(OUT)/%.d $(OUT)/
	avr-gcc -c $(CFLAGS) -MD -MP -MT '$@' -MF '$(@:%.o=%.d)' -o '$@' '$<'

print_sources:
	@$(foreach src,$(SRC),echo $(src);)

size:
	avr-size --mcu=$(MCU) -C '$(PROJ).elf'

clean:
	rm -rf ./bin
	rm -rf '$(PROJ).elf'


DEP=$(patsubst %.c, $(OUT)/%.d, $(SRC))
$(DEP):

include $(wildcard $(DEP))
