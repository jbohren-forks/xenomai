/**
 * @file
 * Hardware driver for 8255 chip
 * @note Copyright (C) 1999 David A. Schleef <ds@schleef.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */


#include <linux/module.h>
#include <linux/ioport.h>
#include <comedi/comedi_driver.h>

#include "8255.h"

#define CALLBACK_ARG		(((subd_8255_t *)subd->priv)->cb_arg)
#define CALLBACK_FUNC		(((subd_8255_t *)subd->priv)->cb_func)

/* Channels descriptor */
static comedi_chdesc_t chandesc_8255 = {
	.mode = COMEDI_CHAN_GLOBAL_CHANDESC,
	.length = 24,
	.chans = { 
		{COMEDI_CHAN_AREF_GROUND, sizeof(sampl_t)},
	},
};

/* Command options mask */
static comedi_cmd_t cmd_mask_8255 = {
  .idx_subd = 0,
  .start_src = TRIG_NOW,
  .scan_begin_src = TRIG_EXT,
  .convert_src = TRIG_FOLLOW,
  .scan_end_src = TRIG_COUNT,
  .stop_src = TRIG_NONE,
};

void subdev_8255_interrupt(comedi_subd_t *subd)
{
	sampl_t d;

	/* Considering the current Comedi API, using asynchronous
	   buffer is only possible on the main read/write subdevice.
	   Then, this function needs only one argument: the device */

	/* Retrieve the sample... */
	d = CALLBACK_FUNC(0, _8255_DATA, 0, CALLBACK_ARG);
	d |= (CALLBACK_FUNC(0, _8255_DATA + 1, 0, CALLBACK_ARG) << 8);

	/* ...and send it */
	comedi_buf_put(subd, &d, sizeof(sampl_t));

	comedi_buf_evt(subd, 0);
}

static int subdev_8255_cb(int dir, int port, int data, unsigned long arg)
{
	unsigned long iobase = arg;

	if (dir) {
		outb(data, iobase + port);
		return 0;
	} else {
		return inb(iobase + port);
	}
}

static void do_config(comedi_subd_t *subd)
{
	int config;
	subd_8255_t *subd_8255 = (subd_8255_t *)subd->priv;

	config = CR_CW;
	/* 1 in io_bits indicates output, 1 in config indicates input */
	if (!(subd_8255->io_bits & 0x0000ff))
		config |= CR_A_IO;
	if (!(subd_8255->io_bits & 0x00ff00))
		config |= CR_B_IO;
	if (!(subd_8255->io_bits & 0x0f0000))
		config |= CR_C_LO_IO;
	if (!(subd_8255->io_bits & 0xf00000))
		config |= CR_C_HI_IO;
	CALLBACK_FUNC(1, _8255_CR, config, CALLBACK_ARG);
}

int subd_8255_cmd(comedi_subd_t *subd, comedi_cmd_t *cmd)
{
	/* FIXME */  
	return 0;
}

int subd_8255_cmdtest(comedi_subd_t *subd, comedi_cmd_t *cmd)
{
	if (cmd->start_arg != 0) {
		cmd->start_arg = 0;
		return -EINVAL;
	}
	if (cmd->scan_begin_arg != 0) {
		cmd->scan_begin_arg = 0;
		return -EINVAL;
	}
	if (cmd->convert_arg != 0) {
		cmd->convert_arg = 0;
		return -EINVAL;
	}
	if (cmd->scan_end_arg != 1) {
		cmd->scan_end_arg = 1;
		return -EINVAL;
	}
	if (cmd->stop_arg != 0) {
		cmd->stop_arg = 0;
		return -EINVAL;
	}

	return 0;
}

int subd_8255_cancel(comedi_subd_t *subd)
{	
	/* FIXME */
	return 0;
}

int subd_8255_insn_bits(comedi_subd_t *subd, comedi_kinsn_t *insn)
{
	subd_8255_t *subd_8255 = (subd_8255_t *)subd->priv;

	if (insn->data[0]) {

		subd_8255->status &= ~insn->data[0];
		subd_8255->status |= (insn->data[0] & insn->data[1]);

		if (insn->data[0] & 0xff)
			CALLBACK_FUNC(1, _8255_DATA, 
				      subd_8255->status & 0xff, CALLBACK_ARG);
		if (insn->data[0] & 0xff00)
			CALLBACK_FUNC(1, _8255_DATA + 1, 
				      (subd_8255->status >> 8) & 0xff, 
				      CALLBACK_ARG);
		if (insn->data[0] & 0xff0000)
			CALLBACK_FUNC(1, _8255_DATA + 2,
				      (subd_8255->status >> 16) & 0xff, 
				      CALLBACK_ARG);
	}

	insn->data[1] = CALLBACK_FUNC(0, _8255_DATA, 0, CALLBACK_ARG);
	insn->data[1] |= (CALLBACK_FUNC(0, _8255_DATA + 1, 0, CALLBACK_ARG) << 8);
	insn->data[1] |= (CALLBACK_FUNC(0, _8255_DATA + 2, 0, CALLBACK_ARG) << 16);

	return 0;
}

