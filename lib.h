#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <string.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <ctype.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <asm/termbits.h>

#define my_block_size 344	// (sizeof(s_pack)=30) + 10 + 16 + 8 + 8 + 6(enc)+1(SANI_POST_AT)+1(reserve)= 80 +256(SANI_AT)+4(MOTOR) +4(HCSR2)
#define mem_buf	2048
#define max_line 1024
#define kol_dev 8	//0=moto 1=gps 2=ds18 3=dht11 4=ADC 5=HC-SR04 6=ENC 7=HCSR2
#define max_known_msg 6

#define max_card	1
#define all_radio	(max_card << 2)

#define tmp_buf_size 512

#define VIO    1            //  бит статуса "модуль вкл.выкл"
#define RDEMP  2            //  бит статуса "буфер для чтения COM-порта готов"
#define WREMP  4            //  бит статуса "буфер для записи в COM-порт готов"
#define MOD_RDempty  8      //  бит статуса "буфер для чтения COM-порта SIM готов"
#define MOD_WRempty  0x10   //  бит статуса "буфер для записи в COM-порт SIM готов"
#define MOD_RES 0x20        //  сигнал сброса, поступающий от модуля
#define CTS 0x40            //  сигнал CTS модема

#define ModPwrOnOff 1	    //  управление вкл.выкл питания модуля
#define ModOnOff    2       //  управление вкл.выкл модуля (конт. ON/OFF)
#define ModRstOnOff 4       //  сигнал сброса модуля (только для PiML)
#define ModeATBuff  8       //  режим синхронизации (0 - старый ражим с синхр. по 0x0d,0x0a, 
			    //  1 - новый режим без синхронизации по 0x0d,0x0a )

#define def_at		15
#define max_err_main	3

#define Max_Len_Rom     128
#define com_wr8_def	1	/* comand for device driver */
#define com_rd8_def	1	/* comand for device driver */
#define com_wr16_def	0x81	/* comand for device driver */
#define com_rd16_def	0x81	/* comand for device driver */


#define ADC_CHAN_SELECT	1
#define ADC_CHAN_FREE	2

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

#define MOTO_STOP	100
#define SET_MODE_PULT	0	//0
#define SET_MODE_SOFT	1	//1

#define _10ms		1
#define _20ms		2
#define _30ms		3
#define _40ms		4
#define _50ms		5
#define _60ms		6
#define _70ms		7
#define _80ms		8
#define _90ms		9
#define _100ms		10
#define _110ms		11
#define _120ms		12
#define _130ms		13
#define _140ms		14
#define _150ms		15
#define _160ms		16
#define _170ms		17
#define _180ms		18
#define _190ms		19
#define _200ms		20
#define _210ms		21
#define _220ms		22
#define _230ms		23
#define _240ms		24
#define _250ms		25
#define _260ms		26
#define _270ms		27
#define _280ms		28
#define _290ms		29
#define _300ms		30
#define _350ms		35
#define _360ms		36
#define _370ms		37
#define _380ms		38
#define _390ms		39
#define _400ms		40
#define _450ms		45
#define _500ms		50
#define _550ms		55
#define _600ms		60
#define _650ms		65
#define _700ms		70
#define _750ms		75
#define _800ms		80
#define _850ms		85
#define _900ms		90
#define _950ms		95
#define _1s		100
#define _1_5s		150
#define _1_6s		160
#define _1_7s		170
#define _1_8s		180
#define _1_9s		190
#define _2s		200
#define _2_1s		210
#define _2_2s		220
#define _2_3s		230
#define _2_5s		250
#define _3s		300
#define _3_5s		350
#define _4s		400
#define _4_5s		450
#define _5s		500
#define _6s		600
#define _7s		700
#define _8s		800
#define _9s		900
#define _10s		1000
#define _20s		2000
#define _30s		3000
#define _40s		4000
#define _50s		5000
#define _60s		6000
#define _1m		6000
#define _2m		(_1m * 2)
#define _3m		(_1m * 3)
#define _4m		(_1m * 4)
#define _5m		(_1m * 5)
#define _10m		(_1m * 10)


