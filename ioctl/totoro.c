#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/time.h>
#include "../arch/arm/mach-at91/include/mach/hardware.h"
#include "../arch/arm/mach-at91/include/mach/io.h"
#include "../arch/arm/mach-at91/include/mach/at91_pio.h"
#include "../arch/arm/mach-at91/include/mach/at91sam9260_matrix.h"

#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/smp_lock.h>

#include "at91_adc.h"

//*************************************************************
#define NCSpin	NCS3
#define NCS3 0x00004000	//NCS3
#define adik 0x40000000	//NCS3

//*************************************************************

#define adc_read(reg)		ioread32(BaseAdrRAMADC + (reg))
#define adc_write(val, reg)	iowrite32((val), BaseAdrRAMADC + (reg))
#define AT91_DEFAULT_CONFIG             AT91_ADC_SHTIM  | \
                                        AT91_ADC_STARTUP | \
                                        AT91_ADC_PRESCAL | \
                                        AT91_ADC_SLEEP
#define CR_RESET	0x00000001	//hardware reset
#define CR_NORESET	0x00000000	//hardware reset clear
//#define CR_START	0x00000002	//begin a-to-d conversion
#define CR_STOP		0x00000000	//stop a-to-d conversion

#define INIT_CHER	0x00000008	//we use this channel (channel 3)
#define INIT_CHDR	0x00000007	//disable all channels (channel 3 - enable)

#define IER_INIT	0x00000000	//no interrupt enable
#define IDR_INIT	0x000F0F0F	//interrupt disable
#define IMR_INIT	0x00000000	//interrupt mask disable

#define SR_EOC3		0x00000008	//channel3 enable and conversion done
#define SR_OVRE3	0x00000800	//channel3 overrun error
#define SR_DRDY		0x00010000	//data ready in CS_ADC_LCDR
#define SR_GOVRE	0x00020000	//general error (data in CS_ADC_LCDR not valid, maybe)
#define SR_EDNRX	0x00040000	//  ???
#define SR_RXBUF	0x00080000	//  ???

//#define LCDR_10		0x000003FF	//  bit10

//*************************************************************

#define SIZE_BUF 65536
#define mem_buf 4096

#define CS_SPEED	0x2000	//  регистр управления скоростью
#define CS_MODE_MOTOR	0x2004	//  регистр управления режимом :  0 - ручной (с пульта)  1 - программный (с вэба)
#define CS_GPRS_STAT	0x1000	//  адрес регистра статуса GPRS-порта
#define RDEMP		2	//  бит статуса "буфер для чтения GPRS-порта готов"
#define CS_GPRS_DATA	0x1010	//  адрес регистра данных GPRS-порта
#define CS_MIRROR_R_AT	0x103A //
//	write : 7 6 5 4 3 2 1 0
//			      rd at
#define CS_GPRS_RESET	0x1034	//  адрес регистра сброса GPRS-порта
#define CS_MIRROR_W_AT	0x103C //
//	write : 7 6 5 4 3 2 1 0
//			      wr at
#define CS_CROSS_BOARD 	0x2700 //  адрес тестирования наличия кросс-платы //  0-запретить 1-разрешить 

#define CS_ROMBOARD 	0x4000 //  адрес ROM кросс-платы
#define CS_ROMBOARD_RST	0x4800 //  сброс адреса ROM кросс-платы
				// bit0 = 1 - сброс
				// bit0 = 0 - сброс снят
//********************      1 wire    ****************************
//
//------------   DS18   ---------------------------------------
#define CS_W1_DATA	0x1210 //  регистр данных (на запись и чтение)
//	запись по 1 wire
#define CS_W1_WR_RST	0x1238 //  начало транзакции записи : записать 1 по этому адресу
#define CS_W1_WR_ADR	0x123C //  регистр для смены адреса на запись (импульс 0/1/0)
#define CS_W1_WR_CLR	0x1234 //  регистр сброса, если обнаружили на шине 1w устройство после ресета
//	чтение по 1 wire
#define CS_W1_RD_INI	0x1236 //  начало транзакции чтения : записать 1 по этому адресу
#define CS_W1_RD_RDY	0x1200 //  готовность по чтению по этому адресу - D9=1 - данные готовы
#define W1_RD_RDY	0x200 // маска готовности данных по чтению D9=1
#define CS_W1_RD_ADR	0x123A //  регистр для смены адреса по чтению (импульс 0/1/0)
#define W1_DEV_RDY	0x400 // маска готовности данных по чтению устройств на шине 1w
//-------------   DHT11   ------------------------------------
#define CS_W1_WR_RST_DHT	0x1438 //  начало транзакции записи : записать 1 по этому адресу
#define CS_W1_RD_RDY_DHT	0x1400 //  готовность : D9=1 - данные готовы, D10=1 - есть устройство
#define CS_W1_RD_ADR_DHT	0x143A //  регистр для смены адреса по чтению (импульс 0/1/0)
//-------------   HС-SR04   ------------------------------------
#define CS_RDY_HCSR	0x1600 //  готовность : D0=1 - данные готовы
#define CS_DAT_HCSR	0x1610 //  данные, 16 разрядов
#define CS_CTL_HCSR	0x163A //  D0=1 - старт .. D0=0 - стоп
#define	HCSR_RDY_MASK	1

#define CS_RDY_HCSR2	0x1500 //  готовность : D0=1 - данные готовы
#define CS_DAT_HCSR2	0x1510 //  данные, 16 разрядов
#define CS_CTL_HCSR2	0x153A //  D0=1 - старт .. D0=0 - стоп

//------------   Encoder   ---------------------------------------
#define CS_ENC	0x1700 //  базовый адрес, пока свободен
#define CS_ENC_WAY	0x1710//младшее слово пути 16bit
#define CS_ENC_WAY2	0x1712//старшее слово пути 16bit
#define CS_ENC_SPEED	0x1714//скорость 16бит
#define CS_ENC_RST	0x1738//сброс устройства


#define Max_Len_Rom 128

#define ADC_CHAN_SELECT	1
#define ADC_CHAN_FREE	2


//------------------------   GSM/3G   -------------------------------------------------


#define max_card 1

#define all_radio (max_card * 4)

#define GET_GSM_STATUS	1
#define SET_GSM_ON	2
#define SET_GSM_OFF	3
#define SET_GSM_PWRON	4
#define SET_GSM_PWROFF	5
#define SET_GSM_RST	6
#define GET_GSM_READY	7
#define SET_GSM_START	8
#define SET_GSM_STOP	9
#define SET_KT_POINT	10

#define SET_MODE_PULT	0	//0
#define SET_MODE_SOFT	1	//1

#define KT_POINT	0x1940

#define def_mir	0xC3//115200+modon+pwroff

#define CS_PiML_AT_COM 	0x1810 //  базовый адрес управления COM-портами AT (данные)
#define CS_PiML_STATUS 	0x1800 //  базовый адрес управления/чтения статуса модулей
//read : 7 6 5 4 3 2 1 0	write : 7 6 5 4 3 2 1 0
//	 W R R W R W R V		\-/ \-/ M S O P
//       R D S R D R D I                 S   S  O P N W
//           T         O                 P   P  D E / R
//       I I   S S E E                   E   E  E E O
//       M M S I I M M                   E   E    D F
//       E E I M M P P                   D   D    2 F
//       I I M                           1   3
//					SPEED1:00-9600, 10-28800, 11-115200 (для CS_PiML_AT_COM)
//					SPEED3:00-9600, 01-50k,   10-100k (для CS_PiML_SIM_COM)
//				                MODE:0-обычный режим приемо-предатчика АТ команд модуля
//						     1-программирование(смена прошивки) модуля
//
//						  SPEED2: не используется
//
//						    ON/OFF вкл./выкл. модуля
//
//						      PWR - вкл./выкл. питания модуля
//
//#define CS_MIRROR 	0x1032 //  базовый адрес управления адресами буферов Алтеры (sim & at)
////	write : 7 6 5 4 3 2 1 0
////		        w r w r
////		        r d r d
////		        a a s s
////			t t i i
////			    m m
//#define CS_MIRROR_ATR 	0x1034 //
//	write : 7 6 5 4 3 2 1 0
//			      atr reset
//#define CS_MIRROR_R_SIM	0x1036 //
//	write : 7 6 5 4 3 2 1 0
//			      rd sim
//#define CS_MIRROR_W_SIM	0x1038 //
//	write : 7 6 5 4 3 2 1 0
//			      wr sim
#define CS_GSM_R_AT	0x003A //
//	write : 7 6 5 4 3 2 1 0
//			      rd at
#define CS_GSM_W_AT	0x003C //
//	write : 7 6 5 4 3 2 1 0
//			      wr at

#define CS_PRESENCE   	0x19E0 	//  адрес идентификатора плат  
#define CS_CARD_RESET	0x19F0 //  базовый адрес сброса субплаты
#define CS_ROM   	0x19D0 //  адрес идентификатора прошивки на субплате

#define MOD_RDempty  8      //  бит статуса "буфер для чтения COM-порта SIM готов"
#define MOD_WRempty  0x10   //  бит статуса "буфер для записи в COM-порт SIM готов"

//#define CS_RESET   	0x19F0 //  базовый адрес сброса плат  
#define G8_ONLY		0x6000//  = 0 - крылья (24 канала),   = 1 - только G4 или G8

#define msk_id		0x0ff0	// A903,B902,C905,D904,E907,F906  - CARD ID

//--------------------------------------------------------------------------------------
#define CLASS_DEV_CREATE(class, devt, device, name) \
	device_create(class, device, devt, NULL, "%s", name)
#define CLASS_DEV_DESTROY(class, devt) \
	device_destroy(class, devt)
static struct class *altera_class = NULL;

static struct class *gsm_class = NULL;


#define	UNIT(file) MINOR(file->f_dentry->d_inode->i_rdev)
#define	NODE(file) MAJOR(file->f_dentry->d_inode->i_rdev)

