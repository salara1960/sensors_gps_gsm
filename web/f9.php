<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<?php
    require_once("imoto.php");

    $txt_sms = "";  $le = 200;
    $t_s = shmop_read($shm_id, $sani_at + 56, $le);//234-14-20=200 символов отклика
    $t2 = strpos($t_s, "\0"); if ($t2 === false) $t2 = $le; $txt_sms = substr($t_s, 0, $t2);

    $a_s = shmop_read($shm_id, $sani_at + 2, 54);//20+14+20 =54 символов команды
    $t2 = strpos($a_s, "\0"); if ($t2 === false) $t2 = 54; $at_coms = substr($a_s, 0, $t2);

?>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=koi8-r">
<title>AT-command</title>

<script type="text/javascript">
function validate_form_1( form )
{
    if( form.elements['AT'].value=="" )
    {
    if (form.elements['lng'].value=="0") alert("Введите АТ-команду"); else alert("Enter AT-command");
        form.elements['AT'].focus(); return false;
    }
    return true;
}
</script>
<style type="text/css">
<!--

.conti {
    width:560px;
    margin:34px 0 0 40px;
}

.Normal-P
        {
        margin:0.0px 0.0px 0.0px 0.0px; text-align:left;
        }
.Normal-C
        {
        font-family:"Verdana", sans-serif; text-align:left; font-size:16.0px; 
        line-height:1.23em; 
        }
.Normal-C13
        {
        font-family:"Verdana", serif; text-align:center; font-size:16.0px;
        line-height:1.00em; color:#0000ff;
        }
.hint
{
    background-color:#006699;
    line-height:10px;
    color:white;
    font-family:verdana;
    font-size:11px;
    width:auto;
    border-top:1px solid white;
    border-right:1px solid white;
    border-bottom:1px solid white;
    border-left:5px solid orange;
    margin:0px;
    padding:8px;
    position:absolute;
    visibility:hidden;
}

.bigi {
    color: white;
    cursor: pointer;
    box-shadow: 0 2px 0 rgba(0, 0, 0, 0.3), 0 2px 2px -1px rgba(0, 0, 0, 0.5), 0 2px 0 rgba(255, 255, 255, 0.3) inset;
    background: linear-gradient(#fafafa, #0000af);
    border: 2px solid gray;
    border-radius: 4px;
}
.bigi {
    padding: 6px 20px;
    font: bold 14px Tahoma;
}
.bigi:hover,.biga:hover{
    background: linear-gradient(#0000af, #fafafa);
}
.bigi:active{
    position: relative;
    top: 1px;
    box-shadow: 0 3px 3px #1f1f1f inset;
}

-->
</style>

<script type="text/javascript" src="../js/mw_hint.js"></script>

</head>

<body>


<form name="AT_command" onsubmit="return validate_form_1(this)" method="post" action="./f91.php"  enctype="application/x-www-form-urlencoded" style="margin:0px; height:420px;">

<table style="height:370px; background:transparent; margin:25px auto;">

    <tr style="width:60%; height:18%;">
        <td style="text-align:right; padding-right:10px;" class="Normal-P"><A class="Normal-C" title="Select channel">Select channel</A></td>
        <td style="width:60%; height:18%; text-align:left;">
<?php
//    $rk = ord(shmop_read($shm_id, $sani_rk, 1));
            for ($i = 0; $i < 4; $i++) {
                if ($i == $rk) {
                    $ch = "CHECKED";
                    $it_color = "#ff0000";
                } else {
                    $ch = "";
                    $it_color = "#00ff00";
                }
                echo "<input id=\"rk_id\" type=\"radio\" style=\"background:$it_color\" name=\"rks\" value=\"$i\"";
                echo $ch;
                echo ">";
            }
?>
        </td>
    </tr>

    <tr style="width:60%; height:18%;">
        <input type="hidden" name="lng" value="1">
        <td style="text-align:right; padding-right:10px;" class="Normal-P"><A class="Normal-C" title="Enter AT-command">AT command </A></td>
        <td style="width:60%; height:18%; text-align:left;">
            <input type="text" name="AT" size="30" maxlength="54" style="font-size:14px; width:90%;" value=<?php echo "'".$at_coms."'"; ?>>
        </td>
    </tr>
    <tr style="width:60%; height:28%">
        <td style="text-align:right; padding-right:10px;" class="Normal-P"><A class="Normal-C" title="Answer">Answer </A></td>
        <td style="width:80%; height:75%; text-align:left;">
            <textarea rows="24" cols="56" name="TEXT" style="height:90%; font-size:14px; width:90%;"><?php echo $txt_sms; ?></textarea>
        </td>
    </tr>
</table>

<center>
    <div style="width:50%; height:50%; text-align:center; margin:20px;">
        <input type="submit" name="run_bt" class="bigi" value="Send">
    </div>
</center>

</form>


</body>
</html>
