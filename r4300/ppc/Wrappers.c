/**
 * Wii64 - Wrappers.c
 * Copyright (C) 2008, 2009, 2010 Mike Slegeir
 * 
 * Interface between emulator code and recompiled code
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

#include <stdlib.h>
#include "../Invalid_Code.h"
#include "../ARAM-blocks.h"
#include "../../gc_memory/memory.h"
#include "../interupt.h"
#include "../r4300.h"
#include "../Recomp-Cache.h"
#include "Recompile.h"
#include "Wrappers.h"

extern u32 instructionCount;
extern void (*interp_ops[64])(void);
u32 update_invalid_addr(u32 addr);
unsigned int dyna_check_cop1_unusable(unsigned int, int);
unsigned int dyna_mem(unsigned int, unsigned int, memType, unsigned int, int);

int noCheckInterrupt = 0;

static PowerPC_instr* link_branch = NULL;
static PowerPC_func* last_func;

/* Recompiled code stack frame:
 *  $sp+12  |
 *  $sp+8   | old cr
 *  $sp+4   | old lr
 *  $sp	    | old sp
 */

inline unsigned int dyna_run(PowerPC_func* func, unsigned int (*code)(void)){
	unsigned int naddr;
	PowerPC_instr* return_addr;

	__asm__ volatile(
		// Create the stack frame for code
		"stwu	1, -16(1) \n"
		"mfcr	14        \n"
		"stw	14, 8(1)  \n"
		// Setup saved registers for code
		"mr	14, %0    \n"
		"mr	15, %1    \n"
		"mr	16, %2    \n"
		"mr	17, %3    \n"
		"mr	18, %4    \n"
		"mr	19, %5    \n"
		"mr	20, %6    \n"
		"mr	21, %7    \n"
		"mr	22, %8    \n"
		"addi	23, 0, 0  \n"
		:: "r" (r4300.gpr), "r" (r4300.reg_cop0),
		   "r" (r4300.fpr_single), "r" (r4300.fpr_double),
		   "r" (&r4300.fcr31), "r" (rdram),
		   "r" (&r4300.last_pc), "r" (&r4300.next_interrupt),
		   "r" (func)
		: "14", "15", "16", "17", "18", "19", "20", "21", "22", "23");

	end_section(TRAMP_SECTION);

	// naddr = code();
	__asm__ volatile(
		// Save the lr so the recompiled code won't have to
		"bl	4         \n"
		"mtctr	%4        \n"
		"mflr	4         \n"
		"addi	4, 4, 20  \n"
		"stw	4, 20(1)  \n"
		// Execute the code
		"bctrl           \n"
		"mr	%0, 3     \n"
		// Get return_addr, link_branch, and last_func
		"lwz	%2, 20(1) \n"
		"mflr	%1        \n"
		"mr	%3, 22    \n"
		// Pop the stack
		"lwz	1, 0(1)   \n"
		: "=r" (naddr), "=r" (link_branch), "=r" (return_addr),
		  "=r" (last_func)
		: "r" (code)
		: "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "22");

	link_branch = link_branch == return_addr ? NULL : link_branch - 1;
	
	return naddr;
}

void dynarec(unsigned int address){
	while(!r4300.stop){
		refresh_stat();
		
		start_section(TRAMP_SECTION);
		PowerPC_block* dst_block = blocks_get(address>>12);
		u32 paddr = update_invalid_addr(address);

		dbg_printf("trampolining to 0x%08x\n", address);
		if(!paddr){ 
			address = paddr = update_invalid_addr(r4300.pc);
			dst_block = blocks_get(address>>12); 
		}
		
		if(!dst_block){
			dbg_printf("block at %08x doesn't exist\n", address&~0xFFF);
			dst_block = malloc(sizeof(PowerPC_block));
			blocks_set(address>>12, dst_block);
			//dst_block->code_addr     = NULL;
			dst_block->funcs         = NULL;
			dst_block->start_address = address & ~0xFFF;
			dst_block->end_address   = (address & ~0xFFF) + 0x1000;
			if((paddr >= 0xb0000000 && paddr < 0xc0000000) ||
			   (paddr >= 0x90000000 && paddr < 0xa0000000)){
				init_block(NULL, dst_block);
			} else {
				init_block(rdram+(((paddr-(address-dst_block->start_address)) & 0x1FFFFFFF)>>2),
						   dst_block);
			}
		} else if(invalid_code_get(address>>12)){
			invalidate_block(dst_block);
		}

		PowerPC_func* func = find_func(&dst_block->funcs, address);

		if(!func || !func->code_addr[(address-func->start_addr)>>2]){
			dbg_printf("code at %08x is not compiled\n", address);
			if((paddr >= 0xb0000000 && paddr < 0xc0000000) ||
			   (paddr >= 0x90000000 && paddr < 0xa0000000))
				dst_block->mips_code =
					ROMCache_pointer((paddr-(address-dst_block->start_address))&0x0FFFFFFF);
			start_section(COMPILER_SECTION);
			func = recompile_block(dst_block, address);
			end_section(COMPILER_SECTION);
		} else {
#ifdef USE_RECOMP_CACHE
			RecompCache_Update(func);
#endif
		}

		int index = (address - func->start_addr)>>2;

		// Recompute the block offset
		unsigned int (*code)(void);
		code = (unsigned int (*)(void))func->code_addr[index];
		
		// Create a link if possible
		if(link_branch && !func_was_freed(last_func))
			RecompCache_Link(last_func, link_branch, func, code);
		clear_freed_funcs();
		
		dbg_printf("calling dyna_run func %08X code %08X\n", func, code);
		r4300.pc = address = dyna_run(func, code);

		if(!noCheckInterrupt){
			r4300.last_pc = r4300.pc;
			// Check for interrupts
			if(r4300.next_interrupt <= Count){
				gen_interupt();
				address = r4300.pc;
			}
		}
		noCheckInterrupt = 0;
	}
	r4300.pc = address;
}

unsigned int decodeNInterpret(MIPS_instr mips, unsigned int pc,
                              int isDelaySlot){
	r4300.delay_slot = isDelaySlot; // Make sure we set r4300.delay_slot properly
	r4300.pc = pc;
	start_section(INTERP_SECTION);
	prefetch_opcode(mips);
	interp_ops[MIPS_GET_OPCODE(mips)]();
	end_section(INTERP_SECTION);
	r4300.delay_slot = 0;

	if(r4300.pc != pc + 4) noCheckInterrupt = 1;

	return r4300.pc != pc + 4 ? r4300.pc : 0;
}

#ifdef COMPARE_CORE
int dyna_update_count(unsigned int pc, int isDelaySlot){
#else
int dyna_update_count(unsigned int pc){
#endif
	Count += (pc - r4300.last_pc)/2;
	r4300.last_pc = pc;

#ifdef COMPARE_CORE
	if(isDelaySlot){
		r4300.pc = pc;
		compare_core();
	}
#endif

	return r4300.next_interrupt - Count;
}

unsigned int dyna_check_cop1_unusable(unsigned int pc, int isDelaySlot){
	// Set state so it can be recovered after exception
	r4300.delay_slot = isDelaySlot;
	r4300.pc = pc;
	// Take a FP unavailable exception
	Cause = (11 << 2) | 0x10000000;
	exception_general();
	// Reset state
	r4300.delay_slot = 0;
	noCheckInterrupt = 1;
	// Return the address to trampoline to
	return r4300.pc;
}

static void invalidate_func(unsigned int addr){
	PowerPC_block* block = blocks_get(addr>>12);
	PowerPC_func* func = find_func(&block->funcs, addr);
	if(func)
		RecompCache_Free(func->start_addr);
}

#define check_memory() \
	if(!invalid_code_get(address>>12)/* && \
	   blocks[address>>12]->code_addr[(address&0xfff)>>2]*/) \
		invalidate_func(address);

unsigned int dyna_mem(unsigned int value, unsigned int addr,
                      memType type, unsigned int pc, int isDelaySlot){
	static unsigned long long dyna_rdword;

	address = addr;
	rdword = &dyna_rdword;
	r4300.pc = pc;
	r4300.delay_slot = isDelaySlot;

	switch(type){
		case MEM_LW:
			read_word_in_memory();
			r4300.gpr[value] = (long long)((s32)dyna_rdword);
			break;
		case MEM_LWU:
			read_word_in_memory();
			r4300.gpr[value] = (unsigned long long)((s32)dyna_rdword);
			break;
		case MEM_LH:
			read_hword_in_memory();
			r4300.gpr[value] = (long long)((short)dyna_rdword);
			break;
		case MEM_LHU:
			read_hword_in_memory();
			r4300.gpr[value] = (unsigned long long)((unsigned short)dyna_rdword);
			break;
		case MEM_LB:
			read_byte_in_memory();
			r4300.gpr[value] = (long long)((signed char)dyna_rdword);
			break;
		case MEM_LBU:
			read_byte_in_memory();
			r4300.gpr[value] = (unsigned long long)((unsigned char)dyna_rdword);
			break;
		case MEM_LD:
			read_dword_in_memory();
			r4300.gpr[value] = dyna_rdword;
			break;
		case MEM_LWC1:
			read_word_in_memory();
			*((s32*)r4300.fpr_single[value]) = (s32)dyna_rdword;
			break;
		case MEM_LDC1:
			read_dword_in_memory();
			*((long long*)r4300.fpr_double[value]) = dyna_rdword;
			break;
		case MEM_SW:
			word = value;
			write_word_in_memory();
			check_memory();
			break;
		case MEM_SH:
			hword = value;
			write_hword_in_memory();
			check_memory();
			break;
		case MEM_SB:
			byte = value;
			write_byte_in_memory();
			check_memory();
			break;
		case MEM_SD:
			dword = r4300.gpr[value];
			write_dword_in_memory();
			check_memory();
			break;
		case MEM_SWC1:
			word = *((s32*)r4300.fpr_single[value]);
			write_word_in_memory();
			check_memory();
			break;
		case MEM_SDC1:
			dword = *((unsigned long long*)r4300.fpr_double[value]);
			write_dword_in_memory();
			check_memory();
			break;
		default:
			r4300.stop = 1;
			break;
	}
	r4300.delay_slot = 0;

	if(r4300.pc != pc) noCheckInterrupt = 1;

	return r4300.pc != pc ? r4300.pc : 0;
}

