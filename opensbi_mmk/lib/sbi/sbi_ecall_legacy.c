/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *   Atish Patra <atish.patra@wdc.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_timer.h>
#include <sbi/sbi_tlb.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_unpriv.h>
#include <sbi/sbi_hart.h>

static void sbi_trap_info(const struct sbi_trap_regs *regs, 
const struct sbi_trap_info *trap)
{
	u32 hartid = current_hartid();

	sbi_printf("%s: [ECALL 9 INFO] hart%d: reg info\n", __func__, hartid);
	sbi_printf("%s: hart%d: scause=0x%" PRILX " stval=0x%" PRILX "\n",
		   __func__, hartid, trap->cause, trap->tval);
	sbi_printf("%s: hart%d: sepc=0x%" PRILX " inst=0x%" PRILX "\n",
		   __func__, hartid, trap->epc, trap->tinst);

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

static int sbi_load_hart_mask_unpriv(ulong *pmask, ulong *hmask,
				     struct sbi_trap_info *uptrap)
{
	ulong mask = 0;

	if (pmask) {
		mask = sbi_load_ulong(pmask, uptrap);
		if (uptrap->cause)
			return SBI_ETRAP;
	} else {
		sbi_hsm_hart_interruptible_mask(sbi_domain_thishart_ptr(),
						0, &mask);
	}
	*hmask = mask;

	return 0;
}

/*
static unsigned long long table[5] = {0,0,0,0,0};
static int extend = 0;

static int sbi_nksatp(unsigned long long epc, unsigned long long target_satp, unsigned long long sp){
 	sbi_printf("[SBI_nk] EPC found: %llx, target satp: %llx \n", epc, target_satp);
 	if((epc > 0x80200000 && epc < 0x80800000) || epc>0xffffffffffffd000){
 		csr_write(CSR_SATP, target_satp);
 		if(extend > 0){
 			extend = extend-1;
 			sbi_printf("[SBI_nk] current sp: %llx.\n",sp);
	
 			sbi_printf("[SBI_nk] modify in %llx SATP: %llx.\n",epc, target_satp);
		}
 		for(int a = 0;a<5;a++){
 			if(table[a]==0){
 				table[a] = target_satp;
 				extend = 10;
 				sbi_printf("[SBI_nk] current sp: %llx.\n",sp);
		
 				sbi_printf("[SBI_nk] modify in %llx SATP: %llx.\n",epc, target_satp);
 				break;
 			}else if(table[a]==target_satp){
 				break;
 			}
 		}
 		__asm__ __volatile__("sfence.vma");
 		return 0;
 	}else{
 		sbi_printf("[SBI_nk] Permission denied for modifying SATP for %llx.\n", epc);
	
 		return -1;
 	}
	
 }
 */
static int sbi_ecall_legacy_handler(unsigned long extid, unsigned long funcid,
				    const struct sbi_trap_regs *regs,
				    unsigned long *out_val,
				    struct sbi_trap_info *out_trap)
{
	int ret = 0;
	struct sbi_tlb_info tlb_info;
	u32 source_hart = current_hartid();
	ulong hmask = 0;
	
	switch (extid) {
	case SBI_EXT_0_1_SET_TIMER:
#if __riscv_xlen == 32
		sbi_timer_event_start((((u64)regs->a1 << 32) | (u64)regs->a0));
#else
		sbi_timer_event_start((unsigned long long)regs->a0);
#endif
		break;
	case SBI_EXT_0_1_CONSOLE_PUTCHAR:
		sbi_putc(regs->a0);
		break;
	case SBI_EXT_0_1_CONSOLE_GETCHAR:
		ret = sbi_getc();
		break;
	case SBI_EXT_0_1_CLEAR_IPI:
		sbi_ipi_clear_smode();
		break;
	case SBI_EXT_0_1_SEND_IPI:
		ret = sbi_load_hart_mask_unpriv((ulong *)regs->a0,
						&hmask, out_trap);
		if (ret != SBI_ETRAP)
			ret = sbi_ipi_send_smode(hmask, 0);
		break;
	case SBI_EXT_0_1_REMOTE_FENCE_I:
		ret = sbi_load_hart_mask_unpriv((ulong *)regs->a0,
						&hmask, out_trap);
		if (ret != SBI_ETRAP) {
			SBI_TLB_INFO_INIT(&tlb_info, 0, 0, 0, 0,
					  sbi_tlb_local_fence_i,
					  source_hart);
			ret = sbi_tlb_request(hmask, 0, &tlb_info);
		}
		break;
	case SBI_EXT_0_1_REMOTE_SFENCE_VMA:
		ret = sbi_load_hart_mask_unpriv((ulong *)regs->a0,
						&hmask, out_trap);
		if (ret != SBI_ETRAP) {
			SBI_TLB_INFO_INIT(&tlb_info, regs->a1, regs->a2, 0, 0,
					  sbi_tlb_local_sfence_vma,
					  source_hart);
			ret = sbi_tlb_request(hmask, 0, &tlb_info);
		}
		break;
	case SBI_EXT_0_1_REMOTE_SFENCE_VMA_ASID:
		ret = sbi_load_hart_mask_unpriv((ulong *)regs->a0,
						&hmask, out_trap);
		if (ret != SBI_ETRAP) {
			SBI_TLB_INFO_INIT(&tlb_info, regs->a1,
					  regs->a2, regs->a3, 0,
					  sbi_tlb_local_sfence_vma_asid,
					  source_hart);
			ret = sbi_tlb_request(hmask, 0, &tlb_info);
		}
		break;
	case SBI_EXT_0_1_SHUTDOWN:
		sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN,
				 SBI_SRST_RESET_REASON_NONE);
		break;

	case SBI_EXT_0_1_NKSATP:
		if(0){
			sbi_trap_info(regs,out_trap);
		}
		//if(sbi_nksatp(regs->mepc, regs->a0, regs->sp)){
		//	sbi_trap_info(regs,out_trap);
		//}
		break;

	default:
		ret = SBI_ENOTSUPP;
	}

	return ret;
}

struct sbi_ecall_extension ecall_legacy;

static int sbi_ecall_legacy_register_extensions(void)
{
	return sbi_ecall_register_extension(&ecall_legacy);
}

struct sbi_ecall_extension ecall_legacy = {
	.extid_start		= SBI_EXT_0_1_SET_TIMER,
	.extid_end		= SBI_EXT_0_1_NKSATP,
	.register_extensions	= sbi_ecall_legacy_register_extensions,
	.handle			= sbi_ecall_legacy_handler,
};
