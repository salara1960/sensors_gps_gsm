<?php
$my_block_size=340;
$sizeofpack=30;
$ds18size=3;
$dht11size=4;
$adcsize=4;
$sani=0;
$sani_wr_speed=1;
$sani_rd_speed=2;
$sani_motor_post=3;
$sani_w1_wr=4;
$sani_w1_rd_done=5;
$sani_w1_rd_dht_done=6;
$sani_adc_chan=7;
$sani_pack=8;
$sani_w1_rd=38;//len+15 byte of data = 16
$sani_ds18=48;//len=3 (znak,cel,dro)
$sani_w1_rd_dht=51;//len+7 byte of data = 8
$sani_dht11=57;//len=4 bytes
$sani_adc=61;//len=4 bytes
$sani_hcsr=65;//len=4 bytes
$sani_msg_type=69;//len=1 bytes
$sani_msg_type_set=70;//len=1 bytes
$sani_msg_type_post=71;//len=1 bytes
$sani_enc_way=72;//len=4 bytes
$sani_enc_speed=76;//len=2 bytes
$sani_motor_data=78;//len=4 bytes; mode,speed,dir,onoff
$sani_post_at=82;//len=1 byte
$sani_rk=83;//len=1 byte (N_RK)
$sani_at=84;//len=256 bytes
$max_known_type=6;
$all_known_type = array('GPRMC','GPGGA','GPGSV','GPGL','GPGSA','GPVTG');
$symb=0x9C;
$key=ftok("/etc/passwd",'R');
$shm_id = shmop_open($key, "w", 0, 0);
if ($shm_id<=0) include "404.php";
$rk=ord(shmop_read($shm_id, $sani_rk, 1));
?>
