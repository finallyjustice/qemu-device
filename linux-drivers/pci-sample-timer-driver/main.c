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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/interrupt.h>

struct pci_sample_timer_data {
	void __iomem *mmio;
	int irq;
	unsigned int count;

	unsigned long mmio_base;
	unsigned long mmio_flags;
	unsigned long mmio_length;
};

static struct pci_sample_timer_data *pst_data;

static const struct pci_device_id pcidevtbl[] = {
	{ 0x1234, 0x0086, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	{ } /* terminated */
};

irqreturn_t pci_sample_timer_handler(int irq, void *dev_id)
{
	printk(KERN_ALERT "PST: pci_sample_timer_handler %u\n", pst_data->count);
	pst_data->count++;
	iowrite8(1, pst_data->mmio+8);

	return IRQ_HANDLED;
}

static int
pci_sample_timer_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{	
	int timer_enabled;
	int err;

	pst_data = (struct pci_sample_timer_data *)
		kmalloc(sizeof(struct pci_sample_timer_data), GFP_KERNEL);

	err = pci_enable_device(pdev);
	if (err) {
		printk(KERN_ALERT "PST: cannot enable pci\n");
	}

	pst_data->mmio_base = pci_resource_start(pdev, 0);
	pst_data->mmio_flags = pci_resource_flags(pdev, 0);
	pst_data->mmio_length = pci_resource_len(pdev, 0);

	pst_data->mmio = pci_iomap(pdev, 0, 0);
	if (!pst_data->mmio) {
		printk(KERN_ALERT "PST: failed to iomap\n");
		return -ENODEV;
	}

	printk(KERN_ALERT "PST: mmio=0x%016lx," 
			"mmio_base=0x%016lx, mmio_flags=0x%016lx, mmio_length=%ld\n", 
			(unsigned long)pst_data->mmio,
			pst_data->mmio_base,
			pst_data->mmio_flags,
			pst_data->mmio_length);

	pst_data->irq = pdev->irq;
	printk(KERN_ALERT "PST: irq is %d\n", pst_data->irq);
	err = request_irq(pst_data->irq, pci_sample_timer_handler, 0, "PCI Sample Timer", pdev);

	timer_enabled = ioread8(pst_data->mmio);
	if (timer_enabled == 0)
		printk(KERN_ALERT "PST: timer is disabled initially\n");
	else
		printk(KERN_ALERT "PST: timer is enabled initially\n");

	pst_data->count = 0;

	iowrite8(1, pst_data->mmio);

	return 0;
}

static void pci_sample_timer_remove(struct pci_dev *pdev)
{
	free_irq(pdev->irq, pdev);
	iounmap(pst_data->mmio);
	pci_release_region(pdev, 0);
	pci_disable_device(pdev);
	kfree(pst_data);

	printk(KERN_ALERT "PST: remove PCI Sample Timer\n");
}

static struct pci_driver pci_sample_timer_driver = {
	.name = "PCI Sample Timer",
	.id_table = pcidevtbl,
	.probe = pci_sample_timer_probe,
	.remove = pci_sample_timer_remove,
};

static int __init pci_sample_timer_init(void)
{
	int rc;

	rc = pci_register_driver(&pci_sample_timer_driver);
	if (rc) {
		printk(KERN_ALERT "PST: failed to register driver\n");
		return rc;
	}

	printk(KERN_ALERT "PST: successfully register driver\n");
	return 0;
}

static void __exit pci_sample_timer_exit(void)
{
	pci_unregister_driver(&pci_sample_timer_driver);
	printk(KERN_ALERT "exit the module\n");
}

module_init(pci_sample_timer_init);
module_exit(pci_sample_timer_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PCI-based Sample Timer");
MODULE_AUTHOR("Dongli Zhang");
