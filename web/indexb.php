<?php

require_once("imoto.php");

    if ($_POST["main_bt"]=="SUBMIT") {

        $sel_chan="?"; $undef="???";
	$rd_data=ord($rdf[$sani_motor_data+1]);//[2]  speed
	if ($shm_id>0) {
	    $pst=0;
	    $ka=$_POST["wd"];
	    if ($ka!="") { $nk=chr($ka); shmop_write($shm_id, $nk, $sani_motor_data+1); } //speed
	    if ($nk!="") { $pst=1; $fl='1'; shmop_write($shm_id, $fl, $sani_motor_post); }
	    $wr_data_temp=$_POST["wd_temp"]; 
	    if ($wr_data_temp!="") {
		$wrt="";
		if ($wr_data_temp=="cmd01") $wrt=chr(51);
		else
		if ($wr_data_temp=="cmd02") $wrt=chr(68);
		else
		if ($wr_data_temp=="cmd03") $wrt=chr(190);
		if ($wrt!="") {
		    $pst=1;
		    shmop_write($shm_id, $wrt, $sani_w1_wr);//wr_data
		}
	    }
	    $sel_temp_adc_chan=$_POST["wr_temp_adc_chan"];
	    if ($sel_temp_adc_chan!="") {
		$wrt="?";
		if ($sel_temp_adc_chan=="chan0") $wrt=chr(0);
		else
		if ($sel_temp_adc_chan=="chan1") $wrt=chr(1);
		else
		if ($sel_temp_adc_chan=="chan2") $wrt=chr(2);
		else
		if ($sel_temp_adc_chan=="chan3") $wrt=chr(3);
		if ($wrt!="?") {
		    $pst=1;
		    shmop_write($shm_id, $wrt, $sani_adc_chan);
		    $sel_chan = $wrt;
		}
	    }
	    if ($pst==1) {
		$cmd='1';
		shmop_write($shm_id, $cmd, $sani);//post
		$waits=1; $j=0;
		while ($waits) {
		    usleep(50000); $j++;
		    $dataready=shmop_read($shm_id, $sani_w1_rd_done, 2);//$sani_w1_rd_done and $sani_w1_rd_dht_done
		    if ($dataready=="11") $waits=0;
		    else if ($j>=5) $waits=0;
		}

		$rd_data=ord(shmop_read($shm_id, $sani_motor_data+1, 1));
		
		$ds18=shmop_read($shm_id, $sani_ds18, $ds18size);
		if (ord($ds18[1])==0xff) $rd_data_temp=$undef;
		else {
		    if (ord($ds18[0])==0) $zn="+"; else $zn="-";
		    $rd_data_temp=sprintf("%s%d,%d%c C ",$zn,ord($ds18[1]),ord($ds18[2]),$symb);
		}

		$rdb=shmop_read($shm_id, $sani_dht11, $dht11size);
		if (ord($rdb[0])!=255) {
		    $rd_data_temp_dht_v=sprintf("%d,%d %%",ord($rdb[0]),ord($rdb[1]));
		    $rd_data_temp_dht_t=sprintf("+%d,%d%c C",ord($rdb[2]),ord($rdb[3]),$symb);
		} else {
		    $rd_data_temp_dht_v=$undef;
		    $rd_data_temp_dht_t=$undef;
		}
		$rdb=shmop_read($shm_id, $sani_adc, $adcsize);
		if (ord($rdb[0])!=255) {
		    $adc_c=(ord($rdb[1])<<8) | ord($rdb[0]);
		    $adc_d=(ord($rdb[3])<<8) | ord($rdb[2]);
		    $adcs=$adc=$adc_c+($adc_d/1000);
		    $fg=5.98;
		    $rd_data_temp_adc=sprintf("%.3f v (%.3f)",$adc*$fg,$adcs);
		} else $rd_data_temp_adc=$undef;
		    
	    }
	    $sel_chan=ord(shmop_read($shm_id, $sani_adc_chan, 1));

	    $r_h=shmop_read($shm_id, $sani_hcsr+2, 2);
	    $rhcsr = ord($r_h[1]) | ((ord($r_h[0]))<<8);
	    if ($rhcsr==0xffff) $rd_data_temp_hcsr=$undef;
			   else $rd_data_temp_hcsr=sprintf("%d",$rhcsr);
	    $r_h=shmop_read($shm_id, $sani_hcsr2+2, 2);
	    $rhcsr2 = ord($r_h[1]) | ((ord($r_h[0]))<<8);
	    if ($rhcsr2==0xffff) $rd_data_temp_hcsr2=$undef;
			   else $rd_data_temp_hcsr2=sprintf("%d",$rhcsr2);
	    $sel_smg_type=$_POST["msg_type"];
	    if ($sel_smg_type!="") {
		$wrtype="?";
		for ($i=0; $i<$max_known_type; $i++) {
		    $tnm="t_".$i;
		    if ($sel_smg_type == $tnm) { $wrtype=chr($i); $save_i=$i; break; }
		}
		if ($wrtype!="?") {
		    if ($mtp!=$save_i) {
			$mtp=$save_i;
			shmop_write($shm_id, $wrtype, $sani_msg_type_set);
			$fltp="1"; shmop_write($shm_id, $fltp, $sani_msg_type_post);
		    }
		}
	    }
	}
	header("Location: ./");
    }    
?>
