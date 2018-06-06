//----------------------------------Hugo Rodrigues------------Beatriz Garrido--------------------------------------



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
#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/wait.h>

MODULE_LICENSE("Dual BSD/GPL");

static dev_t dev;
struct cdev *cdp;
typedef struct {
	struct cdev cdp;
	struct kfifo *rxfifo;
	struct kfifo *txfifo;
	wait_queue_head_t txwq; 
	wait_queue_head_t rxwq; 
	spinlock_t lock_tx; 
	spinlock_t lock_rx;
}sdt;

#define PORTA 0x3f8
sdt *seri=NULL; // Inicialização da estrutura 

//------------------------------------FUNÇÃO PARA INTERRUPT----------------------
irqreturn_t int_handler(int irq, void *dev_id){ //irq-> IRQ that generated the interrupt, dev_id-> passed in the last argument of request_irq
	//they cannot transfer data from/to user level
	// they must hold onto the CPU until they are done, because they are not "scheduleable" entities
	sdt *serih=(sdt*)dev_id;
	unsigned char iir=inb(PORTA+UART_IIR);//O UART_IIR é o que verifica se há interrupt
	unsigned char rdata=0, tdata=0;
	printk(KERN_ALERT"ENTRO NOS INT\n");
	if( iir & UART_IIR_NO_INT){ //UART_IIR_NO_INT verifica se há interrupt pendentes
		printk(KERN_ALERT " Não há interrupts pendentes\n");
		return 0;
	}	
	//Fazer para o caso de receber
	if(UART_IIR_RDI & iir){ //RDI é para o caso de receber dados
		printk(KERN_ALERT"NO INTERRUPT DO RECEBER\n");
		rdata=inb(PORTA+UART_RX);
		kfifo_put(serih->rxfifo,&rdata,sizeof(rdata)); // põe os dados no fifo 
		wake_up_interruptible(&(serih->rxwq));
	}
	//Agora para transmitir
	if((iir & UART_IIR_THRI)){ //UART_IIR_THRI indica se o registo para transmissão está vazio
		if(kfifo_get(serih->txfifo, &tdata, sizeof(tdata))){ //se tiver algo para escrever
			outb(tdata, PORTA+UART_TX);
			wake_up_interruptible(&(serih->txwq));
		} 
	}
	return IRQ_HANDLED;
	
}
//-------------------------------------FUNÇAO OPEN--------------------------------

int my_open(struct inode *inodep, struct file *filep){
	//filep->private_data=cdp;
	filep->private_data= container_of(inodep->i_cdev,sdt,cdp); //cast a member of a structure out to the containing structure 
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
	unsigned long j=0;
	char *buffer=(char*)kzalloc(sizeof(char)*(count),GFP_KERNEL); // zalloc em vez de malloc porque nos estava a imprimir com lixo com malloc
	char c;
	long int retcount=count, enviados;
	sdt *seriw; 
	seriw=filep->private_data; // improvement -> deixar de usar tantas variáveis globais!!!!

	if(copy_from_user(buffer, buff, strlen(buff))) {
		kfree(buffer);
		printk(KERN_ALERT"MERDOU NO COPY\n");
		return -EAGAIN;
	}
	while(retcount>0){ //enquanto ainda existirem caracteres para ler 
		if(filep->f_flags & O_NONBLOCK){
			kfree(buffer);
			printk(KERN_ALERT"DEU AQUELE O_NONBLOCK GOSTOSO\n");
			return -EAGAIN;
		}
		if(!(wait_event_interruptible_timeout(seriw->txwq,kfifo_len(seriw->txfifo)<4096, 700)))
			return -1;
		enviados=kfifo_put(seriw->txfifo,buffer+j,retcount);
		j=j+enviados;
		retcount=retcount-enviados;
		if(inb(PORTA+UART_LSR) & UART_LSR_THRE){
			kfifo_get(seriw->txfifo,&c,sizeof(c));
			outb(c,PORTA+UART_TX);
		}
		if(!retcount)
			printk(KERN_ALERT"MANDOU TUDO\n");
	}
	kfree(buffer);
	return 0;
}

//--------------------------------FUNÇAO READ-----------------------------------------

