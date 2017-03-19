
/* Includes ------------------------------------------------------------------*/
/* Includes ------------------------------------------------------------------*/

/* FatFs includes component */
#include "http_server/http_server.h"

#ifdef USE_FILESYS
#include "ff_gen_drv.h"
#include "sflash_diskio.h"
#include "fatfs_exfuns.h"
#endif

#include "MICO.h"

#ifndef USE_FILESYS
#include "fsdata.h"
#endif

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define fatfs_log(M, ...) custom_log("FatFs", M, ##__VA_ARGS__)
#define fatfs_log_trace() custom_log_trace("FatFs")

#ifdef USE_FILESYS
#define USE_DIR
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
FATFS SFLASHFatFs;           /* File system object for USB disk logical drive */
FIL SLASHFile;                   /* File object */
DIR SLASHdir;
char SFLASHPath[4];          /* USB Host and SFLASH logical drive path */

int *fatfs_mfree( void );
uint8_t fatfs_mkfs();
uint8_t fatfs_sys_creat(void);
#endif

uint8_t datafs_test( void )
{
  char *filename = "index.html";
  uint32_t readlen = 0;
  int offset = 0;
#define buf_size 1024
  char buf[buf_size];
  
  memset(buf, '0', buf_size);

  open_file(filename, 1);
  
  read_file(buf, buf_size, &readlen, &offset);
  
  fatfs_log("data size %d, %.*s", (int)readlen, (int)readlen, buf);
  return 0;
}

#ifdef USE_FILESYS

/* Private function prototypes -----------------------------------------------*/ 
uint8_t fatfs_test( void )
{
  FRESULT res;
  uint32_t byteswritten, bytesread;
  uint8_t wtext[] = "hello MiCO world!";
  uint8_t rtext[100]; 
  bool needmkfs = false;
  
  if( (int)fatfs_mfree() < 1024 )
  {
    fatfs_log("flash size %d", (int)fatfs_mfree());
    needmkfs = true;
    goto mkfs;
  }
  
  fatfs_log("flash size %d", (int)fatfs_mfree());
  
  if(FATFS_LinkDriver(&SFLASHDISK_Driver, SFLASHPath) == 0)
  {
    res = f_mount(&SFLASHFatFs, (TCHAR const*)SFLASHPath, 0);
    if( res != FR_OK )
    {
      fatfs_log("F_mount() err: %d", res);
    }
    else
    {
      res = f_open(&SLASHFile, "1.txt", FA_CREATE_ALWAYS|FA_WRITE);
      if( res != FR_OK )
      {
        needmkfs = true;
        goto mkfs;
      }
      else
      {
        res = f_write(&SLASHFile, wtext, sizeof(wtext), (void *)&byteswritten);
        if( (byteswritten == 0) || (res != FR_OK) )
        {
          needmkfs = true;
          goto mkfs;
        }
        else
        {
          f_close(&SLASHFile);
          fatfs_log("f_write ok");
          
          res = f_open(&SLASHFile, "1.txt", FA_READ);
          if( res != FR_OK )
          {
            fatfs_log("f_open err %d", res);
            needmkfs = true;
            goto mkfs;
          }
          else
          {
            res = f_read(&SLASHFile, rtext, sizeof(rtext), (void *)&bytesread);
            if((bytesread == 0) || (res != FR_OK) || (strcmp((char const*)rtext, (char const*)wtext) != 0))
            {
              fatfs_log("f_read err");
              needmkfs = true;
              goto mkfs;
            }
            else
            {
              fatfs_log("f_read ok");
              f_close(&SLASHFile);
              
              if(bytesread != byteswritten)
              {
                needmkfs = true;
                goto mkfs;
              }
              else
              {
                fatfs_log("check txt ok, needn't mkfs");
              }
            }
          }          
        }
      }      
    }    
  }
  
mkfs:
  FATFS_UnLinkDriver(SFLASHPath);
  if( needmkfs == true )
  {
    fatfs_log("file err %d, need mkfs", res);
    res = (FRESULT)fatfs_sys_creat( );
  }
  return res; 
}


uint8_t fatfs_sys_creat( void )
{
  OSStatus err = kNoErr;
  mico_logic_partition_t* filesys_partition = MicoFlashGetInfo( MICO_PARTITION_FILESYS );
  
  err = MicoFlashErase( MICO_PARTITION_FILESYS, 0x0, filesys_partition->partition_length);
  require_noerr(err, flashErrExit);
  
  err = fatfs_mkfs();
    
  if(err != kNoErr)
  {
    fatfs_log("F_mount() err: %d", err);
  }
  else{
    fatfs_log("f_mkfs OK");
  }
  
flashErrExit:
  return err;
}


int *fatfs_mfree( void )
{
  FRESULT res;
  int *free;
  if(FATFS_LinkDriver(&SFLASHDISK_Driver, SFLASHPath) == 0)
  {
    res = f_mount(&SFLASHFatFs, (TCHAR const*)SFLASHPath, 0);
    if(res != FR_OK)
    {
      fatfs_log("F_mount() err: %d", res);
      free = 0;
    }
    else{
       free = mico_f_showfree((uint8_t*)SFLASHPath);
      
    }
  }

  FATFS_UnLinkDriver(SFLASHPath);
  return free; 
}


uint8_t fatfs_mkfs(void)
{
  FRESULT res;
  if(FATFS_LinkDriver(&SFLASHDISK_Driver, SFLASHPath) == 0)
  {
    res = f_mount(&SFLASHFatFs, (TCHAR const*)SFLASHPath, 0);
    res = f_mkfs((TCHAR const*)SFLASHPath, 0, _MAX_SS);
  }
  
  FATFS_UnLinkDriver(SFLASHPath);
  return res;
}

/***********************display file***************************/
bool is_file_ok = false;
uint8_t open_file(const char *name, uint8_t file_op) //file_op = 0 write; 1 read
{
   FRESULT res = FR_OK;
   
  if(FATFS_LinkDriver(&SFLASHDISK_Driver, SFLASHPath) != 0)
    goto exit;

  res = f_mount(&SFLASHFatFs, (TCHAR const*)SFLASHPath, 0);
  if(res != FR_OK)
  {
    fatfs_log("f_mount() err: %d", res);
    goto exit;
  }
  
  if( file_op == 0 ){
    //f_unlink(name);
    res = f_open(&SLASHFile, name, FA_CREATE_ALWAYS|FA_WRITE);
  }else if( file_op == 1 ){
    res = f_open(&SLASHFile, name, FA_READ);
  }
  if(res != FR_OK)
  {
    fatfs_log("f_open() err: %d", res);
    goto exit;
  }
  is_file_ok = true;
  
  return res;
exit:
  is_file_ok = false;
  res = (FRESULT)close_file();
  return -1;
}

unsigned int get_file_size( void )
{
  return SLASHFile.fsize;
}


uint8_t read_file(char *rtext, size_t buf_len, uint32_t *byteread, int *is_offset)
{
  FRESULT res = FR_OK;
  uint32_t offset = *is_offset;

  if( is_file_ok == false){
    fatfs_log("read file false");
    return res = FR_NO_FILE;
  }
  
  f_lseek(&SLASHFile, offset);
    /* Write data to the text file */
  res = f_read(&SLASHFile, rtext, buf_len, (void *)byteread);
  
  if(res != FR_OK)
  {
    /* file Write or EOF Error */
    fatfs_log("return f_read(): %d", res);
  }
  
  *is_offset += *byteread;
  
  return res;
}

uint8_t write_file(const char *wtext, size_t inLen)
{
  FRESULT res = FR_OK;
  uint32_t byteswritten; 
  
  if( is_file_ok == false)
    return res;

  f_lseek(&SLASHFile, SLASHFile.fsize);
    /* Write data to the text file */
  res = f_write(&SLASHFile, wtext, inLen, (void *)&byteswritten);
  
  if((byteswritten == 0) || (res != FR_OK))
  {
    /* file Write or EOF Error */
    fatfs_log("return f_write(): %d", res);
  }
  
  return res;
}

uint8_t close_file( void )
{
  FRESULT res = FR_OK;
  
  fatfs_log("close file , file size %d", SLASHFile.fsize);
  res = f_close(&SLASHFile);
  
  res = (FRESULT)FATFS_UnLinkDriver(SFLASHPath);
  return res;
}

#else

static uint32_t file_size = 0;
static struct fsdata_file *fsdata;
static bool is_file_ok = false;
/*******************display file end***************************/

void datafs_init( void )
{
  fsdata = malloc(sizeof(struct fsdata_file));
}

uint8_t open_file(const char *name, uint8_t file_op)
{
  int res = kNoErr;
  file_size = 0;
  is_file_ok = false;
  char file_name[30] = {0}; 
  
  sprintf(file_name, "/%s", name);
  
  file_size = datafs_open( fsdata, (char *)file_name );
  if( file_size == 0 ){
    res = -1;
  }
  is_file_ok = true;
  return res;
}

uint8_t read_file(char *rtext, size_t buf_len, uint32_t *byteread, int *is_offset)
{
  int res = kNoErr;
  unsigned const char *file_p = NULL;
  
  if( is_file_ok == false )
    return -1;
  
  file_p = fsdata->data;
  
  *byteread = MIN( buf_len, (file_size - *is_offset));
  memcpy(rtext, &file_p[*is_offset], *byteread);
  
  *is_offset += *byteread;
    
  return res;
}

uint8_t write_file(const char *wtext, size_t inLen)
{
  int res = kNoErr;
  return res;
}

uint8_t close_file( void )
{
  int res = kNoErr;
  
  return res;
}

//#ifdef EMW3081
//SDRAM_DATA_SECTION
//#endif

//uint32_t datafs_open( const struct fsdata_file *df, char *filename)
//{
//  uint32_t filesize = 0;
//  const struct fsdata_file *f;
//  
//  for( f = FS_ROOT; f != NULL; f=f->next){
//    if( !strcmp(filename, (char *)f->name) ){
//      filesize = f->len;
//      memcpy((void *)df, f, sizeof(struct fsdata_file));
//      break;
//    }
//  }
//  return filesize;
//}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
