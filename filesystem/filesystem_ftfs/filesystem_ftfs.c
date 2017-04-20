/**
 ******************************************************************************
 * @file    ftfs.c
 * @author  William Xu
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   First MiCO application to say hello world!
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2016 MXCHIP Inc.
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
 */

#include "mico.h"
#include "mico_filesystem.h"

#define ftfs_log(format, ...)  custom_log("helloworld", format, ##__VA_ARGS__)
#define IMAGE_FILENAME         "image.bin"

const mico_block_device_driver_t tester_block_device_driver;

typedef struct
{
    char* filename;
    FILE* image_file_handle;
    mico_block_device_write_mode_t write_mode;
} tester_block_device_specific_data_t;

tester_block_device_specific_data_t tester_block_device_specific_data =
{
    .filename = IMAGE_FILENAME,
};

const mico_block_device_init_data_t tester_block_device_init_data =
{
    .base_address_offset = 0,
    .maximum_size = 0,
    .volatile_and_requires_format_when_mounting = 0,
};

/* Initialize block device initial data£¬driver and special data.  */
mico_block_device_t tester_block_device =
{
        .init_data = &tester_block_device_init_data,
        .driver = &tester_block_device_driver,
        .device_specific_data = &tester_block_device_specific_data,
};

uint32_t get_file_size( mico_file_t* file_handle ){
    return *((uint32_t *)(file_handle->data.f) +1);
}

void test_ftfs( void )
{
    mico_filesystem_t fs_handle;
    mico_file_t file_handle;
    OSStatus err = kNoErr;
    
    /* Specify the filename in resources directory to be read. */
    const char filename[]="ftfs.h";
    
    char *fs_data = NULL;
    uint32_t fsize = 0;
    uint64_t returned_count = 0;

    memset((void *)&fs_handle,0,sizeof(fs_handle));
    memset((void *)&file_handle,0,sizeof(file_handle));
    
    /* Initialize mico file system. */
    mico_filesystem_init();
    
    /* Mount FTFS file system. */
    mico_filesystem_mount( &tester_block_device, MICO_FILESYSTEM_HANDLE_FTFS, &fs_handle, IMAGE_FILENAME);

    /* Open , in a readonly way, the file with the specific filename, in the resources directory. */
    err = mico_filesystem_file_open( &fs_handle, &file_handle, filename, MICO_FILESYSTEM_OPEN_FOR_READ);
    if(err != kNoErr){
        ftfs_log("File open err.");
        require_noerr( err, exit );
    }
    else{
        fsize = get_file_size( &file_handle );
        ftfs_log( "File open success, file size = %d Bytes.", fsize);
        
        /* Allocate space for ready the file data. When the size fo the file to be read is big, it's not recommended.  */
        fs_data = (char *)malloc(fsize + 1);

        if( fs_data == NULL ){
            ftfs_log("Memory allocation err.");
            require_noerr( 1, exit );
        }

        memset( fs_data, 0, fsize + 1 );
        
        /* Read the whole data of the file to the allocated space, and print it. */
        err = mico_filesystem_file_read( &file_handle, fs_data, fsize, &returned_count);
        if(err != kNoErr)
        {
            ftfs_log("File read err.");
            require_noerr( err, exit );
        }
        ftfs_log("Data is: \n\r %s",fs_data);
    }

exit:
    mico_filesystem_file_close( &file_handle );
    mico_rtos_delete_thread( NULL );
}
int application_start( void )
{
    OSStatus err;
    
    /* Test_ftfs thread. */
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "FileSystem", test_ftfs,
                                   0x2000,
                                   0 );
    require_noerr( err, exit );
    return 0;

exit:
    mico_rtos_delete_thread( NULL );
    return err;
}