ssize_t my_read(struct file *filep, char __user *buff, size_t count, loff_t *offp){   
	unsigned char lsr=0;
	char *buffer=0;
	int i=0,getk=0;
	sdt *serir;
	serir=filep->private_data;
	buffer=(char*)kzalloc(sizeof(char)*count,GFP_KERNEL);
	while((i=kfifo_len(serir->rxfifo))<count){ //verificar o tamanho do kfifo
		if(filep->f_flags& O_NONBLOCK){ //o O_NDELAY também dava
			printk(KERN_ALERT"MANDOU AQUELE O_NONBLOCK GOSTOSO\n");
			kfree(buffer);
			return -EAGAIN;
		}
		if(!wait_event_interruptible_timeout(serir->rxwq,kfifo_len(serir->rxfifo)>i,700))
		break;
		if(wait_event_interruptible_timeout(serir->rxwq,kfifo_len(serir->rxfifo)>i,700)== - ERESTARTSYS){
			kfree(buffer);
			printk(KERN_ALERT " DEU CRTL C\n");
			return -1;
			
		}		
	}

	i=kfifo_len(serir->rxfifo);
	getk=kfifo_get(serir->rxfifo, buffer,i);
	if(getk!=i){ //verificar se leu o número correcto
		printk(KERN_ALERT "GET\n");
		kfree(buffer);
		return -1;
	}
	lsr=inb(PORTA+UART_LSR);
	//printk(KERN_ALERT"Buffer: %s\n",buffer); isto foi para checkar quando isto não estava a funcionar
	if(copy_to_user(buff,buffer,i)){
		printk(KERN_ALERT "COPY\n");
		kfree(buffer);
		return -EAGAIN;
		}
	kfree(buffer);
	return i;

}

//-----------------------------------STRUCT FOPS-----------------------

const struct file_operations fops={
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = my_read,
	.write = my_write,
	.open = my_open,
	.release = my_release,
};

 
//------------------------------FUNÇAO INIT-----------------------------

static int seri_init(void){ 
	unsigned char lcr=0;
	int seri_major=0;
	int bps=0;
	int itr=0; //informo que o meu código não funcionou durante 5 horas porque eu me esqueci disto :)
	seri=(sdt*)kzalloc(sizeof(sdt),GFP_KERNEL); //allocate memory in kernel
	if(request_region(PORTA,1,"seri")==NULL){
		printk(KERN_ALERT"Failed1!!!\n");
		return -1;
	}
	lcr=inb(PORTA+UART_LCR);
	lcr |= UART_LCR_WLEN8 | UART_LCR_STOP | UART_LCR_PARITY | UART_LCR_EPAR | UART_LCR_DLAB; //1->8 bits 2->2 bits stop 3->enable parity 4-> set parity even 5->divisor latch acess bit
	bps=UART_DIV_1200;
	//First, the 16-bit DL register is exported by the UART as two 8-bit registers, the DLL and the DLM, which contain the LSB and the MSB of the DL register, respectively. Second, the DLL and the DLM have the same bus address as other registers. Thus, to access the DLL and the DLM a programmer must first set the DL Access Bit (DLAB) of the LCR, then actually access the DLL/DLM register, and finally reset the DLAB so that the other registers can be accessed.
	outb(lcr,PORTA+UART_LCR); //access LCR
	outb(bps,PORTA+UART_DLL); // access LSB
	outb(bps>>8,PORTA+UART_DLM); //access MSB
	lcr &= ~UART_LCR_DLAB; //reset
	outb(lcr,PORTA+UART_LCR); // mandar a DLAB com o valor depois do reset, i.e 0
	itr=inb(PORTA+UART_IER);
	itr |= UART_IER_THRI | UART_IER_RDI;
	// outb(0,PORTA+UART_IER); // disable interrupt...isto sai no seri
	outb(itr,PORTA+UART_IER);
	alloc_chrdev_region(&dev,0,1,"seri");
	seri_major=MAJOR(dev);
	cdev_init(&(seri->cdp),&fops);
	seri->cdp.ops=&fops;
	seri->cdp.owner = THIS_MODULE;
	spin_lock_init(&(seri->lock_rx));
	spin_lock_init(&(seri->lock_tx));
	init_waitqueue_head(&(seri->rxwq));
	init_waitqueue_head(&(seri->txwq));
	seri->rxfifo=kfifo_alloc(4096,GFP_KERNEL, &(seri->lock_rx));
	seri->txfifo=kfifo_alloc(4096,GFP_KERNEL, &(seri->lock_tx));
	printk(KERN_ALERT "Add: %d\n", cdev_add(&(seri->cdp),dev,1));
	printk(KERN_ALERT " Major device number: %d\n", seri_major);
	if((request_irq(4,int_handler,0,"seri",seri)!=0)){
		printk(KERN_ALERT "Merdou no request irq\n");
		return -1;
	}
	return 0;
}

//-----------------------------FUNÇAO EXIT--------------------------------

static void seri_exit(void)
{	
	cdev_del(&seri->cdp);
	kfifo_free(seri->rxfifo);
	kfifo_free(seri->txfifo);
	free_irq(4,(void *)seri);
	release_region(PORTA,1);
	unregister_chrdev_region(dev,1);
	kfree(seri);
	printk(KERN_ALERT "I'm free!\n");
		
}

module_init(seri_init);
module_exit(seri_exit);
	
		
