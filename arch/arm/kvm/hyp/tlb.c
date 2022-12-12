/*
 * Original code:
 * Copyright (C) 2012 - Virtual Open Systems and Columbia University
 * Author: Christoffer Dall <c.dall@virtualopensystems.com>
 *
 * Mostly rewritten in C by Marc Zyngier <marc.zyngier@arm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <asm/kvm_hyp.h>

/**
 * Flush per-VMID TLBs
 *
 * __kvm_tlb_flush_vmid(struct kvm *kvm);
 *
 * We rely on the hardware to broadcast the TLB invalidation to all CPUs
 * inside the inner-shareable domain (which is the case for all v7
 * implementations).  If we come across a non-IS SMP implementation, we'll
 * have to use an IPI based mechanism. Until then, we stick to the simple
 * hardware assisted version.
 *
 * As v7 does not support flushing per IPA, just nuke the whole TLB
 * instead, ignoring the ipa value.
 */
static void __hyp_text __tlb_flush_vmid(struct kvm *kvm)
{
	dsb(ishst);

	/* Switch to requested VMID */
	kvm = kern_hyp_va(kvm);
	write_sysreg(kvm->arch.vttbr, VTTBR);
	isb();

	write_sysreg(0, TLBIALLIS);
	dsb(ish);
	isb();

	write_sysreg(0, VTTBR);
}

__alias(__tlb_flush_vmid) void __kvm_tlb_flush_vmid(struct kvm *kvm);

static void __hyp_text __tlb_flush_vmid_ipa(struct kvm *kvm, phys_addr_t ipa)
{
	__tlb_flush_vmid(kvm);
}

__alias(__tlb_flush_vmid_ipa) void __kvm_tlb_flush_vmid_ipa(struct kvm *kvm,
							    phys_addr_t ipa);

static void __hyp_text __tlb_flush_vm_context(void)
{
	write_sysreg(0, TLBIALLNSNHIS);
	write_sysreg(0, ICIALLUIS);
	dsb(ish);
}

__alias(__tlb_flush_vm_context) void __kvm_flush_vm_context(void);
