<?php
require_once("imoto.php");

    $pst = $_GET['data'];
    $from = split("/", $pst);
    if (count($from) == 2) {
        $adr = ""; $sp = $from[1];
             if ($from[0] == "mode_id") $adr=$sani_motor_data;//mode
        else if ($from[0] == "onoff_id") $adr=$sani_motor_data + 3;//onoff
        else if ($from[0] == "dir_id") $adr=$sani_motor_data + 2;//dir
        else if ($from[0] == "wr_speed_id") {
                if (((int)$sp < 90) || ((int)$sp > 128)) $adr = "";
                                                    else $adr = $sani_motor_data + 1;//speed
        }
        if ($adr != "") {
            $nk = chr($sp);
            if ($nk != "") {
                shmop_write($shm_id, $nk, $adr);
                $fl = '1'; shmop_write($shm_id, $fl, $sani_motor_post);
                //echo sprintf("write adr=%d nk=%s OK",$adr,$sp);
            }// else echo sprintf("post=%s | error write adr=%d nk=%s",$pst,$adr,$sp);
        }// else echo sprintf("post=%s | error write adr=%d nk=%s",$pst,$adr,$sp);
    }// else echo sprintf("post=%s | error write adr=%d nk=%s",$pst,$adr,$sp);
?>
