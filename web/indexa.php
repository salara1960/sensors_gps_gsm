<?php
header("Cache-Control: no-cache, must-revalidate");
header("Expires: Sat, 26 Jul 1970 05:00:00 GMT");

require_once("imoto.php");

$all_dta = ""; $the = "^"; $undef = "???";

if ($shm_id > 0) {
    $rf = shmop_read($shm_id, $sani, $sani_at);
    $j = $my_block_size - $sani_pack;
    for ($i = 0; $i < $j; $i++) $rdf[$i] = ord($rf[$i + $sani_pack]);
    $data_ok = $rdf[6];//data valid
    $shir_flag = $undef; $dolg_flag = $undef;
    if ($data_ok == 1) {
        $utc_ms = ($rdf[4]<<8)|$rdf[3];
        $shir_grad = ($rdf[8]<<8)|$rdf[7];
        $shir_min = ($rdf[10]<<8)|$rdf[9];
        $shir_sec = ($rdf[12]<<8)|$rdf[11];
        $dolg_grad = ($rdf[15]<<8)|$rdf[14];
        $dolg_min = ($rdf[17]<<8)|$rdf[16];
        $dolg_sec = ($rdf[19]<<8)|$rdf[18];
        $speed_m = ($rdf[22]<<8)|$rdf[21];
        $course_grad = ($rdf[24]<<8)|$rdf[23];
        $course_min = ($rdf[26]<<8)|$rdf[25];
        $utc = sprintf("%02d/%02d/%02d %02d:%02d:%02d.%03d",$rdf[27],$rdf[28],$rdf[29] ,$rdf[0],$rdf[1],$rdf[2],$utc_ms);
        $speed_meas = sprintf("%.3f km/h",$speed_m/1000);
             if ($rdf[13] == 1) $shir_flag=" North";//северная широта
        else if ($rdf[13] == 2) $shir_flag=" South";//южная широта
        $shir = sprintf("%d%c%d'%d\" %s",$shir_grad,$symb, $shir_min, $shir_sec, $shir_flag);
             if ($rdf[20] == 1) $dolg_flag=" East";//восточная долгота
        else if ($rdf[20] == 2) $dolg_flag=" West";//западная долгота
        $dolg = sprintf("%d%c%d'%d\" %s",$dolg_grad,$symb, $dolg_min, $dolg_sec, $dolg_flag);
        $course = sprintf("%d%c%d'",$course_grad,$symb,$course_min);
    } else {
        $shir   = $undef; $dolg       = $undef;
        $utc    = $undef; $speed_meas = $undef;
        $course = $undef;
    }

    $rd_data = ord(shmop_read($shm_id, $sani_motor_data + 1, 1));

    $ds18 = shmop_read($shm_id, $sani_ds18, $ds18size);
    if (ord($ds18[1]) == 0xff) $rd_data_temp=$undef;
    else {
        if (ord($ds18[0]) == 0) $zn = "+"; else $zn = "-";
        $rd_data_temp = sprintf("%s%d,%d%c C ",$zn,ord($ds18[1]),ord($ds18[2]),$symb);
    }

    $rdb = shmop_read($shm_id, $sani_dht11, $dht11size);
    if (ord($rdb[0]) != 255) {
        $rd_data_temp_dht_v = sprintf("%d,%d %%",ord($rdb[0]),ord($rdb[1]));
        $rd_data_temp_dht_t = sprintf("+%d,%d%c C",ord($rdb[2]),ord($rdb[3]),$symb);
    } else {
        $rd_data_temp_dht_v = $undef;
        $rd_data_temp_dht_t = $undef;
    }

    $rdb = shmop_read($shm_id, $sani_adc, $adcsize);
    if (ord($rdb[0]) != 255) {
        $adc_c = (ord($rdb[1])<<8) | ord($rdb[0]);
        $adc_d = (ord($rdb[3])<<8) | ord($rdb[2]);
        $adcs = $adc = $adc_c+($adc_d/1000);
        $fg = 5.98;
        $rd_data_temp_adc = sprintf("%.3f v (%.3f)",$adc*$fg,$adcs);
    } else $rd_data_temp_adc = $undef;

    $srv_dt = strftime('%d/%m/%Y  %H:%M:%S', time());

    $r_h = shmop_read($shm_id, $sani_hcsr + 2, 2);
    $rhcsr = ord($r_h[1]) | ((ord($r_h[0]))<<8);
    if ($rhcsr == 0xffff) $rd_data_temp_hcsr = $undef;
                     else $rd_data_temp_hcsr = sprintf("%d",$rhcsr);
    $r_h = shmop_read($shm_id, $sani_hcsr2 + 2, 2);
    $rhcsr2 = ord($r_h[1]) | ((ord($r_h[0]))<<8);
    if ($rhcsr2 == 0xffff) $rd_data_temp_hcsr2 = $undef;
                      else $rd_data_temp_hcsr2 = sprintf("%d",$rhcsr2);

    $ec0 = shmop_read($shm_id, $sani_enc_way, 6);
    for ($i = 0; $i < 6; $i++) $ec1[$i] = ord($ec0[$i]);
    $ec_way = (int)($ec1[0]<<14 | $ec1[1]<<16 | $ec1[2]<<8 | $ec1[3]);
    $ec_speed = (int)($ec1[4]<<8|$ec1[5]);
    $w_in_sec = ($ec_way/600)*0.22;
    $s_in_sec = (($ec_speed&0x7fff)/600)*0.22; $dst = ""; if ($ec_speed&0x8000) $dst = "-";
    $e_way = sprintf("%.02f m",$w_in_sec);
    $e_speed = sprintf("%s%.02f m/sec",$dst,$s_in_sec);
    $ec = $e_way.$the.$e_speed;

    $all_d = $utc.$the.$shir.$the.$dolg.$the.$speed_meas.$the.$course.$the.$srv_dt.$the.$rd_data.$the.$rd_data_temp;
    $all_d.= $the.$rd_data_temp_dht_v.$the.$rd_data_temp_dht_t.$the.$rd_data_temp_adc.$the.$rd_data_temp_hcsr;
    $all_d.= $the.$ec.$the.$rd_data_temp_hcsr2;

    echo $all_d;

} else include "404.php";


?>
