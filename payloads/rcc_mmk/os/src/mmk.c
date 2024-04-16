#include "mmk.h"
#include "log.h"

int mmk_call(unsigned long id, unsigned long *args, unsigned int arglen, uint64_t *retval)
{
	unsigned long vec[5] = {0,0,0,0,0};
	for(int a = 0;a<5;a++){
		if(a < arglen){
			vec[a] = args[a];
		}
	}
		
	unsigned long ret = 0;
	unsigned long status = 0;
	asm volatile(
		"fence.i \n\t"
		"mv x15, %2 \n\t"
		"mv x17, %3 \n\t"
		"mv x10, %4 \n\t"
		"mv x11, %5 \n\t"
		"mv x12, %6 \n\t"
		"mv x13, %7 \n\t"
		"mv x14, %8 \n\t"
        "jalr x1, x15, 0 \n\t"
        "mv %0, a0 \n\t"
        "mv %1, a1 \n\t"
        "fence.i \n\t"
                : "=r" (ret), "=r" (status)
                : "r" (-0x1000), "r" (id*8),
                "r" (vec[0]), "r" (vec[1]), "r" (vec[2]), "r" (vec[3]), "r" (vec[4])
                : "x10","x11","x12","x13","x14","x17","x15"
            );
        if(status!=0){
        	warn("nkapi return non-0 status!\n");
        }
        *retval = ret;
        return status;
}

int nkapi_time(unsigned long* time){
	unsigned long params[1] = {0};
	return mmk_call(NKAPI_TIME,params,1,time);
}

int nkapi_current_pt(unsigned long* current_pt){
	unsigned long params[1] = {0};
	return mmk_call(NKAPI_CURRENT_PT,params,1,current_pt);
}

int nkapi_translate(unsigned long pt_handle, VirtPageNum vpn, unsigned char write, PhysPageNum *ppn){
	unsigned long params[3] = {pt_handle, vpn, (unsigned long)write};
	return mmk_call(NKAPI_TRANSLATE, params, 3, ppn);
}
int nkapi_translate_va(unsigned long pt_handle, VirtAddr va, PhysAddr *pa){
    PhysPageNum ppn;
	unsigned long retval = nkapi_translate(pt_handle, va >> 12, 0, &ppn);
	*pa = (ppn << 12) + (va & 0xfff);
	return retval;
}
int nkapi_get_pte(unsigned long pt_handle, VirtPageNum vpn, unsigned long *pte){
	unsigned long params[2] = {pt_handle, vpn};
	return mmk_call(NKAPI_GET_PTE, params, 2, pte);
}
int nkapi_fork_pte(unsigned long pt_handle, unsigned long pt_child,
						VirtPageNum vpn, unsigned char cow, PhysPageNum *ppn){
	unsigned long params[4] = {pt_handle, pt_child, vpn, (unsigned long)cow};
	return mmk_call(NKAPI_FORK_PTE, params, 4, ppn);
}
int nkapi_print_pt(unsigned long pt_handle, unsigned long from, unsigned long to){
	unsigned long abort;
	unsigned long params[3] = {pt_handle, from, to};
	return mmk_call(NKAPI_DEBUG, params, 3, &abort);
}

int nkapi_alloc(unsigned long pt_handle, VirtPageNum vpn, 
	MapType map_type, MapPermission map_perm, PhysPageNum *ppn){
	unsigned long params[5] = {pt_handle, vpn, 1, map_type, map_perm};
	return mmk_call(NKAPI_ALLOC, params, 5, ppn);
}
int nkapi_dealloc(unsigned long pt_handle, VirtPageNum vpn){
	unsigned long abort;
	unsigned long params[2] = {pt_handle, vpn};
	return mmk_call(NKAPI_DEALLOC, params, 2, &abort);
}
int nkapi_pt_init(unsigned long pt_handle, unsigned char regenerate){
	unsigned long abort;
	unsigned long params[2] = {pt_handle, (unsigned long)regenerate};
	return mmk_call(NKAPI_PT_INIT, params, 2, &abort);
}
int nkapi_pt_destroy(unsigned long pt_handle){
	unsigned long abort;
	unsigned long params[1] = {pt_handle};
    return mmk_call(NKAPI_PT_DESTROY,pt_handle,1,&abort);
}

int nkapi_activate(unsigned long pt_handle){
	unsigned long abort;
	unsigned long params[1] = {pt_handle};
	return mmk_call(NKAPI_ACTIVATE, params, 1, &abort);
}
int nkapi_write(unsigned long pt_handle, VirtPageNum vpn, uint8_t *data, unsigned long len, unsigned long offset){
	unsigned long abort;
	unsigned long params[5] = {pt_handle, vpn, (unsigned long) data, len, offset};
	return mmk_call(NKAPI_WRITE, params, 5, &abort);
}
int nkapi_set_permission(unsigned long pt_handle, VirtPageNum vpn, MapPermission map_perm){
	unsigned long abort;
	unsigned long params[3] = {pt_handle, vpn, map_perm};
	return mmk_call(NKAPI_SET_PERM, params, 3, &abort);
}

int nkapi_config_user_delegate_handler(unsigned long entry){
	unsigned long abort;
	unsigned long params[2] = {NKCFG_U_DELEGATE, entry};
	return mmk_call(NKAPI_CONFIG, params, 2, &abort);
}

int nkapi_config_kernel_delegate_handler(unsigned long entry){
	unsigned long abort;
	unsigned long params[2] = {NKCFG_S_DELEGATE, entry};
	return mmk_call(NKAPI_CONFIG, params, 2, &abort);
}
int nkapi_config_signal_handler(unsigned long entry){
	unsigned long abort;
	unsigned long params[2] = {NKCFG_SIGNAL, entry};
	return mmk_call(NKAPI_CONFIG, params, 2, &abort);
}
int nkapi_config_allocator_range(unsigned long begin, unsigned long end){
	unsigned long abort;
	unsigned long params1[2] = {NKCFG_ALLOCATOR_START, begin};
	if( mmk_call(NKAPI_CONFIG, params1, 2, &abort) != 0){
		return 1;
	}
	unsigned long params2[2] = {NKCFG_ALLOCATOR_END, end};
	if( mmk_call(NKAPI_CONFIG, params2, 2, &abort) != 0){
		return 1;
	}
	return 0;
}