#define kol_dev 8 //0=moto 1=gps 2=ds18 3=dht11 4=ADC 5=hcsr 6=env 7=hcsr2

#define DevName "totoro"
#define DevNameGSM "gsm"

#define timi_def 5
#define timi2_def 2

#define rd_w1_buf_len 16

#define max_known_type 6

typedef struct
{
    unsigned short  addr;//base adress (status)
    unsigned        present : 1;
    unsigned        type : 7;//type of module : 0=sim300 1=m10 2=sim5215 3=sim900
    unsigned char   mir;
} s_table;


//--------------------------------------------------------------------------------------
static int my_dev_ready=0;

struct resource * cs3_iomem_reg = NULL;
static void __iomem *BaseAdrRAM = NULL;

struct resource * adc_iomem_reg = NULL;
static void __iomem *BaseAdrRAMADC = NULL;

static void __iomem *at91_pioc_base = NULL;
struct clk *at91_adc_clk;

static unsigned char cross_board_presence=0;

static unsigned char *ibuff=NULL;
static unsigned char *ibuffw=NULL;
static unsigned short adr_reset=CS_GPRS_RESET;
static unsigned short adr_speed=CS_SPEED;
static unsigned short adr_mode_motor=CS_MODE_MOTOR;
static unsigned short adr_status=CS_GPRS_STAT;
static unsigned short adr_data=CS_GPRS_DATA;
static unsigned short adr_mir_r=CS_MIRROR_R_AT;
static unsigned short adr_mir_w=CS_MIRROR_W_AT;

static unsigned int my_msec;
static struct timer_list my_timer;

static atomic_t varta;
static atomic_t rx_ready;
static atomic_t begin;

static int Major=0;
module_param(Major, int, 0);
static int Device_Open[kol_dev] = {0};

static int MajorGSM=0;
module_param(MajorGSM, int, 0);
static int Device_OpenGSM[all_radio] = {0};

struct rchan_sio {
  struct cdev cdev;
  struct cdev cdevGSM;
};

static int timi=timi_def;
static int timi2=timi2_def;

static wait_queue_head_t poll_wqh[kol_dev];
static wait_queue_head_t poll_wqhGSM[all_radio];

static atomic_t rd_w1_start;
static atomic_t rd_w1_ready;
static atomic_t rd_w1_len;
static atomic_t rd_w1_len_in;
static unsigned char rd_w1_data[rd_w1_buf_len];

static atomic_t rd_w1_start_dht11;
static atomic_t rd_w1_ready_dht11;
static atomic_t rd_w1_len_dht11;
static atomic_t rd_w1_len_in_dht11;
static unsigned char rd_w1_data_dht11[rd_w1_buf_len];

static atomic_t start_adc;
static atomic_t ready_adc;
static atomic_t data_adc;
static unsigned int mir_cr;
static unsigned int mir_cher;
static unsigned int mir_chdr;
static unsigned int mir_ier;
static unsigned int mir_idr;
static int chan_set=3;

static atomic_t start_hcsr;
static atomic_t ready_hcsr;
static atomic_t data_hcsr;
static atomic_t start_hcsr2;
static atomic_t ready_hcsr2;
static atomic_t data_hcsr2;

static atomic_t start_enc;
static atomic_t ready_enc;
static atomic_t way_enc;
static atomic_t speed_enc;

static const char *known_msg[max_known_type] = {"$GPRMC","$GPGGA","$GPGSV","$GPGLL","$GPGSA","$GPVTG"};
static atomic_t msg_type_ind;//0=$GPRMC

static unsigned char subboard[max_card]={0};
static atomic_t gsm_rx_ready[all_radio];
static atomic_t gsm_rx_begin[all_radio];
static s_table RK_table[all_radio];

static const char *known_type[5] = {"SIM300","M10/M12","SIM5215","SIM900","???"};

