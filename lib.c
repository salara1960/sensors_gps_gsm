#include "lib.h"


pid_t pid_main = 0;
FILE *pid_fp;
const char *pid_name = "/var/run/moto.pid";
const char *known_msg[max_known_msg] = {"$GPRMC","$GPGGA","$GPGSV","$GPGLL","$GPGSA","$GPVTG"};
unsigned char msg_type_ind = 0;//$GPRMC
unsigned char msg_type_ind_set = 0;//$GPRMC
int ErrorSignal = 0;
unsigned char SIGHUPs = 1, SIGTERMs = 1, SIGINTs = 1, SIGKILLs = 1, SIGSEGVs = 1, SIGTRAPs = 1;
unsigned char debug_mode = 0, prints = 0, shup = 0xff, dst_out = 0;
int loops;
int shmid;
unsigned char *my_data;
int fd[kol_dev] = {-1};
int fd_log = -1, fd_bug = -1, gsm_log = -1;
char const *fillog = "log.txt";
char const *filbug = "bug.txt";
char const *filgsm = "gsm.txt";
unsigned char *web_uk = NULL;
unsigned char *SANI = NULL;//+0
unsigned char *SANI_WR_SPEED = NULL;//+1
unsigned char *SANI_RD_SPEED = NULL;//+2
//unsigned char *SANI_CONVERT = NULL;//+3
unsigned char *SANI_MOTOR_POST = NULL;//+3
unsigned char *SANI_W1_WR = NULL;//+4
unsigned char *SANI_W1_RD_DONE = NULL;//+5
unsigned char *SANI_W1_RD_DHT_DONE = NULL;//+6
unsigned char *SANI_ADC_CHAN = NULL;//+7
unsigned char *SANI_PACK = NULL;//+10
unsigned char *SANI_W1_RD = NULL;//16 bytes
unsigned char *SANI_W1_RD_DHT = NULL;//8 bytes
unsigned char *SANI_DS18 = NULL;//указатель на структуру ds18_temp (пока 3 байта)
unsigned char *SANI_DHT11 = NULL;//указатель на структуру dht11 (пока 4 байта)
unsigned char *SANI_ADC = NULL;//указатель на структуру adc (пока 4 байта)
unsigned char *SANI_HCSR = NULL;//указатель на 4 байта значения от датчика HC-SR04
unsigned char sel_chan = 3, tmp_chan = 3;
unsigned char *SANI_MSG_TYPE = NULL;
unsigned char *SANI_MSG_TYPE_SET = NULL;
unsigned char *SANI_TYPE_POST = NULL;
unsigned char *SANI_ENC_WAY = NULL;//4 bytes
unsigned char *SANI_ENC_SPEED = NULL;//2 bytes
unsigned char *SANI_POST_AT = NULL;//1 byte
unsigned char *SANI_RK = NULL;//1 byte
unsigned char *SANI_AT = NULL;//256 bytes
unsigned char *SANI_MOTOR_DATA = NULL;//4 bytes
unsigned char *SANI_HCSR2 = NULL;//указатель на 4 байта значения от датчика HC-SR04 (second)


unsigned char N_RK = 0, N_RK_IN = 0;
unsigned char OneChannel = 0;
unsigned char pwronly = 0;

unsigned char motor_mode = 0;//0-PULT, 1-SOFT

s_pack one_pack;
s_pack web_pack;
s_ds18_temp ds18_temp;
s_dht11 dht11;
s_adc adc;
s_enc enc;
s_motor motor;

unsigned char subboard[max_card] = {0};
int fd_gsm[all_radio] = {-1};
s_table RK_table[all_radio];
unsigned int tmr1[all_radio];
unsigned char gsm_faza[all_radio] = {0};
s_gsm_rk gsm_rk[all_radio];

at_cmd ATCOM;
int max_at=def_at;
const char *commands[def_at];

const char *commands5215[] = {
"ATE0",
"AT+IPR=115200;&W",
"AT+SIDET=1024;+CMGF=0",
"AT+CLIP=1;+CLIR=0",
"AT+CMEE=2;+CREG=2",
"AT+CSCS=\"IRA\"",
"AT+CSDVC=1;+CVHU=0",
"AT+CNMI=1,1,0,1,0",
"AT+GSN",
"AT+CIMI",
"AT+CSCA?",
"AT+CNUM",
"AT+CPSI?",
"AT+CREG?",
"AT+CSQ"};

const char * commands10[] = {
"ATE0",
"AT+IPR=115200;&W",
"AT+QSIDET=0;+CMGF=0",
"AT+CLIP=1;+CLIR=0",
"AT+CMEE=2;+CREG=2",
"AT+CSCS=\"IRA\"",
"AT+QAUDCH=1",
"AT+CNMI=2,1,0,1,0",
"AT+GSN",
"AT+CIMI",
"AT+CSCA?",
"AT+CNUM",
"AT+COPS?",
"AT+CREG?",
"AT+CSQ"};

