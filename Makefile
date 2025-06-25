include include.mk

lab                     ?= $(shell cat .mos-this-lab 2>/dev/null || echo 6)
# ----- MOS BLANK BEGIN -----
CFLAGS                  += -Werror -Wa,--fatal-warnings
HOST_CFLAGS             += -Werror -Wa,--fatal-warnings
# ----- MOS BLANK END -----

target_dir              := target
mos_elf                 := $(target_dir)/mos
user_disk               := $(target_dir)/fs.img
empty_disk              := $(target_dir)/empty.img
swap_disk				:= $(target_dir)/swap.img
qemu_pts                := $(shell [ -f .qemu_log ] && grep -Eo '/dev/pts/[0-9]+' .qemu_log)
link_script             := kernel.lds

modules                 := lib init kern
targets                 := $(mos_elf)

ifneq ($(prog),)
dbg_elf                 := -ex "add-symbol-file $(prog)"
endif

lab-ge = $(shell [ "$$(echo $(lab)_ | cut -f1 -d_)" -ge $(1) ] && echo true)

ifeq ($(call lab-ge,3),true)
	user_modules    += user/bare
endif

ifeq ($(call lab-ge,4),true)
	user_modules    += user
endif

ifeq ($(call lab-ge,5),true)
	user_modules    += fs
	targets         += fs-image
endif

objects                 := $(addsuffix /*.o, $(modules)) $(addsuffix /*.x, $(user_modules))
modules                 += $(user_modules)

CFLAGS                  += -DLAB=$(shell echo $(lab) | cut -f1 -d_)
QEMU_FLAGS              += -cpu 4Kc -m 64 -nographic -M malta \
						$(shell [ -f '$(user_disk)' ] && echo '-drive id=ide0,file=$(user_disk),if=ide,format=raw') \
						$(shell [ -f '$(empty_disk)' ] && echo '-drive id=ide1,file=$(empty_disk),if=ide,format=raw') \
						$(shell [ -f '$(swap_disk)' ] && echo '-drive id=swap,file=$(swap_disk),if=ide,format=raw') \
						-no-reboot

.PHONY: all test tools $(modules) clean run dbg_run dbg_pts dbg objdump fs-image clean-and-all connect

.ONESHELL:
clean-and-all: clean
	$(MAKE) all

test: export test_dir = tests/lab$(lab)
test: clean-and-all

include mk/tests.mk mk/profiles.mk
export CC CFLAGS LD LDFLAGS lab

all: $(targets)

$(target_dir):
	mkdir -p $@

tools:
	CC="$(HOST_CC)" CFLAGS="$(HOST_CFLAGS)" $(MAKE) --directory=$@

$(modules): tools
	$(MAKE) --directory=$@

$(mos_elf): $(modules) $(target_dir)
	$(LD) $(LDFLAGS) -o $(mos_elf) -N -T $(link_script) $(objects)

fs-image: $(target_dir) user
	$(MAKE) --directory=fs image fs-files="$(addprefix ../, $(fs-files))"

fs: user
user: lib

clean:
# ----- MOS BLANK BEGIN -----
	[ -d quizs ] && cp tests/include.mk quizs/tests/
	rm -rf include/preload .mos-crypt-*
# ----- MOS BLANK END -----
	for d in * tools/readelf user/* tests/*; do
		if [ -f $$d/Makefile ]; then
			$(MAKE) --directory=$$d clean
		fi
	done
	rm -rf *.o *~ $(target_dir) include/generated
	find . -name '*.objdump' -exec rm {} ';'

run:
	$(QEMU) $(QEMU_FLAGS) -kernel $(mos_elf)

dbg_run:
	$(QEMU) $(QEMU_FLAGS) -kernel $(mos_elf) -s -S

dbg:
	export QEMU="$(QEMU)"
	export QEMU_FLAGS="$(QEMU_FLAGS)"
	export mos_elf="$(mos_elf)"
	setsid ./tools/run_bg.sh $$$$ &
	exec gdb-multiarch -q $(mos_elf) -ex "target remote localhost:1234" $(dbg_elf)

dbg_pts: QEMU_FLAGS += -serial "pty"
dbg_pts: dbg

connect:
	[ -f .qemu_log ] && screen -R mos $(qemu_pts)

objdump:
	@find * \( -name '*.b' -o -path $(mos_elf) \) -exec sh -c \
	'$(CROSS_COMPILE)objdump {} -aldS > {}.objdump && echo {}.objdump' ';'
# ----- MOS BLANK BEGIN -----
.PHONY: judge quiz quiz-judge check-style fix-style register-git-hooks

judge: test
	judges/judge lab$(lab) $(judge_flags) -v

quiz:
	cp tests/include.mk quizs/tests/
	test_dir=quizs/tests/lab$(lab) $(MAKE) clean-and-all root_dir='$(PWD)'

quiz-judge: quiz
	judges/judge -d quizs/judges/lab$(lab) $(judge_flags) -v

check-style: clean
	judges/check-style

fix-style: clean
	judges/check-style -f

register-git-hooks:
	ln -s ../../judges/pre-commit .git/hooks/pre-commit
# ----- MOS BLANK END -----