//************************************************************
static int reset_card(int np)
{
unsigned short ofs;
unsigned char bt;

    ofs = (unsigned short)CS_CARD_RESET + (unsigned short)(np*0x200);
    bt=0; iowrite8(bt,BaseAdrRAM+ofs);
    bt=1; iowrite8(bt,BaseAdrRAM+ofs);
    printk(KERN_ALERT "%s: Reset subboard %d : addr=0x%04X\n",DevNameGSM,np+1,ofs);

    return 0;

}
//************************************************************
static unsigned short read_card_id(int np)
{
unsigned short id=0xffff, ofs, word, j;
int k;

//type: 0-sim300 1-m10 2-sim5215 3-sim900

    if ((np<0) || (np>=max_card)) return id;

    ofs = (unsigned short)CS_PRESENCE + (unsigned short)(np * 0x200);
    word = ioread16(BaseAdrRAM+ofs);
    if ((word&msk_id) == 0x0900) {
	printk(KERN_ALERT "%s: subboard %d : id=0x%04X addr=0x%04X\n",DevNameGSM,np+1,word,ofs);
	k = np << 2;
	id = word;
	switch (word) {//type of module : 0=sim300 1=m10 2=sim5215 3=sim900
	    case 0xB902 :
		subboard[np] = 1;
		for (j = 0; j < 4; j++) {
		    RK_table[k + j].mir = (unsigned char)def_mir;//115200+modon+pwroff
		    RK_table[k + j].type = 0;//sim300
		    if (!np) RK_table[k + j].present = 1;
			else if (!j) RK_table[k+j].present = 1;
		}
	    break;
	    case 0xC905 :
	    case 0xD904 :
		subboard[np] = 1;
		for (j = 0; j < 4; j++) {
		    RK_table[k + j].mir = (unsigned char)def_mir;//115200+modon+pwroff
		    RK_table[k + j].type = 1;//m10
		    if (!np) RK_table[k + j].present = 1;
			else if (!j) RK_table[k + j].present = 1;
		}
	    break;
	    case 0xF906 :
		subboard[np] = 1;
		for (j = 0; j < 4; j++) {
		    RK_table[k + j].mir = (unsigned char)def_mir;//115200+modon+pwroff
		    RK_table[k + j].type = 2;//sim5215
		    if (!np) RK_table[k + j].present = 1;
			else if (!j) RK_table[k + j].present = 1;
		}
	    break;
	    case 0xE907 :
		subboard[np] = 1;
		for (j = 0; j < 4; j++) {
		    RK_table[k + j].mir = (unsigned char)def_mir;//115200+modon+pwroff
		    RK_table[k + j].type = 3;//sim900
		    if (!np) RK_table[k + j].present = 1;
			else if (!j) RK_table[k + j].present = 1;
		}
	    break;
	}
    }

    return id;
}
//************************************************************
static int read_card_rom(int np)
{
unsigned short ofs;
unsigned char i, bt=0, bbb=0, dl, ret=0;

//    if (reset_card(np)<0) return -1;
    reset_card(np);

    dl=Max_Len_Rom;
    ofs = (unsigned short)CS_ROM + (unsigned short)(np*0x200);
    for (i=0; i<2; i++) {
	iowrite8(bbb,BaseAdrRAM+ofs);
	bbb++;
	ioread8(BaseAdrRAM+ofs);
    }
    if (ibuff!=NULL) {
	i=1;
	memset(ibuff,0,Max_Len_Rom+1);
    } else i=0;
    while (i) {
	iowrite8(bbb,BaseAdrRAM+ofs);
	bt = ioread8(BaseAdrRAM+ofs);
	ibuff[ret] = bt;
	ret++;  dl--;
	if ((!bt) || (!dl)) i = 0;
	bbb++;
    }

    return ret;
}
//************************************************************
static int gsm_rxReady(int rk)
{
    if ( ((ioread8(BaseAdrRAM+RK_table[rk].addr)) & RDEMP) == 0 ) return 1; else return 0;
}
//************************************************************
//************************************************************
//************************************************************
//************************************************************
//************************************************************
static int mux_chan(int chan, int operation)
{
int pin_chan;

    if (chan < 0 || chan > 3) return -EINVAL;

    switch (chan) {
	case 0: pin_chan = AT91_PIN_PC0; break;
	case 1: pin_chan = AT91_PIN_PC1; break;
	case 2: pin_chan = AT91_PIN_PC2; break;
	case 3: pin_chan = AT91_PIN_PC3; break;
    	    default: return -EINVAL;
    }
    if (operation == 1)//chan_select
        at91_set_A_periph(pin_chan, 0);                         //Mux PIN to GPIO
    else               //chan_free
        at91_set_B_periph(pin_chan, 0);                         //Mux PIN to GPIO
    chan_set = chan;

    return 0;
}
//************************************************************
static void adc_reset(void)
{
    mir_cr = (unsigned int)AT91_ADC_SWRST; adc_write(mir_cr, AT91_ADC_CR);   //Reset the ADC
    atomic_set(&start_adc, (int)0);
    atomic_set(&ready_adc, (int)0);
//    mir_cr &= 0xffffffe; adc_write(mir_cr, AT91_ADC_CR);
}
//************************************************************
static void adc_init(int chan)//we work with channel 3
{
unsigned int data=0;

    data = (unsigned int)AT91_DEFAULT_CONFIG; adc_write(data, AT91_ADC_MR);//set mode

    mir_cher = (unsigned int)AT91_ADC_CH(chan);	adc_write(mir_cher, AT91_ADC_CHER);//chan3 - enable
    mir_chdr = (unsigned int)0;			adc_write(mir_chdr, AT91_ADC_CHDR);

    mir_ier = (unsigned int)IER_INIT;	adc_write(mir_ier, AT91_ADC_IER);//no interrupt enable
    mir_idr = (unsigned int)IDR_INIT;	adc_write(mir_idr, AT91_ADC_IDR);//interrupt disable


//    data = AT91_PIN_PC3; at91_set_A_periph(data, 0);//request chan 3
    mux_chan(chan, ADC_CHAN_SELECT);//request chan

}
//************************************************************
static void adc_start_conv(int chan)
{

//    mir_cher = 0;	adc_write(mir_cher, AT91_ADC_CHER);
//    mux_chan(chan, ADC_CHAN_SELECT);//request chan
    iowrite32(1 << chan, at91_pioc_base + 0x60);
    mir_cher = (unsigned int)AT91_ADC_CH(chan); adc_write(mir_cher,AT91_ADC_CHER);// Enable Channel
    mir_cr = (unsigned int)AT91_ADC_START; adc_write(mir_cr,AT91_ADC_CR);// Start the ADC
    atomic_set(&start_adc, (int)1);
    atomic_set(&ready_adc, (int)0);

}
//************************************************************
static void adc_stop_conv(int chan)
{
    mir_cr = (unsigned int)CR_STOP;	adc_write(mir_cr, AT91_ADC_CR);
    atomic_set(&start_adc, (int)0);
    atomic_set(&ready_adc, (int)0);
}
//************************************************************
static void hcsr_start_conv(int ds)
{
unsigned char bt;
unsigned short adr;

    if (!ds) {//первый датчик - 0x1600
	adr = (unsigned short)CS_CTL_HCSR;
	bt = 1; iowrite8(bt,BaseAdrRAM+adr);
	bt = 0; iowrite8(bt,BaseAdrRAM+adr);
	atomic_set(&start_hcsr, (int)1);
	atomic_set(&ready_hcsr, (int)0);
    } else {//второй датчик - 0x1500
	adr = (unsigned short)CS_CTL_HCSR2;
	bt = 1; iowrite8(bt,BaseAdrRAM+adr);
	bt = 0; iowrite8(bt,BaseAdrRAM+adr);
	atomic_set(&start_hcsr2, (int)1);
	atomic_set(&ready_hcsr2, (int)0);
    }
}
//************************************************************
static void hcsr_stop_conv(int ds)
{
    if (ds==0) {//первый датчик - 0x1600
	atomic_set(&start_hcsr, (int)0);
	atomic_set(&ready_hcsr, (int)0);
    } else {//второй датчик - 0x1500
	atomic_set(&start_hcsr2, (int)0);
	atomic_set(&ready_hcsr2, (int)0);
    }
}
//************************************************************
static int adc_data_ready(int chan)
{
int ret=-1;//, chan=3, sr;
unsigned int data;

    data = adc_read(AT91_ADC_SR);
    if (data & AT91_ADC_EOC(chan)) {
	data = adc_read(AT91_ADC_CHR(chan));
	data &= AT91_ADC_DATA;
	ret=data;
    }

    return ret;
}
//************************************************************
static int rd_bytes(int devm, unsigned char *b)
{
int ret=0;
unsigned char bt, yes=0;
unsigned short wordik, slovo=0, adra=0;

    switch (devm) {
	case 2://DS18
	    slovo = (unsigned short)CS_W1_RD_RDY;
	    adra = (unsigned short)CS_W1_RD_ADR;
	    yes = 1;
	break;
	case 3://DHT11
	    slovo = (unsigned short)CS_W1_RD_RDY_DHT;
	    adra = (unsigned short)CS_W1_RD_ADR_DHT;
	    yes = 1;
	break;
    }
    if (yes) {
	wordik = ioread16(BaseAdrRAM+slovo);
	if (wordik & W1_RD_RDY) {//данные на чтение готовы
	    bt = wordik&0xff;
	    *(unsigned char *)b = bt;
	    bt = 0; iowrite8(bt, BaseAdrRAM+adra);
	    bt = 1; iowrite8(bt, BaseAdrRAM+adra);
	    bt = 0; iowrite8(bt, BaseAdrRAM+adra);
	    ret = 1;
	}
    }
    return ret;
}
//************************************************************
static int rxReady(void)
{
    if ( ((ioread8(BaseAdrRAM+adr_status)) & RDEMP) == 0 ) return 1; else return 0;
}
//************************************************************
static void enc_reset(void)
{
unsigned char bt;
unsigned short adra;

    adra = (unsigned short)CS_ENC_RST;
    bt = 0; iowrite8(bt, BaseAdrRAM+adra);//#define CS_ENC_RST	0x1738//сброс устройства
    bt = 1; iowrite8(bt, BaseAdrRAM+adra);
    bt = 0; iowrite8(bt, BaseAdrRAM+adra);
    atomic_set(&start_enc, (int)0);
    atomic_set(&ready_enc, (int)0);
    atomic_set(&way_enc, (int)0);
    atomic_set(&speed_enc, (int)0);
}
//************************************************************
static void enc_data_read(void)
{
unsigned short adra, word_ml, word_st, wrd;
unsigned int data;

    adra = (unsigned short)CS_ENC_WAY;
    word_ml = ioread16(BaseAdrRAM+adra);//ml. slovo of way
    word_st = ioread16(BaseAdrRAM+adra+2);//st. slovo of way
    wrd = ioread16(BaseAdrRAM+adra+4);//slovo of speed
    data = word_st; data <<= 16; data |= word_ml;
    atomic_set(&way_enc, data);
    atomic_set(&speed_enc, (int)wrd);

}
//************************************************************
//		    таймер 2 mcek
//************************************************************
void MyTimer(unsigned long d)
{
int rt=0, i;
unsigned char bt;

    timi--; if (!timi) { timi=timi_def; atomic_inc(&varta); }

    timi2--;
    if (!timi2) {
	timi2=timi2_def;
	for (i=0; i<kol_dev; i++) {// /dev/totoro0 - moto /dev/totoro1 - gps /dev/totoro2 - ds18  /dev/totoro3 - dht11
	    switch (i) {
		case 0: break;// /dev/totoro0 - moto
		case 1:// /dev/totoro1 - gps
		    if (atomic_read(&begin)) {//GPS minor=1
			rt = rxReady();
			atomic_set(&rx_ready, rt);
		    }
		    if (atomic_read(&rx_ready)) wake_up_interruptible(&poll_wqh[i]);
		break;
		case 2:// /dev/totoro2 - ds18
		    if (atomic_read(&rd_w1_start)) {
			if (rd_bytes(i, &bt)) {
			    rd_w1_data[atomic_read(&rd_w1_len)] = bt;
			    atomic_inc(&rd_w1_len);
			    if ( atomic_read(&rd_w1_len) == atomic_read(&rd_w1_len_in) ) {
				atomic_set(&rd_w1_start, (int)0);
				atomic_set(&rd_w1_ready, (int)1);
			    } else iowrite8((unsigned char)1, BaseAdrRAM+CS_W1_RD_INI);// начало тразакции чтения
			}
		    }
		    if (atomic_read(&rd_w1_ready)) wake_up_interruptible(&poll_wqh[i]);
		break;
		case 3:// /dev/totoro3 - dht11
		    if (atomic_read(&rd_w1_start_dht11)) {
			if (rd_bytes(i, &bt)) {
			    rd_w1_data_dht11[atomic_read(&rd_w1_len_dht11)] = bt;
			    atomic_inc(&rd_w1_len_dht11);
			    if ( atomic_read(&rd_w1_len_dht11) == atomic_read(&rd_w1_len_in_dht11) ) {
				atomic_set(&rd_w1_start_dht11, (int)0);
				atomic_set(&rd_w1_ready_dht11, (int)1);
			    }
			}
		    }
		    if (atomic_read(&rd_w1_ready_dht11)) wake_up_interruptible(&poll_wqh[i]);
		break;
		case 4:// /dev/totoro4 - ADC
		    if (atomic_read(&start_adc)) {//ADC minor=4
			rt = adc_data_ready(chan_set);
			if (rt >= 0) {
			    atomic_set(&data_adc, rt);
			    atomic_set(&ready_adc, (int)1);
			    atomic_set(&start_adc, (int)0);
			}
		    }
		    if (atomic_read(&ready_adc)) wake_up_interruptible(&poll_wqh[i]);
		break;
		case 5:// /dev/totoro5 - HCSR
		    if (atomic_read(&start_hcsr)) {//HCSR minor=5
			rt = ioread16(BaseAdrRAM+CS_RDY_HCSR) & HCSR_RDY_MASK;
			if (rt) {
			    rt = ioread16(BaseAdrRAM+CS_DAT_HCSR);
			    atomic_set(&data_hcsr, rt);
			    atomic_set(&ready_hcsr, (int)1);
			    atomic_set(&start_hcsr, (int)0);
			}
		    }
		    if (atomic_read(&ready_hcsr)) wake_up_interruptible(&poll_wqh[i]);
		break;
		case 6:// /dev/totoro6 - ENC
		    if (atomic_read(&start_enc)) {//ENC minor=6
			enc_data_read();
			atomic_set(&ready_enc, (int)1);
		    }
		    if (atomic_read(&ready_enc)) wake_up_interruptible(&poll_wqh[i]);
		break;
		case 7:// /dev/totoro7 - HCSR2
		    if (atomic_read(&start_hcsr2)) {//HCSR2 minor=7
			rt = ioread16(BaseAdrRAM+CS_RDY_HCSR2) & HCSR_RDY_MASK;
			if (rt) {
			    rt = ioread16(BaseAdrRAM+CS_DAT_HCSR2);
			    atomic_set(&data_hcsr2, rt);
			    atomic_set(&ready_hcsr2, (int)1);
			    atomic_set(&start_hcsr2, (int)0);
			}
		    }
		    if (atomic_read(&ready_hcsr2)) wake_up_interruptible(&poll_wqh[i]);
		break;
	    }
	}
	for (i = 0; i < all_radio; i++) {
	    if (RK_table[i].present) {
		if (atomic_read(&gsm_rx_begin[i])) {
		    rt = gsm_rxReady(i);
		    atomic_set(&gsm_rx_ready[i], rt);
		}
		if (atomic_read(&gsm_rx_ready[i])) wake_up_interruptible(&poll_wqhGSM[i]);
	    }
	}
    }

    my_timer.expires = jiffies + 2;//1 mсек.
    mod_timer(&my_timer, jiffies + 2);

    return;
}
//***********************************************************
//		открытие устройства
//************************************************************
static int rchan_open(struct inode *inode, struct file *filp) {

struct rchan_sio *sio;
int unit, ret=-ENODEV, node;
// /dev/totoro0 - moto /dev/totoro1 - gps /dev/totoro2 - ds18 /dev/totoro3 - dht11 /dev/totoro4 - ADC /dev/totoro5 - HC-SR04
// /dev/gsm0..4

    node = NODE(filp);
    unit = UNIT(filp);

	if (node == Major) {//sensors
	    if ((unit >= 0) && (unit < kol_dev)) {
		if (!Device_Open[unit]) {
		    Device_Open[unit]++;
		    sio = container_of(inode->i_cdev, struct rchan_sio, cdev);
		    filp->private_data = sio;
		    switch (unit) {
			case 1://GPS
			    atomic_set(&begin, (int)0);
			    atomic_set(&rx_ready, (int)0);
			break;
			case 2://ds18
			    atomic_set(&rd_w1_start, (int)0);
			    atomic_set(&rd_w1_ready, (int)0);
			break;
			case 3://dht11
			    atomic_set(&rd_w1_start_dht11, (int)0);
			    atomic_set(&rd_w1_ready_dht11, (int)0);
			break;
			case 4://ADC
			    atomic_set(&start_adc, (int)0);
			    atomic_set(&ready_adc, (int)0);
			    //adc_reset();
			    //adc_init(chan_set);
			break;
			case 5://HC-SR04
			    atomic_set(&start_hcsr, (int)0);
			    atomic_set(&ready_hcsr, (int)0);
			break;
			case 6://ENC
			    atomic_set(&start_enc, (int)0);
			    atomic_set(&ready_enc, (int)0);
			    atomic_set(&way_enc, (int)0);
			    atomic_set(&speed_enc, (int)0);
			break;
			case 7://HC-SR04 (second device)
			    atomic_set(&start_hcsr2, (int)0);
			    atomic_set(&ready_hcsr2, (int)0);
			break;
		    }
		    init_waitqueue_head(&poll_wqh[unit]);
		    ret = 0;
		} else ret = -EBUSY;
	    }
	} else if (node==MajorGSM) {//gsm
	    if ((unit >= 0) && (unit < all_radio)) {
		if (!Device_OpenGSM[unit]) {
		    Device_OpenGSM[unit]++;
		    sio = container_of(inode->i_cdev, struct rchan_sio, cdevGSM);
		    filp->private_data = sio;
		    init_waitqueue_head(&poll_wqhGSM[unit]);
		    atomic_set(&gsm_rx_begin[unit], (int)0);
		    atomic_set(&gsm_rx_ready[unit], (int)0);
		    ret=0;
		} else ret = -EBUSY;
	    }
	}

    return ret;
}
//***********************************************************
//		закрытие устройства
//************************************************************
static int rchan_release(struct inode *inode, struct file *filp) 
{
int unit, node, ret=-1;

    node = NODE(filp);
    unit = UNIT(filp);

	if (node==Major) {//sensors
	    if (Device_Open[unit]>0) Device_Open[unit]--;
	    if ((unit >= 0) && (unit < kol_dev)) {
		switch (unit) {
		    case 1://GPS
			atomic_set(&begin, (int)0);
			atomic_set(&rx_ready, (int)0);
		    break;
		    case 2://ds18
			atomic_set(&rd_w1_start, (int)0);
			atomic_set(&rd_w1_ready, (int)0);
		    break;
		    case 3://dht11
			atomic_set(&rd_w1_start_dht11, (int)0);
			atomic_set(&rd_w1_ready_dht11, (int)0);
		    break;
		    case 4://ADC
			atomic_set(&start_adc, (int)0);
			atomic_set(&ready_adc, (int)0);
		    break;
		    case 5://HC-SR04
			atomic_set(&start_hcsr, (int)0);
			atomic_set(&ready_hcsr, (int)0);
		    break;
		    case 6://ENC
			atomic_set(&start_enc, (int)0);
			atomic_set(&ready_enc, (int)0);
		    break;
		    case 7://HC-SR04 (second)
			atomic_set(&start_hcsr2, (int)0);
			atomic_set(&ready_hcsr2, (int)0);
		    break;
		}
		ret = 0;
	    }
	} else if (node==MajorGSM) {//gsm
	    if (Device_OpenGSM[unit]>0) Device_OpenGSM[unit]--;
	    if ((unit >= 0) && (unit < all_radio)) {
		atomic_set(&gsm_rx_begin[unit], (int)0);
		atomic_set(&gsm_rx_ready[unit], (int)0);
		ret = 0;
	    }
	}

    return ret;
}
//************************************************************
static unsigned int rchan_poll(struct file *filp, struct poll_table_struct *wait_table)
{
int ret=0, unit, node, dm=0;

    node = NODE(filp);
    unit = UNIT(filp);

	if (node == Major) {//sensors
	    if ((unit >= 0) && (unit < kol_dev)) {
		poll_wait(filp, &poll_wqh[unit], wait_table);
		switch (unit) {
		    case 0: dm = 0; break;//moto
		    case 1://gps
			if (atomic_read(&rx_ready)) dm++;
		    break;
		    case 2://ds18
			if (atomic_read(&rd_w1_ready)) dm++;
		    break;
		    case 3://dht11
			if (atomic_read(&rd_w1_ready_dht11)) dm++;
		    break;
		    case 4://ADC
			if (atomic_read(&ready_adc)) dm++;
		    break;
		    case 5://HC-SR04
			if (atomic_read(&ready_hcsr)) dm++;
		    break;
		    case 6://ENC
			if (atomic_read(&ready_enc)) dm++;
		    break;
		    case 7://HC-SR04 (second)
			if (atomic_read(&ready_hcsr2)) dm++;
		    break;
		}
	    }
	} else if (node == MajorGSM) {//gsm
	    if ((unit >= 0) && (unit < all_radio)) {
		if (RK_table[unit].present) {
		    poll_wait(filp, &poll_wqhGSM[unit], wait_table);
		    if (atomic_read(&gsm_rx_ready[unit])) dm++;
		}
	    }
	}

    if (dm) ret |= (POLLIN | POLLRDNORM);

    return ret;
}
//***********************************************************
//		чтение
//************************************************************

