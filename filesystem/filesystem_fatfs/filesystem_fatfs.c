/**
 ******************************************************************************
 * @file    fatfs.c
 * @author  Ding QQ
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   MiCO RTOS Fatfs file system demo.
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2014 MXCHIP Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************
 **/

#include "mico.h"
#include "mico_filesystem.h"

#define fatfs_log(M, ...)  custom_log("FATFS...", M, ##__VA_ARGS__)

#define IMAGE_FILENAME "0:"

#define FORMAT_ENABLE 0

/* Uncomment to format mico_filesys_partition, every time the device startup. */
//#define FORMAT_ENABLE 1

const mico_block_device_init_data_t tester_block_device_init_data =
    {
        .base_address_offset = 0,
        .maximum_size = 0,
        .volatile_and_requires_format_when_mounting = 0,
    };

mico_block_device_t tester_block_device =
    {
        .init_data = &tester_block_device_init_data,
        .driver = &tester_block_device_driver,
        .device_size = 0x60000,

        /* for format */
        .erase_block_size = 8,
        .write_block_size = 8,
        .read_block_size = 8,
    };

void scans_files_callback(char *path, char *fn, uint32_t fsize)
{
    fatfs_log("path is %s,filename is %s,filesize is %u Bytes",path,fn,(unsigned int)fsize);
}

uint32_t get_file_size( mico_file_t *file_handle ){
   return file_handle->data.fatfs.fsize;
}

void test_fatfs( )
{

    mico_filesystem_t fs_handle;
    mico_file_t file_handle;

    OSStatus err = kNoErr;
    mico_filesystem_info fatfs_info = { 0 };

    uint8_t mounted_name[]="0:/";
    char filename[] = "MiCO.txt";

    char *wtext[] = { "Hi!"," Welcome to"," MiCO!" };
    uint8_t buff[10] = { 0 };
    uint8_t rtext[100]={0};
    uint64_t byteswritten = 0, bytesread = 0, count = 0, i = 0, j = 0;
    
    /* Initialize mico file system. */
    mico_filesystem_init( );
    
    /* Mount FATFS file system. */
    err = mico_filesystem_mount( &tester_block_device, MICO_FILESYSTEM_HANDLE_FATFS, &fs_handle, IMAGE_FILENAME );

    err = mico_filesystem_get_info( &fs_handle,&fatfs_info,(char *)mounted_name );
    fatfs_log( "filesystem total space is %dKB, free space is %dKB", fatfs_info.total_space, fatfs_info.free_space );

    /* Scan device and show files in the device. */
    err = mico_filesystem_scan_files( &fs_handle,(char *)mounted_name,scans_files_callback );
    
    /* Format the device when there is no space in the device or FORMAT is enabled. */
    if ( fatfs_info.total_space == 0 || FORMAT_ENABLE){
        fatfs_log( "filesystem free space is %d, need format", fatfs_info.total_space );
        fatfs_log( "start format filesystem" );
        err = mico_filesystem_format( &tester_block_device,MICO_FILESYSTEM_HANDLE_FATFS );
        if( err == 0 ){
        fatfs_log( "Mico_filesys_partition format success!");
        }
    }
    
    /* Open file in MICO_FILESYSTEM_OPEN_WRITE_CREATE way. */
    err = mico_filesystem_file_open( &fs_handle, &file_handle, filename, MICO_FILESYSTEM_OPEN_WRITE_CREATE );
    
    /* Write strings to the opened file. */
    count = sizeof( wtext )/sizeof(uint8_t *);
    for( i = 0; i < count; i++){
        err = mico_filesystem_file_write( &file_handle, wtext[i], strlen(wtext[i]), &byteswritten );
        fatfs_log( "fatfs write file,err %d,name:%s,data:%s,byteswritten:%d", err,filename ,wtext[i], (int)byteswritten );
    }
    
    /* Flush the writed data into the file. */
    err = mico_filesystem_file_flush( &file_handle );
    
    err = mico_filesystem_file_close( &file_handle );

    err = mico_filesystem_file_open( &fs_handle, &file_handle, filename, MICO_FILESYSTEM_OPEN_FOR_READ );

    count = get_file_size( &file_handle )/sizeof( buff );
    
    /* Read the whole file data out at times. */
    for( i = 0; i < count; i++ ){
        err = mico_filesystem_file_read( &file_handle, buff, sizeof(buff), &bytesread );
        for( j = 0; j < bytesread; j++)
        {
            rtext[i*sizeof(buff) + j] = buff[j];
        }
    }
    count = get_file_size( &file_handle )%sizeof( buff );
    if( count != 0 ){
            err = mico_filesystem_file_read( &file_handle, buff, count, &bytesread );
            for( j = 0; j < bytesread; j++)
            {
                rtext[i*sizeof(buff) + j] = buff[j];
            }
    }
    
    fatfs_log( "fatfs read file,err %d,name:%s,data:%s",err, filename , rtext );

    mico_filesystem_file_close( &file_handle );

    mico_rtos_delete_thread( NULL );
}

int application_start( void )
{
    OSStatus err;
    
    /* Test_fatfs thread. */
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "FileSystem", test_fatfs,
                                   0x2000,
                                   0 );
    require_noerr( err, exit );
    return 0;

exit:
    mico_rtos_delete_thread( NULL );
    return err;
}

