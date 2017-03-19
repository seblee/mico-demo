/*%*VAR_CONFIG(AP_SSID,ap_ssid);
 VAR_CONFIG(UART_BR,uart_br);
 VAR_CONFIG(UART_DATA,uart_data);
 VAR_CONFIG(UART_PARITY,uart_parity);
 VAR_CONFIG(UART_STOP,uart_stop);
 VAR_CONFIG(UART_FC,uart_fc);
 VAR_CONFIG(FORMAT_MODE,format_mode);
 VAR_CONFIG(FORMAT_LEN,format_len);
 VAR_CONFIG(FORMAT_TIME,format_time);*%*/

function init_uart_setting() {
    var f = document.form_uart_setting;
    f.uart_br.value = uart_br;
    f.uart_data.value = uart_data;
    f.uart_parity.value = uart_parity;
    f.uart_stop.value = uart_stop;
    f.uart_fc.value = uart_fc;
}
function uart_setting_apply()
{
    var f=document.form_uart_setting;
    f.submit();
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
function formart_change()
{
    var c = document.getElementById("format_mode_id");
    if(c != null)
    {
        if(c.value == "disable"){
            enableCon("format_time_id", 0);
            enableCon("format_len_id", 0);
        }else{
            enableCon("format_time_id", 1);
            enableCon("format_len_id", 1);
        }
    }
}
function init_format_setting()
{
    var f = document.form_format_setting;

    if(format_mode == "disable")
        f.format_mode.options.selectedIndex = 0;
    else if(format_mode == "enable")
        f.format_mode.options.selectedIndex = 1;

    f.format_len.value = format_len;
    f.format_time.value = format_time;
    formart_change();
}
function format_setting_apply()
{
    var f = document.form_format_setting;
    if(f.format_mode.value == "enable")
    {
        if( (f.format_time.value == "") || (100>parseInt(f.format_time.value)) ||  (parseInt(f.format_time.value)>1000) )
        {
            alert("Automatic framing time must be greater than 100 and less than 1000");
            return;
        }
        if( (f.format_len.value == "") || (10>parseInt(f.format_len.value)) ||  (parseInt(f.format_len.value)>1024) )
        {
            alert("Auto frame length must be greater than 10 and less than 1024");
            return;
        }
    }
    f.submit();
}