static ssize_t rchan_read(struct file *filp, char __user *buff,	size_t count, loff_t *offp)
{
ssize_t ret=0;
unsigned int into0, a_stat, a_dat;
unsigned char cmds, bt, bbb, bbs, done;
unsigned short st_word, ml_word, offsets, wordik;
int flg, dl, unit, mores, cnt, fnd, node;
char *uk;

    if (count==0) return (atomic_read(&varta));

    node = NODE(filp);
    unit = UNIT(filp);

    flg = 0; ret = 0;
    into0 = count;  into0 &= 0xff;	ml_word = into0;		// из младшего слова берем команду (мл. байт)
    into0 = count;  into0 = (into0 & 0xffff00) >> 8;   st_word = into0;	// из старшего и младшего слова берем смещение
    offsets = st_word;
    cmds = ml_word;						// получили байт команды
    memset(ibuff, 0, mem_buf);

    switch (cmds) {//анализ заданной команды чтения
	case 1:						// чтение 8-ми разрядное
	    ibuff[0] = ioread8(BaseAdrRAM+offsets);
	    ret = 1; flg = 1;
	break;
	case 2:// чтение регистра управления скоростью - 0x2000
	    ibuff[0] = ioread8(BaseAdrRAM+adr_speed);
	    ret = 1; flg = 1;
	break;
	case 3://subboard[] to user level
	    memcpy(ibuff, &subboard[0], max_card);
	    ret = max_card; flg = 1;
	break;
	case 4://RK_table[] to user level
	    dl = sizeof(s_table) * all_radio;
	    memcpy(ibuff, (unsigned char *)&RK_table[0], dl);
	    ret = dl; flg = 1;
	break;
	case 5://чтение - есть ли на шине 1 wire DS18
	    offsets = (unsigned short)CS_W1_RD_RDY;
	    wordik = ioread16(BaseAdrRAM+offsets) & W1_DEV_RDY;	//W1_DEV_RDY	0x400  маска готовности данных по чтению устройств на шине 1w
	    if (!wordik) {//DS18 есть дома
		ret = 1;
		//--------   сброс готовности   --------------
		offsets = (unsigned short)CS_W1_WR_CLR;
		bt = 0; iowrite8(bt, BaseAdrRAM+offsets);
		bt = 1; iowrite8(bt, BaseAdrRAM+offsets);
		bt = 0; iowrite8(bt, BaseAdrRAM+offsets);
	    }
	break;
	case 6://чтение - есть ли на шине 1 wire DHT11
	    offsets = (unsigned short)CS_W1_RD_RDY_DHT;
	    wordik = ioread16(BaseAdrRAM+offsets) & W1_DEV_RDY;	// чтение готовности
	    if (!wordik) ret = 1;//DHT11 есть дома
	break;
	case 7://чтение байта по 1 wire с DS18
	    flg = 0; ret = 0;
	    offsets = (unsigned short)CS_W1_RD_RDY;
	    wordik = ioread16(BaseAdrRAM+offsets);
	    if (wordik & W1_RD_RDY) {//данные на чтение готовы !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		bbb = wordik&0xff;
		ibuff[0] = bbb;
		ret = 1; flg = 1;
		offsets = (unsigned short)CS_W1_RD_ADR;
		bt = 0; iowrite8(bt, BaseAdrRAM+offsets);
		bt = 1; iowrite8(bt, BaseAdrRAM+offsets);
		bt = 0; iowrite8(bt, BaseAdrRAM+offsets);
	    }
	break;
	case 8://  ЧТЕНИЕ МАССИВА    ReadRead(devm,buf)
		if (node == Major) {//sensors
		    switch (unit) {
			case 1:// GPS чтение пакета gprs, конец пакета - 0x0d,0x0a
			    cnt = mem_buf - 1; fnd = 0;
			    bbs = (ioread8(BaseAdrRAM+adr_status)) & RDEMP;
			    if (!bbs) mores = 1; else mores = 0;
			    while (mores) {
				bbb = ioread8(BaseAdrRAM+adr_data);
				if ((bbb < 0x0a) || (bbb > 0x7f)) bbb = '?';
				if (bbb == '$') fnd = 1;
				if (fnd) { ibuff[ret] = bbb; ret++; }
				bt = 0; iowrite8(bt, BaseAdrRAM+adr_mir_r);
				bt = 1; iowrite8(bt, BaseAdrRAM+adr_mir_r);
				bt = 0; iowrite8(bt, BaseAdrRAM+adr_mir_r);
				if ((bbb == 0x0a) && (fnd)) mores = 0;
				else {
				    bbs = (ioread8(BaseAdrRAM+adr_status)) & RDEMP;
				    if ((!bbs) && (ret < cnt)) mores = 1; else mores = 0;
				}
			    }
			    if (ret > 0) {
				mores = atomic_read(&msg_type_ind);
				if (mores != 255) {
				    uk = strstr((char *)ibuff, (char *)known_msg[mores]);//0=$GPRMC
				    if (uk) flg = 1; else ret = 0;
				} else flg = 1;
			    }
			break;
			case 2://DS18 : //начать прием 9 байт -> Read_Start
			    if (atomic_read(&rd_w1_ready)) {
				dl = atomic_read(&rd_w1_len); dl &= 0x0f;
				if (dl > 0) { memcpy(ibuff, rd_w1_data, dl); ret = dl; flg = 1; }
			    }
			break;
			case 3://DHT11 : //чтение 5-ти байт с DHT11
			    if (atomic_read(&rd_w1_ready_dht11)) {
				dl = atomic_read(&rd_w1_len_dht11); dl &= 0x0f;
				if (dl > 0) { memcpy(ibuff, rd_w1_data_dht11, dl); ret = dl; flg = 1; }
			    }
			break;
			case 4://ADC : //чтение 4-х байт (int)
			    flg = 0; ret = 0; cnt = atomic_read(&data_adc);
			    mores = atomic_read(&ready_adc);
			    if (mores > 0) { ret = cnt; atomic_set(&ready_adc, (int)0); atomic_set(&start_adc, (int)0); }
			break;
			case 5://HC-SR04 : //чтение 4-х байт (int)
			    flg = 0; ret = -1; //cnt=atomic_read(&data_hcsr);
			    //mores=atomic_read(&ready_hcsr);
			    if (atomic_read(&ready_hcsr)>0) {
				ret=atomic_read(&data_hcsr);
				atomic_set(&ready_hcsr, (int)0); 
				atomic_set(&start_hcsr, (int)0);
			    }
			break;
			case 6://ENC : //чтение 6-ти байт (int)+(short)
			    ret = 0; cnt = atomic_read(&way_enc);
			    memcpy(&ibuff[ret], &cnt, 4); ret += 4;//way
			    cnt=atomic_read(&speed_enc);
			    wordik = cnt;
			    memcpy(&ibuff[ret], &wordik, 2); ret += 2;//speed
			    flg = 1;
			break;
			case 7://HC-SR04 (srcond) : //чтение 4-х байт (int)
			    flg = 0; ret = -1; //cnt=atomic_read(&data_hcsr2);
			    //mores=atomic_read(&ready_hcsr2);
			    if (atomic_read(&ready_hcsr2)>0) {
				ret = atomic_read(&data_hcsr2);
				atomic_set(&ready_hcsr2, (int)0);
				atomic_set(&start_hcsr2, (int)0);
			    }
			break;
		    }
		} else if (node == MajorGSM) {//gsm
		    if ((unit >= 0) && (unit < all_radio)) {
			ret = 0; cnt = 512; mores = 1; done = 0;
			a_dat = a_stat = RK_table[unit].addr; a_dat += 0x10; a_stat += CS_GSM_R_AT;
			while (mores) {
			    wordik = ioread16(BaseAdrRAM+a_dat); cnt--;
			    bbb = wordik;//data
			    bbs = wordik >> 8;
			    bbs &= RDEMP;//status
			    if (!bbs) {
				bt = 0; iowrite8(bt, BaseAdrRAM+a_stat);
				bt = 1; iowrite8(bt, BaseAdrRAM+a_stat);
				bt = 0; iowrite8(bt, BaseAdrRAM+a_stat);
				if ((cnt > 0) && (ret < 512)) {
				    if ((bbb >= 0x0a) && (bbb < 0x80)) {
					if (bbb >= 0x20) done = 1; //else done = 0;
					if (done) {
					    ibuff[ret] = bbb;   ret++;
					}
					if ((bbb == 0x0d) && (done)) {
					    ibuff[ret] = bbb;   ret++;
					}
					if (bbb == 0x0a) mores = 0;
				    }
				} else mores = 0;
			    } else mores = 0;
			}
			if (ret > 0) {
			    //cnt=atomic_read(&gsm_rx_ready[unit]);
			    //if (cnt) atomic_set(&gsm_rx_ready[unit], (int)0);
			    flg = 1;
			}
		    }
		}
	break;
	//------------------------------------
	case 9://  ЧТЕНИЕ РЕГИСТРА СТАТУСА РК
	    if (node == MajorGSM) {
		ret = ioread8(BaseAdrRAM+RK_table[unit].addr);
		flg = 0;
	    }
	break;
	case 10:
	    ret = -1;
	    if (node == MajorGSM) {
		//ret = gsm_rxReady(unit);
		ret = atomic_read(&gsm_rx_ready[unit]);
		flg = 0;
	    }
	break;
	//-------------------------------------
	case 0x81:			// чтение 16-ти разрядное
	    wordik = ioread16(BaseAdrRAM+offsets);
	    memcpy(ibuff, &wordik, 2);
	    ret = 2;
	    flg = 1;
	break;
	    default : {
		printk(KERN_ALERT "\nKernel totoro_read ERROR (unknown command - 0x%X)\n", cmds);
		ret = -1;
	    }
    }

    if (flg) {
	if (copy_to_user(buff, ibuff, ret)) {
	    printk(KERN_ALERT "\nKernel totoro_read ERROR (copy_to_user) : cmds=%d, ret=%d, adr=%04X ---\n", cmds, ret, offsets);
	    ret = 0;
	}
    }

    return ret;
}
//***********************************************************
//		запись
//************************************************************
static ssize_t rchan_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
ssize_t ret=0;
unsigned char bytes0, bytes, bytes_st, bt, bbs;
unsigned short offsets, wordik = 0, uk, slovo, adiks;
int mycount, unit, node, mores;

    node = NODE(filp);
    unit = UNIT(filp);

    ret = count;

    memset(ibuffw, 0, mem_buf);
    if (copy_from_user(ibuffw, buff, ret)) {
	printk(KERN_ALERT "Kernel totoro_write ERROR (copy_from_user) : count=%u\n", ret);
	return -EFAULT;
    }
    bytes0 = *(ibuffw);

    switch (bytes0) {  //анализ принятой команды
	case 1: //команда 1 - write to mem 4000XXXXH 8bit
	    if (count < 4) return -EFAULT;  	// длинна должна быть не менее 4-х байт !!!
	    bytes = *(ibuffw+1);      	// low byte offsets
	    bytes_st = *(ibuffw+2);   	// high byte offsets
	    wordik = bytes_st; 
	    offsets = ((wordik << 8) | bytes);// формируем адрес смещения из 2-го и 3-го байтов
	    mycount = count - 3;
	    uk = 3;			// указатель на 4-й байт - это данные
	    while (mycount > 0) {
		bytes = *(ibuffw+uk);
		iowrite8(bytes, BaseAdrRAM+offsets);
		mycount--;// = mycount - 1;
		uk++;// = uk+1;
	    }
	break;
	case 2:
	    if (count < 2) return -EFAULT;  	// длинна должна быть не менее 2-х байт !!!
	    bytes = *(ibuffw+1);      	// data
	    iowrite8(bytes, BaseAdrRAM+adr_speed);
	    ret = 1;
	break;
	case 3://reset device
	    ret = 0;
	    switch (unit) {//0=moto 1=gps 2=ds18 3=dht11 4=adc 6=enc
		case 1://reset GPS=1
		    atomic_set(&begin, (int)0);
		    bt = 0; iowrite8(bt, BaseAdrRAM+adr_reset);
		    bt = 1; iowrite8(bt, BaseAdrRAM+adr_reset);
		    bt = 0; iowrite8(bt, BaseAdrRAM+adr_reset);
		    bt = *(ibuffw+1);// clr
		    if (bt) {//clear buffer
			mycount = mem_buf;
			bbs = (ioread8(BaseAdrRAM+adr_status)) & RDEMP;
			if (!bbs) mores = 1; else mores = 0;
			while (mores) {
			    bt = ioread8(BaseAdrRAM+adr_data);
			    mycount--;
			    bt = 0; iowrite8(bt, BaseAdrRAM+adr_mir_r);
			    bt = 1; iowrite8(bt, BaseAdrRAM+adr_mir_r);
			    bt = 0; iowrite8(bt, BaseAdrRAM+adr_mir_r);
			    bbs = (ioread8(BaseAdrRAM+adr_status)) & RDEMP;
			    if ((!bbs) && (mycount > 0)) mores = 1; else mores = 0;
			}
			ret = 2;
		    } else ret = 1;
		    atomic_set(&begin, (int)1);
		break;
		case 2://reset ds18
		    bt = 1; iowrite8(bt, BaseAdrRAM+CS_W1_WR_RST);
		    ret = 1;
		break;
		case 3://reset dht11
		    bt = 1; iowrite8(bt, BaseAdrRAM+CS_W1_WR_RST_DHT);
		    ret = 1;
		break;
		case 4://reset ADC
		    adc_reset();
		    ret = 1;
		break;
		case 6://ENC
		    enc_reset();
		    ret = 1;
		break;
	    }
	break;
	case 4://reset gsm subboard
	    bt = *(ibuffw+1);// np
	    mores = bt;
	    reset_card(mores);
	    ret = 2;
	break;
//	case 6://enable recv. data from GPS
//	    atomic_set(&begin, (int)0);
//	    ret=1;
//	break;
	case 7://запись байта по 1 wire
	    bytes = *(ibuffw+1);//данны для записи
	    offsets = (unsigned short)CS_W1_DATA;
	    iowrite8(bytes, BaseAdrRAM+offsets);		// собственно запись байта
	    offsets = (unsigned short)CS_W1_WR_ADR;
	    bt = 0; iowrite8(bt, BaseAdrRAM+offsets);
	    bt = 1; iowrite8(bt, BaseAdrRAM+offsets);
	    bt = 0; iowrite8(bt, BaseAdrRAM+offsets);
	    ret = 1;
	break;
	case 8: //команда 8 - write to mem 4000XXXXH 8bit  n bytes
	    if (count < 2) return -EFAULT;  	// длинна должна быть не менее 2-х байт !!!
	    offsets = (unsigned short)CS_W1_DATA;
	    slovo = (unsigned short)CS_W1_WR_ADR;
	    mycount = count - 1; ret = 0;
	    uk = 1;			// указатель на 1-й байт данных
	    while (mycount > 0) {
		bytes = *(ibuffw+uk);
		iowrite8(bytes, BaseAdrRAM+offsets);		// собственно запись байта
		bt = 0; iowrite8(bt, BaseAdrRAM+slovo);
		bt = 1; iowrite8(bt, BaseAdrRAM+slovo);
		bt = 0; iowrite8(bt, BaseAdrRAM+slovo);
		ret++; uk++; mycount--;
	    }
	break;
	case 9:
	    iowrite8((unsigned char)1, BaseAdrRAM+CS_W1_RD_INI);// начало тразакции чтения
	break;
	case 10://разррешить прием 9/5/4/4 байт -> Read_Start
	    ret = 0;
	    switch (unit) {
		case 1://GPS
		    bytes = *(ibuffw+1); if (bytes >= max_known_type) bytes = 0;
		    mores = bytes;
		    atomic_set(&msg_type_ind, mores);
		break;
		case 2://DS18
		    if (!atomic_read(&rd_w1_start)) {
			bytes = *(ibuffw+1);      	// надо прочитать столько байт
			mycount = bytes & 0x0f;
			if (mycount > 0) {
			    iowrite8((unsigned char)1, BaseAdrRAM+CS_W1_RD_INI);// начало тразакции чтения
			    atomic_set(&rd_w1_len_in, mycount);
			    atomic_set(&rd_w1_len, (int)0);
			    atomic_set(&rd_w1_ready, (int)0);
			    atomic_set(&rd_w1_start, (int)1);
			    ret = 1;
			}
		    }
		break;
		case 3://DHT11
		    if (!atomic_read(&rd_w1_start_dht11)) {
			bytes = *(ibuffw+1);      	// надо прочитать столько байт
			mycount = bytes & 0x0f;
			if (mycount > 0) {
			    atomic_set(&rd_w1_len_in_dht11, mycount);
			    atomic_set(&rd_w1_len_dht11, (int)0);
			    atomic_set(&rd_w1_ready_dht11, (int)0);
			    atomic_set(&rd_w1_start_dht11, (int)1);
			    ret=1;
			}
		    }
		break;
		case 4://ADC
		    bytes = (*(ibuffw+1)) & 3;
		    adc_start_conv((int)bytes);
//adc_reg_print();
		    ret = 1;
		break;
		case 5://HC-SR04
		    hcsr_start_conv(0);
		    ret = 1;
		break;
		case 6://ENC
		    atomic_set(&ready_enc, (int)0);
		    atomic_set(&start_enc, (int)1);
		    ret = 1;
		break;
		case 7://HC-SR04 (second)
		    hcsr_start_conv(1);
		    ret = 1;
		break;
	    }
	break;
	case 11://аварийный стоп приема байт -> Read_Stop
	    switch (unit) {
		case 1://disable gps_ready
		    atomic_set(&begin, (int)0);
		    ret = 1;
		break;
		case 2://DS18
		    atomic_set(&rd_w1_start, (int)0);
		    atomic_set(&rd_w1_len, (int)0);
		    atomic_set(&rd_w1_ready, (int)0);
		break;
		case 3://DHT11
		    atomic_set(&rd_w1_start_dht11, (int)0);
		    atomic_set(&rd_w1_len_dht11, (int)0);
		    atomic_set(&rd_w1_ready_dht11, (int)0);
		break;
		case 4://ADC
//adc_reg_print();
		    adc_stop_conv(chan_set);
		break;
		case 5://HC-SR04
		    hcsr_stop_conv(0);
		break;
		case 6://ENC
		    atomic_set(&ready_enc, (int)0);
		    atomic_set(&start_enc, (int)0);
		break;
		case 7://HC-SR04 (second)
		    hcsr_stop_conv(1);
		break;
	    }
	break;
	case 0x0E: //write to ctrl register
	    if (node == MajorGSM) {
		bytes = *(ibuffw+1);//MirrorData
		RK_table[unit].mir = bytes;
		iowrite8(bytes, BaseAdrRAM + RK_table[unit].addr);
		ret = 2;
	    } else ret = 0;
	break;
	case 0x0F: //GSMStartStop
	    ret = -1;
	    if (node == MajorGSM) {
		bytes = *(ibuffw+1);//cmd
		switch (bytes) {
		    case SET_GSM_START ://8
			atomic_set(&gsm_rx_begin[unit], (int)1);
			atomic_set(&gsm_rx_ready[unit], (int)0);
			ret = 0;
		    break;
		    case SET_GSM_STOP ://9
			atomic_set(&gsm_rx_begin[unit], (int)0);
			atomic_set(&gsm_rx_ready[unit], (int)0);
			ret = 0;
		    break;
		}
	    }
	break;
	case 0x16: //команда 0x16 - write massiv to module_at_command_port (for bit16 mode only)
	    if (node == MajorGSM) {
		if ((unit >= all_radio) || (count > 514)) return -EFAULT;
		mycount = count-1;
		ret = 0;
		if (mycount > 0) {
		    offsets = adiks = RK_table[unit].addr;
		    offsets += 0x10;//CS_PiML_AT_COM;
		    adiks += CS_GSM_W_AT;
		    uk = 1;			// указатель на 2-й байт - это данные
		    while (mycount > 0) {
			bytes = *(ibuffw+uk);	// взять старший байт данных
			iowrite8(bytes, BaseAdrRAM+offsets);
			mycount--;	uk++;
			bt = 0; iowrite8(bt, BaseAdrRAM+adiks);
			bt = 1; iowrite8(bt, BaseAdrRAM+adiks);
		    }
		    ret = uk - 1;
		}
	    } else return -EFAULT;
	break;
	case 0x81: //команда 81H - write to mem 4000XXXXH 16bit
	    if (count < 5) return -EFAULT;  	// длинна должна быть не менее 4-х байт !!!
	    memcpy(&offsets, ibuffw+1, 2);//адрес
	    memcpy(&slovo, ibuffw+3, 2);//данные
	    iowrite16(slovo, BaseAdrRAM+offsets);
	    ret = 2;
	break;
	    default : {
		printk(KERN_ALERT "\nKernel totoro_write ERROR (unknown command - 0x%X)\n", bytes0);
		ret = -1;
	    }
    }

    return ret;

}
//*************************************************************************************************
#ifdef HAVE_UNLOCKED_IOCTL
static long rchan_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
#else
static int rchan_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
#endif

