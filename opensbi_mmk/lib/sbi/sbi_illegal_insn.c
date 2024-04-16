/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_barrier.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_emulate_csr.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_illegal_insn.h>
#include <sbi/sbi_pmu.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_unpriv.h>
#include <sbi/sbi_console.h>

static void sbi_trap_info(struct sbi_trap_regs *regs)
{
	return;
	
	u32 hartid = current_hartid();

	sbi_printf("%s: hart%d: reg info\n", __func__, hartid);
	//sbi_printf("%s: hart%d: mcause=0x%" PRILX " mtval=0x%" PRILX "\n",
	//	   __func__, hartid, trap->cause, trap->tval);
	sbi_printf("%s: hart%d: mepc=0x%" PRILX " mstatus=0x%" PRILX "\n",
		   __func__, hartid, regs->mepc, regs->mstatus);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "ra", regs->ra, "sp", regs->sp);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "gp", regs->gp, "tp", regs->tp);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "t0", regs->t0, "t1", regs->t1);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "t2", regs->t2, "s0", regs->s0);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "s1", regs->s1, "a0", regs->a0);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "a1", regs->a1, "a2", regs->a2);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "a3", regs->a3, "a4", regs->a4);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "a5", regs->a5, "a6", regs->a6);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "a7", regs->a7, "s2", regs->s2);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "s3", regs->s3, "s4", regs->s4);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "s5", regs->s5, "s6", regs->s6);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "s7", regs->s7, "s8", regs->s8);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "s9", regs->s9, "s10", regs->s10);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "s11", regs->s11, "t3", regs->t3);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "t4", regs->t4, "t5", regs->t5);
	sbi_printf("%s: hart%d: %s=0x%" PRILX "\n", __func__, hartid, "t6",
		   regs->t6);
}

typedef int (*illegal_insn_func)(ulong insn, struct sbi_trap_regs *regs);

static int truly_illegal_insn(ulong insn, struct sbi_trap_regs *regs)
{
	struct sbi_trap_info trap;

	trap.epc = regs->mepc;
	trap.cause = CAUSE_ILLEGAL_INSTRUCTION;
	trap.tval = insn;
	trap.tval2 = 0;
	trap.tinst = 0;
	trap.gva   = 0;
	//sbi_printf("fault instruction: %lx\n",insn);
	return sbi_trap_redirect(regs, &trap);
}

static int misc_mem_opcode_insn(ulong insn, struct sbi_trap_regs *regs)
{
	/* Errata workaround: emulate `fence.tso` as `fence rw, rw`. */
	if ((insn & INSN_MASK_FENCE_TSO) == INSN_MATCH_FENCE_TSO) {
		smp_mb();
		regs->mepc += 4;
		return 0;
	}

	return truly_illegal_insn(insn, regs);
}

