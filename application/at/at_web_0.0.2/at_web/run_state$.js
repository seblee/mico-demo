/*%*VAR_CONFIG(RUN_MODE,run_mode);
 VAR_CONFIG(AP_IP,ap_ip);
 VAR_CONFIG(AP_NETMASK,ap_netmask);
 VAR_CONFIG(STA_CH,sta_ch);
 VAR_CONFIG(STA_POWER,sta_power);
 VAR_CONFIG(STA_IP,sta_ip);
 VAR_CONFIG(STA_NETMASK,sta_netmask);
 VAR_CONFIG(STA_GATEWAY,sta_gateway);
 VAR_CONFIG(RUN_AT_FUN,run_at_fun);
 VAR_CONFIG(RUN_CON1_FUN,run_con1_fun);
 VAR_CONFIG(CON1_PROTOCOL,con1_protocol);
 VAR_CONFIG(RUN_T1S_NUM,run_t1s_num);
 VAR_CONFIG(RUN_T1C_NUM,run_t1c_num);
 VAR_CONFIG(RUN_CON2_FUN,run_con2_fun);
 VAR_CONFIG(CON2_PROTOCOL,con2_protocol);
 VAR_CONFIG(RUN_T2S_NUM,run_t2s_num);
 VAR_CONFIG(RUN_T2C_NUM,run_t2c_num);
 VAR_CONFIG(RUN_TIME,run_time);*%*/

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

function init_run_state() {
    if( run_mode == "sta" ) {
        document.getElementById("ap_func_id").innerHTML = "Disable";
    }else{
        document.getElementById("ap_func_id").innerHTML = "Enable";
    }
    document.getElementById("ap_ip_id").innerHTML = ap_ip;
    document.getElementById("ap_netmask_id").innerHTML = ap_netmask;

    if( run_mode == "ap" ) {
        document.getElementById("sta_func_id").innerHTML = "Disable";
    }else{
        document.getElementById("sta_func_id").innerHTML = "Enable";
    }
    document.getElementById("sta_ch_id").innerHTML =sta_ch;
    document.getElementById("sta_power_id").innerHTML =sta_power;
    document.getElementById("sta_ip_id").innerHTML = sta_ip;
    document.getElementById("sta_netmask_id").innerHTML = sta_netmask;
    document.getElementById("sta_gateway_id").innerHTML =sta_gateway;

    if( run_at_fun == 0 ){
        document.getElementById("run_at_fun_id").innerHTML = "Disable";
    }else{
        document.getElementById("run_at_fun_id").innerHTML = "Enable";
    }

    if( run_con1_fun == 0 ){
        document.getElementById("run_con1_fun_id").innerHTML = "Disable";
    }else{
        document.getElementById("run_con1_fun_id").innerHTML = "Enable";
    }

    if( con1_protocol == "server" ){
        document.getElementById("con1_protocol_id").innerHTML = "TCP server";
        hide("con1_client_id");
    }else if(con1_protocol == "client"){
        document.getElementById("con1_protocol_id").innerHTML = "TCP client";
        hide("con1_server_id");
    }else if(con1_protocol == "unicast"){
        document.getElementById("con1_protocol_id").innerHTML = "UDP unicast";
        hide("con1_client_id");
        hide("con1_server_id");
    }else{
        document.getElementById("con1_protocol_id").innerHTML = "UDP boardcast";
        hide("con1_client_id");
        hide("con1_server_id");
    }
    document.getElementById("run_t1s_num_id").innerHTML =run_t1s_num;
    if( run_t1c_num == 0 ) {
        document.getElementById("run_t1c_num_id").innerHTML ="Disconnected";
    }else{
        document.getElementById("run_t1c_num_id").innerHTML ="Connected";
    }

    if( run_con2_fun == 0 ){
        document.getElementById("run_con2_fun_id").innerHTML = "Disable";
    }else{
        document.getElementById("run_con2_fun_id").innerHTML = "Enable";
    }

    if( con2_protocol == "server" ){
        document.getElementById("con2_protocol_id").innerHTML = "TCP server";
        hide("con2_client_id");
    }else if(con2_protocol == "client"){
        document.getElementById("con2_protocol_id").innerHTML = "TCP client";
        hide("con2_server_id");
    }else if(con2_protocol == "unicast"){
        document.getElementById("con2_protocol_id").innerHTML = "UDP unicast";
        hide("con2_client_id");
        hide("con2_server_id");
    }else if(con2_protocol == "boardcast"){
        document.getElementById("con2_protocol_id").innerHTML = "UDP boardcast";
        hide("con2_client_id");
        hide("con2_server_id");
    }else{
        document.getElementById("con2_protocol_id").innerHTML = "无";
        hide("con2_client_id");
        hide("con2_server_id");
    }
    document.getElementById("run_t2s_num_id").innerHTML =run_t2s_num;
    if( run_t2c_num == 0 ){
        document.getElementById("run_t2c_num_id").innerHTML ="Disconnected";
    }else{
        document.getElementById("run_t2c_num_id").innerHTML ="Connected";
    }
    document.getElementById("run_time_id").innerHTML = run_time;
}