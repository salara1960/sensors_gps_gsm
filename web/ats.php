<?php 
require_once("imoto.php"); 

    if ($_POST["run_bt"]=="Send") {//если нажата клавиша Send

//	$ka = $_POST["rks"];
//	if ($ka!="") {
//	    if ($ka != "$rk") {
//		$nk=chr($ka);
//		shmop_write($shm_id, $nk, $sani_rk);
//	    }
//	}

	$at_coma=$_POST["AT"];	$at_com=stripslashes($at_coma);	//54=20+14+20;
	$zero=shmop_read($shm_id, $sani_at+2, 54);//читаем 54 символа команды
	for ($i=0; $i<54; $i++) $zero[$i]=chr(0);	//очищаем нулями
	shmop_write($shm_id, $zero, $sani_at+2);//пишем нули в shared_memory
	shmop_write($shm_id, $at_com, $sani_at+2);//54 символа команды

	$fl="1";  shmop_write($shm_id, $fl, $sani_post_at);//установить флаг F9
	usleep(250000);
	$i=16;//wait answer maximum 4 sec.
	while (($i) && (shmop_read($shm_id, $sani_at, 1)!='0')) {//читаем ATCOM.flag_
	    $i--; usleep(250000);
	}
	header("Location: at.php");
    }

?>

