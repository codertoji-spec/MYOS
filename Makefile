# Compiler and linker
CC = clang
LD = lld-link

# Directories
BOOT_DIR = boot
BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/iso
DOOM_DIR = programs/doom
DG_DIR = $(DOOM_DIR)/doomgeneric/doomgeneric

# Targets
BOOTLOADER = $(BUILD_DIR)/BOOTX64.EFI
KERNEL = $(BUILD_DIR)/kernel.elf
HELLO_BIN = $(BUILD_DIR)/hello.elf
DOOM_BIN = $(BUILD_DIR)/doom.elf

# Flags
CFLAGS = -target x86_64-unknown-windows \
         -ffreestanding \
         -fshort-wchar \
         -mno-red-zone \
         -Wall \
         -Wextra \
         -I boot

LDFLAGS = -subsystem:efi_application \
          -entry:efi_main \
          -nodefaultlib

KERNEL_CFLAGS = -target x86_64-elf -ffreestanding -mno-red-zone -Wall -Wextra
USER_CFLAGS   = -target x86_64-elf -ffreestanding -mno-red-zone -mcmodel=large -Iprograms/libc/include
DOOM_CFLAGS   = -target x86_64-elf -ffreestanding -mno-red-zone -mcmodel=large -msse2 -fno-stack-protector -fno-builtin -O2 -DNORMALUNIX -D_DEFAULT_SOURCE -Iprograms/libc/include -I$(DG_DIR)

# DOOM Sources
DOOM_SRCS_NAMES = dummy.c am_map.c doomdef.c doomstat.c dstrings.c d_event.c d_items.c d_iwad.c d_loop.c d_main.c d_mode.c d_net.c f_finale.c f_wipe.c g_game.c hu_lib.c hu_stuff.c info.c i_cdmus.c i_endoom.c i_joystick.c i_scale.c i_sound.c i_system.c i_timer.c memio.c m_argv.c m_bbox.c m_cheat.c m_config.c m_controls.c m_fixed.c m_menu.c m_misc.c m_random.c p_ceilng.c p_doors.c p_enemy.c p_floor.c p_inter.c p_lights.c p_map.c p_maputl.c p_mobj.c p_plats.c p_pspr.c p_saveg.c p_setup.c p_sight.c p_spec.c p_switch.c p_telept.c p_tick.c p_user.c r_bsp.c r_data.c r_draw.c r_main.c r_plane.c r_segs.c r_sky.c r_things.c sha1.c sounds.c statdump.c st_lib.c st_stuff.c s_sound.c tables.c v_video.c wi_stuff.c w_checksum.c w_file.c w_main.c w_wad.c z_zone.c w_file_stdc.c i_input.c i_video.c doomgeneric.c

DOOM_OBJS = $(addprefix $(BUILD_DIR)/doom_, $(DOOM_SRCS_NAMES:.c=.o)) $(BUILD_DIR)/doomgeneric_myos.o

LIBC_OBJS = $(BUILD_DIR)/libc_string.o $(BUILD_DIR)/libc_stdlib.o $(BUILD_DIR)/libc_stdio.o
LIBC_ALL_OBJS = $(LIBC_OBJS) $(BUILD_DIR)/libc_math.o $(BUILD_DIR)/libc_setjmp.o

.PHONY: all clean iso doom

all: $(BOOTLOADER) $(KERNEL) $(HELLO_BIN) $(DOOM_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BOOTLOADER): $(BOOT_DIR)/main.c $(BOOT_DIR)/uefi.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(BOOT_DIR)/main.c -o $(BUILD_DIR)/main.o
	$(LD) $(LDFLAGS) $(BUILD_DIR)/main.o -out:$(BOOTLOADER)

