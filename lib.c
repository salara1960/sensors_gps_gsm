#include "lib.h"


pid_t pid_main = 0;
FILE *pid_fp;
const char *pid_name = "/var/run/moto.pid";
const char *known_msg[max_known_msg] = {"$GPRMC","$GPGGA","$GPGSV","$GPGLL","$GPGSA","$GPVTG"};
uint8_t msg_type_ind = 0;//$GPRMC
uint8_t msg_type_ind_set = 0;//$GPRMC
int ErrorSignal = 0;
uint8_t SIGHUPs = 1, SIGTERMs = 1, SIGINTs = 1, SIGKILLs = 1, SIGSEGVs = 1, SIGTRAPs = 1;
uint8_t debug_mode = 0, prints = 0, shup = 0xff, dst_out = 0;
int loops;
int shmid;
uint8_t *my_data;
int fd[kol_dev] = {-1};
int fd_log = -1, fd_bug = -1, gsm_log = -1;
char const *fillog = "log.txt";
char const *filbug = "bug.txt";
char const *filgsm = "gsm.txt";
uint8_t *web_uk = NULL;
uint8_t *SANI = NULL;//+0
uint8_t *SANI_WR_SPEED = NULL;//+1
uint8_t *SANI_RD_SPEED = NULL;//+2
uint8_t *SANI_MOTOR_POST = NULL;//+3
uint8_t *SANI_W1_WR = NULL;//+4
uint8_t *SANI_W1_RD_DONE = NULL;//+5
uint8_t *SANI_W1_RD_DHT_DONE = NULL;//+6
uint8_t *SANI_ADC_CHAN = NULL;//+7
uint8_t *SANI_PACK = NULL;//+10
uint8_t *SANI_W1_RD = NULL;//16 bytes
uint8_t *SANI_W1_RD_DHT = NULL;//8 bytes
uint8_t *SANI_DS18 = NULL;//указатель на структуру ds18_temp (пока 3 байта)
uint8_t *SANI_DHT11 = NULL;//указатель на структуру dht11 (пока 4 байта)
uint8_t *SANI_ADC = NULL;//указатель на структуру adc (пока 4 байта)
uint8_t *SANI_HCSR = NULL;//указатель на 4 байта значения от датчика HC-SR04
uint8_t sel_chan = 3, tmp_chan = 3;
uint8_t *SANI_MSG_TYPE = NULL;
uint8_t *SANI_MSG_TYPE_SET = NULL;
uint8_t *SANI_TYPE_POST = NULL;
uint8_t *SANI_ENC_WAY = NULL;//4 bytes
uint8_t *SANI_ENC_SPEED = NULL;//2 bytes
uint8_t *SANI_POST_AT = NULL;//1 byte
uint8_t *SANI_RK = NULL;//1 byte
uint8_t *SANI_AT = NULL;//256 bytes
uint8_t *SANI_MOTOR_DATA = NULL;//4 bytes
uint8_t *SANI_HCSR2 = NULL;//указатель на 4 байта значения от датчика HC-SR04 (second)

uint8_t N_RK = 0, N_RK_IN = 0;
uint8_t OneChannel = 0;
uint8_t pwronly = 0;

uint8_t motor_mode = 0;//0-PULT, 1-SOFT

s_pack one_pack;
s_pack web_pack;
s_ds18_temp ds18_temp;
s_dht11 dht11;
s_adc adc;
s_enc enc;
s_motor motor;

uint8_t subboard[max_card] = {0};
int fd_gsm[all_radio] = {-1};
s_table RK_table[all_radio];
uint32_t tmr1[all_radio];
uint8_t gsm_faza[all_radio] = {0};
s_gsm_rk gsm_rk[all_radio];

at_cmd ATCOM;
int max_at = def_at;
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
"AT+CSQ"
};

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
"AT+CSQ"
};