int ret = -EINVAL, ch, unit, node, yes = 0;

    node = NODE(filp);
    unit = UNIT(filp);

    if (node == Major) {//sensors
	if (!unit) {//motor
	    switch (cmd) {
		case SET_MODE_SOFT ://1
		    ch = 1;
		break;
		    default : ch = 0;//SET_MODE_PULT//0
	    }
	    iowrite8((unsigned char)ch, BaseAdrRAM + adr_mode_motor);
	} else if (unit == 4) {//ADC
#ifdef HAVE_UNLOCKED_IOCTL
	    lock_kernel();
#endif
	    ch = arg & 3;
	    switch (cmd) {
		case ADC_CHAN_SELECT :
		    if (ch != chan_set) {
			ret = mux_chan(chan_set, 0);
			ret = mux_chan(ch, 1);
			yes = 1;
		    }
		break;
		case ADC_CHAN_FREE :
		    ret = mux_chan(ch, 0); yes = 1;
		break;
		    default : ret = -EINVAL;
	    }
#ifdef HAVE_UNLOCKED_IOCTL
	    unlock_kernel();
#endif
	} else ret = -EINVAL;
    } else if (node == MajorGSM) {
	if ((unit >= 0) && (unit < all_radio)) {
//#ifdef HAVE_UNLOCKED_IOCTL
//	    lock_kernel();
//#endif
	    switch (cmd) {
		case GET_GSM_STATUS ://1 //get vio,....
		    ret = ioread8(BaseAdrRAM + RK_table[unit].addr);
		break;
		case SET_GSM_ON ://2
		case SET_GSM_OFF ://3
		case SET_GSM_PWRON ://4
		case SET_GSM_PWROFF ://5
		case SET_GSM_RST ://6
		    RK_table[unit].mir = (unsigned char)arg;//mir
		    iowrite8(RK_table[unit].mir, BaseAdrRAM + RK_table[unit].addr);
		    ret = 0;
		break;
		case GET_GSM_READY ://есть готовность данных на чтение, можно читать буфер приемника ат-команд
		    ret = atomic_read(&gsm_rx_ready[unit]);
		    //ret=gsm_rxReady(unit);
		break;
		case SET_GSM_START :
		    atomic_set(&gsm_rx_begin[unit], (int)1);
		    atomic_set(&gsm_rx_ready[unit], (int)0);
		    ret = 0;
		break;
		case SET_GSM_STOP :
		    atomic_set(&gsm_rx_begin[unit], (int)0);
		    atomic_set(&gsm_rx_ready[unit], (int)0);
		    ret = 0;
		break;
		case SET_KT_POINT ://case 0x0D: //set commut_KT
		    ch = (unit >> 2) * 0x0040;
		    iowrite8((unsigned char)unit, BaseAdrRAM + (unsigned short)KT_POINT + (unsigned short)ch);
		    ret = 0;
		break;
	    }
//#ifdef HAVE_UNLOCKED_IOCTL
//	    unlock_kernel();
//#endif
	}
    }