$(KERNEL): kernel/core/main.c kernel/arch/x86_64/gdt.c kernel/arch/x86_64/gdt_flush.S kernel/arch/x86_64/idt.c kernel/arch/x86_64/isr.c kernel/arch/x86_64/interrupts.S kernel/mm/pmm.c kernel/mm/vmm.c kernel/mm/heap.c kernel/hw/acpi.c kernel/hw/apic.c kernel/hw/ioapic.c kernel/hw/keyboard.c kernel/hw/mouse.c kernel/task/sched.c kernel/task/thread.c kernel/hw/ata.c kernel/fs/vfs.c kernel/fs/fat32.c kernel/fs/elf.c kernel/gui/compositor.c kernel/shell/shell.c kernel/linker.ld | $(BUILD_DIR)
	clang $(KERNEL_CFLAGS) -c kernel/core/main.c -o $(BUILD_DIR)/kernel_main.o
	clang $(KERNEL_CFLAGS) -c kernel/arch/x86_64/gdt.c -o $(BUILD_DIR)/gdt.o
	clang $(KERNEL_CFLAGS) -c kernel/arch/x86_64/gdt_flush.S -o $(BUILD_DIR)/gdt_flush.o
	clang $(KERNEL_CFLAGS) -c kernel/arch/x86_64/idt.c -o $(BUILD_DIR)/idt.o
	clang $(KERNEL_CFLAGS) -c kernel/arch/x86_64/isr.c -o $(BUILD_DIR)/isr.o
	clang $(KERNEL_CFLAGS) -c kernel/arch/x86_64/interrupts.S -o $(BUILD_DIR)/interrupts.o
	clang $(KERNEL_CFLAGS) -c kernel/mm/pmm.c -o $(BUILD_DIR)/pmm.o
	clang $(KERNEL_CFLAGS) -c kernel/mm/vmm.c -o $(BUILD_DIR)/vmm.o
	clang $(KERNEL_CFLAGS) -c kernel/mm/heap.c -o $(BUILD_DIR)/heap.o
	clang $(KERNEL_CFLAGS) -c kernel/hw/acpi.c -o $(BUILD_DIR)/acpi.o
	clang $(KERNEL_CFLAGS) -c kernel/hw/apic.c -o $(BUILD_DIR)/apic.o
	clang $(KERNEL_CFLAGS) -c kernel/hw/ioapic.c -o $(BUILD_DIR)/ioapic.o
	clang $(KERNEL_CFLAGS) -c kernel/hw/keyboard.c -o $(BUILD_DIR)/keyboard.o
	clang $(KERNEL_CFLAGS) -c kernel/hw/mouse.c -o $(BUILD_DIR)/mouse.o
	clang $(KERNEL_CFLAGS) -c kernel/task/sched.c -o $(BUILD_DIR)/sched.o
	clang $(KERNEL_CFLAGS) -c kernel/task/thread.c -o $(BUILD_DIR)/thread.o
	clang $(KERNEL_CFLAGS) -c kernel/hw/ata.c -o $(BUILD_DIR)/ata.o
	clang $(KERNEL_CFLAGS) -c kernel/fs/vfs.c -o $(BUILD_DIR)/vfs.o
	clang $(KERNEL_CFLAGS) -c kernel/fs/fat32.c -o $(BUILD_DIR)/fat32.o
	clang $(KERNEL_CFLAGS) -c kernel/fs/elf.c -o $(BUILD_DIR)/elf.o
	clang $(KERNEL_CFLAGS) -c kernel/gui/compositor.c -o $(BUILD_DIR)/compositor.o
	clang $(KERNEL_CFLAGS) -c kernel/shell/shell.c -o $(BUILD_DIR)/shell.o
	ld.lld -T kernel/linker.ld $(BUILD_DIR)/kernel_main.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/gdt_flush.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/isr.o $(BUILD_DIR)/interrupts.o $(BUILD_DIR)/pmm.o $(BUILD_DIR)/vmm.o $(BUILD_DIR)/heap.o $(BUILD_DIR)/acpi.o $(BUILD_DIR)/apic.o $(BUILD_DIR)/ioapic.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/mouse.o $(BUILD_DIR)/sched.o $(BUILD_DIR)/thread.o $(BUILD_DIR)/ata.o $(BUILD_DIR)/vfs.o $(BUILD_DIR)/fat32.o $(BUILD_DIR)/elf.o $(BUILD_DIR)/compositor.o $(BUILD_DIR)/shell.o -o $(KERNEL)

