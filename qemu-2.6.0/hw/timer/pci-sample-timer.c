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
#include "qemu/timer.h"
#include "exec/address-spaces.h"
#include "qemu/error-report.h"
#include "hw/hw.h"
#include "hw/pci/pci.h"
#include "sysemu/sysemu.h"

#define TYPE_PCI_SAMPLE_TIMER "pci-sample-timer"

typedef struct PCIPstDevState {
	PCIDevice parent_obj;
	MemoryRegion mmio;

	QEMUTimer *irq_timer;
	int timer_enabled;
} PCIPstDevState;

#define PCI_PST_DEV(obj) \
	OBJECT_CHECK(PCIPstDevState, (obj), TYPE_PCI_SAMPLE_TIMER)

static void pst_irq_timer(void *opaque)
{
	PCIPstDevState *d = opaque;
	PCIDevice *pci_dev = PCI_DEVICE(d);
	pci_irq_assert(pci_dev);
	if (d->timer_enabled == 1)
		timer_mod(d->irq_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL)+NANOSECONDS_PER_SECOND);
}

static uint64_t
pci_pstdev_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
	PCIPstDevState *d = opaque;

	printf("read pst timer: %d\n", d->timer_enabled);

	return d->timer_enabled;
}

static void
pci_pstdev_mmio_write(void *opaque, hwaddr addr, uint64_t val,
						unsigned size)
{
	PCIPstDevState *d = opaque;
	PCIDevice *pci_dev = PCI_DEVICE(d);

	switch(addr) {
	case 0:
		printf("write pst timer: %d-->%ld\n", d->timer_enabled, val);
		d->timer_enabled = val;

		if (d->timer_enabled == 1) {
			if (d->irq_timer == NULL)
				d->irq_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, pst_irq_timer, d);
			timer_mod(d->irq_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL)+NANOSECONDS_PER_SECOND);
		}
		break;
	case 0x8:
		pci_irq_deassert(pci_dev);
		break;
	default:
		printf("write pst timer error: 0x%lx\n", addr);
	}

	return;
}

static const MemoryRegionOps pci_pstdev_mmio_ops = {
	.read = pci_pstdev_mmio_read,
	.write = pci_pstdev_mmio_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
	.impl = {
		.min_access_size = 1,
		.max_access_size = 1,
	},
};

static int pci_pstdev_init(PCIDevice *pci_dev)
{
	PCIPstDevState *d = PCI_PST_DEV(pci_dev);
	uint8_t *pci_conf;

	pci_conf = pci_dev->config;

	pci_conf[PCI_INTERRUPT_PIN] = 1;

	memory_region_init_io(&d->mmio, OBJECT(d), &pci_pstdev_mmio_ops, d,
							"pci-pstdev-mmio", 128);
	pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &d->mmio);

	d->timer_enabled = 0;

	printf("load pst timer\n");

	return 0;
}

static void
pci_pstdev_uninit(PCIDevice *pci_dev)
{
	PCIPstDevState *d = PCI_PST_DEV(pci_dev);
	timer_del(d->irq_timer);
	timer_free(d->irq_timer);
	printf("unload pst timer\n");
}

static void qdev_pci_pstdev_reset(DeviceState *dev)
{
	printf("reset pst timer\n");
}

static void pci_pstdev_class_init(ObjectClass *kclass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(kclass);
	PCIDeviceClass *k = PCI_DEVICE_CLASS(kclass);

	k->init = pci_pstdev_init;
	k->exit = pci_pstdev_uninit;
	k->vendor_id = 0x1234;
	k->device_id = 0x0086;
	k->revision = 0x00;
	k->class_id = PCI_CLASS_OTHERS;
	dc->desc = "PCI-based Sample Timer";
	set_bit(DEVICE_CATEGORY_MISC, dc->categories);
	dc->reset = qdev_pci_pstdev_reset;
}

static const TypeInfo pci_pst_info = {
	.name			= TYPE_PCI_SAMPLE_TIMER,
	.parent			= TYPE_PCI_DEVICE,
	.instance_size	= sizeof(PCIPstDevState),
	.class_init		= pci_pstdev_class_init,
};

static void pci_sample_timer_register_types(void)
{
	type_register_static(&pci_pst_info);
}

type_init(pci_sample_timer_register_types)
