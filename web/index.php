<?php

require_once("imoto.php");

    $sel_chan="?"; $undef="???";

    if ($shm_id>0) {
	$rf=shmop_read($shm_id, $sani, $sani_at);//from 0 to $sani_at

	$motor_mode  = ord($rf[$sani_motor_data]);
	$motor_speed = ord($rf[$sani_motor_data+1]);
	$motor_dir   = ord($rf[$sani_motor_data+2]);
	$motor_onoff = ord($rf[$sani_motor_data+3]);
	
	$j=$my_block_size-$sani_pack;
	for ($i=0; $i<$j; $i++) $rdf[$i] = ord($rf[$i+$sani_pack]);
	$data_ok=$rdf[6];//data valid //4
	if ($data_ok==1) {
	    $utc_ms=($rdf[4]<<8)|$rdf[3];
	    $shir_grad=($rdf[8]<<8)|$rdf[7];
	    $shir_min=($rdf[10]<<8)|$rdf[9];
	    $shir_sec=($rdf[12]<<8)|$rdf[11];
	    $dolg_grad=($rdf[15]<<8)|$rdf[14];
	    $dolg_min=($rdf[17]<<8)|$rdf[16];
	    $dolg_sec=($rdf[19]<<8)|$rdf[18];
	    $speed_m=($rdf[22]<<8)|$rdf[21];
	    $course_grad=($rdf[24]<<8)|$rdf[23];
	    $course_min=($rdf[26]<<8)|$rdf[25];
	    $utc=sprintf("%02d/%02d/%02d %02d:%02d:%02d.%03d",$rdf[27],$rdf[28],$rdf[29] ,$rdf[0],$rdf[1],$rdf[2],$utc_ms);
	    $speed_meas=sprintf("%.3f km/h",$speed_m/1000);
	    if ($rdf[13]==1) $shir_flag=" North";//северная широта
	    else if ($rdf[13]==2) $shir_flag=" South";//южная широта
	    else $shir_flag=$undef;
	    $shir=sprintf("%d%c%d'%d\" %s",$shir_grad,$symb, $shir_min, $shir_sec, $shir_flag);
	    if ($rdf[20]==1) $dolg_flag=" East";//восточная долгота
	    else if ($rdf[20]==2) $dolg_flag=" West";//западная долгота
	    else $dolg_flag=$undef;
	    $dolg=sprintf("%d%c%d'%d\" %s",$dolg_grad,$symb, $dolg_min, $dolg_sec, $dolg_flag);
	    $course=sprintf("%d%c%d'",$course_grad,$symb,$course_min);
	} else {
	    $shir=$undef; $shir_flag=$undef;
	    $dolg=$undef; $dolg_flag=$undef;
	    $utc=$undef; $speed_m=$undef;
	    $course=$undef;
	}
	$sel_chan=ord(shmop_read($shm_id, $sani_adc_chan, 1));

	$mtp=ord(shmop_read($shm_id, $sani_msg_type, 1));
	if ($mtp>=$max_known_type) $msg_type=sprintf("??? (%d)",$mtp);
			      else $msg_type=sprintf("$%s",$all_known_type[$mtp]);
	
	$enc0=shmop_read($shm_id, $sani_enc_way, 6);
	for ($i=0; $i<6; $i++) $enc1[$i]=ord($enc0[$i]);
	$ec_way=(int)($enc1[0]<<24 | $enc1[1]<<16 | $enc1[2]<<8 | $enc1[3]);
	$ec_speed=(int)($enc1[4]<<8 | $enc1[5]);
	$w_in_sec=($ec_way/600)*0.22;
	$s_in_sec=(($ec_speed&0x7fff)/600)*0.22; if ($ec_speed&0x8000) $dst="-";
	$enc_way=sprintf("%.02f m",$w_in_sec);
	$enc_speed=sprintf("%s%.02f m/sec",$dst,$s_in_sec);
	

    } else include "404.php";


    $srv_dt=strftime('%d/%m/%Y  %H:%M:%S', time());

    