char tstr[64];

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
            memset(dattim, 0, 64);
            sprintf(st,"%s | RK_tables :\n", DTPrn(dattim));
            for (i = 0; i < all_radio; i++) {
                if (RK_table[i].present)
                    sprintf(st+strlen(st),"[%02d] : present=%d type=%d addr=0x%04x mir=%02x\n",
                                i + 1,
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
    if (read(fd[0], subboard, 3) != max_card) return -1; else return 0;//читаем subboard[]
}
//*******************************************************************************
int get_tables(int fp)
{
    int len = sizeof(s_table) * all_radio;

    memset((uint8_t *)&RK_table[0], 0, len);

    int ret = read(fd[0], (uint8_t *)&RK_table[0], 4);//читаем структуру RK_table[nk]

    if (ret != len) ret =- 1; else ret = 0;

    if (fp) print_tables();

    return ret;
}
//*******************************************************************************
void reset_card(uint8_t np)
{
uint8_t bu[2] = {4,0};

    bu[1] = np;
    write(fd[0], bu, 2);

}
//*******************************************************************************
uint32_t get_timer_sec(uint32_t t)
{
    return ((uint32_t)time(NULL) + t);
}
//******************************************************************
int check_delay_sec(uint32_t t)
{
    if ((uint32_t)time(NULL) >= t)  return 1; else return 0;
}
// **************************************************************************
uint32_t get_timer_msec(uint32_t t)
{
uint8_t m[4];

    return (read(fd[0], m, 0) + t);
}
//******************************************************************
int check_delay_msec(uint32_t t)
{
uint8_t m[4];

    if (read(fd[0], m, 0) >= t ) return 1; else return 0;
}
// **************************************************************************
void print_str(char * st, uint8_t dst)
{
    if (prints) {
        if (dst)
            write(fd_log, st, strlen(st));
        else
            printf("%s", st);
    }
}
//------------------------------------------------------------------------
char *DTPrn(char *st1)
{
struct tm *ctimka;
struct timeval tvl;

    if (st1) {
        gettimeofday(&tvl,NULL);
        ctimka = localtime(&tvl.tv_sec);
        sprintf(st1, "%02d.%02d %02d:%02d:%02d.%03d | ",
                 ctimka->tm_mday,
                 ctimka->tm_mon + 1,
                 ctimka->tm_hour,
                 ctimka->tm_min,
                 ctimka->tm_sec,
                 (int)(tvl.tv_usec / 1000));
    }

    return (st1);
}
//------------------------------------------------------------------------
void print_rk(char * st)
{
    if (gsm_log > 0) {
        char *sdt = DTPrn(tstr);
        write(gsm_log, sdt, strlen(sdt));
        write(gsm_log, st, strlen(st));
    }
}
//--------------------------------------------------------------------------
void gps_log(const char *st)
{
char *sdt = DTPrn(tstr);

    print_str(sdt, dst_out);//0-stdout , 1-file
    print_str((char *)st, dst_out);//0-stdout , 1-file
}
//-------------------------------------------------------------------------
void bug_log(const char *st)
{
    if ((fd_bug > 0) && debug_mode) {
        char *sdt = DTPrn(tstr);
        write(fd_bug, sdt, strlen(sdt));
        write(fd_bug, st, strlen(st));
    }

}
//--------------------------------------------------------------------------
void PrintErrorSignal(int es)
{
char stz[80] = {0};

    sprintf(stz, "GOT SIGNAL : %d\n", es);
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
        print_str("SIGTERM : termination signal (term)\n", dst_out);
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
        print_str("SIGINT : interrupt from keyboard (term)\n", dst_out);
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
        print_str("SIGKILL : kill signal (term)\n", dst_out);
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
        print_str("SIGSEGV : invalid memory reference (core)\n", dst_out);
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
        print_str("SIGTRAP : trace/breakpoint trap (core)\n", dst_out);
        ErrorSignal = SIGTRAP;
        PrintErrorSignal(ErrorSignal);
    }
    loops = 0;
}
//--------------------------------------------------------------------------
void _USR1Sig(int sig)
{
uint8_t ch = N_RK, findik = 1;

    ch++;
    if (ch >= all_radio) ch = 0;

    if (!RK_table[ch].present) {
        while ((findik) && (!RK_table[ch].present)) {
            ch++;
            if (ch >= all_radio) {
                ch = 0;
                findik = 0;
            }
        }
    }

    N_RK = ch;
    ch = (*(uint8_t *)SANI_RK);
    if ((ch < all_radio) && (ch != N_RK)) N_RK = ch;
}
//-----------------------------------------------------------------------
void _USR2Sig(int sig)
{
//    PrnRK_tabl(N_RK);
}
//------------------ Init shared memory block -------------------------------------------
int my_init_block()
{
key_t key;   /* key to be passed to shmget() */
int shmflg;  /* shmflg to be passed to shmget() */
int it_size; /* size to be passed to shmget() */
char chaka[80] = {0};

    it_size = my_block_size;
    shmflg = (0664 | IPC_CREAT);
    key = ftok("/etc/passwd", 'R');
    shmid = shmget(key, it_size, shmflg);
    if (shmid < 0) {
        strcpy(chaka, "shmget: shmget failed\n");
        bug_log(chaka);
        return -1;
    } else {
        sprintf(chaka, "shmget: shmget returned %d\n", shmid);
        bug_log(chaka);
    }
    my_data = shmat(shmid, NULL, SHM_R | SHM_W);

    if (my_data == (uint8_t *) -1) {
        strcpy(chaka, "shmat: attach segment to our data space failed\n");
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
        strcpy(chaka, "shmdt: detaching segment to our data space failed\n");
        bug_log(chaka);
        return -1;
    }
    if (shmctl(shmid, IPC_RMID, NULL) != 0 ) {
        strcpy(chaka, "shmctl: delete shm segment ERROR\n");
        bug_log(chaka);
        return -1;
    } else {
        strcpy(chaka, "shmctl: Shm segment released. Bye...\n");
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
    motor.mode  = 0;   //pult
    motor.speed = 100; //нулевая скорость
    motor.dir   = 0;   //направление вперед
    motor.onoff = 0;   //manual
    memcpy(SANI_MOTOR_DATA, (uint8_t *)&motor, sizeof(s_motor));
    moto_write(motor.speed);
}
//-------------------   MOTOR functions   ---------------------------------
int moto_write(uint8_t bt)
{
uint8_t bf[2] = {2, 100};
int ret;

    bf[1] = bt;
    ret = write(fd[0], bf, 2);//moto
    motor.speed = bt;

    return ret;
}
//--------------------------------------------------------------------------
uint8_t moto_read()
{
uint8_t bf[2] = {0};

    read(fd[0], bf, 2);
    motor.speed = bf[0];

    return (bf[0]);
}
//--------------------------------------------------------------------------
void SetModeMotor(int md)
{
    ioctl(fd[0], md, 0);
    motor_mode = (uint8_t)md;
    motor.mode = motor_mode;
}
// *************************************************************************
void KT_Point(uint8_t nk)
{
    ioctl(fd_gsm[nk], SET_KT_POINT, nk);
}
// *************************************************************************
uint8_t get_status(uint8_t nk)
{
    return ((uint8_t)ioctl(fd_gsm[nk], GET_GSM_STATUS, nk));
}
// *************************************************************************
int check_vio(uint8_t nk)
{
    return (ioctl(fd_gsm[nk], GET_GSM_STATUS, nk) & VIO);
}
// *************************************************************************
void ModulePWR_ON(uint8_t nk)
{
const char *laka = "ModulePWR_ON\n";
char lll[80] = {0};

    RK_table[nk].mir &= (ModPwrOnOff^0xFF);// bit0 = 0

    ioctl(fd_gsm[nk], SET_GSM_PWRON, RK_table[nk].mir);

    if (nk == N_RK) {
        if (gsm_log > 0) {
            int dl = sprintf(lll, "[%02d] [%02X] %s", nk + 1, RK_table[nk].mir, laka);
            write(gsm_log, DTPrn(tstr), strlen(tstr));
            write(gsm_log, lll, dl);
        }
    }

}
// *************************************************************************
//   процедура выключения питания модуля
// *************************************************************************
void ModulePWR_OFF(uint8_t nk)
{
const char *laka = "ModulePWR_OFF\n";
char lll[80] = {0};

    RK_table[nk].mir = (RK_table[nk].mir | ModPwrOnOff); // bit0 = 1

    ioctl(fd_gsm[nk], SET_GSM_PWROFF, RK_table[nk].mir);

    if (nk == N_RK) {
        if (gsm_log > 0) {
            int dl = sprintf(lll, "[%02d] [%02X] %s", nk + 1, RK_table[nk].mir, laka);
            write(gsm_log, DTPrn(tstr), strlen(tstr));
            write(gsm_log, lll, dl);
        }
    }

}
// *************************************************************************
//   процедура подачи пассивного уровня на вход ON/OFF модуля
// *************************************************************************
void Module_ON(uint8_t nk)
{
const char *laka = "Module_ON\n";
char lll[80] = {0};

    RK_table[nk].mir |= ModOnOff; //bit1=1

    ioctl(fd_gsm[nk], SET_GSM_ON, RK_table[nk].mir);

    if (nk == N_RK) {
        if (gsm_log > 0) {
            int dl = sprintf(lll, "[%02d] [%02X] %s", nk + 1, RK_table[nk].mir, laka);
            write(gsm_log, DTPrn(tstr), strlen(tstr));
            write(gsm_log, lll, dl);
        }
    }

}
// *************************************************************************
//   процедура подачи активного уровня на вход ON/OFF модуля GR47/PiML
// *************************************************************************

void Module_OFF(uint8_t nk)
{
const char *laka = "Module_OFF\n";
char lll[80] = {0};

    RK_table[nk].mir &= (ModOnOff ^ 0xFF); //bit1 = 0

    ioctl(fd_gsm[nk], SET_GSM_OFF, RK_table[nk].mir);

    if (nk == N_RK) {
        if (gsm_log > 0) {
            int dl = sprintf(lll, "[%02d] [%02X] %s", nk + 1, RK_table[nk].mir, laka);
            write(gsm_log, DTPrn(tstr), strlen(tstr));
            write(gsm_log, lll, dl);
        }
    }

}
//--------------------------------------------------------------------
void GSMStartStop(uint8_t nk, uint8_t cmd)
{
    ioctl(fd_gsm[nk], cmd, nk);
}
//----------------------------------------------------------------------
int RXbytes22(uint8_t nk, char *outs)
{
char bu[514] = {0};
uint32_t len = 0, rz = 8;//команда 0x08 - конец пакета 0x0d

    len = read(fd_gsm[nk], bu, rz);

    if (len > 512) len = 512;

    if (len > 0)
        memcpy(outs, bu, len);
    else
        len = 0;

    return len;
}
// *************************************************************************
void put_AT_com(const char *st, uint8_t nk)
{
    memset(gsm_rk[nk].TX, 0, tmp_buf_size);
    gsm_rk[nk].tx_len = 0;

    sprintf((char *)gsm_rk[nk].TX, "\r\n%s\r\n", st);

    gsm_rk[nk].tx_faza = 1; //активизировать выдачу команды модулю

}
// *************************************************************************
int TXbyte_all(uint8_t nk, uint8_t *st, uint16_t len)
{
uint8_t ob[514];
int i = len + 1, ret = 0;

    if (i > 514) i = 514;

    ob[0] = 0x16;

    memcpy(&ob[1], st, i - 1);

    ret = write(fd_gsm[nk], ob, i);

    return ret;
}
// *************************************************************************
uint8_t TXCOM2_all(uint8_t nk)
{
char stx[1024];
char *uk;
int rt = 0;
uint16_t dl = strlen((char *)gsm_rk[nk].TX);

    rt = TXbyte_all(nk, (uint8_t *)&gsm_rk[nk].TX[0], dl);//записываем на передачу

    if ((rt > 0) && (rt <= 512)) gsm_rk[nk].tx_len = rt;

    if (nk == N_RK) {
        if ((gsm_rk[nk].TX[0] == 0x0a) || (gsm_rk[nk].TX[0] == 0x0d)) {
            uk = (char *)&gsm_rk[nk].TX[2];
            dl = rt - 2;
        } else {
            uk = (char *)&gsm_rk[nk].TX[0];
            dl = rt - 1;
        }
        gsm_rk[nk].TX[dl] = 0;
        sprintf(stx, "[%02d] tx: %s\n", nk + 1, uk);
        print_rk(stx);
    }

    memset(gsm_rk[nk].TX, 0, tmp_buf_size);
    gsm_rk[nk].tx_len = 0;

    return 0;
}
//**********************************************************************************************
//**********************************************************************************************
//**********************************************************************************************
void PrintPack(uint8_t fl)
{
char dattim[64], grd = 0x9C;
float r;

    if (fd_log <= 0) return;

    if (one_pack.status) {
        char *st = (char *)calloc(1, 512);
        if (!st) return;
        sprintf(st, "%sGPS:\n", DTPrn(dattim));
        sprintf(st+strlen(st), "\tutc:\t%02d/%02d/%02d %02d:%02d:%02d.%03d\n",
            one_pack.date.den, one_pack.date.mes, one_pack.date.god,
            one_pack.utc.chas, one_pack.utc.min, one_pack.utc.sec, one_pack.utc.ms);
        if (one_pack.latitude.flag == 1) sprintf(dattim,"north latitude");
        else
        if (one_pack.latitude.flag == 2) sprintf(dattim,"south latitude");
        else sprintf(dattim,"??? latitude");
        sprintf(st+strlen(st),"\t\t%d%c %d' %d\" %s\n",
                one_pack.latitude.gradus, grd, one_pack.latitude.minuta, one_pack.latitude.minuta2, dattim);
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
int cnt = 0, len = dline, dl, done = 0, k, i1, i2, delitel;
char *uk_start = NULL, *uk = NULL, *uke = NULL;
char tmp[max_line], one[32], param[16];
float r, r2;

    if ((len < 7) || (len >= max_line)) return cnt;
    memset(tmp, 0, max_line);
    memcpy(tmp, lin, len);
    len = strlen(tmp);
    uk_start = &tmp[0];
    uk = strstr(uk_start, "$GPRMC,");
    if (uk) {
        memset((uint8_t *)&one_pack.utc.chas, 0, sizeof(s_pack));
        uk_start = uk + 7;//указатель на UTC
        while (!done) {
            uk = strchr(uk_start, ',');
            if (!uk) uk = strstr(uk_start, "\r\n");
            if (uk) {
                dl = uk - uk_start;
                if (dl > 0) {
                    dl &= 0x1f;//dl<=31
                    memset(one, 0, 32);
                    memcpy(one, uk_start, dl);
                    switch (cnt) {
                        case 0:
                            uke = strchr(one, '.');
                            if (uke) {
                                memset(param, 0, sizeof(param)); memcpy(param, one, 2);     one_pack.utc.chas = (uint8_t)atoi(param);//часы
                                memset(param, 0, sizeof(param)); memcpy(param, &one[2], 2); one_pack.utc.min  = (uint8_t)atoi(param);//минуты
                                memset(param, 0, sizeof(param)); memcpy(param, &one[4], 2); one_pack.utc.sec  = (uint8_t)atoi(param);//секунды
                                uke++; one_pack.utc.ms = (uint16_t)atoi(uke);//милисекунды
                            }
                        break;
                        case 1:
                            if (one[0] != 'A') {
                                done = 1;
                                cnt = 0;
                            } else one_pack.status = 1;
                        break;
                        case 2://Широта
                            uke = strchr(one, '.');
                            if (uke) {
                                memset(param, 0, sizeof(param)); memcpy(param, one, 2);     one_pack.latitude.gradus = (uint16_t)atoi(param);//grad
                                memset(param, 0, sizeof(param)); memcpy(param, &one[2], 2); one_pack.latitude.minuta = (uint16_t)atoi(param);
                                uke++;
                                k = atoi(uke);
                                dl = strlen(uke);
                                if (dl > 6) dl = 6;
                                delitel = 1; for (i1 = 0; i1 < dl; i1++) delitel *= 10;//delitel=10000 - для старого модуля,  delitel=1000000 - для нового модуля
                                r = k; r /= delitel; r *= 60; k = r;
                                one_pack.latitude.minuta2 = (uint16_t)k;
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
                                memset(param, 0, sizeof(param)); memcpy(param, one, 3);     one_pack.longitude.gradus = (uint16_t)atoi(param);//grad
                                memset(param, 0, sizeof(param)); memcpy(param, &one[3], 2); one_pack.longitude.minuta = (uint16_t)atoi(param);
                                uke++;
                                k = atoi(uke);
                                dl = strlen(uke);
                                if (dl > 6) dl = 6;
                                delitel = 1; for (i1 = 0; i1 < dl; i1++) delitel *= 10;//delitel=10000 - для старого модуля,  delitel=1000000 - для нового модуля
                                r = k; r /= delitel; r *= 60; k = r;
                                one_pack.longitude.minuta2 = (uint16_t)k;
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
                                *(char *)(uke - 1) = '\0';
                                i1 = atoi(one);//целая часть
                                r = i1;
                                r2 /= 100; r += r2; r *= 1852; k = r;
                                one_pack.speed = (uint16_t)k;
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
                                one_pack.course.gradus = (uint16_t)i1;
                                one_pack.course.minuta = (uint16_t)k;
                            }
                        break;
                        case 8://date
                            memset(param, 0, sizeof(param)); memcpy(param, one, 2);     one_pack.date.den = (uint8_t)atoi(param);//день
                            memset(param, 0, sizeof(param)); memcpy(param, &one[2], 2); one_pack.date.mes = (uint8_t)atoi(param);//месяц
                            memset(param, 0, sizeof(param)); memcpy(param, &one[4], 2); one_pack.date.god = (uint8_t)atoi(param);//год
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

    memcpy((uint8_t *)&pks, (uint8_t *)&one_pack, sizeof(s_pack));

    pks.utc.ms = htons(pks.utc.ms);

    pks.latitude.gradus   = htons(pks.latitude.gradus);
    pks.latitude.minuta   = htons(pks.latitude.minuta);
    pks.latitude.minuta2  = htons(pks.latitude.minuta2);

    pks.longitude.gradus  = htons(pks.longitude.gradus);
    pks.longitude.minuta  = htons(pks.longitude.minuta);
    pks.longitude.minuta2 = htons(pks.longitude.minuta2);

    pks.speed = htons(pks.speed);

    pks.course.gradus = htons(pks.course.gradus);
    pks.course.minuta = htons(pks.course.minuta);

    memcpy((uint8_t *)pk, (uint8_t *)&pks, sizeof(s_pack));

}
//*************************************************************************
void help_param_show()
{
    char *inf = (char *)calloc(1, 1024);
    if (inf) {
        strcpy(inf, "\nSupport keys:\n\tdebug\t\t- debug mode (with print src string)\n");
        strcat(inf, "\tsize\t\t- show sizeof(s_pack)\n");
        strcat(inf, "\tprint\t\t- stdout ON (default)\n");
        strcat(inf, "\tnoprint\t\t- stdout OFF\n");
        strcat(inf, "\tnodebug\t\t- write to bug_log.txt OFF\n");
        strcat(inf, "\thelp or --help\t- show this message\n\r\n");
        printf("%s", inf);
        free(inf);
    }
}
//**********************************************************************************
int rst_dev(uint8_t devm, uint8_t b)//0=moto 1=gps 2=ds18 3=dht11 6=enc
{
char bu[2] = {3, 0};

    if (devm >= kol_dev) return -1;

    bu[1] = b;//for reset GPS only

    return (write(fd[devm], bu, 2));
}
//**********************************************************************************
int dev_present(uint8_t devm)
{

    if (devm >= kol_dev) return 0;

    return (read(fd[devm], NULL, (int)5));//1-на шине есть устройство, 0-никого нет дома

}
//**********************************************************************************
int cmd_wr_dev(uint8_t devm, uint8_t *uk, int dln)//запись массива (не более 255 байт)
{
uint8_t bu[16] = {0};

    if (devm >= kol_dev) return -1;
    if (dln <= 0) return 0;

    bu[0] = 8;
    memcpy(&bu[1], uk, dln & 0x0f);

    return (write(fd[devm], bu, dln + 1));
}
// **************************************************************************
int ReadStart(uint8_t devm, uint8_t dl)//1-GPS, 2-DS18, 3-DHT11, 4-ADC, 5-HCSR, 6-ENC, 7-HCSR2
{
uint8_t bu[2] = {0};

    bu[0] = 10;
    bu[1] = dl;//if devm==1 (GPS) dl-msg_type_ind

    return (write(fd[devm], bu, 2));
}
// **************************************************************************
int ReadRead(uint8_t devm, uint8_t *st)
{
int ret = 0;
uint32_t rs = 8;//devm: 1-GPS  2-DS18  3-DHT11  4-ADC  5-HCSR  6-ENC  7-HCSR2
uint8_t bu[512] = {0};

    if (devm >= kol_dev) return -1;

    ret = read(fd[devm], bu, rs);

    if ((devm == DEV_HCSR1) || (devm == DEV_HCSR2)) return ret;//5 or 7

    if ((devm < DEV_ADC) || (devm == DEV_ENC)) {//не ADC и не HCSR04
        if (devm != DEV_GPS) ret &= 0x0f;//не GPS
        if (ret > 0) memcpy(st, bu, ret);
    }

    return ret;

}
// **************************************************************************
int ReadStop(uint8_t devm)
{
uint8_t bu[2] = {11, 0};

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
        strcpy(name, "/www/root/moto/imoto.php");
        fdf = open(name, O_RDWR | O_CREAT | O_TRUNC, 0664);
        if (fdf > 0) {
            j = i = 0;
            strcpy(str, "<?php\n");
            sprintf(str+strlen(str), "$my_block_size=%d;\n", my_block_size);
            sprintf(str+strlen(str), "$sizeofpack=%lu;\n", sizeof(s_pack));//28
            sprintf(str+strlen(str), "$ds18size=%lu;\n", sizeof(s_ds18_temp));//3
            sprintf(str+strlen(str), "$dht11size=%lu;\n", sizeof(s_dht11));//4
            sprintf(str+strlen(str), "$adcsize=%lu;\n", sizeof(s_adc));//4
            sprintf(str+strlen(str), "$sani=%d;\n", i);//SANI
            SANI = my_data;
            i++;
            sprintf(str+strlen(str),"$sani_wr_speed=%d;\n", i);//+1
            SANI_WR_SPEED = my_data + i;
            i++;
            sprintf(str+strlen(str),"$sani_rd_speed=%d;\n", i);//+2
            SANI_RD_SPEED = my_data + i;
            i++;
            sprintf(str+strlen(str),"$sani_motor_post=%d;\n", i);//+3
            SANI_MOTOR_POST = my_data + i;
            i++;
            sprintf(str+strlen(str),"$sani_w1_wr=%d;\n", i);//+4
            SANI_W1_WR = my_data + i;
            i++;
            sprintf(str+strlen(str),"$sani_w1_rd_done=%d;\n", i);//+5
            SANI_W1_RD_DONE = my_data + i;
            i++;
            sprintf(str+strlen(str),"$sani_w1_rd_dht_done=%d;\n", i);//+6
            SANI_W1_RD_DHT_DONE = my_data + i;
            i++;
            sprintf(str+strlen(str),"$sani_adc_chan=%d;\n", i);//+7
            SANI_ADC_CHAN = my_data + i;
            sprintf(str+strlen(str),"$sani_pack=%d;\n", j + 8);//+8
            SANI_PACK = my_data + j + 8;
            j = j + 8 + sizeof(s_pack);//len+15 byte of data
            sprintf(str+strlen(str),"$sani_w1_rd=%d;//len+15 byte of data = 16\n", j);//len+15 byte of data = 16
            SANI_W1_RD = my_data + j;//len=1+9=10
            j += 10;
            SANI_DS18 = my_data + j;//len=3
            sprintf(str+strlen(str),"$sani_ds18=%d;//len=3 (znak,cel,dro)\n", j);//len=3 bytes (znak,cel,dro)
            j += 3;
            sprintf(str+strlen(str),"$sani_w1_rd_dht=%d;//len+7 byte of data = 8\n", j);//len+5 byte of data = 6
            SANI_W1_RD_DHT = my_data + j;
            j += 6;
            sprintf(str+strlen(str),"$sani_dht11=%d;//len=4 bytes\n", j);//len=4 bytes
            SANI_DHT11 = my_data + j;
            j += 4;
            sprintf(str+strlen(str),"$sani_adc=%d;//len=4 bytes\n", j);//len=4 bytes
            SANI_ADC = my_data + j;
            j += 4;
            sprintf(str+strlen(str),"$sani_hcsr=%d;//len=4 bytes\n",j);//len=4 bytes
            SANI_HCSR = my_data + j;
            j += 4;
            sprintf(str+strlen(str),"$sani_hcsr2=%d;//len=4 bytes\n", j);//len=4 bytes
            SANI_HCSR2 = my_data + j;
            j += 4;
            sprintf(str+strlen(str),"$sani_msg_type=%d;//len=1 bytes\n", j);//len=1 byte
            SANI_MSG_TYPE = my_data + j;
            j++;
            sprintf(str+strlen(str),"$sani_msg_type_set=%d;//len=1 bytes\n", j);//len=1 byte
            SANI_MSG_TYPE_SET = my_data + j;
            j++;
            sprintf(str+strlen(str),"$sani_msg_type_post=%d;//len=1 bytes\n", j);//len=1 byte
            SANI_TYPE_POST = my_data + j;
            j++;
            sprintf(str+strlen(str),"$sani_enc_way=%d;//len=4 bytes\n", j);//len=4 bytes
            SANI_ENC_WAY = my_data + j;
            j += 4;
            sprintf(str+strlen(str),"$sani_enc_speed=%d;//len=2 bytes\n", j);//len=2 bytes
            SANI_ENC_SPEED = my_data + j;
            j += 2;
            sprintf(str+strlen(str),"$sani_motor_data=%d;//len=4 bytes; mode,speed,dir,onoff\n", j);//len=4 bytes
            SANI_MOTOR_DATA = my_data + j;
            j += 4;
            sprintf(str+strlen(str),"$sani_post_at=%d;//len=1 byte\n", j);//len=1 byte
            SANI_POST_AT = my_data + j;
            j++;
            sprintf(str+strlen(str),"$sani_rk=%d;//len=1 byte (N_RK)\n", j);//len=1 byte
            SANI_RK = my_data + j;
            j++;
            sprintf(str+strlen(str),"$sani_at=%d;//len=256 bytes\n", j);//len=256 bytes
            SANI_AT = my_data + j;
            sprintf(str+strlen(str),"$max_known_type=%d;\n", max_known_msg);
            strcat(str, "$all_known_type = array('GPRMC','GPGGA','GPGSV','GPGL','GPGSA','GPVTG');\n");
            strcat(str, "$symb=0x9C;\n");
            strcat(str, "$key=ftok(\"/etc/passwd\",'R');\n");
            strcat(str, "$shm_id = shmop_open($key, \"w\", 0, 0);\n");
            strcat(str, "if ($shm_id<=0) include \"404.php\";\n");
            strcat(str, "$rk=ord(shmop_read($shm_id, $sani_rk, 1));\n");
            strcat(str, "?>\n");
            ret = strlen(str);
            write(fdf, str, ret);
            close(fdf);
            //-----------------------------------------
            strcpy(name, "/var/run/imoto.php");
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
void DS18_Gradus(uint8_t *sta, uint8_t tout)
{
uint16_t word;
uint32_t i;
float j;

    if (tout) {
        if (SANI_DS18) memset(SANI_DS18, 0xff, sizeof(s_ds18_temp));
        return;
    }

    memset(&ds18_temp, 0, sizeof(s_ds18_temp));

    memcpy((uint8_t *)&word, sta, 2);
    ds18_temp.znak = (uint8_t)(word >> 12);// знак
    i = word & 0xf;//дробная часть
    j = i * 0.0625;
    j *= 100;
    i = j;
    ds18_temp.dro = (uint8_t)i;//дробная часть
    word &= 0x0ff0; word >>= 4;
    ds18_temp.cel = (uint8_t)word;//целая часть

    if (SANI_DS18) memcpy(SANI_DS18, (uint8_t *)&ds18_temp.znak, sizeof(s_ds18_temp));

}
//**********************************************************************************
void DHT11_ALL(uint8_t *sta)
{
    memset(&dht11, 0, sizeof(s_dht11));
    memcpy((uint8_t *)&dht11, sta, 4);
    if (SANI_DHT11) memcpy(SANI_DHT11, (uint8_t *)&dht11, sizeof(s_dht11));
}
//**********************************************************************************
void ADC_ALL(float v)
{
    memset(&adc, 0, sizeof(s_adc));
    adc.cel = v;
    adc.dro = (v - adc.cel) * 1000;
    if (SANI_ADC) memcpy(SANI_ADC, (uint8_t *)&adc, sizeof(s_adc));
}
//**********************************************************************************
void OutOfJob()
{
int i;

    for (i = 0; i < kol_dev; i++) if (fd[i] > 0) close(fd[i]);
    for (i = 0; i < all_radio; i++) {
        if (RK_table[i].present) {
            if (fd_gsm[i] > 0) {
                GSMStartStop(i, (uint8_t)SET_GSM_STOP);
                ModulePWR_OFF(i);
                close(fd_gsm[i]);
            }
        }
    }
    if (shmid >= 0) my_del_block();
    if (fd_log > 0) close(fd_log);
    if (fd_bug > 0) close(fd_bug);
    if (pid_main) unlink(pid_name);

}
//**********************************************************************************
int select_adc_chan(uint8_t nk)
{
int ret = -1;

    ret = ioctl(fd[4], ADC_CHAN_SELECT, (nk&3));
    if (!ret) {
        sel_chan = nk & 3;
        *(uint8_t *)SANI_ADC_CHAN = sel_chan;
    }

    return ret;
}
// ************************************************************************
void send_to_web(uint8_t *da, int j)
{
int k;

    if ((!SANI_AT) || (!j)) return;// (всего 256 байт)
    k = j; if (k > 199) k = 199;
    memset(SANI_AT + 56, 0, 199);
    memcpy(SANI_AT + 56, da, k);//answer
    *(uint8_t *)(SANI_AT) = (ATCOM.flag_ + 0x30);//ATCOM.flag_=1
}
// ************************************************************************
