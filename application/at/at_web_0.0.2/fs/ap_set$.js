/*%*VAR_CONFIG(AP_SSID,ap_ssid);
 VAR_CONFIG(AP_SEC,ap_sec);
 VAR_CONFIG(AP_KEY,ap_key);
 VAR_CONFIG(AP_IP,ap_ip);
 VAR_CONFIG(AP_NETMASK,ap_netmask);
 VAR_CONFIG(AP_GATEWAY,ap_gateway);*%*/

function ap_setting_set() {
    var f = document.form_ap_setting;

    f.ap_ssid.value = ap_ssid;
    f.ap_key.value = ap_key;

    if (ap_sec == "disable")
        f.ap_sec.options.selectedIndex = 0;
    else if (ap_sec == "enable") {
        f.ap_sec.options.selectedIndex = 1;
    }
    auth_change();
}

function init_dhcp_setting() {
    var f = document.form_dhcp_setting;

    f.ap_ip.value = ap_ip;
    f.ap_netmask.value = ap_netmask;
    f.ap_gateway.value = ap_gateway;
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

    var m = document.form_ap_setting.ap_sec.options.selectedIndex;
    if (m == 0) {
        hide("ap_key_id");
        enableCon("key_text1", 0);
    } else {
        show("ap_key_id");
        enableCon("key_text1", 1);
    }
}

function init_ap_setting() {
    ap_setting_set();
}

function ap_form_apply() {
    var f = document.form_ap_setting;

    if (f.ap_ssid.value == "") {
        alert("SSID is empty!");
        return;
    }

    if (f.ap_ssid.value.length > 32) {
        alert("SSID less than or equal to 32!");
        return;
    }

    if (f.ap_sec.options.selectedIndex == 0) {
        f.ap_sec.value = "disable";
    }
    else if (f.ap_sec.options.selectedIndex == 1) {
        f.ap_sec.value = "enable";
    }

    if (f.ap_sec.options.selectedIndex != 0) {
        if(f.ap_key.value == ""){
            alert("possword is empty!");
            return;
        }
        if(8 > f.ap_key.value.length ){
            alert("possword more than to 8!");
            return;
        }
    }

    f.submit();
}
function dhcp_form_apply() {
    var f = document.form_dhcp_setting;

    if (!ipAdd(f.ap_ip.value)) {
        alert("IP invalid!");
        return;
    }

    if (!ipAdd(f.ap_netmask.value)) {
        alert("netmask invalid!");
        return;
    }

    if (!ipAdd(f.ap_gateway.value)) {
        alert("gateway invalid!");
        return;
    }

    f.submit();
}
