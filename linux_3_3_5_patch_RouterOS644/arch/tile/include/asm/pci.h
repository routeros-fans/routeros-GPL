/*
 * Copyright 2010 Tilera Corporation. All Rights Reserved.
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *   NON INFRINGEMENT.  See the GNU General Public License for
 *   more details.
 */

#ifndef _ASM_TILE_PCI_H
#define _ASM_TILE_PCI_H

#include <linux/dma-mapping.h>
#include <linux/pci.h>
#include <linux/numa.h>
#include <asm-generic/pci_iomap.h>

#ifndef __tilegx__

/*
 * Structure of a PCI controller (host bridge)
 */
struct pci_controller {
	int index;		/* PCI domain number */
	struct pci_bus *root_bus;

	int first_busno;
	int last_busno;

	int hv_cfg_fd[2];	/* config{0,1} fds for this PCIe controller */
	int hv_mem_fd;		/* fd to Hypervisor for MMIO operations */

	struct pci_ops *ops;

	int irq_base;		/* Base IRQ from the Hypervisor	*/
	int plx_gen1;		/* flag for PLX Gen 1 configuration */

	/* Address ranges that are routed to this controller/bridge. */
	struct resource mem_resources[3];
};

/*
 * This flag tells if the platform is TILEmpower that needs
 * special configuration for the PLX switch chip.
 */
extern int tile_plx_gen1;

static inline void pci_iounmap(struct pci_dev *dev, void __iomem *addr) {}

#define	TILE_NUM_PCIE	2

/*
 * The hypervisor maps the entirety of CPA-space as bus addresses, so
 * bus addresses are physical addresses.  The networking and block
 * device layers use this boolean for bounce buffer decisions.
 */
#define PCI_DMA_BUS_IS_PHYS     1

/* generic pci stuff */
#include <asm-generic/pci.h>

#else

#include <asm/page.h>
#include <gxio/trio.h>

/**
 * We reserve the hugepage-size address range at the top of the 64-bit address
 * space to serve as the PCI window, emulating the BAR0 space of an endpoint
 * device. This window is used by the chip-to-chip applications running on
 * the RC node. The reason for carving out this window is that Mem-Maps that
 * back up this window will not overlap with those that map the real physical
 * memory.
 */
#define PCIE_HOST_BAR0_SIZE		HPAGE_SIZE
#define PCIE_HOST_BAR0_START		HPAGE_MASK

/**
 * The first PAGE_SIZE of the above "BAR" window is mapped to the
 * gxpci_host_regs structure.
 */
#define PCIE_HOST_REGS_SIZE		PAGE_SIZE

/*
 * This is the PCI address where the Mem-Map interrupt regions start.
 * We use the 2nd to the last huge page of the 64-bit address space.
 * The last huge page is used for the rootcomplex "bar", for C2C purpose.
 */
#define	MEM_MAP_INTR_REGIONS_BASE	(HPAGE_MASK - HPAGE_SIZE)

/*
 * Each Mem-Map interrupt region occupies 4KB.
 */
#define	MEM_MAP_INTR_REGION_SIZE	(1 << TRIO_MAP_MEM_LIM__ADDR_SHIFT)

/*
 * Allocate the PCI BAR window right below 4GB.
 */
#define	TILE_PCI_BAR_WINDOW_TOP		(1ULL << 32)

/*
 * Allocate 1GB for the PCI BAR window.
 */
#define	TILE_PCI_BAR_WINDOW_SIZE	(1 << 30)

/*
 * This is the highest bus address targeting the host memory that
 * can be generated by legacy PCI devices with 32-bit or less
 * DMA capability, dictated by the BAR window size and location.
 */
#define	TILE_PCI_MAX_DIRECT_DMA_ADDRESS \
	(TILE_PCI_BAR_WINDOW_TOP - TILE_PCI_BAR_WINDOW_SIZE - 1)

/*
 * We shift the PCI bus range for all the physical memory up by the whole PA
 * range. The corresponding CPA of an incoming PCI request will be the PCI
 * address minus TILE_PCI_MEM_MAP_BASE_OFFSET. This also implies
 * that the 64-bit capable devices will be given DMA addresses as
 * the CPA plus TILE_PCI_MEM_MAP_BASE_OFFSET. To support 32-bit
 * devices, we create a separate map region that handles the low
 * 4GB.
 */