#pragma pack(push,1)
typedef struct {
    unsigned char chas;
    unsigned char min;
    unsigned char sec;
    unsigned short ms;
    unsigned char none;
} s_timka;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
    unsigned char den;
    unsigned char mes;
    unsigned char god;
} s_dimka;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
    unsigned short gradus;
    unsigned short minuta;
    unsigned short minuta2;
    unsigned char flag;//1-северная широта(N) или восточная долгота(E) / 
		  //2-южная широта(S) или западная долгота(W) /
		  //0-неопределено !
} s_coordinata;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
    unsigned short gradus;
    unsigned short minuta;
} s_course;//[16];//248.05 - Измеренное направление движения относительно истинного направления на Север(градусы)
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
    s_timka utc;//char utc[16];// 161229.487 - hhmmss.sss ? время UTC
    unsigned char status;//=1 A - данные валидны (=0 V - данные не валидны)
    s_coordinata latitude;//    char latitude[16];//4828.1251 - ddmm.mmmm - широта
    s_coordinata longitude;//  char longitude[16];//03502.8942 - dddmm.mmmm - долгота
    unsigned short speed;//0.00 -  Измеренная горизонтальная скорость (<<<--- узлы-1.852 км/ч )
    s_course course;//[16];//248.05 - Измеренное направление движения относительно истинного направления на Север(градусы)
    s_dimka date;//char date[16];//241214 - ddmmyy - дата
} s_pack;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
    unsigned char znak;
    unsigned char cel;
    unsigned char dro;
} s_ds18_temp;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
    unsigned char rh_cel;
    unsigned char rh_dro;
    unsigned char tp_cel;
    unsigned char tp_dro;
} s_dht11;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
    unsigned short cel;
    unsigned short dro;
} s_adc;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
    unsigned int way;
    unsigned short speed;
} s_enc;
#pragma pack(pop)


typedef struct
{
unsigned short	addr;//base adress (status)
unsigned	present : 1;
unsigned	type : 7;//type of module : 0=sim300 1=m10 2=sim5215 3=sim900
unsigned char	mir;
} s_table;

typedef struct
{
unsigned char faza;
unsigned char rx_faza;
unsigned char tx_faza;
unsigned short rx_len;
unsigned short tx_len;
char RX[tmp_buf_size];
unsigned char rx_zero;
char TX[tmp_buf_size];
unsigned char tx_zero;
unsigned int rx_tmr;
unsigned int tx_tmr;
unsigned char mesg;
unsigned char err_count;
unsigned char err_main;
unsigned char cmd_ind;
unsigned char creg_ind;
unsigned rx_ready : 1;
unsigned data : 1;
unsigned idle : 1;
unsigned reg_ok : 1;
unsigned block : 1;
unsigned answer : 1;
unsigned err : 1;
unsigned cmd : 1;
} s_gsm_rk;

typedef struct
{			// sani_at
unsigned char flag_;	//+0
unsigned char dl_;	//+1
char com_[54];		//+2
char txt_[200];		//+56    //итого 256 байт
} at_cmd;


typedef struct
{			// sani_motor_data
unsigned char mode;	//+0   0-pult, 1-soft
unsigned char speed;	//+1
unsigned char dir;	//+2   0-вперед,  1-назад
unsigned char onoff;	//+3   0-стоп, 1-старт
} s_motor;



extern pid_t pid_main;
extern FILE *pid_fp;
extern const char * pid_name;// = "/var/run/moto.pid";
extern const char * known_msg[max_known_msg];// = {"$GPRMC","$GPGGA","$GPGSV","$GPGLL","$GPGSA","$GPVTG"};
extern unsigned char msg_type_ind;//=0;//$GPRMC
extern unsigned char msg_type_ind_set;//=0;//$GPRMC
extern int ErrorSignal;//=0;
extern unsigned char SIGHUPs, SIGTERMs, SIGINTs, SIGKILLs, SIGSEGVs, SIGTRAPs;
extern unsigned char debug_mode, prints, shup, dst_out;
extern int loops;
extern int shmid;
extern unsigned char *my_data;
extern int fd[kol_dev];//={-1};
extern int fd_log, fd_bug, gsm_log;
extern char const * fillog;// = "log.txt";
extern char const * filbug;// = "bug.txt";
extern char const * filgsm;// = "gsm.txt";

extern unsigned char *web_uk;//=NULL;
extern unsigned char *SANI;//=NULL;//+0
extern unsigned char *SANI_WR_SPEED;//=NULL;//+1
extern unsigned char *SANI_RD_SPEED;//=NULL;//+2
//extern unsigned char *SANI_CONVERT;//=NULL;//+3
extern unsigned char *SANI_MOTOR_POST;//=NULL;//+3
extern unsigned char *SANI_W1_WR;//=NULL;//+4
extern unsigned char *SANI_W1_RD_DONE;//=NULL;//+5
extern unsigned char *SANI_W1_RD_DHT_DONE;//=NULL;//+6
extern unsigned char *SANI_ADC_CHAN;//=NULL;//+7
extern unsigned char *SANI_PACK;//=NULL;//+10
extern unsigned char *SANI_W1_RD;//=NULL;//16 bytes
extern unsigned char *SANI_W1_RD_DHT;//=NULL;//8 bytes
extern unsigned char *SANI_DS18;//=NULL;//указатель на структуру ds18_temp (пока 3 байта)
extern unsigned char *SANI_DHT11;//=NULL;//указатель на структуру dht11 (пока 4 байта)
extern unsigned char *SANI_ADC;//=NULL;//указатель на структуру adc (пока 4 байта)
extern unsigned char *SANI_HCSR;//=NULL;//указатель на 4 байта значения от датчика HC-SR04
extern unsigned char sel_chan, tmp_chan;
extern unsigned char *SANI_MSG_TYPE;//=NULL;
extern unsigned char *SANI_MSG_TYPE_SET;//=NULL;
extern unsigned char *SANI_TYPE_POST;//=NULL;
extern unsigned char *SANI_ENC_WAY;//=NULL;//4 bytes
extern unsigned char *SANI_ENC_SPEED;//=NULL;//2 bytes
extern unsigned char *SANI_POST_AT;//1 byte
extern unsigned char *SANI_RK;//1 byte
extern unsigned char *SANI_AT;//256 bytes
extern unsigned char *SANI_MOTOR_DATA;//4 bytes
extern unsigned char *SANI_HCSR2;//=NULL;//указатель на 4 байта значения от датчика HC-SR04 (second)

