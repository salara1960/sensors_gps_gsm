<?php 
require_once("imoto.php"); 

    $nk=chr($_GET["rk"]);
    if ($nk<4) shmop_write($shm_id, $nk, $sani_rk);

?>