//*******************************************************************************
//*******************************************************************************
//*******************************************************************************
void print_tables()
{
char dattim[64];
int i = 1024;

    if (gsm_log > 0) {
	char *st = (char *)calloc(1, i);
	if (st) {
	    memset(dattim, 0, 64); sprintf(dattim, "%s", DTPrn());
	    sprintf(st,"%s | RK_tables :\n", dattim);
	    for (i = 0; i < all_radio; i++) {
		if (RK_table[i].present)
		    sprintf(st+strlen(st),"[%02d] : present=%d type=%d addr=0x%04x mir=%02x\n",
				i+1,
				RK_table[i].present,
				RK_table[i].type,
				RK_table[i].addr,
				RK_table[i].mir);
	    }
	    write(gsm_log, st, strlen(st));
	    free(st);
	}
    }
}
//*******************************************************************************
int get_cards()
{
int ret;
unsigned int res = 3;//читаем subboard[]

//    memset((unsigned char *)&subboard[0],0,max_card);

    ret = read(fd[0], subboard, res);

    if (ret != max_card) ret = -1; else ret = 0;

    return ret;
}
//*******************************************************************************
int get_tables(int fp)
{
int ret, len;
unsigned int res = 4;//читаем структуру RK_table[nk]

    len = sizeof(s_table) * all_radio;

    memset((unsigned char *)&RK_table[0], 0, len);

    ret = read(fd[0], (unsigned char *)&RK_table[0], res);

    if (ret != len) ret =- 1; else ret = 0;

    if (fp) print_tables();

    return ret;
}
//*******************************************************************************
void reset_card(unsigned char np)
{
unsigned char bu[2] = {4,0};

    bu[1] = np;
    write(fd[0], bu, 2);

}
//*******************************************************************************
unsigned int get_timer_sec(unsigned int t)
{
    return ((unsigned int)time(NULL) + t);
}
//******************************************************************
int check_delay_sec(unsigned int t)
{
    if ((unsigned int)time(NULL) >= t)  return 1; else return 0;
}
// **************************************************************************
unsigned int get_timer_msec(unsigned int t)
{
unsigned char m[4];

    return (read(fd[0], m, 0) + t);
}
//******************************************************************
int check_delay_msec(unsigned int t)
{
unsigned char m[4];

    if (read(fd[0], m, 0) >= t ) return 1; else return 0;
}
// **************************************************************************
void print_str(char * st, unsigned char dst)
{
    if (prints) {
	if (dst) write(fd_log, st, strlen(st)); else printf(st);
    }
}
//------------------------------------------------------------------------
char *DTPrn()
{
char *st;
char st1[64];
struct tm *ctimka;
time_t it_ct;
struct timeval tvl;
int i_day, i_mon;
int i_hour, i_min, i_sec;

    gettimeofday(&tvl,NULL);
    it_ct = tvl.tv_sec;
    ctimka = localtime(&it_ct);
    i_day = ctimka->tm_mday;	i_mon = ctimka->tm_mon+1;
    i_hour = ctimka->tm_hour;	i_min = ctimka->tm_min;	i_sec = ctimka->tm_sec;
    memset(st1, 0, 64);
    sprintf(st1,"%02d.%02d %02d:%02d:%02d.%03d | ",i_day, i_mon, i_hour, i_min, i_sec, (int)(tvl.tv_usec/1000));
    st = &st1[0];

    return (st);
}
//------------------------------------------------------------------------
void print_rk(char * st)
{
char dattim[64];

    if (gsm_log > 0) {
	memset(dattim, 0, 64); sprintf(dattim, "%s", DTPrn());
	write(gsm_log, dattim, strlen(dattim));
	write(gsm_log, st, strlen(st));
    }
}
//--------------------------------------------------------------------------
void gps_log(const char *st)
{
char dattim[64];

    memset(dattim, 0, 64);
    sprintf(dattim, "%s", DTPrn());
    print_str(dattim, dst_out);//0-stdout , 1-file
    print_str((char *)st, dst_out);//0-stdout , 1-file
}
//-------------------------------------------------------------------------
void bug_log(const char *st)
{
char dattim[64];

    if ((fd_bug > 0) && debug_mode) {
	memset(dattim, 0, 64); sprintf(dattim, "%s", DTPrn());
	write(fd_bug, dattim, strlen(dattim));
	write(fd_bug, st, strlen(st));
    }

}
//--------------------------------------------------------------------------
void PrintErrorSignal(int es)
{
char stz[80] = {0};

    sprintf(stz,"GOT SIGNAL : %d\n", es);
    bug_log(stz);
}
//--------------------  function for recive SIGNAL from system -----------
void _ReadSig(int sig)
{
    ErrorSignal = SIGHUP;
    PrintErrorSignal(ErrorSignal);
    shup = ~shup;
    debug_mode = ~debug_mode;
}
//--------------------------------------------------------------------------
void _TermSig(int sig)
{
    if (SIGTERMs) {
	SIGTERMs = 0;
	print_str("SIGTERM : termination signal (term)\n",dst_out);
	ErrorSignal = SIGTERM;
	PrintErrorSignal(ErrorSignal);
    }
    loops = 0;
}
//--------------------------------------------------------------------------
void _IntSig(int sig)
{
    if (SIGINTs) {
	SIGINTs = 0;
	print_str("SIGINT : interrupt from keyboard (term)\n",dst_out);
	ErrorSignal = SIGINT;
	PrintErrorSignal(ErrorSignal);
    }
    loops = 0;
}
//----------------------------------------------------------------------------------
void _KillSig(int sig)
{
    if (SIGKILLs) {
	SIGKILLs = 0;
	print_str("SIGKILL : kill signal (term)\n",dst_out);
	ErrorSignal = SIGKILL; 
	PrintErrorSignal(ErrorSignal);
    }
    loops = 0;
}
//--------------------------------------------------------------------------
void _SegvSig(int sig)
{
    if (SIGSEGVs) {
	SIGSEGVs = 0;
	print_str("SIGSEGV : invalid memory reference (core)\n",dst_out);
	ErrorSignal = SIGSEGV;
	PrintErrorSignal(ErrorSignal);
    }
    loops = 0;
}
//--------------------------------------------------------------------------
void _TrapSig(int sig)
{
    if (SIGTRAPs) {
	SIGTRAPs = 0;
	print_str("SIGTRAP : trace/breakpoint trap (core)\n",dst_out);
	ErrorSignal = SIGTRAP;
	PrintErrorSignal(ErrorSignal);
    }
    loops = 0;
}
//--------------------------------------------------------------------------
void _USR1Sig(int sig)
{
unsigned char ch, findik = 1;

    ch = N_RK; ch++; if (ch >= all_radio) ch = 0;

    if (!RK_table[ch].present) {
	while ((findik) && (!RK_table[ch].present)) {
	    ch++;
	    if (ch >= all_radio) {
		ch = 0; findik = 0;
	    }
	}
    }

    N_RK = ch;
    ch = (*(unsigned char *)SANI_RK); if ((ch < all_radio) && (ch != N_RK)) N_RK = ch;
//    for (ch=0; ch<all_radio; ch++) {
//	if (ch==N_RK) gsm_rk[ch].block=0; else gsm_rk[ch].block=1;
//    }
}
//-----------------------------------------------------------------------
void _USR2Sig(int sig)
{
//    PrnRK_tabl(N_RK);
}
//------------------ Init shared memory block -------------------------------------------
int my_init_block()
{
key_t key; 		/* key to be passed to shmget() */
int shmflg; 		/* shmflg to be passed to shmget() */
int it_size; 		/* size to be passed to shmget() */
char chaka[80] = {0};

    it_size = my_block_size;
    shmflg = (0664 | IPC_CREAT);
    key = ftok("/etc/passwd", 'R');
    shmid = shmget(key, it_size, shmflg);
    if (shmid < 0) {
	sprintf(chaka,"shmget: shmget failed\n");
	bug_log(chaka);
	return -1;
    } else {
	sprintf(chaka,"shmget: shmget returned %d\n", shmid);
	bug_log(chaka);
    }
    my_data = shmat(shmid, NULL, SHM_R | SHM_W);

    if (my_data == (unsigned char *) -1) {
	memset(chaka, 0, 80);
	sprintf(chaka,"shmat: attach segment to our data space failed\n");
	bug_log(chaka);
	return -1;
    }

    return 0;
}
//--------------------  Release shared memory block  --------------------------
int my_del_block()
{
char chaka[128] = {0};

    if (shmdt(my_data) == -1) {
	sprintf(chaka,"shmdt: detaching segment to our data space failed\n");
	bug_log(chaka);
	return -1;
    }
    memset(chaka, 0, sizeof(chaka));
    if (shmctl(shmid, IPC_RMID, NULL) != 0 ) {
	sprintf(chaka,"shmctl: delete shm segment ERROR\n");
	bug_log(chaka);
	return -1;
    } else {
        sprintf(chaka,"shmctl: Shm segment released. Bye...\n");
        bug_log(chaka);
	my_data = NULL;
	return 0;
    }
}
//**********************************************************************************************
//**********************************************************************************************
//------------------------------  check subboard card -----------------------------------
// *************************************************************************
// *************************************************************************
//--------------------------------------------------------------------------
void init_motor()
{
    motor.mode = 0;	//pult
    motor.speed = 100;	//нулевая скорость
    motor.dir = 0;	//направление вперед
    motor.onoff = 0;	//manual
    memcpy(SANI_MOTOR_DATA, (unsigned char *)&motor, sizeof(s_motor));
    moto_write(motor.speed);
}
//			MOTOR functions
int moto_write(unsigned char bt)
{
unsigned char bf[2] = {2,100};
int ret;

    bf[1] = bt; ret = write(fd[0], bf, 2);//moto
    motor.speed = bt;
    return ret;
}
//--------------------------------------------------------------------------
unsigned char moto_read()
{
unsigned char bf[2] = {0,0};

    read(fd[0], bf, 2);
    motor.speed = bf[0];
    return (bf[0]);

}
//--------------------------------------------------------------------------
void SetModeMotor(int md)
{
    ioctl(fd[0], md, 0);
    motor_mode = (unsigned char)md;
    motor.mode = motor_mode;
}
// *************************************************************************
void KT_Point(unsigned char nk)
{
    ioctl(fd_gsm[nk], SET_KT_POINT, nk);
}
// *************************************************************************
unsigned char get_status(unsigned char nk)
{
    return ((unsigned char)ioctl(fd_gsm[nk], GET_GSM_STATUS, nk));
}
// *************************************************************************
int check_vio(unsigned char nk)
{
    return (ioctl(fd_gsm[nk], GET_GSM_STATUS, nk) & VIO);
}
// *************************************************************************
void ModulePWR_ON(unsigned char nk)
{
const char *laka = "ModulePWR_ON\n";
char lll[80] = {0};

    RK_table[nk].mir &= (ModPwrOnOff^0xFF);//	bit0 = 0

    ioctl(fd_gsm[nk], SET_GSM_PWRON, RK_table[nk].mir);

    if (nk == N_RK) {
	if (gsm_log > 0) {
	    sprintf(lll, "[%02d] [%02X] %s", nk+1, RK_table[nk].mir, laka);
	    write(gsm_log, DTPrn(), strlen(DTPrn()));
	    write(gsm_log, lll, strlen(lll));
	}
    }

}
// *************************************************************************
//   процедура выключения питания модуля
// *************************************************************************
void ModulePWR_OFF(unsigned char nk)
{
const char *laka = "ModulePWR_OFF\n";
char lll[80] = {0};

    RK_table[nk].mir = (RK_table[nk].mir | ModPwrOnOff);		// bit0 = 1

    ioctl(fd_gsm[nk], SET_GSM_PWROFF, RK_table[nk].mir);

    if (nk == N_RK) {
	if (gsm_log > 0) {
	    sprintf(lll, "[%02d] [%02X] %s", nk+1, RK_table[nk].mir, laka);
	    write(gsm_log, DTPrn(), strlen(DTPrn()));
	    write(gsm_log, lll, strlen(lll));
	}
    }

}
// *************************************************************************
//   процедура подачи пассивного уровня на вход ON/OFF модуля 
// *************************************************************************
void Module_ON(unsigned char nk)
{
const char *laka = "Module_ON\n";
char lll[80] = {0};

    RK_table[nk].mir |= ModOnOff;		//bit1=1

    ioctl(fd_gsm[nk], SET_GSM_ON, RK_table[nk].mir);

    if (nk == N_RK) {
	if (gsm_log > 0) {
	    sprintf(lll, "[%02d] [%02X] %s", nk+1, RK_table[nk].mir, laka);
	    write(gsm_log, DTPrn(), strlen(DTPrn()));
	    write(gsm_log, lll, strlen(lll));
	}
    }

}
// *************************************************************************
//   процедура подачи активного уровня на вход ON/OFF модуля GR47/PiML
// *************************************************************************

