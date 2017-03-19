
#include "stdio.h"
#include "stdint.h"
#include <string.h>
#include "http_replace.h"

#define MIN(a,b) (a) < (b) ? (a) : (b)

static void find_rep_head(http_replace_t *h)
{
  if( !strncmp((h->workspace + h->state_left), "/*%*", 4) ){
    h->state_left += 4;
    h->parsing_state = PARSE;
  }else{
    h->state_left ++;
    h->parsing_state = DATA;
  }
} 


static int find_head(http_replace_t *h)
{
  while(1)
  {
    if( *(h->workspace + h->state_left) == '/'){
      h->parsing_state = DATA;
      return 0;
    }
    
    h->state_left ++;
    if( h->state_left == h->state_size){
      h->parsing_state = DATA;
      return 0;
    }
  }
}

static int find_req_key_value(http_replace_t *h)
{
  int i = 0;
  int keylen = 0;
  int valuelen = 0;
  while(1){
    if( *(h->workspace + h->state_left) == 'V' ){
      if( !strncmp( (h->workspace + h->state_left), "VAR_CONFIG", 10) ){
        h->state_left += 10;
        if( *(h->workspace + h->state_left) == '(' ){
          h->state_left ++;
          h->state_point = h->state_left;
          for(i=0; i<=REP_KEY_LEN; i++){
            if( *(h->workspace + h->state_left) == ',' ){
              h->state_left++;
              keylen = h->state_left - h->state_point - 1;
              memcpy(h->key, (h->workspace + h->state_point), keylen);
              memcpy((h->key+keylen), "\0", 2);
              h->state_point = h->state_left;
              break;	
            }
            h->state_left ++;
            if(i == REP_VALUE_LEN){
              return -1;
            }		
          }
          for(i=0; i<=REP_VALUE_LEN; i++){
            if( *(h->workspace + h->state_left) == ')' ){
              h->state_left++;
              valuelen = h->state_left - h->state_point - 1;
              memcpy(h->value, (h->workspace + h->state_point), valuelen);
              memcpy((h->value+valuelen), "\0", 2);
              h->state_point = h->state_left;
              http_replace_rep_callback(h->key, h->value);
              h->parsing_state = 	PARSE;						
              return 0;
            }
            h->state_left ++;
            if(i == REP_VALUE_LEN){
              return -1;
            }
            
          }
        } else{
          return -1;
        }
      }else{
        return -1;
      }
    }
    h->state_left ++;
  } 
  
}

static int find_req_end(http_replace_t *h)
{
  if( !strncmp( (h->workspace + h->state_left), ";*%*/", 5) ){
    h->state_left += 5;
    h->state_point = h->state_left;
    h->find_food = 1;
    h->parsing_state = PADDING;
    return 0;
  } 
  
  h->parsing_state = PARSE;
  return 0;
} 

static void do_replace_state(http_replace_t *h)
{
  int len;
  switch(h->parsing_state){
  case NONE:
    find_head(h);
    break;
  case HEAD:
    find_rep_head(h);
    break; 
  case PARSE:
    find_req_key_value(h);
    h->parsing_state = END;
    break;
  case DATA:
    len = h->state_left - h->state_point;
    if(len){
      http_replace_data_callback(h->workspace + h->state_point, len);
      h->state_point = h->state_left;
    }
    
    if( *(h->workspace + h->state_left) == '/'){
      h->parsing_state = HEAD;
    }else{
      h->parsing_state = NONE;
    } 	 		
    break;	
  case END:
    find_req_end(h);
    break;
  case PADDING:
    len = h->state_size - h->state_left;
    if(len){
      http_replace_data_callback(h->workspace + h->state_left, len);
      h->state_point += len;
      h->state_left = h->state_size;
    }				
    break;
  default:
    break;
  }
}

int http_replace_init(http_replace_t *h, char *workspace, unsigned int workspacesize)
{
  h->parsing_state = NONE;
  h->workspace = workspace;
  h->workspacesize = workspacesize;
  h->pos = 0;
  h->find_food = 0;
  return 0;
}

int http_replace_process(http_replace_t *h, uint8_t *data, uint32_t len)
{
  int new_used = 0;
  do{
    new_used =  MIN(len, h->workspacesize - h->pos);
    if(new_used){
      memcpy(h->workspace + h->pos, data, new_used);
      h->state_size = new_used;
      h->state_left = 0;
      h->state_point = 0;
      len -= new_used;
      data += new_used;
      h->pos += new_used;
    }
    do_replace_state(h);
  }while( len > 0);
  
  while( h->state_left < h->pos )
    do_replace_state(h);
  
  if( h->find_food == 0){
    if(h->state_left - h->state_point){
      http_replace_data_callback(h->workspace + h->state_point, h->state_left - h->state_point);
    }
  }
  
  return 0;
}