extern s_pack one_pack;
extern s_pack web_pack;
extern s_ds18_temp ds18_temp;
extern s_dht11 dht11;
extern s_adc adc;
extern s_enc enc;
extern s_motor motor;


extern unsigned char N_RK, N_RK_IN;

extern unsigned char OneChannel;
extern unsigned char pwronly;

extern unsigned char motor_mode;//0-PULT, 1-SOFT

extern unsigned char subboard[max_card];
extern int fd_gsm[all_radio];
extern s_table RK_table[all_radio];
extern unsigned int tmr1[all_radio];
extern unsigned char gsm_faza[all_radio];

extern s_gsm_rk gsm_rk[all_radio];

extern at_cmd ATCOM;

extern int max_at;
extern const char * commands[def_at];
extern const char * commands5215[];
extern const char * commands10[];

//******************************************************************

extern void print_tables();
extern int get_cards();
extern int get_tables(int fp);
extern void reset_card(unsigned char np);

extern unsigned int get_timer_sec(unsigned int t);
extern int check_delay_sec(unsigned int t);
extern unsigned int get_timer_msec(unsigned int t);
extern int check_delay_msec(unsigned int t);
extern void print_rk(char *st);
extern void print_str(char * st, unsigned char dst);
extern char * DTPrn();
extern void gps_log(const char *st);
extern void bug_log(const char *st);
extern void PrintErrorSignal(int es);
extern void _USR1Sig(int sig);
extern void _USR2Sig(int sig);
extern void _ReadSig(int sig);
extern void _TermSig(int sig);
extern void _IntSig(int sig);
extern void _KillSig(int sig);
extern void _SegvSig(int sig);
extern void _TrapSig(int sig);

extern int my_init_block();
extern int my_del_block();

extern void init_motor();
extern int moto_write(unsigned char bt);
extern unsigned char moto_read();
extern void SetModeMotor(int md);

extern void KT_Point(unsigned char nk);
extern unsigned char get_status(unsigned char nk);
extern int check_vio(unsigned char nk);
extern void ModulePWR_ON(unsigned char nk);
extern void ModulePWR_OFF(unsigned char nk);
extern void Module_ON(unsigned char nk);
extern void Module_OFF(unsigned char nk);
//extern void Module_RSTOFF(unsigned char nk);
extern void GSMStartStop(unsigned char nk, unsigned char cmd);
//extern int RXdone(unsigned char nk);
extern int RXbytes22(unsigned char nk, char * outs);
extern void put_AT_com(const char *st, unsigned char nk);
extern int TXbyte_all(unsigned char nk, unsigned char * st, unsigned short len);
extern unsigned char TXCOM2_all(unsigned char nk);

extern void PrintPack(unsigned char fl);
extern int parse_line(char *lin, int dline);
extern void conver_pack(s_pack * pk);
extern void help_param_show();
extern int rst_dev(unsigned char devm, unsigned char b);//0=moto 1=gps 2=ds18 3=dht11 6=enc
extern int dev_present(unsigned char devm);
extern int cmd_wr_dev(unsigned char devm, unsigned char *uk, int dln);//запись массива (не более 255 байт)
extern int ReadStart(unsigned char devm, unsigned char dl);//1-GPS, 2-DS18, 3-DHT11, 4-ADC, 5-HCSR, 6-ENC, 7-HCSR2
extern int ReadRead(unsigned char devm, unsigned char *st);
extern int ReadStop(unsigned char devm);//1-GPS, 2-DS18, 3-DHT11, 4-ADC, 5-HCSR04, 6-ENC, 7-HCSR2
extern int CreateInitvarPHP();
extern void DS18_Gradus(unsigned char *sta, unsigned char tout);
extern void DHT11_ALL(unsigned char *sta);
extern void ADC_ALL(float v);
extern void OutOfJob();
extern int select_adc_chan(unsigned char nk);

extern void send_to_web(unsigned char *da, int j);

