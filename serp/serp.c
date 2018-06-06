#include <linux/ioport.h>
#include <asm/io.h>
#include "serial_reg.h"
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/aio.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <asm/errno.h>
#include <linux/jiffies.h>

MODULE_LICENSE("Dual BSD/GPL");

static dev_t dev;
struct cdev *cdp;

#define PORTA 0x3f8

//-------------------------------------FUNÇAO OPEN--------------------------------

int my_open(struct inode *inodep, struct file *filep){
	filep->private_data=cdp;
	printk(KERN_ALERT "Ficheiro aberto\n");
	nonseekable_open(inodep,filep);
	return 0;
}

//--------------------------------------FUNÇAO RELEASE--------------------------

int my_release(struct inode *inodep, struct file *filep){
	//printk(KERN_ALERT "Invoked\n");
	return 0;
}


//--------------------------------FUNÇAO WRITE----------------------------------------

ssize_t my_write(struct file *filep, const char __user *buff, size_t count, loff_t *offp){
	unsigned long ubytes,i=0;
	char *buffer=(char*)kzalloc(sizeof(char)*(count),GFP_KERNEL); // zalloc em vez de malloc porque nos estava a imprimir com lixo com malloc

	ubytes=copy_from_user(buffer,buff,count); // não me esquecer de adicionar a mensagem de erro caso isto seja diferente de 0, posso deixar para depois, assuntos mais urgentes agora

	while(i<count){
		if(inb(PORTA+UART_LSR) & UART_LSR_THRE){ // se o Transmit Holding Register está vazio ele tem luz verde para mandar 

			outb(buffer[i],PORTA+UART_TX); // adicionado no 6.2, agora levou a alteração para funcionar para as strings
			//printk(KERN_ALERT"%s\n",buffer);
			i++;
		}
		else {
			schedule();
		}
	}		
	kfree(buffer);
	return count;	
}


//--------------------------------FUNÇAO READ-----------------------------------------

ssize_t my_read(struct file *filep, char __user *buff, size_t count, loff_t *offp){
	unsigned long ubits;   
	unsigned char lsr=0;
	char *buffer=0;
	int i=0;
	int time=0;
	buffer=(char*)kzalloc(sizeof(char)*(count),GFP_KERNEL);
	time=jiffies;
	while(i<count){
		lsr= inb(PORTA+UART_LSR);
		if(lsr & UART_LSR_OE) {
			printk(KERN_ALERT "Error!!\n");
			kfree(buffer);
			return -EIO;
	}
		if(lsr & UART_LSR_DR){ //verificar se há algum caracter disponível para ser lido, adicionar o jifffies para ele esperar um sibo 
			buffer[i]=inb(PORTA+UART_RX);
			//printk(KERN_ALERT"%s\n",buffer);
			i++;
		}
		else if(jiffies-time > (7*HZ)) break;
	}	
		ubits=copy_to_user(buff,buffer,i);
		if(ubits!=0){
			printk(KERN_ALERT "Erro ao ler!\n");
			kfree(buffer);
			return -EAGAIN;
		}
	
	kfree(buffer);
	return i;
}


//-----------------------------------STRUCT-----------------------

const struct file_operations fops={
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = my_read,
	.write = my_write,
	.open = my_open,
	.release = my_release,
};

 
//------------------------------FUNÇAO INIT-----------------------------

static int serp_init(void){ 
	unsigned char lcr=0;
	int serp_major=0;
	int bps=0;
	if(request_region(PORTA,1,"serp")==NULL){
		printk(KERN_ALERT"Failed1!!!\n");
		return -1;
	}
	lcr= UART_LCR_WLEN8 | UART_LCR_STOP | UART_LCR_PARITY | UART_LCR_EPAR | UART_LCR_DLAB; //1->8 bits 2->2 bits stop 3->enable parity 4-> set parity even 5->divisor latch acess bit
	bps=UART_DIV_1200;
	//First, the 16-bit DL register is exported by the UART as two 8-bit registers, the DLL and the DLM, which contain the LSB and the MSB of the DL register, respectively. Second, the DLL and the DLM have the same bus address as other registers. Thus, to access the DLL and the DLM a programmer must first set the DL Access Bit (DLAB) of the LCR, then actually access the DLL/DLM register, and finally reset the DLAB so that the other registers can be accessed.
	outb(lcr,PORTA+UART_LCR); //access LCR
	outb(bps,PORTA+UART_DLL); // access LSB
	outb(bps>>8,PORTA+UART_DLM); //access MSB
	lcr &= ~UART_LCR_DLAB; //reset
	outb(lcr,PORTA+UART_LCR); // mandar a DLAB com o valor depois do reset, i.e 0
	//podemos ignorar por agora a parte "Waiting for the Transmitter to be ready" devido aos 10ms
	outb(0,PORTA+UART_IER); // disable interrupt
	alloc_chrdev_region(&dev,0,1,"serp");
	serp_major=MAJOR(dev);
	cdp=cdev_alloc();
	cdp->ops = &fops;
	cdp->owner = THIS_MODULE;
	printk(KERN_ALERT "Add: %d\n", cdev_add(cdp,dev,1));
	//outb('N',PORTA+UART_TX); //remover quando fizermos o write
	printk(KERN_ALERT " Major device number: %d\n", serp_major);
	return 0;
}

//-----------------------------FUNÇAO EXIT--------------------------------

static void serp_exit(void)
{	
	release_region(PORTA,1);
	unregister_chrdev_region(dev,1);
	printk(KERN_ALERT "I'm free!\n");
		
}

module_init(serp_init);
module_exit(serp_exit);
	
		