//if (yes) printk(KERN_ALERT "\nKernel totoro_ioctl : cmd=%d chan=%d\n", (int)cmd, (int)arg);

    return ret;
}
//*************************************************************
static struct file_operations rchan_fops = {

  .owner   = THIS_MODULE,
  .open    = rchan_open,
  .release = rchan_release,
  .read    = rchan_read,
  .write   = rchan_write,
  .poll    = rchan_poll,
#ifdef HAVE_UNLOCKED_IOCTL
  .unlocked_ioctl = rchan_unlocked_ioctl,
#else
  .ioctl          = rchan_ioctl,
#endif
};
//************************************************************
//
//************************************************************
static void init_sio(struct rchan_sio *sio){

  dev_t dev = MKDEV(Major,0);
  dev_t devGSM = MKDEV(MajorGSM,0);

  cdev_init(&sio->cdev, &rchan_fops);
  cdev_init(&sio->cdevGSM, &rchan_fops);

  cdev_add(&sio->cdev, dev, kol_dev);
  cdev_add(&sio->cdevGSM, devGSM, all_radio);

}
//************************************************************
//
//************************************************************
static void deinit_sio(struct rchan_sio *sio) {

  cdev_del(&sio->cdev);
  cdev_del(&sio->cdevGSM);

}