#define	TILE_PCI_MEM_MAP_BASE_OFFSET	(1ULL << CHIP_PA_WIDTH())

/*
 * Start of the PCI memory resource, which starts at the end of the
 * maximum system physical RAM address.
 */
#define	TILE_PCI_MEM_START	(1ULL << CHIP_PA_WIDTH())

/*
 * Structure of a PCI controller (host bridge) on Gx.
 */
struct pci_controller {

	/* Pointer back to the TRIO that this PCIe port is connected to. */
	gxio_trio_context_t *trio;
	int mac;		/* PCIe mac index on the TRIO shim */
	int trio_index;		/* Index of TRIO shim that contains the MAC. */

	int pio_mem_index;	/* PIO region index for memory access */

#ifdef CONFIG_TILE_PCI_IO
	int pio_io_index;	/* PIO region index for I/O space access */
#endif

	/*
	 * Mem-Map regions for all the memory controllers so that Linux can
	 * map all of its physical memory space to the PCI bus.
	 */
	int mem_maps[MAX_NUMNODES];

	int index;		/* PCI domain number */
	struct pci_bus *root_bus;

	/* PCI memory space resource for this controller. */
	struct resource mem_space;
	char mem_space_name[32];

	/* PCI I/O space resource for this controller. */
	struct resource io_space;
	char io_space_name[32];

	uint64_t mem_offset;	/* cpu->bus memory mapping offset. */

	int first_busno;

	struct pci_ops *ops;

	/* Table that maps the INTx numbers to Linux irq numbers. */
	int irq_intx_table[4];
};

extern struct pci_controller pci_controllers[TILEGX_NUM_TRIO * TILEGX_TRIO_PCIES];
extern gxio_trio_context_t trio_contexts[TILEGX_NUM_TRIO];

extern void pci_iounmap(struct pci_dev *dev, void __iomem *);

/*
 * The PCI address space does not equal the physical memory address
 * space (we have an IOMMU). The IDE and SCSI device layers use this
 * boolean for bounce buffer decisions.
 */
#define PCI_DMA_BUS_IS_PHYS     0

static inline void
pcibios_resource_to_bus(struct pci_dev *dev, struct pci_bus_region *region,
			 struct resource *res)
{
	region->start = res->start;
	region->end = res->end;

	if (res->flags & IORESOURCE_MEM) {
		struct pci_controller *controller =
			(struct pci_controller *) dev->sysdata;
		region->start -= controller->mem_offset;
		region->end -= controller->mem_offset;
	}
}

static inline void
pcibios_bus_to_resource(struct pci_dev *dev, struct resource *res,
			struct pci_bus_region *region)
{
	res->start = region->start;
	res->end = region->end;

	if (res->flags & IORESOURCE_MEM) {
		struct pci_controller *controller =
			(struct pci_controller *) dev->sysdata;
		res->start += controller->mem_offset;
		res->end += controller->mem_offset;
	}
}
#endif /* __tilegx__ */

int __init tile_pci_init(void);
int __init pcibios_init(void);

void __devinit pcibios_fixup_bus(struct pci_bus *bus);

#define pci_domain_nr(bus) (((struct pci_controller *)(bus)->sysdata)->index)

/*
 * This decides whether to display the domain number in /proc.
 */
static inline int pci_proc_domain(struct pci_bus *bus)
{
	return 1;
}

/*
 * pcibios_assign_all_busses() tells whether or not the bus numbers
 * should be reassigned, in case the BIOS didn't do it correctly, or
 * in case we don't have a BIOS and we want to let Linux do it.
 */
static inline int pcibios_assign_all_busses(void)
{
	return 1;
}

#define PCIBIOS_MIN_MEM		0
/* Minimum PCI I/O address, starting at the page boundary. */
#define PCIBIOS_MIN_IO		PAGE_SIZE

/* Use any cpu for PCI. */
#define cpumask_of_pcibus(bus) cpu_online_mask

/* implement the pci_ DMA API in terms of the generic device dma_ one */
#include <asm-generic/pci-dma-compat.h>

#endif /* _ASM_TILE_PCI_H */
