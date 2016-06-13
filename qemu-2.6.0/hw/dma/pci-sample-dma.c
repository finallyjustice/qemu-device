/** File:		main.c
 ** Author:		Dongli Zhang
 ** Contact:	dongli.zhang0129@gmail.com
 **
 ** Copyright (C) Dongli Zhang 2013
 **
 ** This program is free software;  you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY;  without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 ** the GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program;  if not, write to the Free Software 
 ** Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "exec/address-spaces.h"
#include "qemu/error-report.h"
#include "hw/hw.h"
#include "hw/pci/pci.h"
#include "sysemu/sysemu.h"

#define TYPE_PCI_SAMPLE_DMA "pci-sample-dma"

typedef struct PCIPsdDevState {
	PCIDevice parent_obj;
	MemoryRegion mmio;
} PCIPsdDevState;

#define PCI_PSD_DEV(obj) \
	OBJECT_CHECK(PCIPsdDevState, (obj), TYPE_PCI_SAMPLE_DMA)

static uint64_t
pci_psddev_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
	printf("read addr: 0x%lx, %d\n", addr, size);
	return 0;
}

static void
pci_psddev_mmio_write(void *opaque, hwaddr addr, uint64_t val,
						unsigned size)
{
	printf("write addr: 0x%lx, 0x%lx, %d\n", addr, val, size);
	return;
}

static void
pci_psddev_uninit(PCIDevice *pci_dev)
{
	printf("unload psd dma\n");
}

static void qdev_pci_psddev_reset(DeviceState *dev)
{
	printf("reset psd dma\n");
}

static const MemoryRegionOps pci_psddev_mmio_ops = {
	.read = pci_psddev_mmio_read,
	.write = pci_psddev_mmio_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
	.impl = {
		.min_access_size = 1,
		.max_access_size = 1,
	},
};

static int pci_psddev_init(PCIDevice *pci_dev)
{
	PCIPsdDevState *d = PCI_PSD_DEV(pci_dev);
	uint8_t *pci_conf;

	pci_conf = pci_dev->config;

	pci_conf[PCI_INTERRUPT_PIN] = 1;

	memory_region_init_io(&d->mmio, OBJECT(d), &pci_psddev_mmio_ops, d,
							"pci-psddev-mmio", 128);

	pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &d->mmio);

	printf("load psd dma\n");

	return 0;
}

static void pci_psddev_class_init(ObjectClass *kclass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(kclass);
	PCIDeviceClass *k = PCI_DEVICE_CLASS(kclass);

	k->init = pci_psddev_init;
	k->exit = pci_psddev_uninit;
	k->vendor_id = 0x1235;
	k->device_id = 0x0086;
	k->revision = 0x00;
	k->class_id = PCI_CLASS_OTHERS;
	dc->desc = "PCI-based Sample DMA";
	set_bit(DEVICE_CATEGORY_MISC, dc->categories);
	dc->reset = qdev_pci_psddev_reset;
}

static const TypeInfo pci_psd_info = {
	.name			= TYPE_PCI_SAMPLE_DMA,
	.parent			= TYPE_PCI_DEVICE,
	.instance_size	= sizeof(PCIPsdDevState),
	.class_init		= pci_psddev_class_init,
};

static void pci_sample_dma_register_types(void)
{
	type_register_static(&pci_psd_info);
}

type_init(pci_sample_dma_register_types)
