#ifndef __CONST_H__
#define __CONST_H__


const int MYDBM_FILE_PATH_LEN     = 1024;
const int MYDBM_MAX_BUF_LEN       = 2048;

/*******************************/
/* Error information           */
/*******************************/
const int MYDBM_OK                = 0;

const int MYDBM_ERR_OPEN          = 1;
const int MYDBM_ERR_RD            = 2;
const int MYDBM_ERR_WR            = 3;
const int MYDBM_ERR_STAT          = 4;

const int MYDBM_ERR_NOT_EXIST     = 11;
const int MYDBM_ERR_EXIST         = 12;

const int MYDBM_ERR_MEM           = 21;


#endif