?>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="">
<head>
    <meta charset="UTF-8">
    <title>Мой сосед Тоторо</title>

<!--    <script src="../js/jquery.min.js"></script> -->

<script language="javascript" type="text/javascript">
<!--
var tID = null;
var xmlHttp = new XMLHttpRequest();
function setInnerText(element, text) {
    if (!element) return false;
    if(typeof(element.innerText)!= 'undefined') element.innerText=text;
					   else element.textContent=text;
}

function Refresh() {
var url = "indexa.php";
xmlHttp.open("GET", url, false);
xmlHttp.send(null);
    if (xmlHttp.readyState == 4) {
	var response = xmlHttp.responseText;
	var alls = response.split("^");
	var dl = alls.length;
	if (dl>=15) {
	    setInnerText(document.getElementById("id_utc"), alls[0]);
	    setInnerText(document.getElementById("id_shir"), alls[1]);
	    setInnerText(document.getElementById("id_dolg"), alls[2]);
	    setInnerText(document.getElementById("id_spd"), alls[3]);
	    setInnerText(document.getElementById("id_course"), alls[4]);
	    setInnerText(document.getElementById("id_srv_dt"), alls[5]);
	    setInnerText(document.getElementById("rd_speed"), alls[6]);
	    setInnerText(document.getElementById("rd_temp"), alls[7]);
	    setInnerText(document.getElementById("rd_temp_dht_v"), alls[8]);
	    setInnerText(document.getElementById("rd_temp_dht_t"), alls[9]);
	    setInnerText(document.getElementById("rd_temp_adc"), alls[10]);
	    setInnerText(document.getElementById("rd_temp_hcsr"), alls[11]);
	    setInnerText(document.getElementById("rd_temp_hcsr2"), alls[14]);
	    setInnerText(document.getElementById("enc_way_id"), alls[12]);
	    setInnerText(document.getElementById("enc_speed_id"), alls[13]);
	}
    }
    tID = setTimeout(Refresh,1000);
}

function StartRefreshTimer() { tID = setTimeout(Refresh,1000); }

window.onload = StartRefreshTimer;
-->
</script>

<script>
function mychk(ID,VALUE) {
    var val;
    var uri;
	if (ID == 'wr_speed_id') val = document.getElementById("wr_speed_id").value;
	else val = VALUE;
	uri='indexc.php?data='+ID+'/'+val;
	xhr=new XMLHttpRequest();
	xhr.open('GET',uri,true);
	xhr.send(null);
//    if (xhr.readyState == 4) {
//	var res = xhr.responseText;
//	alert('uri='+uri+' | res=['+res+']');
//    }

}
</script>


</head>

<style>

body, ul, li {
	margin: 0;
	padding: 0;
	background:#e3ebf0;
	
}

.conti {
	width: 860px;
	padding: 2px;
	margin-top: 18px;
	font-family: Arial;
	font-size: 12px;
}

table {
    width: 860px;
    border: 1px solid #B2B2B2;
    margin: 0 auto 0;
    background-color: #c7c7c7;
    border-spacing: 1px;
    border-radius: 5px;
    box-shadow: 0 1px 0 rgba(0, 0, 0, 0.3), 0 2px 2px -1px rgba(0, 0, 0, 0.5), 0 1px 0 rgba(255, 255, 255, 0.3) inset;
}

tr:hover {
    background-color: #d9d9d9;
}

th {
    width:16%;
    padding: 5px 0;
    background-color: #F0F0F0;
    border-bottom: 1px solid #999;
    text-align: center;
}

th2 {
    width:100%;
    padding: 8px 8px;
    background-color: #c7c7c7;
    text-align: center;
    font-family: Tahoma;
    font-size: 16px;
    border-radius: 5px;
}

td {
    color: #333;
    padding: 3px 3px;
    text-align: center;
}

/* bottoms */

