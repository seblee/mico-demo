/*%*VAR_CONFIG(USER_NAME,user_name);
 VAR_CONFIG(USER_KEY,user_key);
 VAR_CONFIG(MODEL,model);
 VAR_CONFIG(MAC,mac);
 VAR_CONFIG(SDK_VER,sdk_ver);
 VAR_CONFIG(FW_VER,fw_ver);
 VAR_CONFIG(REBOOT_MODE,reboot_mode);
 VAR_CONFIG(REBOOT_TIME,reboot_time);*%*/

function enableCon(c, v) {
    var con = document.getElementById(c);
    if (con != null) {
        if (v == 0) {
            con.disabled = "disabled";
        }
        else {
            con.disabled = "";
        }
    }
}

function init_user_setting() {
    var f = document.form_user_setting;
    f.user_name.value = user_name;
    f.user_key.value = user_key;
}
function init_model_ver() {
    document.getElementById("model_id").innerHTML = model;
    document.getElementById("mac_id").innerHTML = mac;
    document.getElementById("sdk_ver_id").innerHTML = sdk_ver;
    document.getElementById("fw_ver_id").innerHTML = fw_ver;
    document.getElementById("web_ver_id").innerHTML = "web_V1.0.0";
}
function user_setting_apply()
{
    var f=document.form_user_setting;
    f.submit();
}

function time_reboot_change()
{
    var c = document.getElementById("time_reboot_mode_id");
    if(c != null)
    {
        if(c.value == "disable"){
            enableCon("time_reboot_time_id", 0);
        }else{
            enableCon("time_reboot_time_id", 1);
        }
    }
}

function init_time_reboot_setting()
{
    var f = document.form_time_reboot_setting;

    if(reboot_mode == "disable")
        f.reboot_mode.options.selectedIndex = 0;
    else if(reboot_mode == "enable")
        f.reboot_mode.options.selectedIndex = 1;

    f.reboot_time.value = reboot_time;

    time_reboot_change();
}

function time_reboot_setting_apply()
{
    var f = document.form_time_reboot_setting;
    if(f.reboot_mode.value == "enable")
    {
        if( (f.reboot_mode.value == "") || (0>parseInt(f.reboot_mode.value)) ||  (parseInt(f.reboot_mode.value)==0) )
        {
            alert("Time Reboot time must be greater than 0s");
            return;
        }
    }
    f.submit();
}

function reboot_setting_apply(){
    var f=document.form_reboot_setting;
    f.submit();
}
function factory_setting_apply(){
    var f=document.form_factory_setting;
    f.submit();
}
function apply()
{
    var f=document.form_ota;
    if(f.ota_fw.value=="")
    {
        alert("Please select a file!");
        return ;
    }
    document.getElementById("up_before").style.visibility = "hidden";
    document.getElementById("up_before").style.display = "";
    document.getElementById("up_after").style.visibility = "visible";
    document.getElementById("up_after").style.display = "";
    f.submit();
}