int subd_8255_insn_config(comedi_subd_t *subd, comedi_kinsn_t *insn)
{
	unsigned int mask;
	unsigned int bits;
	subd_8255_t *subd_8255 = (subd_8255_t *)subd->priv;

	mask = 1 << CR_CHAN(insn->chan_desc);

	if (mask & 0x0000ff) {
		bits = 0x0000ff;
	} else if (mask & 0x00ff00) {
		bits = 0x00ff00;
	} else if (mask & 0x0f0000) {
		bits = 0x0f0000;
	} else {
		bits = 0xf00000;
	}

	switch (insn->data[0]) {
	case INSN_CONFIG_DIO_INPUT:
		subd_8255->io_bits &= ~bits;
		break;
	case INSN_CONFIG_DIO_OUTPUT:
		subd_8255->io_bits |= bits;
		break;
	case INSN_CONFIG_DIO_QUERY:
		insn->data[1] = (subd_8255->io_bits & bits) ? 
			COMEDI_OUTPUT : COMEDI_INPUT;
		return 0;
		break;
	default:
		return -EINVAL;
	}

	do_config(subd);

	return 0;
}

void subdev_8255_init(comedi_subd_t *subd)
{
	subd_8255_t *subd_8255 = (subd_8255_t *)subd->priv;
	/* Initializes the subdevice structure */
	memset(&subd, 0, sizeof(comedi_subd_t));

	/* Subdevice filling part */

	subd->flags = COMEDI_SUBD_DIO;
	subd->flags |= COMEDI_SUBD_CMD;
	subd->chan_desc = &chandesc_8255;
	subd->insn_bits = subd_8255_insn_bits;
	subd->insn_config = subd_8255_insn_config;
      
	if(subd_8255->have_irq) {
		subd->cmd_mask = &cmd_mask_8255;
		subd->do_cmdtest = subd_8255_cmdtest;
		subd->do_cmd = subd_8255_cmd;
		subd->cancel = subd_8255_cancel;
	}

	/* 8255 setting part */

	if(CALLBACK_FUNC == NULL)
		CALLBACK_FUNC = subdev_8255_cb;

	do_config(subd);
}

/*

   Start of the 8255 standalone device

 */

int dev_8255_attach(comedi_dev_t *dev, comedi_lnkdesc_t *arg)
{
	unsigned long *addrs; 	
	int i, err = 0;

	if(arg->opts == NULL || arg->opts_size == 0) {
		comedi_err(dev, 
			   "dev_8255_attach: unable to detect any 8255 chip, "
			   "chips addresses must be passed as attach arguments\n");
		return -EINVAL;
	}

	addrs = (unsigned long*) arg->opts;

	for(i = 0; i < (arg->opts_size / sizeof(unsigned long)); i++) {
		comedi_subd_t * subd;
		subd_8255_t *subd_8255;

		subd = comedi_alloc_subd(sizeof(subd_8255_t), NULL);
		if(subd == NULL) {
			comedi_err(dev, 
				   "dev_8255_attach: "
				   "unable to allocate subdevice\n");
			/* There is no need to free previously
			   allocated structure(s), the comedi layer will do
			   it for us */
			err = -ENOMEM;
			goto out_attach;
		}		

		memset(&subd, 0, sizeof(comedi_subd_t));
		memset(subd->priv, 0, sizeof(subd_8255_t));

		subd_8255 = (subd_8255_t *)subd->priv;		
		
		if(request_region(addrs[i], _8255_SIZE, "Comedi 8255") == 0) {	       
			subd->flags = COMEDI_SUBD_UNUSED;
			comedi_warn(dev, 
				    "dev_8255_attach: "
				    "I/O port conflict at 0x%lx\n", addrs[i]);
		}
		else {
			subd_8255->cb_arg = addrs[i];
			subdev_8255_init(subd);
		}

		err = comedi_add_subd(dev, subd);
		if(err < 0) {
			comedi_err(dev, 
				   "dev_8255_attach: "
				   "comedi_add_subd() failed (err=%d)\n", err);
			goto out_attach;
		}
	}

out_attach:
	return err;
}

int dev_8255_detach(comedi_dev_t *dev)
{
	comedi_subd_t *subd;
	int i = 0;

	while((subd = comedi_get_subd(dev, i++)) != NULL) {
		subd_8255_t *subd_8255 = (subd_8255_t *) subd->priv;
		if(subd_8255 != NULL && subd_8255->cb_arg != 0)
			release_region(subd_8255->cb_arg, _8255_SIZE);
	}

	return 0;
}

static comedi_drv_t drv_8255 = {
	.owner = THIS_MODULE,
	.board_name = "8255",
	.attach = dev_8255_attach,
	.detach = dev_8255_detach,	
	.privdata_size = 0,
};

static int __init drv_8255_init(void)
{
	return comedi_register_drv(&drv_8255);
}

static void __exit drv_8255_cleanup(void)
{
	comedi_unregister_drv(&drv_8255);
}

module_init(drv_8255_init);
module_exit(drv_8255_cleanup);