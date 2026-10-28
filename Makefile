#---------------------------------------------------------------------------------
# Abyssal Eclipse - Nintendo 3DS (devkitARM + libctru + citro2d/citro3d)
# Build:   export DEVKITPRO=/opt/devkitpro DEVKITARM=$DEVKITPRO/devkitARM ; make
# Output:  AbyssalEclipse.3dsx (Homebrew) or AbyssalEclipse.cia (make cia)
#---------------------------------------------------------------------------------
.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif
TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET      := AbyssalEclipse
BUILD       := build
SOURCES     := source
DATA        :=
INCLUDES    := source
APP_TITLE   := Abyssal Eclipse
APP_DESCRIPTION := Dark fantasy roguelite
APP_AUTHOR  := MrJuaumBR

# CIA and Banner Tools (located in the root folder)
BANNERTOOL  := ./bannertool.exe
MAKEROM     := ./makerom.exe
CIA_NAME    := $(TARGET).cia
RSF_FILE    := $(TARGET).rsf
BANNER_PNG  := banner.png
BANNER_BNR  := banner.bnr
ICON_PNG    := icon.png
SMDH_FILE   := $(TARGET).smdh

# Old 3DS = ARM11 MPCore (ARMv6K), hard-float VFP11. Never target New-3DS-only features.
ARCH   := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -g -Wall -O2 -mword-relocations -ffunction-sections -fdata-sections $(ARCH)
CFLAGS += $(INCLUDE) -D__3DS__
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17
ASFLAGS  := -g $(ARCH)
LDFLAGS   = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map) -Wl,--gc-sections
LIBS    := -lcitro2d -lcitro3d -lctru -lm
LIBDIRS := $(CTRULIB)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
export LD := $(CXX)
export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.png)
	ifneq (,$(findstring $(TARGET).png,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).png
	else ifneq (,$(findstring icon.png,$(icons)))
		export APP_ICON := $(TOPDIR)/icon.png
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_SMDH)),)
	export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
endif

.PHONY: all clean regen cia aseprite-export aseprite-import FORCE

# Prefer Python 3, then python, then the Windows py launcher used by some MSYS2 installs.
PYTHON ?= $(shell if command -v python3 >/dev/null 2>&1; then echo python3; elif command -v python >/dev/null 2>&1; then echo python; elif command -v py >/dev/null 2>&1; then echo 'py -3'; fi)
DATA_INPUTS := tools/gen_data.py tools/art_raster.py tools/data/mock_data.json tools/data/art.json tools/data/project.json

all: $(BUILD)

# The generated art/spell tables are a prerequisite of the recursive build.
$(BUILD): source/data_gen.cpp
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

source/data_gen.cpp: $(DATA_INPUTS)
	@if [ -z "$(strip $(PYTHON))" ]; then echo "Python 3 not found. Install Python 3 or run make PYTHON='py -3'." >&2; exit 127; fi
	$(PYTHON) tools/gen_data.py

regen: FORCE
	@if [ -z "$(strip $(PYTHON))" ]; then echo "Python 3 not found. Install Python 3 or run make PYTHON='py -3'." >&2; exit 127; fi
	$(PYTHON) tools/gen_data.py

FORCE:

aseprite-export:
	$(PYTHON) tools/aseprite_sync.py export

aseprite-import:
	$(PYTHON) tools/aseprite_sync.py import

# --- CIA Build Target ---
cia: all $(BANNER_BNR) $(SMDH_FILE)
	@echo "Building CIA..."
	$(MAKEROM) -f cia -target t -exefslogo -o $(CIA_NAME) -elf $(TARGET).elf -rsf $(RSF_FILE) -banner $(BANNER_BNR) -icon $(SMDH_FILE)
	@echo "CIA built: $(CIA_NAME)"

# --- Icon and Banner Generation ---
$(SMDH_FILE): $(ICON_PNG)
	@echo "Generating SMDH..."
	$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "$(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i $(ICON_PNG) -o $@

$(BANNER_BNR): $(BANNER_PNG)
	@echo "Generating Banner..."
	$(BANNERTOOL) makebanner -i "$(CURDIR)/$(BANNER_PNG)" -a "$(CURDIR)/silence.wav" -o "$(CURDIR)/$@"

# --- Clean and Regen ---
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(OUTPUT).smdh $(TARGET).elf $(CIA_NAME) $(SMDH_FILE) $(BANNER_BNR)


else
DEPENDS := $(OFILES:.o=.d)
$(OUTPUT).3dsx : $(OUTPUT).elf $(OUTPUT).smdh
$(OUTPUT).elf : $(OFILES)
-include $(DEPENDS)
endif
