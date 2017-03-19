/**/
/*%*VAR_CONFIG(AP_SSID,ap_ssid);
VAR_CONFIG(AP_KEY,ap_psk);*%*/
/*
var ap_ssid="name";
var ap_psk="possword";
*/

#ifndef HTTP_REPLACE
#define HTTP_REPLACE

#define REP_VALUE_LEN 15
#define REP_KEY_LEN   15

typedef struct http_replace{
	enum{
		NONE,
		HEAD, 
		PARSE,
		DATA,
		PADDING,
		END,
		ABORT
	}parsing_state;
	unsigned int pos;
	unsigned int state_size;
	unsigned int state_left;
	unsigned int state_point;
	char *workspace;
	char key[REP_KEY_LEN];
	char value[REP_VALUE_LEN];
	unsigned int workspacesize;
        unsigned int find_food;
}http_replace_t;

extern int http_replace_rep_callback(char *key, char *value);

extern int http_replace_data_callback(char *data, unsigned int len);

int http_replace_init(http_replace_t *h, char *workspace, unsigned int workspacesize);

int http_replace_process(http_replace_t *h, uint8_t *inbuf, uint32_t inlen);

#endif 