void Module_OFF(unsigned char nk)
{
const char *laka = "Module_OFF\n";
char lll[80]={0};

    RK_table[nk].mir &= (ModOnOff ^ 0xFF);		//bit1 = 0

    ioctl(fd_gsm[nk], SET_GSM_OFF, RK_table[nk].mir);

    if (nk == N_RK) {
	if (gsm_log > 0) {
	    sprintf(lll, "[%02d] [%02X] %s", nk+1, RK_table[nk].mir, laka);
	    write(gsm_log, DTPrn(), strlen(DTPrn()));
	    write(gsm_log, lll, strlen(lll));
	}
    }

}
//--------------------------------------------------------------------
void GSMStartStop(unsigned char nk, unsigned char cmd)
{
    ioctl(fd_gsm[nk], cmd, nk);
}
//----------------------------------------------------------------------
int RXbytes22(unsigned char nk, char * outs)
{
char bu[514] = {0};
unsigned int len = 0, rz = 8;//команда 0x08 - конец пакета 0x0d

    len = read(fd_gsm[nk], bu, rz);

    if (len > 512) len = 512;

    if (len > 0)
	memcpy(outs, bu, len);
    else
	len = 0;

    return len;
}
// *************************************************************************
void put_AT_com(const char *st, unsigned char nk)
{
    memset(gsm_rk[nk].TX, 0, tmp_buf_size); gsm_rk[nk].tx_len = 0;

    sprintf((char *)gsm_rk[nk].TX, "\r\n%s\r\n", st);

    gsm_rk[nk].tx_faza = 1; //активизировать выдачу команды модулю

}
// *************************************************************************
int TXbyte_all(unsigned char nk, unsigned char * st, unsigned short len)
{
unsigned char ob[514];
int i, ret = 0;

    i = len+1; if (i > 514) i = 514;

    ob[0] = 0x16;

    memcpy(&ob[1], st, i-1);

    ret = write(fd_gsm[nk], ob, i);

    return ret;
}
// *************************************************************************
unsigned char TXCOM2_all(unsigned char nk)
{
char stx[1024];
char *uk;
unsigned short dl;
int rt = 0;

    dl = strlen((char *)gsm_rk[nk].TX);

    rt = TXbyte_all(nk, (unsigned char *)&gsm_rk[nk].TX[0], dl);//записываем на передачу

    if ((rt > 0) && (rt <= 512)) gsm_rk[nk].tx_len = rt;

    if (nk == N_RK) {
	if ((gsm_rk[nk].TX[0] == 0x0a) || (gsm_rk[nk].TX[0] == 0x0d)) { 
	    uk = (char *)&gsm_rk[nk].TX[2];
	    dl = rt - 2;
	} else {
	    uk = (char *)&gsm_rk[nk].TX[0];
	    dl = rt-1;
	}
	gsm_rk[nk].TX[dl] = 0;
	memset(stx, 0, 1024);
	sprintf(stx, "[%02d] tx: %s\n", nk+1, uk);
	print_rk(stx);
    }

    memset(gsm_rk[nk].TX, 0, tmp_buf_size); gsm_rk[nk].tx_len = 0;

    return 0;
}
//**********************************************************************************************
//**********************************************************************************************
//**********************************************************************************************
void PrintPack(unsigned char fl)
{
char dattim[64], grd = 0x9C;
float r;

    if (fd_log <= 0) return;
/*
s_timka utc;//char utc[16];// 161229.487 - hhmmss.sss ? время UTC
    unsigned char one_pack.utc.chas
    unsigned char one_pack.utc.min
    unsigned char one_pack.utc.sec
    unsigned short one_pack.utc.ms
    unsigned char one_pack.utc.none
unsigned char status;//=1 A - данные валидны (=0 V - данные не валидны)
s_coordinata latitude;//    char latitude[16];//4828.1251 - ddmm.mmmm - широта
    unsigned short one_pack.latitude.gradus;
    unsigned short one_pack.latitude.minuta;
    unsigned short one_pack.latitude.minuta2;
    unsigned char one_pack.latitude.flag;
s_coordinata longitude;//  char longitude[16];//03502.8942 - dddmm.mmmm - долгота
    unsigned short one_pack.longitude.gradus;
    unsigned short one_pack.longitude.minuta;
    unsigned short one_pack.longitude.minuta2;
    unsigned char one_pack.longitude.flag;
unsigned short one_pack.speed;//0.00 -  Измеренная горизонтальная скорость (<<<--- узлы-1.852 км/ч )
s_course course;//[16];//248.05 - Измеренное направление движения относительно истинного направления на Север(градусы)
    unsigned short one_pack.course.gradus;
    unsigned short one_pack.course.minuta;
s_dimka date;//char date[16];//241214 - ddmmyy - дата
    unsigned char one_pack.date.den;
    unsigned char one_pack.date.mes;
    unsigned char one_pack.date.god;
*/
    if (one_pack.status) {
	char *st = (char *)calloc(1, 512); if (!st) return;
	memset(dattim, 0, 64); sprintf(dattim, "%s", DTPrn());
	sprintf(st, "%sGPS:\n", dattim);
	sprintf(st+strlen(st), "\tutc:\t%02d/%02d/%02d %02d:%02d:%02d.%03d\n",
	    one_pack.date.den, one_pack.date.mes, one_pack.date.god,
	    one_pack.utc.chas, one_pack.utc.min, one_pack.utc.sec, one_pack.utc.ms);
	memset(dattim, 0, 64);
	if (one_pack.latitude.flag == 1) sprintf(dattim,"north latitude");
	else
	if (one_pack.latitude.flag == 2) sprintf(dattim,"south latitude");
	else sprintf(dattim,"??? latitude");
	sprintf(st+strlen(st),"\t\t%d%c %d' %d\" %s\n",
		one_pack.latitude.gradus, grd, one_pack.latitude.minuta, one_pack.latitude.minuta2, dattim);
	memset(dattim, 0, 64);
	if (one_pack.longitude.flag == 1) sprintf(dattim,"east longitude");
	else
	if (one_pack.longitude.flag == 2) sprintf(dattim,"west longitude");
	else sprintf(dattim,"??? longitude");
	sprintf(st+strlen(st),"\t\t%d%c %d' %d\" %s\n",
		one_pack.longitude.gradus, grd, one_pack.longitude.minuta, one_pack.longitude.minuta2, dattim);
	r = one_pack.speed; r /= 1000;
	sprintf(st+strlen(st),"\tspeed:\t%.03f km/h (%d m/h)\n", r, one_pack.speed);
	sprintf(st+strlen(st),"\tcourse:\t%d%c %d'\n",one_pack.course.gradus, grd, one_pack.course.minuta);

	print_str(st, fl);

	free(st);
    }
}
//----------------------------------------------------------------------------------
int parse_line(char *lin, int dline)
{
int cnt = 0, len, dl, done = 0, k, i1, i2, delitel;
char *uk_start = NULL, *uk = NULL, *uke = NULL;
char tmp[max_line], one[32], param[16];
float r, r2;

    len = dline; if ((len < 7) || (len >= max_line)) return cnt;
    memset(tmp, 0, max_line);
    memcpy(tmp, lin, len);
    len = strlen(tmp);
    uk_start = &tmp[0];
    uk = strstr(uk_start,"$GPRMC,");
    if (uk) {
	memset((unsigned char *)&one_pack.utc.chas, 0, sizeof(s_pack));
	uk_start = uk + 7;//указатель на UTC
	while (!done) {
	    uk = strchr(uk_start,',');
	    if (!uk) uk = strstr(uk_start,"\r\n");
	    if (uk) {
		dl = uk - uk_start;
		if (dl > 0) {
		    dl &= 0x1f;//dl<=31
		    memset(one, 0, 32);
		    memcpy(one, uk_start, dl);
		    switch (cnt) {
			case 0:
			    uke = strchr(one,'.');
			    if (uke) {
				memset(param, 0, 16); memcpy(param, one, 2); one_pack.utc.chas = (unsigned char)atoi(param);//часы
				memset(param, 0, 16); memcpy(param, &one[2], 2); one_pack.utc.min = (unsigned char)atoi(param);//минуты
				memset(param, 0, 16); memcpy(param, &one[4], 2); one_pack.utc.sec = (unsigned char)atoi(param);//секунды
				uke++; one_pack.utc.ms = (unsigned short)atoi(uke);//милисекунды
			    }
			break;
			case 1:
			    if (one[0] != 'A') { done = 1; cnt = 0; } else one_pack.status = 1;
			break;
			case 2://Широта
			    uke = strchr(one,'.');
			    if (uke) {
				memset(param, 0, 16); memcpy(param, one, 2); one_pack.latitude.gradus = (unsigned short)atoi(param);//grad
				memset(param, 0, 16); memcpy(param, &one[2], 2); one_pack.latitude.minuta = (unsigned short)atoi(param);
				uke++; k = atoi(uke); 
				dl = strlen(uke); if (dl > 6) dl = 6;
				delitel = 1; for (i1 = 0; i1 < dl; i1++) delitel *= 10;
				//delitel=10000 - для старого модуля,  delitel=1000000 - для нового модуля
				r = k; r /= delitel; r *= 60; k = r;
				one_pack.latitude.minuta2 = (unsigned short)k;
			    }
			break;
			case 3://Широта
			    if (one[0]=='N') one_pack.latitude.flag = 1;//Северная широта
			    else
			    if (one[0]=='S') one_pack.latitude.flag = 2;//Южная широта
			break;
			case 4://Долгота
			    uke = strchr(one,'.');
			    if (uke) {
				memset(param, 0, 16); memcpy(param, one, 3); one_pack.longitude.gradus = (unsigned short)atoi(param);//grad
				memset(param, 0, 16); memcpy(param, &one[3], 2); one_pack.longitude.minuta = (unsigned short)atoi(param);
				uke++; k = atoi(uke);
				dl = strlen(uke); if (dl > 6) dl = 6;
				delitel = 1; for (i1 = 0; i1 < dl; i1++) delitel *= 10;
				//delitel=10000 - для старого модуля,  delitel=1000000 - для нового модуля
				r = k; r /= delitel; r *= 60; k = r;
				one_pack.longitude.minuta2 = (unsigned short)k;
			    }
			break;
			case 5://Долгота
			    if (one[0] == 'E') one_pack.longitude.flag = 1;//Восточная долгота
			    else
			    if (one[0] == 'W') one_pack.longitude.flag = 2;//Западная долгота
			break;
			case 6://speed
			    uke = strchr(one,'.');
			    if (uke) {
				uke++;
				i2 = atoi(uke);//дробная часть
				r2 = i2;
				*(char *)(uke-1) = '\0';
				i1 = atoi(one);//целая часть
				r = i1;
				r2 /= 100; r += r2; r *= 1852; k = r;
				one_pack.speed = (unsigned short)k;
			    }
			break;
			case 7:
			    uke = strchr(one,'.');
			    if (uke) {
				uke++;
				i2 = atoi(uke);//дробная часть
				r2 = i2; r2 /= 100; r2 *= 60; k = r2;
				*(char *)(uke-1) = '\0';
				i1 = atoi(one);//целая часть
				one_pack.course.gradus = (unsigned short)i1;
				one_pack.course.minuta = (unsigned short)k;
			    }
			break;
			case 8://date
			    memset(param, 0, 16); memcpy(param, one, 2); one_pack.date.den = (unsigned char)atoi(param);//день
			    memset(param, 0, 16); memcpy(param, &one[2], 2); one_pack.date.mes = (unsigned char)atoi(param);//месяц
			    memset(param, 0, 16); memcpy(param, &one[4], 2); one_pack.date.god = (unsigned char)atoi(param);//год
			    done = 1;
			break;
		    }
		    uk_start = uk + 1;
		    if (!done) if (cnt < 8) cnt++;
		} else done = 1;
	    } else done = 1;
	}
    }

    return cnt;
}
//----------------------------------------------------------------------------------
void conver_pack(s_pack * pk)
{
s_pack pks;

    memcpy((unsigned char *)&pks, (unsigned char *)&one_pack, sizeof(s_pack));

    pks.utc.ms = htons(pks.utc.ms);

    pks.latitude.gradus  = htons(pks.latitude.gradus);
    pks.latitude.minuta  = htons(pks.latitude.minuta);
    pks.latitude.minuta2 = htons(pks.latitude.minuta2);

    pks.longitude.gradus = htons(pks.longitude.gradus);
    pks.longitude.minuta = htons(pks.longitude.minuta);
    pks.longitude.minuta2 = htons(pks.longitude.minuta2);

    pks.speed = htons(pks.speed);

    pks.course.gradus = htons(pks.course.gradus);
    pks.course.minuta = htons(pks.course.minuta);

    memcpy((unsigned char *)pk, (unsigned char *)&pks, sizeof(s_pack));

}
//*************************************************************************
void help_param_show()
{
int len = 1024;

    char *inf = (char *)calloc(1, len);
    if (inf) {
	sprintf(inf,"\nSupport keys:\n\tdebug\t\t- debug mode (with print src string)\n");
	sprintf(inf+strlen(inf),"\tsize\t\t- show sizeof(s_pack)\n");
//	sprintf(inf+strlen(inf),"\tconvert\t\t- convert unsigned short data to network format (htons)\n");
	sprintf(inf+strlen(inf),"\tprint\t\t- stdout ON (default)\n");
	sprintf(inf+strlen(inf),"\tnoprint\t\t- stdout OFF\n");
	sprintf(inf+strlen(inf),"\tnodebug\t\t- write to bug_log.txt OFF\n");
	sprintf(inf+strlen(inf),"\thelp or --help\t- show this message\n");
	sprintf(inf+strlen(inf),"\r\n");
	printf(inf);
	free(inf);
    }
}
//**********************************************************************************
int rst_dev(unsigned char devm, unsigned char b)//0=moto 1=gps 2=ds18 3=dht11 6=enc
{
char bu[2] = {3,0};

    if (devm >= kol_dev) return -1;

    bu[1] = b;//for reset GPS only

    return (write(fd[devm], bu, 2));
}
//**********************************************************************************
int dev_present(unsigned char devm)
{

    if (devm >= kol_dev) return 0;

    return (read(fd[devm], NULL, (int)5));//1-на шине есть устройство, 0-никого нет дома

}
//**********************************************************************************
int cmd_wr_dev(unsigned char devm, unsigned char *uk, int dln)//запись массива (не более 255 байт)
{
unsigned char bu[16] = {0};

    if (devm >= kol_dev) return -1;
    if (dln <= 0) return 0;

    bu[0] = 8;
    memcpy(&bu[1], uk, dln & 0x0f);
    return (write(fd[devm], bu, dln+1));

}
// **************************************************************************
int ReadStart(unsigned char devm, unsigned char dl)//1-GPS, 2-DS18, 3-DHT11, 4-ADC, 5-HCSR, 6-ENC, 7-HCSR2
{
unsigned char bu[2] = {0};

    bu[0] = 10;
    bu[1] = dl;//if devm==1 (GPS) dl-msg_type_ind
    return (write(fd[devm], bu, 2));

}
// **************************************************************************
int ReadRead(unsigned char devm, unsigned char *st)
{
int ret = 0;
unsigned int rs = 8;//devm: 1-GPS  2-DS18  3-DHT11  4-ADC  5-HCSR  6-ENC  7-HCSR2
unsigned char bu[512] = {0};

    if (devm >= kol_dev) return -1;

    ret = read(fd[devm], bu, rs);

    if ((devm == 5) || (devm == 7)) return ret;

    if ((devm < 4) || (devm == 6)) {//не ADC и не HCSR04
	if (devm != 1) ret &= 0x0f;//не GPS
	if (ret > 0) memcpy(st, bu, ret);
    }

    return ret;

}
// **************************************************************************
int ReadStop(unsigned char devm)//1-GPS, 2-DS18, 3-DHT11, 4-ADC, 5-HCSR04, 6-ENC, 7-HCSR2
{
unsigned char bu[2] = {11,0};

    return (write(fd[devm], bu, 1));

}
// **************************************************************************
int CreateInitvarPHP()
{
int fdf, ret = -1, i, j;
char name[64] = {0};

    if (!my_data) return ret;

    char *str = (char *)calloc(1, 2048);
    if (str) {
	sprintf(name, "/www/root/moto/imoto.php");
	fdf = open(name, O_RDWR | O_CREAT | O_TRUNC, 0664);
	if (fdf > 0) {
	    j = i = 0;
	    sprintf(str,"<?php\n");
	    sprintf(str+strlen(str),"$my_block_size=%d;\n",my_block_size);
	    sprintf(str+strlen(str),"$sizeofpack=%d;\n",sizeof(s_pack));//28
	    sprintf(str+strlen(str),"$ds18size=%d;\n",sizeof(s_ds18_temp));//3
	    sprintf(str+strlen(str),"$dht11size=%d;\n",sizeof(s_dht11));//4
	    sprintf(str+strlen(str),"$adcsize=%d;\n",sizeof(s_adc));//4
	    sprintf(str+strlen(str),"$sani=%d;\n",i);//SANI
	    SANI = my_data;
	    i++;
	    sprintf(str+strlen(str),"$sani_wr_speed=%d;\n",i);//+1
	    SANI_WR_SPEED = my_data + i;
	    i++;
	    sprintf(str+strlen(str),"$sani_rd_speed=%d;\n",i);//+2
	    SANI_RD_SPEED = my_data + i;
	    i++;
	    //sprintf(str+strlen(str),"$sani_convert=%d;\n",i);//+3
	    sprintf(str+strlen(str),"$sani_motor_post=%d;\n",i);//+3
	    //SANI_CONVERT = my_data + i;
	    SANI_MOTOR_POST = my_data + i;
	    i++;
	    sprintf(str+strlen(str),"$sani_w1_wr=%d;\n",i);//+4
	    SANI_W1_WR = my_data + i;
	    i++;
	    sprintf(str+strlen(str),"$sani_w1_rd_done=%d;\n",i);//+5
	    SANI_W1_RD_DONE = my_data + i;
	    i++;
	    sprintf(str+strlen(str),"$sani_w1_rd_dht_done=%d;\n",i);//+6
	    SANI_W1_RD_DHT_DONE = my_data + i;
	    i++;
	    sprintf(str+strlen(str),"$sani_adc_chan=%d;\n",i);//+7
	    SANI_ADC_CHAN = my_data + i;
	    sprintf(str+strlen(str),"$sani_pack=%d;\n",j+8);//+8
	    SANI_PACK = my_data + j + 8;
	    j = j + 8 + sizeof(s_pack);//len+15 byte of data
	    sprintf(str+strlen(str),"$sani_w1_rd=%d;//len+15 byte of data = 16\n",j);//len+15 byte of data = 16
	    SANI_W1_RD = my_data + j;//len=1+9=10
	    j += 10;
	    SANI_DS18 = my_data + j;//len=3
	    sprintf(str+strlen(str),"$sani_ds18=%d;//len=3 (znak,cel,dro)\n",j);//len=3 bytes (znak,cel,dro)
	    j += 3;
	    sprintf(str+strlen(str),"$sani_w1_rd_dht=%d;//len+7 byte of data = 8\n",j);//len+5 byte of data = 6
	    SANI_W1_RD_DHT = my_data + j;
	    j += 6;
	    sprintf(str+strlen(str),"$sani_dht11=%d;//len=4 bytes\n",j);//len=4 bytes
	    SANI_DHT11 = my_data + j;
	    j += 4;
	    sprintf(str+strlen(str),"$sani_adc=%d;//len=4 bytes\n",j);//len=4 bytes
	    SANI_ADC = my_data + j;
	    j += 4;
	    sprintf(str+strlen(str),"$sani_hcsr=%d;//len=4 bytes\n",j);//len=4 bytes
	    SANI_HCSR = my_data + j;
	    j += 4;
	    sprintf(str+strlen(str),"$sani_hcsr2=%d;//len=4 bytes\n",j);//len=4 bytes
	    SANI_HCSR2 = my_data + j;
	    j += 4;
	    sprintf(str+strlen(str),"$sani_msg_type=%d;//len=1 bytes\n",j);//len=1 byte
	    SANI_MSG_TYPE = my_data + j;
	    j += 1;
	    sprintf(str+strlen(str),"$sani_msg_type_set=%d;//len=1 bytes\n",j);//len=1 byte
	    SANI_MSG_TYPE_SET = my_data + j;
	    j += 1;
	    sprintf(str+strlen(str),"$sani_msg_type_post=%d;//len=1 bytes\n",j);//len=1 byte
	    SANI_TYPE_POST = my_data + j;
	    j += 1;
	    sprintf(str+strlen(str),"$sani_enc_way=%d;//len=4 bytes\n",j);//len=4 bytes
	    SANI_ENC_WAY = my_data + j;
	    j += 4;
	    sprintf(str+strlen(str),"$sani_enc_speed=%d;//len=2 bytes\n",j);//len=2 bytes
	    SANI_ENC_SPEED = my_data + j;
	    j += 2;
	    sprintf(str+strlen(str),"$sani_motor_data=%d;//len=4 bytes; mode,speed,dir,onoff\n",j);//len=4 bytes
	    SANI_MOTOR_DATA = my_data + j;
	    j += 4;
	    sprintf(str+strlen(str),"$sani_post_at=%d;//len=1 byte\n",j);//len=1 byte
	    SANI_POST_AT = my_data + j;
	    j++;
	    sprintf(str+strlen(str),"$sani_rk=%d;//len=1 byte (N_RK)\n",j);//len=1 byte
	    SANI_RK = my_data + j;
	    j++;
	    sprintf(str+strlen(str),"$sani_at=%d;//len=256 bytes\n",j);//len=256 bytes
	    SANI_AT = my_data + j;
	    sprintf(str+strlen(str),"$max_known_type=%d;\n",max_known_msg);
	    sprintf(str+strlen(str),"$all_known_type = array('GPRMC','GPGGA','GPGSV','GPGL','GPGSA','GPVTG');\n");
	    sprintf(str+strlen(str),"$symb=0x9C;\n");
	    sprintf(str+strlen(str),"$key=ftok(\"/etc/passwd\",'R');\n");
	    sprintf(str+strlen(str),"$shm_id = shmop_open($key, \"w\", 0, 0);\n");
	    sprintf(str+strlen(str),"if ($shm_id<=0) include \"404.php\";\n");
	    sprintf(str+strlen(str),"$rk=ord(shmop_read($shm_id, $sani_rk, 1));\n");
	    sprintf(str+strlen(str),"?>\n");
	    ret = strlen(str);
	    write(fdf, str, ret);
	    close(fdf);
	    //-----------------------------------------
	    sprintf(name,"/var/run/imoto.php");
	    fdf = open(name, O_RDWR | O_CREAT | O_TRUNC, 0664);
	    if (fdf > 0) {
		write(fdf, str, ret);
		close(fdf);
	    }
	    //-----------------------------------------
	}
	free(str);
    }
    return ret;
}
//**********************************************************************************
void DS18_Gradus(unsigned char *sta, unsigned char tout)
{
unsigned short word;
unsigned int i;
float j;

    if (tout) {
	if (SANI_DS18) memset(SANI_DS18, 0xff, sizeof(s_ds18_temp));
	return;
    }

    memset(&ds18_temp, 0, sizeof(s_ds18_temp));

    memcpy((unsigned char *)&word, sta, 2);
    ds18_temp.znak = (unsigned char)(word >> 12);// знак
    i = word & 0xf;//дробная часть
    j = i * 0.0625; j *= 100; i = j;
    ds18_temp.dro = (unsigned char)i;//дробная часть
    word &= 0x0ff0; word >>= 4;
    ds18_temp.cel = (unsigned char)word;//целая часть

    if (SANI_DS18) memcpy(SANI_DS18, (unsigned char *)&ds18_temp.znak, sizeof(s_ds18_temp));

}
//**********************************************************************************
void DHT11_ALL(unsigned char *sta)
{
    memset(&dht11, 0, sizeof(s_dht11));
    memcpy((unsigned char *)&dht11, sta, 4);
    if (SANI_DHT11) memcpy(SANI_DHT11, (unsigned char *)&dht11, sizeof(s_dht11));

}
//**********************************************************************************
void ADC_ALL(float v)
{
    memset(&adc,0,sizeof(s_adc));
    adc.cel = v;
    adc.dro = (v - adc.cel) * 1000;
    if (SANI_ADC) memcpy(SANI_ADC, (unsigned char *)&adc, sizeof(s_adc));
}
//**********************************************************************************
void OutOfJob()
{
int i;

    for (i = 0; i < kol_dev; i++) if (fd[i] > 0) close(fd[i]);
    for (i = 0; i < all_radio; i++) 
	if (RK_table[i].present) {
	    if (fd_gsm[i] > 0) {
		GSMStartStop(i, (unsigned char)SET_GSM_STOP);
		ModulePWR_OFF(i);
		close(fd_gsm[i]);
	    }
	}
    if (shmid >= 0) my_del_block();
    if (fd_log > 0) close(fd_log); if (fd_bug > 0) close(fd_bug);
    if (pid_main) unlink(pid_name);

}
//**********************************************************************************
int select_adc_chan(unsigned char nk)
{
int ret = -1;

    ret = ioctl(fd[4], ADC_CHAN_SELECT, (nk&3));
    if (!ret) {
	sel_chan = nk & 3;
	*(unsigned char *)SANI_ADC_CHAN = sel_chan;
    }

    return ret;
}
// ************************************************************************ 
void send_to_web(unsigned char *da, int j)
{
int k;

    if ((!SANI_AT) || (!j)) return;// (всего 256 байт)
    k = j; if (k > 199) k = 199;
    memset(SANI_AT+56, 0, 199);
    memcpy(SANI_AT+56, da, k);//answer
    *(unsigned char *)(SANI_AT) = (ATCOM.flag_+0x30);//ATCOM.flag_=1
}
// ************************************************************************ 