input[type="submit"] {
    display: block;
    margin: 10px auto;
    padding: 8px 8px;
    color: #555;
    cursor: pointer;
    font: bold 14px Tahoma;
    box-shadow: 0 1px 0 rgba(0, 0, 0, 0.3), 0 2px 2px -1px rgba(0, 0, 0, 0.5), 0 1px 0 rgba(255, 255, 255, 0.3) inset;
    background: -moz-linear-gradient(#FDFDFD, #D3D3D3);
    background: -o-linear-gradient(#FDFDFD, #D3D3D3);
    border: 1px solid #9D9D9D;
    border-radius: 3px;
}

input[type="submit"]:hover {
    background: -moz-linear-gradient(#FDFDFD, #E1E1E1);
    background: -o-linear-gradient(#FDFDFD, #E1E1E1);
}

input[type="submit"]:active {
    position: relative;
    top: 2px;
    background: #E9E9E9;
    box-shadow: 0 1px 1px #353535 inset;
}

input[type="password"],
input[type="text"] {
    margin: 2px;
    padding: 1px 1px;
    color:blue;
    background-color:f3f3f3;
    cursor: pointer;
    font-size:12.5px;
    font-family:Tahoma;
    border-radius: 2px;
}

.speed {
    width: 60px;
    text-align: center;
}


</style>

<body>

<div style="text-left:right; margin:18px;">
    <th2 id="id_srv_dt"><?=$srv_dt?></th2>
    <a target="_blank" href="at.php">AT COMMANDS</a>
</div>

<center>

<div class="conti">

    <form action="indexb.php" method="post">
    <table>
        <thead>
            <tr>
                <th2>Motor</th2>
            </tr>
	    <tr>
		<th>Mode</th>
		<th>Current speed</th>
                <th>Set speed</th>
		<th>Manual/Stop/Start</th>
		<th>Direction</th>
            </tr>
        </thead>
        <tbody>
	    <tr>
		<td>pult
<input id="mode_id" type="radio" name="m_mode" value="0" <?php if ($motor_mode==0) echo "CHECKED";?> onclick="mychk(id,value)">
<input id="mode_id" type="radio" name="m_mode" value="1" <?php if ($motor_mode==1) echo "CHECKED";?> onclick="mychk(id,value)">
		    soft
		</td>
		<td id="rd_speed"><?=$rd_data?></td>
                <td>
		    <input type="text" id="wr_speed_id" name="wd" value="<?=$wr_data?>" class="speed" maxlength="3" >
		    <input type="button" id="speed_id" name="wd_but" value="set" onclick="mychk('wr_speed_id',value)">
		</td>
		<td>
<input id="onoff_id" type="radio" name="m_onoff" value="0" <?php if ($motor_onoff==0) echo "CHECKED";?> onclick="mychk(id,value)">
<input id="onoff_id" type="radio" name="m_onoff" value="1" <?php if ($motor_onoff==1) echo "CHECKED";?> onclick="mychk(id,value)">
<input id="onoff_id" type="radio" name="m_onoff" value="2" <?php if ($motor_onoff==2) echo "CHECKED";?> onclick="mychk(id,value)">
		</td>
		<td>->
<input id="dir_id" type="radio" name="m_dir" value="0" <?php if ($motor_dir==0) echo "CHECKED";?> onclick="mychk(id,value)">
<input id="dir_id" type="radio" name="m_dir" value="1" <?php if ($motor_dir==1) echo "CHECKED";?> onclick="mychk(id,value)">
		    <-
		</td>
            </tr>
        </tbody>
    </table>
    <br>
    <br>
    <table>
        <thead>
	    <tr>
                <th2>GPS</th2>
            </tr>
        </thead>
	<thead>
	    <tr>
		<th>UTC</th>
                <th>Latitude</th>
                <th>Longitude</th>
                <th>Meas.Speed</th>
		<th>Course</th>
		<th>Msg type</th>
            </tr>
        </thead>
	<tbody>
	    <tr>
		<td id="id_utc"><?=$utc?></td>
		<td id="id_shir"><?=$shir?></td>
		<td id="id_dolg"><?=$dolg?></td>
		<td id="id_spd"><?=$speed_meas?></td>
		<td id="id_course"><?=$course?></td>
		<td>
		    <select style="padding: 3px;" id="id_msg_type" name="msg_type" size="1">
			<?php
			    if ($mtp<$max_known_type) {
				for ($i=0; $i<$max_known_type; $i++) {
				    echo "<option value=\"t_$i\"";
				    if ($i==$mtp) echo "selected";
				    echo ">\$$all_known_type[$i]</option>";
				}
			    } else {
				echo "<option value=\"\" selected>Select</option>";
				for ($i=0; $i<$max_known_type; $i++)
				    echo "<option value=\"t_$i\">\$$all_known_type[$i]</option>";
			    }			    
			?>
		    </select>
		</td>
            </tr>
        </tbody>
    </table>
    <br>
    <table>
        <thead>
            <tr>
                <th2>Temperature DS18B20</th2>
            </tr>
	    <tr>
                <th>Command</th>
                <th>Answer</th>
            </tr>
        </thead>
        <tbody>
	    <tr>
                <td>
		    <select style="padding: 3px;" id="combik" name="wd_temp" size="1">
			<option value="" selected>Select command</option>
			<option value="cmd01">0xCC 0x33</option>
			<option value="cmd02">0xCC 0x44</option>
			<option value="cmd03">0xCC 0xBE</option>
		    </select>
		</td>
                <td id="rd_temp"><?=$rd_data_temp?></td>
            </tr>
        </tbody>
    </table>
    <br>
    <table>
        <thead>
            <tr>
                <th2>Humidity DHT11</th2>
            </tr>
	    <tr>
                <th>RH</th>
		<th>Temp</th>
            </tr>
        </thead>
        <tbody>
	    <tr>
                <td id="rd_temp_dht_v"><?=$rd_data_temp_dht_v?></td>
		<td id="rd_temp_dht_t"><?=$rd_data_temp_dht_t?></td>
            </tr>
        </tbody>
    </table>
    <br>
    <table>
        <thead>
            <tr>
                <th2>ADC</th2>
            </tr>
	    <tr>
                <th>Channel</th>
                <th>Value</th>
            </tr>
        </thead>
        <tbody>
	    <tr>
		<td>
		    <select style="padding: 3px;" id="combik2" name="wr_temp_adc_chan" size="1">
			<?php
			    $se=array();
			    for ($i=0; $i<4; $i++) {
				$val="chan".$i; $soo="Channel ".$i; 
				switch ($sel_chan) {
				    case 0: $se[0]="selected"; break;
				    case 1: $se[1]="selected"; break;
				    case 2: $se[2]="selected"; break;
				    case 3: $se[3]="selected"; break;
				}
				echo "<option value=\"$val\"$se[$i]>$soo</option>";
			    }
			?>
		    </select>
		</td>
                <td id="rd_temp_adc"><?=$rd_data_temp_adc?></td>
            </tr>
        </tbody>
    </table>
    <br>
    <table>
        <thead>
            <tr>
                <th2>HC-SR04</th2>
            </tr>
	    <tr>
                <th>Distance (in centimeter's)</th>
                <th>Distance (in centimeter's)</th>
            </tr>
        </thead>
        <tbody>
	    <tr>
		<td id="rd_temp_hcsr"><?=$rd_data_temp_hcsr?></td>
		<td id="rd_temp_hcsr2"><?=$rd_data_temp_hcsr2?></td>
            </tr>
        </tbody>
    </table>
    <br>
    <table>
        <thead>
            <tr>
                <th2>Encoder</th2>
            </tr>
	    <tr>
                <th>Way</th>
                <th>Speed</th>
            </tr>
        </thead>
        <tbody>
	    <tr>
                <td id="enc_way_id"><?=$enc_way?></td>
                <td id="enc_speed_id"><?=$enc_speed?></td>
            </tr>
        </tbody>
    </table>
    <input type="submit" name="main_bt" value="SUBMIT">

</form>

</div>  <!-- end of my context  -->

</center>

</body>
</html>