$(BUILD_DIR)/libc_string.o: programs/libc/string.c | $(BUILD_DIR)
	clang $(USER_CFLAGS) -c programs/libc/string.c -o $@

$(BUILD_DIR)/libc_stdlib.o: programs/libc/stdlib.c | $(BUILD_DIR)
	clang $(USER_CFLAGS) -c programs/libc/stdlib.c -o $@

$(BUILD_DIR)/libc_stdio.o: programs/libc/stdio.c | $(BUILD_DIR)
	clang $(USER_CFLAGS) -c programs/libc/stdio.c -o $@

$(BUILD_DIR)/libc_math.o: programs/libc/math.c | $(BUILD_DIR)
	clang $(USER_CFLAGS) -c programs/libc/math.c -o $@

$(BUILD_DIR)/libc_setjmp.o: programs/libc/setjmp.S | $(BUILD_DIR)
	clang $(USER_CFLAGS) -c programs/libc/setjmp.S -o $@

$(HELLO_BIN): programs/hello/hello.c programs/hello/linker.ld $(LIBC_OBJS) | $(BUILD_DIR)
	clang $(USER_CFLAGS) -c programs/hello/hello.c -o $(BUILD_DIR)/hello.o
	ld.lld -T programs/hello/linker.ld $(BUILD_DIR)/hello.o $(LIBC_OBJS) -o $(HELLO_BIN)

$(BUILD_DIR)/doom_%.o: $(DG_DIR)/%.c | $(BUILD_DIR)
	clang $(DOOM_CFLAGS) -c $< -o $@

$(BUILD_DIR)/doomgeneric_myos.o: $(DOOM_DIR)/doomgeneric_myos.c | $(BUILD_DIR)
	clang $(DOOM_CFLAGS) -c $< -o $@

$(DOOM_BIN): $(DOOM_OBJS) $(LIBC_ALL_OBJS) programs/hello/linker.ld
	ld.lld -T programs/hello/linker.ld $(DOOM_OBJS) $(LIBC_ALL_OBJS) -o $(DOOM_BIN)

doom: $(DOOM_BIN)

iso: $(BOOTLOADER) $(KERNEL) $(HELLO_BIN) $(DOOM_BIN)
	mkdir -p $(ISO_DIR)/EFI/BOOT
	cp $(BOOTLOADER) $(ISO_DIR)/EFI/BOOT/BOOTX64.EFI
	dd if=/dev/zero of=$(BUILD_DIR)/fat.img bs=1M count=64
	mformat -i $(BUILD_DIR)/fat.img -F -F ::
	mmd -i $(BUILD_DIR)/fat.img ::/EFI
	mmd -i $(BUILD_DIR)/fat.img ::/EFI/BOOT
	mcopy -i $(BUILD_DIR)/fat.img $(BOOTLOADER) ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i $(BUILD_DIR)/fat.img $(KERNEL) ::/kernel.elf
	mcopy -i $(BUILD_DIR)/fat.img $(HELLO_BIN) ::/hello.elf
	mcopy -i $(BUILD_DIR)/fat.img $(DOOM_BIN) ::/doom.elf
	@if [ -f "$(DOOM_DIR)/doom1.wad" ]; then mcopy -o -i $(BUILD_DIR)/fat.img $(DOOM_DIR)/doom1.wad ::/doom1.wad; elif [ -f "$(DOOM_DIR)/DOOM1.WAD" ]; then mcopy -o -i $(BUILD_DIR)/fat.img $(DOOM_DIR)/DOOM1.WAD ::/doom1.wad; fi

clean:
	rm -rf $(BUILD_DIR)
