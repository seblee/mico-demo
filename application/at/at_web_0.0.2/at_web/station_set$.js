/*%*VAR_CONFIG(STA_SSID,sta_ssid);
 VAR_CONFIG(STA_SEC,sta_sec);
 VAR_CONFIG(STA_KEY,sta_key);
 VAR_CONFIG(STA_SSID1,sta_ssid1);
 VAR_CONFIG(STA_KEY1,sta_key1);
 VAR_CONFIG(STA_SSID2,sta_ssid2);
 VAR_CONFIG(STA_KEY2,sta_key2);
 VAR_CONFIG(STA_SSID3,sta_ssid3);
 VAR_CONFIG(STA_KEY3,sta_key3);
 VAR_CONFIG(STA_SSID4,sta_ssid4);
 VAR_CONFIG(STA_KEY4,sta_key4);
 VAR_CONFIG(STA_SSID5,sta_ssid5);
 VAR_CONFIG(STA_KEY5,sta_key5);
 VAR_CONFIG(STA_DHCP,sta_dhcp);
 VAR_CONFIG(STA_IP,sta_ip);
 VAR_CONFIG(STA_NETMASK,sta_netmask);
 VAR_CONFIG(STA_GATEWAY,sta_gateway);*%*/

function sta_setting_set() {
    var f = document.form_sta_setting;

    f.sta_ssid.value = sta_ssid;
    f.sta_key.value = sta_key;

    if (sta_sec == "disable")
        f.sta_sec.options.selectedIndex = 0;
    else if (sta_sec == "enable") {
        f.sta_sec.options.selectedIndex = 1;
    }

    auth_change();
}

function init_sta_manage_wifi_setting() {
    var f = document.form_manage_wifi_setting;

    f.sta_ssid1.value = sta_ssid1;
    f.sta_key1.value = sta_key1;
    f.sta_ssid2.value = sta_ssid2;
    f.sta_key2.value = sta_key2;
    f.sta_ssid3.value = sta_ssid3;
    f.sta_key3.value = sta_key3;
    f.sta_ssid4.value = sta_ssid4;
    f.sta_key4.value = sta_key4;
    f.sta_ssid5.value = sta_ssid5;
    f.sta_key5.value = sta_key5;
}

function init_dhcp_setting() {
    var f = document.form_dhcp_setting;

    if(sta_dhcp == "dhcp_client")
        f.sta_dhcp.options.selectedIndex = 0;
    else if(sta_dhcp == "dhcp_disable")
        f.sta_dhcp.options.selectedIndex = 1;

    f.sta_ip.value = sta_ip;
    f.sta_netmask.value = sta_netmask;
    f.sta_gateway.value = sta_gateway;

    dhcp_change();
}

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
function show(v) {
    var c = document.getElementById(v);
    if (c != null) {
        c.style.visibility = "visible";
        c.style.display = "";
    }
}

function hide(v) {
    var c = document.getElementById(v);
    if (c != null) {
        c.style.visibility = "hidden";
        c.style.display = "none";
    }
}

function ipAdd(v) {
    var re = /^(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$/;
    return re.test(v);
}

function auth_change() {

    var m = document.form_sta_setting.sta_sec.options.selectedIndex;
    if (m == 0) {
        hide("sta_key_id");
        enableCon("key_text1", 0);
    }
    else {
        show("sta_key_id");
        enableCon("key_text1", 1);
    }
}
function dhcp_change() {
    var c;

    c = document.getElementById("sta_dhcp_id");
    if(c != null)
    {
        if (c.value == "dhcp_client") {
            enableCon("sta_ip_id", 0);
            enableCon("sta_netmask_id", 0);
            enableCon("sta_gateway_id", 0);
        } else {
            enableCon("sta_ip_id", 1);
            enableCon("sta_netmask_id", 1);
            enableCon("sta_gateway_id", 1);
        }
    }
}

function init_sta_setting() {
    sta_setting_set();
}

function sta_form_apply() {
    var f = document.form_sta_setting;

    if (f.sta_ssid.value == "") {
        alert("SSID is empty!");
        return;
    }

    if (f.sta_ssid.value.length > 32) {
        alert("SSID less than or equal to 32!！");
        return;
    }

    if (f.sta_sec.options.selectedIndex == 0) {
        f.sta_sec.value = "disable";
    }
    else if (f.sta_sec.options.selectedIndex == 1) {
        f.sta_sec.value = "enable";
    }

    if (f.sta_sec.options.selectedIndex != 0) {
        if(f.sta_key.value == ""){
            alert("possword is empty!");
            return;
        }
        if(8 > f.sta_key.value.length ){
            alert("possword more than to 8!");
            return;
        }
    }

    f.submit();
}

function sta_manage_wifi_apply() {
    var f = document.form_manage_wifi_setting;
    f.submit();
}

function dhcp_form_apply() {
    var f = document.form_dhcp_setting;

    if(f.sta_dhcp.value == "dhcp_disabble" )
    {
        if (!ipAdd(f.sta_ip.value)) {
            alert("IP invalid!");
            return;
        }

        if (!ipAdd(f.sta_netmask.value)) {
            alert("netmask invalid!");
            return;
        }

        if (!ipAdd(f.sta_gateway.value)) {
            alert("gateway invalid!");
            return;
        }
    }

    f.submit();
}
var gg_ssid = "";

function to_surver(gg_ssid)
{
    var gg=document.form_sta_setting;
    sta_ssid = gg_ssid;
    sta_setting_set();
}

function set_value(ssid) {
    gg_ssid = ssid;
}

function scan()
{
    hide("sta_web_id");
    show("scan_web_id");
    document.getElementById("sta_scan_iframe").src="station_scan_en$.html";
    document.getElementById("refresh").disable=true;
    setTimeout("eb()", 3000);
}

function en()
{
    document.getElementById("refresh").disable = false;
}