static struct rchan_sio chan_sio;

// ************************************************************
//		инициализация модуля
// ************************************************************
static int __init rchan_init(void)
{

unsigned short word, word1;
unsigned int adresok;
dev_t dev;
dev_t devGSM;
int i, lp, rc;
unsigned char dl_cross_rom, bt;
u32 data;
char st[rd_w1_buf_len] = {0};

    printk(KERN_ALERT "\n");

//-----------------------   Sensors device   ------------------------------------------

    if (!Major) {
	if ((alloc_chrdev_region(&dev, 0, kol_dev, DevName)) < 0){
	    printk(KERN_ALERT "%s: Allocation device failed\n", DevName);
	    return 1;
	}
	Major = MAJOR(dev);
	printk(KERN_ALERT "%s: %d device allocated with major number %d (MOTOR,GPS,DS18,DHT11,ADC,HCSR,ENC,HCSR2)\n", DevName, kol_dev, Major);
    } else {
	if (register_chrdev_region(MKDEV(Major, 0), kol_dev, DevName) < 0){
	    printk(KERN_ALERT "%s: Registration failed\n", DevName);
	    return 1;
	}
	printk(KERN_ALERT "%s: %d devices registered\n", DevName, kol_dev);
    }

//-----------------------    GSM/3G device  ------------------------------------------

    if (!MajorGSM) {
	if ((alloc_chrdev_region(&devGSM, 0, all_radio, DevNameGSM)) < 0){
	    printk(KERN_ALERT "%s: Allocation device failed\n", DevNameGSM);
	    return 1;
	}
	MajorGSM = MAJOR(devGSM);
	printk(KERN_ALERT "%s: %d device allocated with major number %d\n", DevNameGSM, all_radio, MajorGSM);
    } else {
	if (register_chrdev_region(MKDEV(MajorGSM, 0), all_radio, DevNameGSM) < 0){
	    printk(KERN_ALERT "%s: Registration failed\n", DevNameGSM);
	    return 1;
	}
	printk(KERN_ALERT "%s: %d devices registered\n", DevNameGSM, all_radio);
    }

    init_sio(&chan_sio);
    altera_class = class_create(THIS_MODULE, "cyclon");
    for (i = 0; i < kol_dev; i++) {
	memset(st, 0, rd_w1_buf_len);	sprintf(st,"totoro%d",i);
	CLASS_DEV_CREATE(altera_class, MKDEV(Major, i), NULL, st);
    }
    gsm_class = class_create(THIS_MODULE, "gsm");
    for (i = 0; i < all_radio; i++) {
	memset(st, 0, rd_w1_buf_len);	sprintf(st,"gsm%d",i);
	CLASS_DEV_CREATE(gsm_class, MKDEV(MajorGSM, i), NULL, st);
    }

    my_dev_ready = 1;

//--------------------------------------------------------------------
	// Assign CS3 to SMC
	data = at91_sys_read(AT91_MATRIX_EBICSA);
	data &= ~(AT91_MATRIX_CS3A);
	data |= (AT91_MATRIX_CS3A_SMC);
	at91_sys_write(AT91_MATRIX_EBICSA, data);
	// Configure PIOC for using CS3
	at91_sys_write(AT91_PIOC + PIO_PDR, (1 << 14)); /* Disable Register */
	data = at91_sys_read(AT91_PIOC + PIO_PSR); /* Status Register */
	at91_sys_write(AT91_PIOC + PIO_ASR, (1 << 14)); /* Peripheral A Select Register */
	data = at91_sys_read(AT91_PIOC + PIO_ABSR); /* AB Status Register */

	// Configure SMC CS3 timings
	at91_sys_write(AT91_SMC + 0x30 + 0x0, 0x01020102);
	at91_sys_write(AT91_SMC + 0x30 + 0x4, 0x0f0d0f0d);
	at91_sys_write(AT91_SMC + 0x30 + 0x8, 0x00150015);
	at91_sys_write(AT91_SMC + 0x30 + 0xc, 0x10111003);

/*
	// Configure SMC CS4 timings
	at91_sys_write(AT91_SMC + 0x40 + 0x0, 0x01020102);
	at91_sys_write(AT91_SMC + 0x40 + 0x4, 0x0f0d0f0d);
	at91_sys_write(AT91_SMC + 0x40 + 0x8, 0x00150015);
	at91_sys_write(AT91_SMC + 0x40 + 0xc, 0x10111003);
*/
	// Request and remap i/o memory region for cs3
	if (check_mem_region(AT91_CHIPSELECT_3, 0x10000)) {
		printk(KERN_ALERT "%s: i/o memory region for NCS3 already used\n", DevName);
		goto err_out;
	}
	if (!(cs3_iomem_reg = request_mem_region(AT91_CHIPSELECT_3, 0x10000, "totoro"))) {
		printk(KERN_ALERT "%s: can't request i/o memory region for NCS3\n", DevName);
		goto err_out;
	}
	if (!(BaseAdrRAM = ioremap_nocache(AT91_CHIPSELECT_3, 0x10000))) {
		printk(KERN_ALERT "%s: can't remap i/o memory for NCS3\n", DevName);
		goto err_out;
	}

	at91_adc_clk = clk_get(NULL,"adc_clk");
        clk_enable(at91_adc_clk);
	// Request and remap i/o memory region for ADC //CS_ADC_BASE
	if (check_mem_region(AT91SAM9260_BASE_ADC, 256)) {
		printk(KERN_ALERT "%s: i/o memory region for ADC already used\n", DevName);
		goto err_out;
	}
	if (!(adc_iomem_reg = request_mem_region(AT91SAM9260_BASE_ADC, 256, "totoro"))) {
		printk(KERN_ALERT "%s: can't request i/o memory region for ADC\n", DevName);
		goto err_out;
	}
	if (!(BaseAdrRAMADC = ioremap_nocache(AT91SAM9260_BASE_ADC, 256))) {
		printk(KERN_ALERT "%s: can't remap i/o memory for ADC\n", DevName);
		goto err_out;
	}
	at91_pioc_base = ioremap_nocache(AT91_BASE_SYS + AT91_PIOC, 512);
	if(!at91_pioc_base) {
	    printk(KERN_ALERT "%s: can't remap i/o memory for PIOC\n", DevName);
	    goto err_out;
	}

//--------------------------------------------------------------------

	ibuff = kmalloc(SIZE_BUF,GFP_KERNEL);
	if (!ibuff) {
	    printk(KERN_ALERT "%s: KM for reading buffer allocation failed\n", DevName);
	    goto err_out;
	}
	ibuffw = kmalloc(SIZE_BUF,GFP_KERNEL);
	if (!ibuffw) {
	    printk(KERN_ALERT "%s: KM for writing buffer allocation failed\n", DevName);
	    goto err_out;
	}

//	printk(KERN_ALERT "%s: ADC mapping to 0x%08X (256)\n",DevName,(unsigned int)BaseAdrRAMADC);
//	printk(KERN_ALERT "%s: PIOC mapping to 0x%08X (512)\n",DevName,(unsigned int)at91_pioc_base);
//	printk(KERN_ALERT "%s: NCS3 mapping to 0x%08X (64k)\n",DevName,(unsigned int)BaseAdrRAM);

//--------------------------------------------------------------------

    //тестирование наличия кросс-платы
    cross_board_presence = 0;

    adresok = (unsigned int)(BaseAdrRAM + CS_CROSS_BOARD);
    word = 0xa5f0;
    iowrite16(word, adresok);
    msleep(1);
    word1 = ioread16(adresok);
    if (word == word1) {
	msleep(1);
	word = 0x5a0f;
	iowrite16(word, adresok);
	msleep(1);
	word1 = ioread16(adresok);
	if (word == word1) cross_board_presence = 1;
		      else printk(KERN_ALERT "\n\n%s : ......... --- Error --- ........\n\n", DevName);
    } else printk(KERN_ALERT "%s: Addr 0x%X : 0x%X | 0x%X  ERROR !\n", DevName, adresok, word, word1);

    if (cross_board_presence == 1) {
	//чтение версии прошивки Алтеры на субплате радиоканалов
	iowrite8(0, BaseAdrRAM+CS_ROMBOARD_RST);
	mdelay(1);
	iowrite8(1, BaseAdrRAM+CS_ROMBOARD_RST);
	mdelay(1);
	iowrite8(0, BaseAdrRAM+CS_ROMBOARD_RST);
	mdelay(1);
	memset(ibuff, 0, 256);
	bt = ioread8(BaseAdrRAM+CS_ROMBOARD);//холостое чтение (тип записи) - так надо
	dl_cross_rom = ioread8(BaseAdrRAM+CS_ROMBOARD);//длинна нашего сообщения
	dl_cross_rom = ioread8(BaseAdrRAM+CS_ROMBOARD);//длинна нашего сообщения -  ТАК НАДО
	dl_cross_rom = Max_Len_Rom;
	i = 0; lp = 1;
	while (lp)  {
	    bt = ioread8(BaseAdrRAM+CS_ROMBOARD);
	    ibuff[i] = bt;
	    i++;  dl_cross_rom--;
	    if ((!bt) || (!dl_cross_rom)) lp = 0;
	}
	printk(KERN_ALERT "%s: crossboard PRESENT : %s\n", DevName, ibuff);

	//------- инициализация таймера и проч.  --------------------------------------------------

	for (i = 0; i < kol_dev; i++) Device_Open[i] = 0;
	my_msec = 0;
	//GPS
	adr_reset = (unsigned short)CS_GPRS_RESET;
	adr_speed = (unsigned short)CS_SPEED;
	adr_mode_motor = (unsigned short)CS_MODE_MOTOR;
	iowrite8((unsigned char)0, BaseAdrRAM + adr_mode_motor);
	adr_status = (unsigned short)CS_GPRS_STAT;
	adr_data = (unsigned short)CS_GPRS_DATA;
	
	adr_mir_r = (unsigned short)CS_MIRROR_R_AT;
	adr_mir_w = (unsigned short)CS_MIRROR_W_AT;
	atomic_set(&rx_ready, my_msec);
	atomic_set(&varta, my_msec);
	atomic_set(&begin, my_msec);
	atomic_set(&msg_type_ind, my_msec);//"$GPMRC"
	//DS18
	atomic_set(&rd_w1_start, my_msec);
	atomic_set(&rd_w1_ready, my_msec);
	atomic_set(&rd_w1_len, my_msec);
	atomic_set(&rd_w1_len_in, my_msec);
	memset(rd_w1_data,0,rd_w1_buf_len);
	//DHT11
	atomic_set(&rd_w1_start_dht11, my_msec);
	atomic_set(&rd_w1_ready_dht11, my_msec);
	atomic_set(&rd_w1_len_dht11, my_msec);
	atomic_set(&rd_w1_len_in_dht11, my_msec);
	memset(rd_w1_data_dht11,0,rd_w1_buf_len);
	//ADC
	adc_reset();
	adc_init(chan_set);
	//HCSR04 (first and second)
	my_msec = 0;
	atomic_set(&start_hcsr,my_msec);
	atomic_set(&ready_hcsr,my_msec);
	atomic_set(&data_hcsr,my_msec);
	atomic_set(&start_hcsr2,my_msec);
	atomic_set(&ready_hcsr2,my_msec);
	atomic_set(&data_hcsr2,my_msec);
	//ENC
	enc_reset();
	//GSM/3G
	memset(&RK_table[0], 0, (sizeof(s_table) * all_radio));
//	iowrite8((unsigned char)1,BaseAdrRAM+G8_ONLY);// ste G8_ONLY mode
	for (i = 0; i < max_card; i++) {
	    word = read_card_id(i);
	    if (word != 0xffff) {
		rc = read_card_rom(i);
		if (rc > 0) printk(KERN_ALERT "%s: subboard %d : id=0x%04X, ROM '%s'\n", DevNameGSM, i+1, word, ibuff);
	    } else printk(KERN_ALERT "%s: subboard %d not present, id=0x%04X\n", DevNameGSM, i+1, word);
	}
	for (i = 0; i < all_radio; i++) {
	    Device_OpenGSM[i] = 0;
	    atomic_set(&gsm_rx_begin[i], (int)0);
	    atomic_set(&gsm_rx_ready[i], (int)0);
	    RK_table[i].addr = (unsigned short)( (((i >> 2) << 9) | ((i & 3) << 6)) + CS_PiML_STATUS);//базовый адрес канала (регистра статуса)
	    if (RK_table[i].present) {
		bt = RK_table[i].type; if (bt > 3) bt = 4;
		word = (unsigned short)RK_table[i].addr + (unsigned short)CS_GSM_R_AT;
		word1 = (unsigned short)RK_table[i].addr + (unsigned short)CS_GSM_W_AT;
		iowrite8((unsigned char)0, BaseAdrRAM + word);
		iowrite8((unsigned char)0, BaseAdrRAM + word1);
		iowrite8(RK_table[i].mir, BaseAdrRAM + (unsigned short)RK_table[i].addr);
		printk(KERN_ALERT "RK_table[%02d] : present=%d addr=0x%04x|0x%04x|0x%04x mir=0x%02x type=%d (%s)",
				i+1,
				RK_table[i].present,
				RK_table[i].addr,
				word,word1,
				RK_table[i].mir,
				RK_table[i].type,
				known_type[bt]);
		
	    }
	}
	//Timer start
	timi = timi_def;
	timi2 = timi2_def;
	init_timer(&my_timer);
	my_timer.function = MyTimer;
	my_timer.expires = jiffies + 100;	// 100 msec
	add_timer(&my_timer);

    } else {
	printk(KERN_ALERT "%s/%s: crossboard NOT PRESENT. Release all...\n", DevName, DevNameGSM);
	unregister_chrdev_region(MKDEV(Major, 0), kol_dev);
	unregister_chrdev_region(MKDEV(MajorGSM, 0), all_radio);
	printk(KERN_ALERT "%s/%s: All devices unregistered\n\n",DevName, DevNameGSM);
	deinit_sio(&chan_sio);
	goto err_out;
    }

    return 0;

err_out:

    if (my_dev_ready == 1) {
	for (i = 0; i < kol_dev; i++) CLASS_DEV_DESTROY(altera_class, MKDEV(Major, i));
	class_destroy(altera_class);
	for (i = 0; i < all_radio; i++) CLASS_DEV_DESTROY(gsm_class, MKDEV(MajorGSM, i));
	class_destroy(gsm_class);
    }

    rc = -ENOMEM;

    if (cs3_iomem_reg) release_mem_region(AT91_CHIPSELECT_3, 0x10000);
    if (adc_iomem_reg) release_mem_region(AT91SAM9260_BASE_ADC, 256);

    if (at91_pioc_base) iounmap(at91_pioc_base);
    if (BaseAdrRAM) iounmap(BaseAdrRAM);
    if (BaseAdrRAMADC) iounmap(BaseAdrRAMADC);

    if (ibuff) kfree(ibuff);
    if (ibuffw) kfree(ibuffw);

    clk_disable(at91_adc_clk);
    clk_put(at91_adc_clk);

    return rc;

}
//************************************************************
//		выгрузка модуля
//************************************************************
static void __exit rchan_exit(void)
{
int i;

    del_timer(&my_timer);		//Stop Timer

    atomic_set(&begin, (int)0);		//GPS
    atomic_set(&rx_ready, (int)0);	//DS18
    atomic_set(&varta, (int)0);		//DHT11
    adc_stop_conv(chan_set);		//ADC
    hcsr_stop_conv(0);			//HC-SR04 (first)
    hcsr_stop_conv(1);			//HC-SR04 (second)
    atomic_set(&ready_enc, (int)0);//ENC
    atomic_set(&start_enc, (int)0);//ENC

    iowrite8((unsigned char)0, BaseAdrRAM + adr_mode_motor);

    for (i = 0; i < all_radio; i++) {
	atomic_set(&gsm_rx_begin[i], (int)0);//GSM
	atomic_set(&gsm_rx_ready[i], (int)0);//GSM
    }

    if (cs3_iomem_reg) release_mem_region(AT91_CHIPSELECT_3, 0x10000);
    if (adc_iomem_reg) release_mem_region(AT91SAM9260_BASE_ADC, 256);

    if (at91_pioc_base) iounmap(at91_pioc_base);
    if (BaseAdrRAM) iounmap(BaseAdrRAM);
    if (BaseAdrRAMADC) iounmap(BaseAdrRAMADC);

    if (ibuff) kfree(ibuff);
    if (ibuffw) kfree(ibuffw);

    clk_disable(at91_adc_clk);
    clk_put(at91_adc_clk);

    unregister_chrdev_region(MKDEV(Major, 0), kol_dev);
    unregister_chrdev_region(MKDEV(MajorGSM, 0), all_radio);

    deinit_sio(&chan_sio);

    for(i = 0; i < kol_dev; i++) CLASS_DEV_DESTROY(altera_class, MKDEV(Major, i));
    class_destroy(altera_class);
    for(i = 0; i < all_radio; i++) CLASS_DEV_DESTROY(gsm_class, MKDEV(MajorGSM, i));
    class_destroy(gsm_class);

    printk(KERN_ALERT "%s: %d devices unregistered\n", DevName, kol_dev);
    printk(KERN_ALERT "%s: %d devices unregistered\n", DevNameGSM, all_radio);
    printk(KERN_ALERT "%s/%s: stop timer, release memory buffers\n\n", DevName, DevNameGSM);

    return;
}

module_init(rchan_init);
module_exit(rchan_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("SalaraSoft <a.ilminsky@gmail.com>");

