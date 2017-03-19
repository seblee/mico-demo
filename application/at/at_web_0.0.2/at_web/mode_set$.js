/*%*VAR_CONFIG(MODE,mode);*%*/

function init_mode_setting() {
    initradio("mode", mode);
}
function initradio(rName, rValue) {
    var rObj = document.getElementsByName(rName);
    for (var i = 0; i < rObj.length; i++) {
        if (rObj[i].value == rValue) {
            rObj[i].checked = 'checked';
        }
    }
}
function mode_setting_apply()
{
    var f=document.form_mode_setting;
    f.submit();
}