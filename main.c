#include "lib.h"

int main(int argc, char **argv)
{
uint8_t symbol = 0, bt = 1, prn = 0, sz = 0, en = 0, fst = 1;
uint8_t pst = 0, byte = 0, pst_dht = 0, pst_hcsr = 0, pst_hcsr2 = 0, pst_enc = 0, stop_all_rk = 0;
uint8_t data_wr_w1 = 0, pst_adc = 0, id, busy = 0, cmd_reg_5215, cmd_reg_10;
char chaka[128] = {0};
char stroka[80] = {0};
struct sigaction Act_t, OldAct_t, Act_i, OldAct_i, Act_k, OldAct_k, Act_r1, OldAct_r1, Act_k1,OldAct_k1;
struct sigaction Act_r, OldAct_r, Act_u, OldAct_u, Act_u2, OldAct_u2;
uint8_t wr_cmd[16], rd_cmd[16], rd_dht[16], rd_adc[16], rd_enc[16];
int res, lens, rd_uk = 0, rd_cnt = 9, i, j, eol = 0;
int rd_uk_dht = 0, rd_cnt_dht = 5;
int rd_cnt_hcsr = 4, rd_cnt_hcsr2 = 4;
int rd_cnt_enc = 6;
struct timeval mytv;
fd_set my_read_Fds;
char buf[mem_buf];
uint32_t tmrs, tmr_w1 = 0, tmr_loop, tmr_hcsr, tmr_hcsr2, msec_def = _500ms, tmr_enc, msec_enc = _500ms;
char *endp = NULL;
char ds18_str[32];
char dht11_str[32];
int my_max_fd;
int data_adc = -1;
float data_adc_v;
float enc_int_float, enc_short_float;
int data_hcsr = -1, data_hcsr2 = -1;
int data_hcsr_n = -1, data_hcsr2_n = -1;
uint32_t enc_int;
uint16_t enc_short, gsm_len = 0;
char stx[tmp_buf_size + 1] = {0};
char stz[128];
uint32_t enc_way_sm = 0;
int tdl = 0;

    pid_main = getpid();

    if (!(pid_fp = fopen(pid_name, "w"))) {
        printf("moto: unable to create pid file %s: %s\n", pid_name, strerror(errno));
        return -1;
    } else {
        fprintf(pid_fp, "%i\n", (int)pid_main);
        fclose(pid_fp);
    }

    if (argc > 1) {
        memset(chaka, 0, sizeof(chaka));
        strcpy(chaka, argv[1]);
             if (!strcmp(chaka,"debug")) debug_mode = 1;
        else if (!strcmp(chaka,"nodebug")) debug_mode = 0;
        else if (!strcmp(chaka,"size")) sz = 1;
        else if (!strcmp(chaka,"print")) prints = 1;
        else if (!strcmp(chaka,"noprint")) prints = 0;
        else if (!strcmp(chaka,"nopackprn")) shup = 0;
        else if (!strcmp(chaka,"packprn")) shup = 1;
        else if (!strcmp(chaka,"stdout")) dst_out = 0;
        else if (!strcmp(chaka,"fileout")) dst_out = 1;
        else if (!strcmp(chaka,"onechannel")) OneChannel = 1;
        else if (!strcmp(chaka,"pwronly")) pwronly = 1;
        else if ((!strcmp(chaka,"help")) || (!strcmp(chaka,"--help"))) {
            help_param_show();
            if (pid_main) unlink(pid_name);
            return 0;
        }
        if (argc > 2) {
            memset(chaka, 0, sizeof(chaka));
            strcpy(chaka, argv[2]);
                 if (!strcmp(chaka,"debug")) debug_mode = 1;
            else if (!strcmp(chaka,"nodebug")) debug_mode = 0;
            else if (!strcmp(chaka,"size")) sz = 1;
            else if (!strcmp(chaka,"print")) prints = 1;
            else if (!strcmp(chaka,"noprint")) prints = 0;
            else if (!strcmp(chaka,"nopackprn")) shup = 0;
            else if (!strcmp(chaka,"packprn")) shup = 1;
            else if (!strcmp(chaka,"stdout")) dst_out = 0;
            else if (!strcmp(chaka,"fileout")) dst_out = 1;
            else if (!strcmp(chaka,"onechannel")) OneChannel = 1;
            else if (!strcmp(chaka,"pwronly")) pwronly = 1;
            if (argc > 3) {
                memset(chaka, 0, sizeof(chaka));
                strcpy(chaka, argv[3]);
                     if (!strcmp(chaka,"debug")) debug_mode = 1;
                else if (!strcmp(chaka,"nodebug")) debug_mode = 0;
                else if (!strcmp(chaka,"size")) sz = 1;
                else if (!strcmp(chaka,"print")) prints = 1;
                else if (!strcmp(chaka,"noprint")) prints = 0;
                else if (!strcmp(chaka,"nopackprn")) shup = 0;
                else if (!strcmp(chaka,"packprn")) shup = 1;
                else if (!strcmp(chaka,"stdout")) dst_out = 0;
                else if (!strcmp(chaka,"fileout")) dst_out = 1;
                else if (!strcmp(chaka,"onechannel")) OneChannel = 1;
                else if (!strcmp(chaka,"pwronly")) pwronly = 1;
                if (argc > 4) {
                    memset(chaka, 0, sizeof(chaka));
                    strcpy(chaka, argv[4]);
                         if (!strcmp(chaka,"debug")) debug_mode = 1;
                    else if (!strcmp(chaka,"nodebug")) debug_mode = 0;
                    else if (!strcmp(chaka,"size")) sz = 1;
                    else if (!strcmp(chaka,"print")) prints = 1;
                    else if (!strcmp(chaka,"noprint")) prints = 0;
                    else if (!strcmp(chaka,"nopackprn")) shup = 0;
                    else if (!strcmp(chaka,"packprn")) shup = 1;
                    else if (!strcmp(chaka,"stdout")) dst_out = 0;
                    else if (!strcmp(chaka,"fileout")) dst_out = 1;
                    else if (!strcmp(chaka,"onechannel")) OneChannel = 1;
                    else if (!strcmp(chaka,"pwronly")) pwronly = 1;
                    if (argc > 5) {
                        memset(chaka, 0, sizeof(chaka));
                        strcpy(chaka, argv[4]);
                             if (!strcmp(chaka,"debug")) debug_mode = 1;
                        else if (!strcmp(chaka,"nodebug")) debug_mode = 0;
                        else if (!strcmp(chaka,"size")) sz = 1;
                        else if (!strcmp(chaka,"print")) prints = 1;
                        else if (!strcmp(chaka,"noprint")) prints = 0;
                        else if (!strcmp(chaka,"nopackprn")) shup = 0;
                        else if (!strcmp(chaka,"packprn")) shup = 1;
                        else if (!strcmp(chaka,"stdout")) dst_out = 0;
                        else if (!strcmp(chaka,"fileout")) dst_out = 1;
                        else if (!strcmp(chaka,"onechannel")) OneChannel = 1;
                        else if (!strcmp(chaka,"pwronly")) pwronly = 1;
                    }
                }
            }
        }
    }

    SIGHUPs  = 1; Act_r.sa_handler  = &_ReadSig; Act_r.sa_flags  = 0; sigaction(SIGHUP , &Act_r,  &OldAct_r);
    SIGTERMs = 1; Act_t.sa_handler  = &_TermSig; Act_t.sa_flags  = 0; sigaction(SIGTERM, &Act_t,  &OldAct_t);
    SIGINTs  = 1; Act_i.sa_handler  = &_IntSig;  Act_i.sa_flags  = 0; sigaction(SIGINT , &Act_i,  &OldAct_i);
    SIGKILLs = 1; Act_k.sa_handler  = &_KillSig; Act_k.sa_flags  = 0; sigaction(SIGKILL, &Act_k,  &OldAct_k);
    SIGSEGVs = 1; Act_r1.sa_handler = &_SegvSig; Act_r1.sa_flags = 0; sigaction(SIGSEGV, &Act_r1, &OldAct_r1);
    SIGTRAPs = 1; Act_k1.sa_handler = &_TrapSig; Act_k1.sa_flags = 0; sigaction(SIGTRAP, &Act_k1, &OldAct_k1);
                  Act_u.sa_handler  = &_USR1Sig; Act_u.sa_flags  = 0; sigaction(SIGUSR1, &Act_u,  &OldAct_u);
                  Act_u2.sa_handler = &_USR2Sig; Act_u2.sa_flags = 0; sigaction(SIGUSR2, &Act_u2, &OldAct_u2);

//-----------------------------------------------------------------------------------

    gsm_log = open(filgsm, O_WRONLY | O_APPEND | O_CREAT, 0664);
    if (gsm_log < 0) printf("Can't open %s file\n", filgsm);
    else {
        tdl = sprintf(chaka, "%s START MAIN with PID %d\n", DTPrn(tstr), pid_main);
        write(gsm_log, chaka, tdl);
    }

    fd_bug = open(filbug, O_WRONLY | O_APPEND | O_CREAT, 0664);
    if (fd_bug < 0) printf("Can't open %s file\n", filbug);
    else {
        tdl = sprintf(chaka,"%s START MAIN with PID %d\n", DTPrn(tstr), pid_main);
        write(fd_bug, chaka, tdl);
    }

    fd_log = open(fillog, O_WRONLY | O_APPEND | O_CREAT, 0664);
    if (fd_log < 0) printf("Can't open %s file\n", fillog);
    else {
        tdl = sprintf(chaka,"%s START MAIN with PID %d\n", DTPrn(tstr), pid_main);
        write(fd_log, chaka, tdl);
    }

    if (sz) printf("size_of_pack=%lu\n", sizeof(s_pack));

//----------------   init shared memory block ---------------------------------------
    if (my_init_block() == -1) {
        if (pid_main) unlink(pid_name);
        if (fd_log > 0) close(fd_log);
        if (fd_bug > 0) close(fd_bug);
        return -1;
    }

    CreateInitvarPHP();

    memset(SANI, 0, my_block_size);
//---------------- открываем device радиоканалов ----------------------------------------
// /dev/totoro0 - moto  /dev/totoro1 - gps  /dev/totoro2 - ds18  /dev/totoro3 - dht11  /dev/totoro4 - ADC  /dev/totoro5 - HCSR  /dev/totoro7 - HCSR2
    for (i = 0; i < kol_dev; i++) {//0=moto 1=gps 2=ds18 3=dht11 4=ADC 5-HCSR 7=HCSR2
        sprintf(stroka, "/dev/totoro%d", i);
        if ((fd[i] = open(stroka, O_RDWR) ) < 0) {
            sprintf(chaka, "Can't open /dev/totoro%d (%s)\n", i, strerror(errno));
            bug_log(chaka);
            OutOfJob();
            return -1;
        } else {
            sprintf(chaka, "Open /dev/totoro%d OK\n", i);
            bug_log(chaka);
        }
    }
    ReadStop(DEV_DS18);//DS18
    ReadStop(DEV_DHT11);//DHT11
    select_adc_chan(tmp_chan);
    *(uint8_t *)SANI_ADC_CHAN = tmp_chan;
    ReadStop(DEV_HCSR1);//HCSR
    ReadStop(DEV_HCSR2);//HCSR2
    ReadStop(DEV_ENC);//ENC

//------------------------------   GSM   ------------------------------------------

    get_cards();//get subboard[]
    get_tables(1);//get RK_table[]

    i = sizeof(s_gsm_rk) * all_radio;
    memset((uint8_t *)&gsm_rk[0], 0, i);

    cmd_reg_5215 = cmd_reg_10 = 255;
    for (i = 0; i < max_at; i++) {
        if (strstr((char *)commands5215[i],"AT+CREG?") != NULL) {
            cmd_reg_5215 = (uint8_t)i;
            break;
        }
    }
    for (i = 0; i < max_at; i++) {
        if (strstr((char *)commands10[i],"AT+CREG?") != NULL) {
            cmd_reg_10 = (uint8_t)i;
            break;
        }
    }
    *(uint8_t *)SANI_RK = N_RK;

    for (i = 0; i < all_radio; i++) {
        if (RK_table[i].present) {
            sprintf(stroka, "/dev/gsm%d", i);
            if ((fd_gsm[i] = open(stroka, O_RDWR) ) < 0) {
                sprintf(chaka, "Can't open /dev/gsm%d (%s)\n", i, strerror(errno));
                printf("%s", chaka);
                print_rk(chaka);
            } else {
                sprintf(chaka, "Open /dev/gsm%d OK\n",i);
                printf("%s", chaka);
                print_rk(chaka);

                if (OneChannel) {
                    if (i == N_RK) gsm_rk[i].block = 0; else gsm_rk[i].block = 1;
                } else {
                    gsm_rk[i].block = 0;
                }
                if (pwronly) gsm_rk[i].block = 0;
                gsm_rk[i].faza = 0;
                gsm_rk[i].rx_faza = gsm_rk[i].tx_faza = 0;
                gsm_rk[i].data = 0;
                if (RK_table[i].type == 2) gsm_rk[i].creg_ind = cmd_reg_5215;
                                      else gsm_rk[i].creg_ind = cmd_reg_10;
                tmr1[i] = get_timer_msec(_5s);
                Module_ON(i);
            }
        }
    }

    //---------------------------------------------------------------------------------

    memset((uint8_t *)&enc.way,           0, sizeof(s_enc));
    memset((uint8_t *)&one_pack.utc.chas, 0, sizeof(s_pack));
    memset((uint8_t *)&web_pack.utc.chas, 0, sizeof(s_pack));

    *(uint8_t *)SANI_W1_WR = 0;
    *(uint8_t *)SANI_W1_RD = 0;
    symbol = 0x30;
    web_uk = (uint8_t *)&one_pack;
    *(uint8_t *)SANI_MSG_TYPE     = msg_type_ind;
    *(uint8_t *)SANI_MSG_TYPE_SET = msg_type_ind;
    *(uint8_t *)SANI              = 0x30;
    *(uint8_t *)SANI_TYPE_POST    = 0x30;
    memcpy(SANI_ENC_WAY, (uint8_t *)&enc.way, sizeof(s_enc));
    rst_dev(DEV_ENC, 0);//6=ENC

    init_motor();

    loops = 1;
    memset(buf, 0, mem_buf);
    bt = 1;
    tmrs = get_timer_sec(1);
    tmr_hcsr = tmr_hcsr2 = get_timer_msec(msec_def);
    tmr_enc = get_timer_msec(msec_enc);
    tmr_loop = 0;

    ATCOM.flag_ = 0;
    ATCOM.dl_   = 0;
    memset(ATCOM.com_, 0, 54);
    memset(ATCOM.txt_, 0, 200);

    while (loops) {

        //-------------------------- post from F9 -------------------------------
        symbol = *(uint8_t *)(SANI_POST_AT);//есть ли новый пост от F9
        if (symbol == 0x31) {
            if (!ATCOM.flag_) {
                ATCOM.flag_ = 1;
                *(uint8_t *)(SANI_AT) = (ATCOM.flag_ + 0x30);//ATCOM.flag_=1
                ATCOM.dl_ = 0;
                memset(ATCOM.com_, 0, 54);
                memset(ATCOM.txt_, 0, 200);
                memcpy(&ATCOM.com_[0], (char *)(SANI_AT + 2), 54);//указатель на АТ команду
            }
            *(uint8_t *)(SANI_POST_AT) = 0x30;//сбросить флаг F9
        }
        //-----------------------------------------------------------------------

        symbol = *(uint8_t *)SANI_TYPE_POST;
        if (symbol == 0x31) {// есть пост по смене типа сообщения у GPS
            *(uint8_t *)SANI_TYPE_POST = 0x30;
            //for GPS message type
            msg_type_ind_set = *(uint8_t *)SANI_MSG_TYPE_SET;
            if (msg_type_ind_set != msg_type_ind) {
                msg_type_ind = msg_type_ind_set;
                *(uint8_t *)SANI_MSG_TYPE = msg_type_ind;
                rst_dev(1, 1);//reset GPS : 1=gps,bt=0/1
                ReadStart(1, msg_type_ind);// set msg_type to 0 - $GPMRC
                if (fst) fst = 0;
            }
        }

        if (check_delay_sec(tmr_loop)) {
            if (!busy) {
                tmr_loop = get_timer_sec(2);//3//5
                *(uint8_t *)(SANI_W1_WR) = 0xAA;//цепочка 0xCC 0x44, 0xCC 0xBE
                *(uint8_t *)SANI = 0x31;
            }
        }

        //-------------   motor   --------------------------------------------------------
        if (*(uint8_t *)SANI_MOTOR_POST == 0x31) {// есть пост от вэба для моторчика
            *(uint8_t *)SANI_MOTOR_POST = 0x30;
            memcpy((uint8_t *)&motor, SANI_MOTOR_DATA, sizeof(s_motor));
                 if (!motor.mode) i = SET_MODE_PULT;
            else if (motor.mode == 1) i = SET_MODE_SOFT;
            else i = -1;
            if (i != -1) SetModeMotor(i);
            switch (motor.mode) {
                case 0 ://pult
                    motor.onoff = 0;//manual
                    motor.speed = 100;//speed=0
                    moto_write(motor.speed);
                break;
                case 1 ://soft
                    switch (motor.onoff) {
                        case 0 ://manual
                            moto_write(motor.speed);
                        break;
                        case 1 : //stop
                            motor.speed = 100; moto_write(motor.speed);
                            if (motor.dir) {//назад
                                usleep(50000);
                                motor.speed = 94; moto_write(motor.speed);
                                usleep(50000);
                                motor.speed = 100; moto_write(motor.speed);
                            }
                        break;
                        case 2 : //start
                            if (!motor.dir) motor.speed++;//вперед
                                       else motor.speed--;//назад
                            moto_write(motor.speed);
                        break;
                    }
                break;//soft
            }
            moto_read();
            memcpy(SANI_MOTOR_DATA, (uint8_t *)&motor, sizeof(s_motor));
        }

        symbol = *(uint8_t *)SANI;
        if (symbol == 0x31) {// есть пост от вэба
            tmp_chan = *(uint8_t *)SANI_ADC_CHAN;
            if (tmp_chan != sel_chan) {
                if (select_adc_chan(tmp_chan)) {
                    sprintf(chaka, " Select_adc_chan ERROR : old=%d new=%d\n", sel_chan, tmp_chan);
                    gps_log(chaka);
                    bug_log(chaka);
                }
            }

            memset(ds18_str,  0, sizeof(ds18_str));
            memset(dht11_str, 0, sizeof(dht11_str));
            busy = 1;

            //-------------   temperature   --------------------------------------------------
            rd_cnt = 0;
            pst = 0;
            memset(rd_cmd, 0, sizeof(rd_cmd));
            memset(SANI_W1_RD, 0, 16);
            data_wr_w1 = *(uint8_t *)(SANI_W1_WR);//данные на запись
            switch (data_wr_w1) {
                case 0xAA ://0xcc=204 0x44=68 , 0xcc=204 0xbe=190
                    *(uint8_t *)SANI_W1_RD_DONE = '0';
                    rd_cnt = 0;
                    rd_uk = 0;
                    rst_dev(DEV_DS18, 0);//0=moto 1=gps 2=ds18 3=dht11
                    strcpy(chaka, "DS18  : reset begin...");
                    usleep(1000);
                    if (dev_present(2)) {//DS18  //0=moto 1=gps 2=ds18 3=dht11 4=ADC
                        memset(wr_cmd, 0, sizeof(wr_cmd));
                        wr_cmd[0] = 0xCC;
                        wr_cmd[1] = 0x44;
                        cmd_wr_dev(DEV_DS18, wr_cmd, 2);
                        sprintf(chaka+strlen(chaka), " device DS18 present, write 0x%X 0x%X\n", wr_cmd[0], wr_cmd[1]);
                        gps_log(chaka);
                        bug_log(chaka);
                        //-----------------------------------------------------------
                        usleep(1000);
                        rd_cnt = 9;
                        rd_uk = 0;
                        rst_dev(DEV_DS18, 0);//0=moto 1=gps 2=ds18 3=dht11 4=ADC
                        strcpy(chaka, "DS18  : reset begin...");
                        if (SANI_DS18) memset(SANI_DS18, 0xff, sizeof(s_ds18_temp));
                        usleep(1000);
                        if (dev_present(DEV_DS18)) {//DS18  //0=moto 1=gps 2=ds18 3=dht11 4=ADC
                            memset(wr_cmd, 0, sizeof(wr_cmd));
                            wr_cmd[0] = 0xCC;
                            wr_cmd[1] = 0xBE;
                            cmd_wr_dev(DEV_DS18, wr_cmd, 2);
                            sprintf(chaka+strlen(chaka), " device DS18 present, write 0x%X 0x%X\n", wr_cmd[0], wr_cmd[1]);
                            gps_log(chaka);
                            bug_log(chaka);
                            ReadStart(DEV_DS18, rd_cnt);
                            pst = 1;
                        } else {
                            *(uint8_t *)SANI_W1_RD_DONE = '1';
                            strcat(chaka, " no DS18 device.\n");
                            gps_log(chaka);
                            bug_log(chaka);
                        }
                        //-----------------------------------------------------------
                    } else {
                        if (SANI_DS18) memset(SANI_DS18, 0xff, sizeof(s_ds18_temp));
                        *(uint8_t *)SANI_W1_RD_DONE = '1';
                        strcat(chaka, " no DS18 device.\n");
                        gps_log(chaka);
                        bug_log(chaka);
                    }
                break;
                case 0xbe ://0xcc=204 0xbe=190
                    *(uint8_t *)SANI_W1_RD_DONE = '0';
                    rd_cnt = 9; rd_uk = 0;
                    rst_dev(DEV_DS18, 0);//0=moto 1=gps 2=ds18 3=dht11 4=ADC
                    strcpy(chaka, "DS18  : reset begin...");
                    if (SANI_DS18) memset(SANI_DS18, 0xff, sizeof(s_ds18_temp));
                    usleep(1000);
                    if (dev_present(2)) {//DS18  //0=moto 1=gps 2=ds18 3=dht11 4=ADC
                        memset(wr_cmd, 0, sizeof(wr_cmd));
                        wr_cmd[0] = 0xCC;
                        wr_cmd[1] = 0xBE;
                        cmd_wr_dev(DEV_DS18, wr_cmd, 2);
                        sprintf(chaka+strlen(chaka), " device DS18 present, write 0x%X 0x%X\n", wr_cmd[0], wr_cmd[1]);
                        gps_log(chaka);
                        bug_log(chaka);
                        pst = 1;
                        rd_uk = 0;
                        ReadStart(DEV_DS18, rd_cnt);
                    } else {
                        strcat(chaka, " no DS18 device.\n");
                        gps_log(chaka);
                        bug_log(chaka);
                    }
                break;
                case 0x33 ://0xcc=204 0x33=51
                    *(uint8_t *)SANI_W1_RD_DONE = '0';
                    rd_cnt = 9;
                    rd_uk = 0;
                    rst_dev(DEV_DS18, 0);//0=moto 1=gps 2=ds18 3=dht11 4=ADC
                    strcpy(chaka, "DS18  : reset begin...");
                    if (SANI_DS18) memset(SANI_DS18, 0xff, sizeof(s_ds18_temp));
                    usleep(1000);
                    if (dev_present(DEV_DS18)) {//DS18  //0=moto 1=gps 2=ds18 3=dht11 4=ADC
                        memset(wr_cmd, 0, sizeof(wr_cmd));
                        wr_cmd[0] = 0xCC;
                        wr_cmd[1] = 0x33;
                        cmd_wr_dev(DEV_DS18, wr_cmd, 2);
                        sprintf(chaka+strlen(chaka), " device DS18 present, write 0x%X 0x%X\n", wr_cmd[0], wr_cmd[1]);
                        gps_log(chaka);
                        bug_log(chaka);
                        pst = 1;
                        ReadStart(DEV_DS18, rd_cnt);
                    } else {
                        strcat(chaka, " no DS18 device.\n");
                        gps_log(chaka);
                        bug_log(chaka);
                    }
                break;
                case 0x44 ://0xcc=204 0x44=68
                    *(uint8_t *)SANI_W1_RD_DONE = '1';
                    rd_cnt = 0;
                    rd_uk = 0;
                    rst_dev(DEV_DS18, 0);//0=moto 1=gps 2=ds18 3=dht11 4=ADC
                    strcpy(chaka, "DS18  : reset begin...");
                    if (SANI_DS18) memset(SANI_DS18, 0xff, sizeof(s_ds18_temp));
                    usleep(1000);
                    if (dev_present(DEV_DS18)) {//DS18  //0=moto 1=gps 2=ds18 3=dht11 4=ADC
                        memset(wr_cmd, 0, sizeof(wr_cmd));
                        wr_cmd[0] = 0xCC;
                        wr_cmd[1] = 0x44;
                        cmd_wr_dev(DEV_DS18, wr_cmd, 2);
                        sprintf(chaka+strlen(chaka), " device DS18 present, write 0x%X 0x%X\n", wr_cmd[0], wr_cmd[1]);
                        gps_log(chaka);
                        bug_log(chaka);
                    } else {
                        strcat(chaka, " no DS18 device.\n");
                        gps_log(chaka);
                        bug_log(chaka);
                    }
                break;
            }//switch
            //-------------------   DHT11   ------------------------------------
            *(uint8_t *)SANI_W1_RD_DHT_DONE = '0';
            rd_cnt_dht = 5;
            rd_uk_dht = 0;
            pst_dht = 0;
            memset(rd_dht, 0, sizeof(rd_dht));
            memset(SANI_W1_RD_DHT, 0, 8); //memset(SANI_DHT11,0xff,sizeof(s_dht11));
            rst_dev(DEV_DHT11, 0);//0=moto 1=gps 2=ds18 3=dht11 4=ADC
            pst_dht = 1;
            if (SANI_DHT11) memset(SANI_DHT11, 0xff, sizeof(s_dht11));
            usleep(1000);
            ReadStart(DEV_DHT11, 5);//rd_cnt_dht
            strcpy(chaka, "DHT11 : reset+start begin, wait data ...\n");
            gps_log(chaka);
            //-------------------   ADC   --------------------------------------
            if (SANI_ADC) memset(SANI_ADC, 0, sizeof(s_adc));
            pst_adc = 1;
            ReadStart(DEV_ADC, sel_chan);
            sprintf(chaka, "ADC (%d) : start, wait data ...\n", sel_chan);
            gps_log(chaka);
            //-------------------------------------------------------------------

            tmr_w1 = get_timer_sec(4);

            *(uint8_t *)SANI = 0x30;

        }


        if (!pst_hcsr) {//-------------------   HC-SR04   ----------------------------------
            if (check_delay_msec(tmr_hcsr)) {
                //-------------------   HC-SR04   ----------------------------------
                if (SANI_HCSR) memset(SANI_HCSR, 0xff, sizeof(int));
                pst_hcsr = 1;
                rd_cnt_hcsr = 4;
                ReadStart(DEV_HCSR1, rd_cnt_hcsr);
                strcpy(chaka, "HCSR : start, wait data ...\n");
                gps_log(chaka);
                bug_log(chaka);
                tmr_hcsr = get_timer_msec(msec_def);
                //-------------------------------------------------------------------
            }
        } else {
            if (check_delay_msec(tmr_hcsr)) {
                data_hcsr = data_hcsr_n = -1;
                if (SANI_HCSR) memset(SANI_HCSR, 0xff, sizeof(int));
                strcpy(chaka, "HCSR  timeout.\n");
                bug_log(chaka);
                gps_log(chaka);
                pst_hcsr = 0;
                ReadStop(DEV_HCSR1);//HCSR
                tmr_hcsr = get_timer_msec(msec_def);
            }
        }

        if (!pst_hcsr2) {//-------------------   HC-SR04-2   ----------------------------------
            if (check_delay_msec(tmr_hcsr2)) {
                //-------------------   HC-SR04-2   ----------------------------------
                if (SANI_HCSR2) memset(SANI_HCSR2, 0xff, sizeof(int));
                pst_hcsr2 = 1;
                rd_cnt_hcsr2 = 4;
                ReadStart(DEV_HCSR2, rd_cnt_hcsr2);
                strcpy(chaka, "HCSR2 : start, wait data ...\n");
                gps_log(chaka);
                bug_log(chaka);
                tmr_hcsr2 = get_timer_msec(msec_def);
                //-------------------------------------------------------------------
            }
        } else {
            if (check_delay_msec(tmr_hcsr2)) {
                data_hcsr2 = data_hcsr2_n = -1;
                if (SANI_HCSR2) memset(SANI_HCSR2, 0xff, sizeof(int));
                strcpy(chaka, "HCSR2 timeout.\n");
                bug_log(chaka);
                gps_log(chaka);
                pst_hcsr2 = 0;
                ReadStop(DEV_HCSR2);//HCSR2
                tmr_hcsr2 = get_timer_msec(msec_def);
            }
        }

        if (!pst_enc) {//-------------------   ENC  ----------------------------------
            if (check_delay_msec(tmr_enc)) {
                pst_enc = 1;
                ReadStart(DEV_ENC, 6);//надо читать 6 байт
                tmr_enc = get_timer_msec(msec_enc);
                //-------------------------------------------------------------------
            }
        }

        if (!pst && !pst_dht && !pst_adc && !pst_hcsr && !pst_hcsr2 && !pst_enc) busy = 0;

        if (fst) {
            fst = 0;
            rst_dev(DEV_GPS, bt);//reset GPS : 1=gps,bt=0/1
            ReadStart(DEV_GPS, msg_type_ind);// set msg_type to 0 - $GPMRC
            if (SANI_MSG_TYPE) *(uint8_t *)SANI_MSG_TYPE = msg_type_ind;
        }

        if (check_delay_sec(tmrs)) {
            tmrs = get_timer_sec(2);
            if (!en) en = 1;
        }

        mytv.tv_sec = 0;
        mytv.tv_usec = 10000;
        FD_ZERO(&my_read_Fds);
        my_max_fd = fd[1];
        for (id = 1; id < kol_dev; id++) {
            FD_SET(fd[id], &my_read_Fds);
            if (fd[id] > my_max_fd) my_max_fd = fd[id];
        }
        for (id = 0; id < all_radio; id++) {
            if (fd_gsm[id] > 0) {
                FD_SET(fd_gsm[id], &my_read_Fds);
                if (fd_gsm[id] > my_max_fd) my_max_fd = fd_gsm[id];
            }
        }

        if (select(my_max_fd + 1, &my_read_Fds, NULL, NULL, &mytv) > 0) {
            //--------------------   sensors device   ---------------------------------------
            for (id = 1; id < kol_dev; id++) {
                if (FD_ISSET(fd[id], &my_read_Fds)) {
                    switch (id) {
                        case DEV_GPS://1://from GPS
                            memset(buf, 0, mem_buf);
                            lens = ReadRead(id, (uint8_t *)buf);//GPS //конец пакета 0x0d,0x0a
                            if ((lens > 0) && (lens < mem_buf)) {
                                endp = strstr(buf, "\r\n");
                                if (endp) eol = 1; else eol = 0;
                                if (eol) {
                                    memset((uint8_t *)&one_pack.utc.chas, 0, sizeof(s_pack));
                                    if ( (debug_mode) && (en) ) gps_log(buf);
                                    res = parse_line(buf,lens);
                                    if ( (res == 8) && (en) && (shup) ) PrintPack(prn);//0-вывод на stdout,  1-вывод в файл log.txt
                                    memcpy((uint8_t *)SANI_PACK, web_uk, sizeof(s_pack));
                                    if (en) en = 0;
                                }// else gps_log(buf);
                            }
                        break;
                        case DEV_DS18://2://from DS18
                            if (pst) {
                                memset(rd_cmd, 0, sizeof(rd_cmd));
                                rd_uk = ReadRead(id, rd_cmd);//DS18
                                if (rd_uk > 0) {
                                    if ((data_wr_w1 == 0xBE) || (data_wr_w1 == 0xAA)) {//пересчет температуры
                                        DS18_Gradus(rd_cmd, 0);
                                        if (!ds18_temp.znak) sprintf(ds18_str, "[+");
                                                        else sprintf(ds18_str, "[-");
                                        sprintf(ds18_str+strlen(ds18_str),"%d,%d%cC]",ds18_temp.cel,ds18_temp.dro,0x9c);
                                    }
                                    byte = rd_uk;
                                    byte &= 0x0f;
                                    *(uint8_t *)SANI_W1_RD = byte;//количество прочитанных байт
                                    memcpy(SANI_W1_RD + 1, rd_cmd, byte);
                                    *(uint8_t *)SANI_W1_RD_DONE = '1';
                                    pst = 0;
                                    sprintf(chaka, "DS18 %s (%d): ", ds18_str, rd_uk);
                                    rd_cnt &= 0x0F;
                                    for (i = 0; i < rd_cnt; i++) sprintf(chaka+strlen(chaka), "0x%X ", rd_cmd[i]);
                                    strcat(chaka, "\n");
                                    gps_log(chaka);
                                    bug_log(chaka);
                                    ReadStop(id);//DS18
                                }
                            }
                        break;
                        case DEV_DHT11://3://from DHT11
                            if (pst_dht) {
                                pst_dht = 0;
                                memset(rd_dht, 0, sizeof(rd_dht));
                                rd_uk_dht = ReadRead(id, rd_dht);//DHT11
                                if (rd_uk_dht > 0) {
                                    DHT11_ALL(rd_dht);
                                    sprintf(dht11_str, "[%d,%d%% +%d,%d%cC]",
                                                       dht11.rh_cel, dht11.rh_dro, dht11.tp_cel, dht11.tp_dro,0x9c);
                                    byte = rd_uk_dht;
                                    byte &= 0x07;
                                    *(uint8_t *)SANI_W1_RD_DHT = byte;//количество прочитанных байт
                                    memcpy(SANI_W1_RD_DHT + 1, rd_dht, byte);
                                    *(uint8_t *)SANI_W1_RD_DHT_DONE = '1';
                                    sprintf(chaka, "DHT11 %s (%d): ", dht11_str, rd_uk_dht);
                                    rd_cnt_dht &= 0x0F;
                                    for (i = 0; i < rd_cnt_dht; i++) sprintf(chaka+strlen(chaka), "0x%X ", rd_dht[i]);
                                    strcat(chaka, "\n");
                                } else sprintf(chaka, "DHT11 error (rx=%d)\n", rd_uk_dht);
                                gps_log(chaka);
                                bug_log(chaka);
                                ReadStop(id);//DHT11
                            }
                        break;
                        case DEV_ADC://4://ADC
                            if (pst_adc) {
                                pst_adc = 0;
                                data_adc = ReadRead(id, rd_adc);//ADC
                                memset(SANI_ADC, 0, sizeof(s_adc));
                                data_adc_v = data_adc; data_adc_v *= 3222; data_adc_v /= 1000000;
                                ADC_ALL(data_adc_v);
                                sprintf(chaka, "ADC (%d) [%d,%d v] (%d)\n", sel_chan, adc.cel, adc.dro, data_adc);
                                bug_log(chaka);
                                gps_log(chaka);
                                ReadStop(id);//ADC
                            }
                        break;
                        case DEV_HCSR1://5://HC-SR04
                            if (pst_hcsr) {
                                pst_hcsr = 0;
                                data_hcsr = ReadRead(id,NULL);//HCSR
                                if (SANI_HCSR) {
                                    memset(SANI_HCSR, 0, sizeof(int));
                                    data_hcsr_n = htonl(data_hcsr / 58);//в сантиметрах
                                    memcpy(SANI_HCSR, (uint8_t *)&data_hcsr_n, sizeof(int));
                                }
                                sprintf(chaka, "HCSR [%d c.] (%d)\n", data_hcsr / 58, data_hcsr);
                                bug_log(chaka);
                                gps_log(chaka);
                                ReadStop(id);//HCSR
                                tmr_hcsr = get_timer_msec(msec_def);
                            }
                        break;
                        case DEV_ENC://6://ENC
                            if (pst_enc) {
                                pst_enc = 0;
                                memset(rd_enc, 0, sizeof(rd_enc));
                                memset(SANI_ENC_WAY, 0, sizeof(s_enc));
                                rd_cnt_enc = ReadRead(id, rd_enc);//ENC
                                if (rd_cnt_enc == 6) {
                                    memcpy(&enc_int, rd_enc, 4);
                                    memcpy(&enc_short, rd_enc + 4, 2);
                                    enc.way = htonl(enc_int);
                                    enc.speed = htons(enc_short);
                                    memcpy(SANI_ENC_WAY, (uint8_t *)&enc.way, sizeof(s_enc));
                                    enc_int_float = (enc_int / 600) * 0.22;
                                    enc_short_float = ((enc_short & 0x7fff) / 600) * 0.22;
                                    enc_way_sm = enc_short_float * 100;//в сантиметрах
                                    sprintf(chaka, "ENC [way=%.02f m (0x%08X) (%d sm), speed=%.02f m/sec (0x%04X)]\n",
                                                enc_int_float,
                                                enc_int,
                                                enc_way_sm,
                                                enc_short_float,
                                                enc_short);
                                } else sprintf(chaka, "ENC invalid data read, wait 6 but got %d bytes\n", rd_cnt_enc);
                                bug_log(chaka);
                                gps_log(chaka);
                                ReadStop(id);//ENC
                            }
                        break;
                        case DEV_HCSR2://7://HC-SR04-2
                            if (pst_hcsr2) {
                                pst_hcsr2 = 0;
                                data_hcsr2 = ReadRead(id, NULL);//HCSR2
                                if (SANI_HCSR2) {
                                    memset(SANI_HCSR2, 0, sizeof(int));
                                    data_hcsr2_n = htonl(data_hcsr2 / 58);//в сантиметрах
                                    memcpy(SANI_HCSR2, (uint8_t *)&data_hcsr2_n, sizeof(int));
                                }
                                sprintf(chaka, "HCSR2 [%d c.] (%d)\n", data_hcsr2 / 58, data_hcsr2);
                                bug_log(chaka);
                                gps_log(chaka);
                                ReadStop(id);//HCSR2
                                tmr_hcsr2 = get_timer_msec(msec_def);
                            }
                        break;
                    }//switch
                }
            }//for  by  sensors device
            //--------------------   GSM device  ------------------------------
            for (id = 0; id < all_radio; id++) {
                if (fd_gsm[id] > 0) {
                    if (FD_ISSET(fd_gsm[id], &my_read_Fds)) {
                        if (!gsm_rk[id].data) gsm_rk[id].data = 1;
                    }
                }
            }
            //----------------------------------------------------------
        } else usleep(1000);

        if (check_delay_sec(tmr_w1)) {
            if (pst) {
                byte = rd_uk;
                byte &= 0x0f;
                if (byte >= 9) {
                    if ((data_wr_w1 == 0xbe) || (data_wr_w1 == 0xAA)) {//пересчет температуры
                        DS18_Gradus(rd_cmd, 1);
                        if (!ds18_temp.znak) strcpy(ds18_str, "[+");
                                        else strcpy(ds18_str, "[-");
                        sprintf(ds18_str+strlen(ds18_str), "%d,%d%cC]", ds18_temp.cel, ds18_temp.dro, 0x9c);
                    }
                } else if (SANI_DS18) memset(SANI_DS18, 0xff, sizeof(s_ds18_temp));
                *(uint8_t *)SANI_W1_RD = byte;//количество прочитанных байт
                memcpy(SANI_W1_RD + 1, rd_cmd, byte);
                *(uint8_t *)SANI_W1_RD_DONE = '1';
                if (rd_uk == rd_cnt) sprintf(chaka, "DS18 %s (%d) timeout : ", ds18_str, rd_uk);//прочитано 9 байт
                else {
                    sprintf(chaka, "DS18 %s (%d) timeout : ", ds18_str, rd_uk);
                    rd_cnt = rd_uk;
                }
                rd_cnt &= 0x0F;
                if (rd_cnt > 0) {
                    for (i = 0; i < rd_cnt; i++) sprintf(chaka+strlen(chaka), "0x%X ", rd_cmd[i]);
                }
                strcat(chaka, "\n");
                gps_log(chaka);
                bug_log(chaka);
            } else *(uint8_t *)SANI_W1_RD_DONE = '1';
            pst = 0;
            ReadStop(2);//DS18

            if (pst_dht) {
                byte = rd_uk_dht;
                byte &= 0x07;
                if (byte >= 5) {
                    DHT11_ALL(rd_dht);
                    sprintf(dht11_str, "[%d,%d%% +%d,%d%cC]",
                                       dht11.rh_cel, dht11.rh_dro, dht11.tp_cel, dht11.tp_dro, 0x9c);
                }
                if (SANI_DHT11) memset(SANI_DHT11, 0xff, sizeof(s_dht11));
                *(uint8_t *)SANI_W1_RD_DHT = byte;//количество прочитанных байт
                memcpy(SANI_W1_RD_DHT+1, rd_dht, byte);
                *(uint8_t *)SANI_W1_RD_DHT_DONE = '1';
                if (rd_uk_dht == rd_cnt_dht) sprintf(chaka, "DHT11 %s (%d) timeout : ", dht11_str, rd_uk_dht);//прочитано 5 байт
                else {
                    sprintf(chaka, "DHT11 %s (%d) timeout : ", dht11_str, rd_uk_dht);
                    rd_cnt_dht = rd_uk_dht;
                }
                rd_cnt_dht &= 0x0F;
                if (rd_cnt_dht > 0) {
                    for (i = 0; i < rd_cnt_dht; i++) sprintf(chaka+strlen(chaka), "0x%X ", rd_dht[i]);
                }
                strcat(chaka, "\n");
                gps_log(chaka);
                bug_log(chaka);
            } else *(uint8_t *)SANI_W1_RD_DHT_DONE = '1';
            pst_dht = 0;
            ReadStop(DEV_DHT11);//DHT11

            if (pst_adc) {
                data_adc = ReadRead(4, rd_adc);//ADC
                memset(SANI_ADC, 0xff, sizeof(s_adc));
                data_adc_v = data_adc;
                ADC_ALL(data_adc_v);
                sprintf(chaka, "ADC (%d) timeout. (%d)\n", sel_chan, data_adc);
                bug_log(chaka);
                gps_log(chaka);
            }
            pst_adc = 0;
            ReadStop(DEV_ADC);//ADC

            tmr_w1 = get_timer_sec(10);
            busy = 0;
        }

        //-----------------  GSM   ----------------------

        N_RK_IN = *(uint8_t *)SANI_RK & 3;//max 4 channel
        if (N_RK != N_RK_IN) {
            N_RK = N_RK_IN;
            *(uint8_t *)SANI_RK = N_RK;
        }

        for (i = 0; i < all_radio; i++) {
            if (RK_table[i].present) {
                switch (gsm_rk[i].faza) {
                    case 0:
                        if (!gsm_rk[i].block) {
                            gsm_rk[i].faza = 1;
                            gsm_rk[i].reg_ok = 0;
                        }
                    break;
                    case 1:
                        if (check_delay_msec(tmr1[i])) {
                            gsm_rk[i].cmd = 0;
                            gsm_rk[i].cmd_ind = 0;
                            gsm_rk[i].err_main = 0;
                            gsm_rk[i].err = 0;
                            gsm_rk[i].err_count = 0;
                            GSMStartStop(i, (uint8_t)SET_GSM_START);
                            ModulePWR_ON(i);
                            tmr1[i] = get_timer_msec(_2s);
                            gsm_rk[i].faza = 2;
                        }
                    break;
                    case 2:
                        if (check_delay_msec(tmr1[i])) {
                            Module_OFF(i);
                            tmr1[i] = get_timer_msec(_3s);
                            gsm_rk[i].faza = 3;
                        }
                    break;
                    case 3:
                        if (check_delay_msec(tmr1[i])) {
                            Module_ON(i);
                            tmr1[i] = get_timer_msec(_1s);
                            gsm_rk[i].faza = 4;//to send at-commands or sleep
                        }
                    break;
                    case 4://loop   aka sfaza8
                        if ((gsm_rk[i].block) || (!loops) || (gsm_rk[i].err_main>=max_err_main)) {
                            Module_OFF(i);
                            tmr1[i] = get_timer_msec(_5s);
                            gsm_rk[i].faza = 6;
                        } else {//at-commands loop (by list or from web
                            if ((ATCOM.flag_) && (i == N_RK)) {
                                if (gsm_rk[i].cmd) {//если канал свободен
                                    if (strlen((char *)ATCOM.com_) > 0) {
                                        gsm_rk[i].cmd = 0;
                                        gsm_rk[i].answer = 0;
                                        gsm_rk[i].idle = 0;
                                        gsm_rk[i].err_count = 0;
                                        gsm_rk[i].err = 0; gsm_rk[i].answer = 0;
                                        memset(stz, 0, sizeof(stz)); strcpy(stz, ATCOM.com_); put_AT_com(stz, i);
                                        memset(ATCOM.txt_, 0, 200);
                                        tmr1[i] = get_timer_msec(_20s);
                                        gsm_rk[i].faza = 5;
                                    } else {
                                        ATCOM.flag_ = 0;
                                        tmr1[i] = get_timer_msec(_1s);
                                        gsm_rk[i].idle = 1;
                                    }
                                } else {
                                    if (check_delay_msec(tmr1[i])) {
                                        gsm_rk[i].cmd = 1;
                                        tmr1[i] = get_timer_msec(_20s);
                                    }
                                }
                            } else {
                                if (!pwronly) {
                                    if (check_delay_msec(tmr1[i])) {
                                        if (gsm_rk[i].cmd) {
                                            gsm_rk[i].cmd = 0;
                                            switch (RK_table[i].type) {
                                                case 2://5215
                                                    put_AT_com(commands5215[gsm_rk[i].cmd_ind], i);
                                                break;
                                                default : put_AT_com(commands10[gsm_rk[i].cmd_ind], i);
                                            }
                                            gsm_rk[i].answer = 0;
                                        }
                                    }
                                }//sleep
                            }
                        }
                    break;
                    case 5://AT-command
                        if ((gsm_rk[i].cmd) && (gsm_rk[i].answer)) {
                            tmr1[i] = get_timer_msec(_1s);
                            gsm_rk[i].idle = 1;
                            gsm_rk[i].faza = 4;//8
                        }
                        if (check_delay_msec(tmr1[i])) {
                            tmr1[i] = get_timer_msec(_1s);
                            gsm_rk[i].idle = 1;
                            gsm_rk[i].faza = 4;//8
                        }
                    break;
                    case 6:
                        if ((!check_vio(i)) || (check_delay_msec(tmr1[i]))) {
                            GSMStartStop(i, (uint8_t)SET_GSM_STOP);
                            ModulePWR_OFF(i);
                            tmr1[i] = get_timer_msec(_30s);//_2s//_1s
                            gsm_rk[i].faza    = 0;
                            gsm_rk[i].rx_faza = 0;
                            gsm_rk[i].tx_faza = 0;
                            if (!loops) stop_all_rk |= ((uint8_t)1 << i);
                        }
                    break;
                }

                switch (gsm_rk[i].rx_faza) {
                    case 0:
                        if (check_vio(i)) gsm_rk[i].rx_faza = 1;
                    break;
                    case 1:
                        if (gsm_rk[i].data) {
                            memset(&gsm_rk[i].RX[0], 0, tmp_buf_size);
                            gsm_rk[i].rx_len   = 0;
                            gsm_rk[i].rx_ready = 0;
                            gsm_rk[i].rx_faza  = 2;
                            gsm_rk[i].rx_tmr   = get_timer_msec(_30s);
                        }
                    break;
                    case 2:
                        gsm_len = RXbytes22(i, &gsm_rk[i].RX[gsm_rk[i].rx_len]);
                        if (gsm_len > 0) {
                            if ((gsm_rk[i].rx_len + gsm_len) > tmp_buf_size) {
                                gsm_len = tmp_buf_size - gsm_rk[i].rx_len - 1;
                                gsm_rk[i].rx_ready = 1;
                            }
                            gsm_rk[i].rx_len += gsm_len;
                            if (strchr((char *)&gsm_rk[i].RX[0], 0x0d) != NULL) gsm_rk[i].rx_ready = 1;
                            if (gsm_rk[i].rx_ready) {
                                if (i == N_RK) {
                                    sprintf(stx, "[%02d] rx: %s", i + 1, (char *)&gsm_rk[i].RX[0]);
                                    print_rk(stx);
                                }
                                if (ATCOM.flag_) {
                                    j = ATCOM.dl_ + gsm_rk[i].rx_len;
                                    if (j < 200) j = gsm_rk[i].rx_len; else j = 200 - ATCOM.dl_;
                                    memcpy(&ATCOM.txt_[ATCOM.dl_], (char *)gsm_rk[i].RX, j);
                                    ATCOM.dl_ += (uint8_t)j;
                                }
                                gsm_rk[i].rx_faza = 3;
                            } else {
                                gsm_rk[i].rx_faza = 1;
                                gsm_rk[i].data    = 0;
                            }
                        } else {
                            if (!gsm_rk[i].rx_len) {
                                gsm_rk[i].rx_faza = 1;
                                gsm_rk[i].data    = 0;
                            } else {
                                if (check_delay_msec(gsm_rk[i].rx_tmr)) {
                                    gsm_rk[i].rx_faza = 1;
                                    gsm_rk[i].data    = 0;
                                }
                            }
                        }
                    break;
                    case 3://анализ сообщения/ответа от модуля
                        if (strstr((char *)&gsm_rk[i].RX[0],"PB DONE") != NULL) {
                            gsm_rk[i].cmd = 1;
                            gsm_rk[i].err = 0;
                        } else if (strstr((char *)&gsm_rk[i].RX[0],"+CPIN: READY") != NULL) {
                            if (RK_table[i].type != 2) {
                                gsm_rk[i].cmd = 1;
                                gsm_rk[i].err = 0;
                            }
                        } else if (strstr((char *)&gsm_rk[i].RX[0],"OK") != NULL) {
                            gsm_rk[i].cmd    = 1;
                            gsm_rk[i].err    = 0;
                            gsm_rk[i].answer = 1;
                        } else if (strstr((char *)&gsm_rk[i].RX[0],"ERR") != NULL) {
                            gsm_rk[i].cmd    = 1;
                            gsm_rk[i].err    = 1;
                            gsm_rk[i].answer = 1;
                            if (strstr((char *)&gsm_rk[i].RX[0],"SIM failure") != NULL) {
                                gsm_rk[i].answer   = 0;
                                gsm_rk[i].err_main = max_err_main;
                            }
                        } else if (strstr((char *)&gsm_rk[i].RX[0],"+CREG: ") != NULL) {
                            if ( (strstr((char *)&gsm_rk[i].RX[0]," 2,1") != NULL) ||
                                    (strstr((char *)&gsm_rk[i].RX[0]," 0,1") != NULL) ||
                                    (strstr((char *)&gsm_rk[i].RX[0]," 2,5") != NULL) ||
                                    (strstr((char *)&gsm_rk[i].RX[0]," 0,5") != NULL) ) gsm_rk[i].reg_ok = 1;
                            else if ( (strstr((char *)&gsm_rk[i].RX[0]," 2,0") != NULL) ||
                                        (strstr((char *)&gsm_rk[i].RX[0]," 0,0") != NULL) ||
                                        (strstr((char *)&gsm_rk[i].RX[0]," 0,3") != NULL) ||
                                        (strstr((char *)&gsm_rk[i].RX[0]," 2,3") != NULL) ||
                                        (strstr((char *)&gsm_rk[i].RX[0]," 2,2") != NULL) ||
                                        (strstr((char *)&gsm_rk[i].RX[0]," 0,2") != NULL) ) gsm_rk[i].reg_ok = 0;
                        } else gsm_rk[i].answer = 0;

                        if (gsm_rk[i].answer) {
                            if (ATCOM.flag_) {
                                ATCOM.flag_ = 0;
                                send_to_web((uint8_t *)&ATCOM.txt_[0], ATCOM.dl_);
                                gsm_rk[i].err       = 0;
                                gsm_rk[i].err_count = 0;
                                tmr1[i] = get_timer_msec(_1s);
                            } else {
                                if (gsm_rk[i].err) {
                                    gsm_rk[i].err_count++;
                                    if (gsm_rk[i].err_count > 2) {
                                        gsm_rk[i].err       = 0;
                                        gsm_rk[i].err_count = 0;
                                        gsm_rk[i].cmd_ind++;
                                        if (gsm_rk[i].cmd_ind >= max_at) {
                                            if (!gsm_rk[i].reg_ok) gsm_rk[i].cmd_ind = 0;
                                            else {
                                                if (gsm_rk[i].creg_ind>=max_at) gsm_rk[i].cmd_ind = 0;
                                                                           else gsm_rk[i].cmd_ind = gsm_rk[i].creg_ind;
                                            }
                                            tmr1[i] = get_timer_msec(_1m);
                                        }
                                        gsm_rk[i].err_main++;
                                    } else tmr1[i] = get_timer_msec(_500ms);
                                } else {
                                    gsm_rk[i].err       = 0;
                                    gsm_rk[i].err_count = 0;
                                    gsm_rk[i].cmd_ind++;
                                    if (gsm_rk[i].cmd_ind >= max_at) {
                                        if (!gsm_rk[i].reg_ok) gsm_rk[i].cmd_ind = 0;
                                        else {
                                            if (gsm_rk[i].creg_ind >= max_at) gsm_rk[i].cmd_ind = 0;
                                                                         else gsm_rk[i].cmd_ind = gsm_rk[i].creg_ind;
                                        }
                                        tmr1[i] = get_timer_msec(_1m);
                                    }
                                }
                            }
                        }

                        gsm_rk[i].rx_faza = 1;
                        gsm_rk[i].data    = 0;
                    break;
                    default : {
                        gsm_rk[i].data    = 0;
                        gsm_rk[i].rx_faza = 1;
                    }
                }

                switch (gsm_rk[i].tx_faza) {
                    case 0 : break;
                    case 1 :
                        gsm_rk[i].tx_len  = 0;  //предустановить количество переданных символов команды
                        gsm_rk[i].tx_faza = 2; //на выдачу команды
                    break;
                    case 2 :
                        gsm_rk[i].tx_faza = TXCOM2_all(i);
                    break;
                    default : gsm_rk[i].tx_faza = 0;
                }
            }//rk present
        }
        usleep(1000);
        //-----------------------------------------------

    }//while (loops).... end

    for (i = 0; i < all_radio; i++) tmr1[i] = get_timer_msec(_2s);

    tmrs = get_timer_sec(10);
    while (1 < 2) {
        for (i = 0; i < all_radio; i++) {
            en = 0;
            en = (uint8_t)1 << i;
            if (RK_table[i].present) {
                if (check_vio(i)) {//есть vio
                    sz = RK_table[i].mir & ModOnOff;//get module_on/off bit
                    if (sz) {//Module_OFF не выполнено
                        Module_OFF(i);
                        tmr1[i] = get_timer_msec(_5s);
                    } else {//Module_OFF выполнено
                        if (check_delay_msec(tmr1[i])) {
                            if (!(stop_all_rk & en)) {
                                stop_all_rk |= en;
                                GSMStartStop(i, (uint8_t)SET_GSM_STOP);
                                ModulePWR_OFF(i);
                                tmr1[i] = get_timer_msec(_5s);
                            }
                        }
                    }
                } else {//нет vio
                    if (!(stop_all_rk & en)) {
                        stop_all_rk |= en;
                        GSMStartStop(i, (uint8_t)SET_GSM_STOP);
                        ModulePWR_OFF(i);
                    }
                }
            } else stop_all_rk |= en;
        }
        if (stop_all_rk >= 0x0f) {
            strcpy(chaka, "All channels shutdown.\n");
            print_rk(chaka);
            break;
        } else if (check_delay_sec(tmrs)) {
            strcpy(chaka, "All channels shutdown by timer.\n");
            print_rk(chaka);
            break;
        }
    }
//---------------------------------------------------------------------------------
    ReadStop(DEV_ENC);
    ReadStop(DEV_HCSR1);
    ReadStop(DEV_HCSR2);
    ReadStop(DEV_ADC);
    ReadStop(DEV_DHT11);
    ReadStop(DEV_DS18);
    ReadStop(DEV_GPS);
    init_motor();
    SetModeMotor(SET_MODE_PULT);
    OutOfJob();

    return 0;

}