static int system_opcode_insn(ulong insn, struct sbi_trap_regs *regs)
{
	int do_write, rs1_num = (insn >> 15) & 0x1f;
	ulong rs1_val = GET_RS1(insn, regs);
	int csr_num   = (u32)insn >> 20;
	ulong prev_mode = (regs->mstatus & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT;
	ulong csr_val, new_csr_val;

	if (prev_mode == PRV_M) {
		sbi_printf("%s: Failed to access CSR %#x from M-mode",
			__func__, csr_num);
		return SBI_EFAIL;
	}

	/* TODO: Ensure that we got CSR read/write instruction */

	if (sbi_emulate_csr_read(csr_num, regs, &csr_val))
		return truly_illegal_insn(insn, regs);

	do_write = rs1_num;
	switch (GET_RM(insn)) {
	case 1:
		new_csr_val = rs1_val;
		do_write    = 1;
		break;
	case 2:
		new_csr_val = csr_val | rs1_val;
		break;
	case 3:
		new_csr_val = csr_val & ~rs1_val;
		break;
	case 5:
		new_csr_val = rs1_num;
		do_write    = 1;
		break;
	case 6:
		new_csr_val = csr_val | rs1_num;
		break;
	case 7:
		new_csr_val = csr_val & ~rs1_num;
		break;
	default:
		return truly_illegal_insn(insn, regs);
	}

	if (do_write && sbi_emulate_csr_write(csr_num, regs, new_csr_val))
		return truly_illegal_insn(insn, regs);

	SET_RD(insn, regs, csr_val);

	regs->mepc += 4;

	return 0;
}

static const illegal_insn_func illegal_insn_table[32] = {
	truly_illegal_insn, /* 0 */
	truly_illegal_insn, /* 1 */
	truly_illegal_insn, /* 2 */
	misc_mem_opcode_insn, /* 3 */
	truly_illegal_insn, /* 4 */
	truly_illegal_insn, /* 5 */
	truly_illegal_insn, /* 6 */
	truly_illegal_insn, /* 7 */
	truly_illegal_insn, /* 8 */
	truly_illegal_insn, /* 9 */
	truly_illegal_insn, /* 10 */
	truly_illegal_insn, /* 11 */
	truly_illegal_insn, /* 12 */
	truly_illegal_insn, /* 13 */
	truly_illegal_insn, /* 14 */
	truly_illegal_insn, /* 15 */
	truly_illegal_insn, /* 16 */
	truly_illegal_insn, /* 17 */
	truly_illegal_insn, /* 18 */
	truly_illegal_insn, /* 19 */
	truly_illegal_insn, /* 20 */
	truly_illegal_insn, /* 21 */
	truly_illegal_insn, /* 22 */
	truly_illegal_insn, /* 23 */
	truly_illegal_insn, /* 24 */
	truly_illegal_insn, /* 25 */
	truly_illegal_insn, /* 26 */
	truly_illegal_insn, /* 27 */
	system_opcode_insn, /* 28 */
	truly_illegal_insn, /* 29 */
	truly_illegal_insn, /* 30 */
	truly_illegal_insn  /* 31 */
};

int sbi_illegal_insn_handler(ulong insn, struct sbi_trap_regs *regs)
{
	struct sbi_trap_info uptrap;
	{
		insn = sbi_get_insn(regs->mepc, &uptrap);

		ulong epc = regs->mepc;
		ulong csr = insn >> 20;
		ulong fn = (insn >> 12) & 0x7;
		ulong src = (insn >> 15) & 0b11111;
		ulong src2 = (insn >> 20) & 0b11111;
		ulong dst = (insn >> 7) & 0b11111;
		ulong opcode = insn & 0b1111111;
		ulong fncode = insn >> 25;

		//the inst below is handling csrw satp.
		if (csr == 0x180 && opcode == 0x73) {
			regs->mepc += 4;
			if (fn != 1 || fn != 5){
				//sbi_printf("[SBI_satp] only csrrw are supported! current: %ld\n",fn);
				//return 0;
			}
			if((epc < 0x80800000) || epc>0xffffffffffffd000){
				ulong write_val = ((ulong*)regs)[src];
				if(dst != 0){
					((ulong*)regs)[dst] = csr_read(CSR_SATP);
				}
				if(src != 0){
					csr_write(CSR_SATP,write_val);
					asm("sfence.vma");
				}
				//sbi_printf("[SBI_satp] satp operate success: %lx\n", csr_read(CSR_SATP));
				return 0;
			}else{
				sbi_printf("[SBI_satp] permission denied for mepc = %lx.\n",epc);
				return 0;
			}
		}

		//the inst below is handling sfence.vma.
		if(opcode == 0x73 && fncode == 0x09){
			regs->mepc += 4;
			ulong vaddr = ((ulong*)regs)[src];
			ulong asid = ((ulong*)regs)[src2];
			asm("sfence.vma %0, %1" :: "r"(vaddr), "r"(asid));
			//sbi_printf("sfence.vma: %lx %lx \n", vaddr, asid);
			return 0;
		}
	}

//	sbi_printf("M mode illgal inst handled at %lx.\n",regs->mepc);
	sbi_trap_info(regs);
	/*
	 * We only deal with 32-bit (or longer) illegal instructions. If we
	 * see instruction is zero OR instruction is 16-bit then we fetch and
	 * check the instruction encoding using unprivilege access.
	 *
	 * The program counter (PC) in RISC-V world is always 2-byte aligned
	 * so handling only 32-bit (or longer) illegal instructions also help
	 * the case where MTVAL CSR contains instruction address for illegal
	 * instruction trap.
	 */

	sbi_pmu_ctr_incr_fw(SBI_PMU_FW_ILLEGAL_INSN);
	if (unlikely((insn & 3) != 3)) {
		insn = sbi_get_insn(regs->mepc, &uptrap);

		if (uptrap.cause) {
			uptrap.epc = regs->mepc;
			return sbi_trap_redirect(regs, &uptrap);
		}
		if ((insn & 3) != 3)
			return truly_illegal_insn(insn, regs);
	}

	return illegal_insn_table[(insn & 0x7c) >> 2](insn, regs);
}
