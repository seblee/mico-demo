/*%*VAR_CONFIG(CON1_PROTOCOL,con1_protocol);
 VAR_CONFIG(CON1_LPORT,con1_lport);
 VAR_CONFIG(CON1_RPORT,con1_rport);
 VAR_CONFIG(CON1_RSERVER,con1_rserver);
 VAR_CONFIG(CON2_PROTOCOL,con2_protocol);
 VAR_CONFIG(CON2_LPORT,con2_lport);
 VAR_CONFIG(CON2_RPORT,con2_rport);
 VAR_CONFIG(CON2_RSERVER,con2_rserver);*%*/

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
function checkPort(v)
{
    var re=/^[0-9]\d*$/;
    return re.test(v);
}
function init_con1_setting()
{
    var f = document.form_con1_setting;

    if(con1_protocol == "server")
        f.con1_protocol.options.selectedIndex = 0;
    else if(con1_protocol == "client")
        f.con1_protocol.options.selectedIndex = 1;
    else if(con1_protocol == "unicast")
        f.con1_protocol.options.selectedIndex = 2;
    else if(con1_protocol == "boardcast")
        f.con1_protocol.options.selectedIndex = 3;

    f.con1_lport.value = con1_lport;
    f.con1_rport.value = con1_rport;
    f.con1_rserver.value = con1_rserver;
    con1_change();
}
function con1_change() {
    m =  document.form_con1_setting.con1_protocol.options.selectedIndex;
    if( m == 0 ){
        show("con1_lport_id");
        hide("con1_rport_id");
        hide("con1_rserver_id");
    }else if( m == 1){
        hide("con1_lport_id");
        show("con1_rport_id");
        show("con1_rserver_id");
    }else if( m == 2){
        show("con1_lport_id");
        show("con1_rport_id");
        show("con1_rserver_id");
    }else if( m == 3){
        show("con1_lport_id");
        show("con1_rport_id");
        hide("con1_rserver_id");
    }
}
function con1_setting_apply()
{
    var f = document.form_con1_setting;
    if(parseInt(f.con1_lport.value)>=65535){
        alert("Local port value must be less than 65535!");
        return;
    }
    if(!checkPort(f.con1_lport.value)){
        alert("Local port invalid!");
        return;
    }
    if(parseInt(f.con1_rport.value)>=65535){
        alert("Remote port value must be less than 65535!");
        return;
    }
    if(!checkPort(f.con1_rport.value)){
        alert("Remote port invalid!");
        return;
    }
    f.submit();
}
function init_con2_setting()
{
    var f = document.form_con2_setting;

    if(con2_protocol == "disable")
        f.con2_protocol.options.selectedIndex = 0;
    else if(con2_protocol == "server")
        f.con2_protocol.options.selectedIndex = 1;
    else if(con2_protocol == "client")
        f.con2_protocol.options.selectedIndex = 2;
    else if(con2_protocol == "unicast")
        f.con2_protocol.options.selectedIndex = 3;
    else if(con2_protocol == "boardcast")
        f.con2_protocol.options.selectedIndex = 4;

    f.con2_lport.value = con2_lport;
    f.con2_rport.value = con2_rport;
    f.con2_rserver.value = con2_rserver;
    con2_change();
}
function con2_change() {
    m =  document.form_con2_setting.con2_protocol.options.selectedIndex;
    if( m == 0 ){
        hide("con2_lport_id");
        hide("con2_rport_id");
        hide("con2_rserver_id");
    }else if( m == 1){
        show("con2_lport_id");
        hide("con2_rport_id");
        hide("con2_rserver_id");
    }else if( m == 2){
        hide("con2_lport_id");
        show("con2_rport_id");
        show("con2_rserver_id");
    }else if( m == 3){
        show("con2_lport_id");
        show("con2_rport_id");
        show("con2_rserver_id");
    }else if( m == 4){
        show("con2_lport_id");
        show("con2_rport_id");
        hide("con2_rserver_id");
    }
}
function con2_setting_apply()
{
    var f = document.form_con2_setting;
    if(parseInt(f.con2_lport.value)>=65535){
        alert("Local port value must be less than 65535!");
        return;
    }
    if(!checkPort(f.con2_lport.value)){
        alert("Local port invalid!");
        return;
    }
    if(parseInt(f.con2_rport.value)>=65535){
        alert("Remote port value must be less than 65535!");
        return;
    }
    if(!checkPort(f.con2_rport.value)){
        alert("Remote port invalid!");
        return;
    }
    f.submit();
}