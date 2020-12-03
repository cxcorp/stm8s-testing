#**********************************************************#
#file     makefile
#author   Rajmund Szymanski
#date     06.09.2018
#brief    STM8 makefile.
#**********************************************************#

SDCC       := 
STVP       := "C:\Program Files (x86)\STMicroelectronics\st_toolset\stvp\STVP_CmdLine.exe" -BoardName=ST-LINK -Port=USB -ProgMode=SWIM -Device=STM8S103F3 -verif -no_loop -no_log

#----------------------------------------------------------#

PROJECT    ?=
DEFS       ?=
DIRS       ?=
INCS       ?=
LIBS       ?=
KEYS       ?=
SCRIPT     ?=

#----------------------------------------------------------#

DEFS       += STM8S103
KEYS       += .sdcc .stm8 .stm8s *
LIBS       += stm8 IntrOS

#----------------------------------------------------------#

AS         := $(SDCC)sdasstm8
CC         := $(SDCC)sdcc
LD         := $(SDCC)sdld
AR         := $(SDCC)sdar
COPY       := $(SDCC)sdobjcopy
DBG        := $(SDCC)sdcdb
SIM        := $(SDCC)sstm8

SIZE       := size

RM         := del /F/Q

#----------------------------------------------------------#

DTREE       = $(foreach d,$(foreach k,$(KEYS),$(wildcard $1$k)),$(dir $d) $(call DTREE,$d/))

VPATH      := $(sort $(call DTREE,) $(foreach d,$(DIRS),$(call DTREE,$d/)))

#----------------------------------------------------------#

INC_DIRS   := $(sort $(dir $(foreach d,$(VPATH),$(wildcard $d*.h))))
LIB_DIRS   := $(sort $(dir $(foreach d,$(VPATH),$(wildcard $d*.lib))))
AS_SRCS    :=              $(foreach d,$(VPATH),$(wildcard $d*.s))
CC_SRCS    :=              $(foreach d,$(VPATH),$(wildcard $d*.c))
LIB_SRCS   :=     $(notdir $(foreach d,$(VPATH),$(wildcard $d*.lib)))

ifeq ($(strip $(PROJECT)),)
PROJECT    :=     $(notdir $(CURDIR))
endif

#----------------------------------------------------------#

ELF        := $(PROJECT).elf
HEX        := $(PROJECT).hex
LIB        := $(PROJECT).lib
MAP        := $(PROJECT).map
CDB        := $(PROJECT).cdb
LKF        := $(PROJECT).lk

OBJS       := $(AS_SRCS:.s=.rel)
OBJS       += $(CC_SRCS:.c=.rel)
ASMS       := $(OBJS:.rel=.asm)
LSTS       := $(OBJS:.rel=.lst)
RSTS       := $(OBJS:.rel=.rst)
SYMS       := $(OBJS:.rel=.sym)
ADBS       := $(OBJS:.rel=.adb)
DEPS       := $(OBJS:.rel=.d)

#----------------------------------------------------------#

CORE_F      = -mstm8
COMMON_F    = --opt-code-size #--debug
AS_FLAGS    = -l -o -s
CC_FLAGS    = --std-sdcc11 -MD
LD_FLAGS    =

#----------------------------------------------------------#

DEFS_F     := $(DEFS:%=-D%)

INC_DIRS   += $(INCS:%=%/)
INC_DIRS_F := $(INC_DIRS:%/=-I%)

SRC_DIRS   := $(sort $(dir $(AS_SRCS) $(CC_SRCS)))
SRC_DIRS_F := $(SRC_DIRS:%/=--directory=%)

LIB_DIRS_F := $(LIB_DIRS:%/=-L%)
LIBS_F     := $(LIBS:%=-l%)
LIBS_F     += $(LIB_SRCS:%.lib=-l%)

AS_FLAGS   +=
CC_FLAGS   += $(CORE_F) $(COMMON_F) $(DEFS_F) $(INC_DIRS_F)
LD_FLAGS   += $(CORE_F) $(COMMON_F) $(LIBS_F) $(LIB_DIRS_F)

#----------------------------------------------------------#

all : $(ELF)

lib : $(LIB)

$(ELF) : $(OBJS)
	$(info Linking target: $(ELF))
	$(CC) --out-fmt-elf $(LD_FLAGS) $(OBJS) -o $@

$(LIB) : $(OBJS)
	$(info Building library: $(LIB))
	$(AR) -r $@ $?

$(OBJS) : $(MAKEFILE_LIST)

%.rel : %.s
	$(info Assembling file: $<)
	$(AS) $(AS_FLAGS) $@ $<

%.rel : %.c
	$(info Compiling file: $<)
	$(CC) -c $(CC_FLAGS) $< -o $@

$(HEX) : $(OBJS)
	$(info Creating HEX image: $(HEX))
	$(CC) $(LD_FLAGS) $(OBJS) -o $@

GENERATED = $(BIN) $(ELF) $(HEX) $(LIB) $(LSS) $(MAP) $(CDB) $(LKF) $(LSTS) $(OBJS) $(ASMS) $(DEPS) $(LSTS) $(RSTS) $(SYMS) $(ADBS)

clean :
	$(info Removing all generated output files)
	for %%f in ($(addprefix ",$(addsuffix ",$(subst /,\,$(GENERATED))))) do (if exist %%f del /F %%f)

flash : all $(HEX)
	$(info Programing device...)
	$(STVP) -FileProg=$(HEX)

debug : all $(HEX)
	$(info Debugging device...)
	$(DBG) $(PROJECT) $(SRC_DIRS_F)
#	$(SIM) $(HEX)

.PHONY : all lib clean flash debug

-include $(DEPS)