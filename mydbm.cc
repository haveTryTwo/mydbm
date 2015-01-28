#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <iostream>

#include "log.h"
#include "mydbm.h"
#include "crc32.h"
#include "const.h"

using namespace std;


MYDBM::MYDBM(const string &p) : path(p)
{
    this->init();
}

MYDBM::MYDBM(const MYDBM &dbm)
{
    this->path = dbm.path;
    this->idxFp = dbm.idxFp;
    this->datFp = dbm.datFp;
    this->index = dbm.index;
}

MYDBM& MYDBM::operator= (const MYDBM &dbm)
{
    this->destroy();

    this->path = dbm.path;
    this->idxFp = dbm.idxFp;
    this->datFp = dbm.datFp;
    this->index = dbm.index;

    return *this;
}

MYDBM::~MYDBM()
{
    this->destroy();
}

int MYDBM::destroy()
{
    if (this->idxFp != NULL) 
    {
        fclose(this->idxFp);
        this->idxFp = NULL;
    }
    if (this->datFp != NULL)
    {
        fclose(this->datFp);
        this->datFp = NULL;
    }
    if (this->index != NULL)
    {
        delete this->index;
        this->index = NULL;
    }

    return MYDBM_OK;
}

int MYDBM::init()
{
    int ret = MYDBM_OK;
    string type("r+");
    bool first = false;
    string idxPath(this->path);
    string datPath(this->path);

    idxPath.append(".idx");
    datPath.append(".dat");

    // Create index
    this->index = new Index;

    // Create or Open index file
    ret = access(idxPath.c_str(), F_OK);
    if (ret == -1) 
    {
        type.assign("w+");
        first = true;
    }
    this->idxFp = fopen(idxPath.c_str(), type.c_str());
    if (this->idxFp == NULL)
    {
        LOG_ERR("Failed to open file:%s, errno:%d", idxPath.c_str(), errno);
        goto idxErr;
    }
    ret = this->initIndex(first);
    if (ret != MYDBM_OK) goto idxInitErr;

    // Create or Open index file
    ret = access(datPath.c_str(), F_OK);
    if (ret == -1)
        type.assign("w+");
    else 
        type.assign("r+");
    this->datFp = fopen(datPath.c_str(), type.c_str());
    if (this->datFp == NULL)
    {
        LOG_ERR("Failed to open file:%s, errno:%d", datPath.c_str(), errno);
        goto datErr;
    }

    return MYDBM_OK;

datErr:
idxInitErr:
    fclose(this->idxFp);
    this->idxFp = NULL;
idxErr:
    return MYDBM_ERR_OPEN;
}

int MYDBM::initIndex(bool first)
{
    uint32_t buf[2] = {0};

    if (first)
    {
        this->index->maxNum = MYDBM_MAX_SIZE;
        this->index->usedCnt = 0;
        buf[0] = this->index->maxNum;
        buf[1] = this->index->usedCnt;

        fwrite(buf, sizeof(uint32_t), 2, this->idxFp);
        uint64_t tmp[MYDBM_MAX_SIZE] = {0};
        fwrite((void*)&tmp, sizeof(uint64_t), MYDBM_MAX_SIZE, this->idxFp); 
        fflush(this->idxFp);
    }
    else
    {
        fread(buf, sizeof(uint32_t), 2, this->idxFp);
        this->index->maxNum = *(uint32_t*)&buf[0];
        this->index->usedCnt = *(uint32_t*)&buf[1];
    }

    assert(this->index->maxNum > 0);

    return MYDBM_OK;
}

int MYDBM::insert(const string &key, const string &value)
{
    Bucket bkt;
    Bucket newBkt;
    string tmpVal;
    int ret = MYDBM_OK;
    uint64_t curPos = 0;
    uint64_t idxPos = 0;
    uint64_t datPos = 0;
    bool firstKey = true;
    uint32_t keyHash = 0;
    uint32_t hashPos = 0;
    uint64_t nextPos = 0;

    ret = this->fetch(key, tmpVal);
    if (ret == MYDBM_OK) return MYDBM_ERR_EXIST;

    keyHash = dbm_crc32(key);
    hashPos = (keyHash % this->index->maxNum) * sizeof(uint64_t) + sizeof(uint32_t) * 2;

    fseek(this->idxFp, hashPos, SEEK_SET);
    ret = fread((void*)&nextPos, sizeof(uint64_t), 1, this->idxFp); 
    assert(ret > 0);

    if (nextPos != 0) firstKey = false;

    // find the last bucket of same hash
    while (nextPos != 0)
    {
        curPos = nextPos;
        fseek(this->idxFp, nextPos, SEEK_SET);
        ret = fread((void*)&bkt, sizeof(Bucket), 1, this->idxFp);
        assert(ret != 0);
        nextPos = bkt.next;
    }

    ret = this->getLastPos(idxPos, ".idx");
    assert(ret == MYDBM_OK);
    ret = this->getLastPos(datPos, ".dat");
    assert(ret == MYDBM_OK);
    tmpVal.assign(key);
    tmpVal.append(value);

    cout << tmpVal << endl;
    fseek(this->datFp, datPos, SEEK_SET);
    ret = fwrite(tmpVal.c_str(), sizeof(char), tmpVal.size(), this->datFp);
    if ((size_t)ret != tmpVal.size()) return MYDBM_ERR_WR;
    newBkt.next = 0;
    newBkt.keyHash = keyHash;
    newBkt.keySize = key.size();
    newBkt.valSize = value.size();
    newBkt.datPos = datPos; 
    memcpy(newBkt.preKey, key.data(), MYDBM_KEY_PRE_SIZE < key.size() ? MYDBM_KEY_PRE_SIZE : key.size());

    fseek(this->idxFp, idxPos, SEEK_SET);
    ret = fwrite((void*)&newBkt, sizeof(Bucket), 1, this->idxFp);
    if (ret != 1) return MYDBM_ERR_WR;

    if (firstKey)
    {
        fseek(this->idxFp, hashPos, SEEK_SET);
        ret = fwrite((void*)&idxPos, sizeof(uint64_t), 1, this->idxFp);
        if (ret != 1) return MYDBM_ERR_WR;
    }
    else
    {
        bkt.next = idxPos;
        fseek(this->idxFp, curPos, SEEK_SET);
        ret = fwrite((void*)&bkt, sizeof(Bucket), 1, this->idxFp);
        if (ret != 1) return MYDBM_ERR_WR;
    }

    this->index->usedCnt++;
    fseek(this->idxFp, sizeof(uint32_t), SEEK_SET);
    ret = fwrite((void*)&(this->index->usedCnt), sizeof(uint32_t), 1, this->idxFp);
    if (ret != 1) return MYDBM_ERR_WR;

    fflush(this->idxFp);
    fflush(this->datFp);

    return MYDBM_OK;
}

int MYDBM::getLastPos(uint64_t &pos, const string &suf)
{
    struct stat st;
    string idxPath(this->path);
    
    idxPath.append(suf);
    int ret = stat(idxPath.c_str(), &st);
    if (ret == -1) return MYDBM_ERR_STAT;

    pos = st.st_size;

    return MYDBM_OK;
}

int MYDBM::fetch(const string &key, string &value)
{
    Bucket bkt;
    size_t size = 0;
    char *data = NULL;
    uint32_t keyHash = 0;
    uint64_t nextPos = 0;
    uint32_t hashPos = 0;

    keyHash = dbm_crc32(key);
    hashPos = (keyHash % this->index->maxNum) * sizeof(uint64_t) + sizeof(uint32_t) * 2;

    fseek(this->idxFp, hashPos, SEEK_SET);
    size = fread((void*)&nextPos, sizeof(uint32_t), 1, this->idxFp); 
    if (size == 0) return MYDBM_ERR_NOT_EXIST;

    memset((void*)&bkt, 0, sizeof(Bucket));
    while (nextPos != 0)
    {
        fseek(this->idxFp, nextPos, SEEK_SET);
        size = fread((void*)&bkt, sizeof(Bucket), 1, this->idxFp);
        assert(size > 0);

        if (keyHash == bkt.keyHash && key.size() == bkt.keySize 
                && memcmp(key.c_str(), bkt.preKey, 
                    MYDBM_KEY_PRE_SIZE < key.size() ? MYDBM_KEY_PRE_SIZE : key.size()) == 0)
        {
            data = (char*) malloc(bkt.keySize + bkt.valSize);
            if (data == NULL) return MYDBM_ERR_MEM;

            fseek(this->datFp, bkt.datPos, SEEK_SET); 
            size = fread(data, sizeof(char), bkt.keySize + bkt.valSize, this->datFp);
            if (size != 0 && memcmp(key.c_str(), data, key.size()) == 0)
            {
                value.assign(data + bkt.keySize, bkt.valSize);
                free(data);
                data = NULL;
                return MYDBM_OK;
            }
            free(data);
            data = NULL;
        }

        nextPos = bkt.next;
    }

    return MYDBM_ERR_NOT_EXIST;
}

int MYDBM::remove(const string &key)
{
    Bucket bkt;
    Bucket curBkt;
    int ret = MYDBM_OK;
    char *data = NULL;
    uint64_t curPos = 0;
    bool firstKey = true;
    uint32_t keyHash = 0;
    uint32_t hashPos = 0;
    uint64_t nextPos = 0;

    keyHash = dbm_crc32(key);
    hashPos = (keyHash % this->index->maxNum) * sizeof(uint64_t) + sizeof(uint32_t) * 2;

    fseek(this->idxFp, hashPos, SEEK_SET);
    ret = fread((void*)&nextPos, sizeof(uint64_t), 1, this->idxFp); 
    assert(ret > 0);

    if (nextPos == 0) return MYDBM_ERR_NOT_EXIST;

    // find the last bucket of same hash
    while (nextPos != 0)
    {
        fseek(this->idxFp, nextPos, SEEK_SET);
        ret = fread((void*)&bkt, sizeof(Bucket), 1, this->idxFp);
        assert(ret > 0);

        if (keyHash == bkt.keyHash && key.size() == bkt.keySize 
                && memcmp(key.c_str(), bkt.preKey, 
                    MYDBM_KEY_PRE_SIZE < key.size() ? MYDBM_KEY_PRE_SIZE : key.size()) == 0)
        {
            data = (char*) malloc(bkt.keySize + bkt.valSize);
            if (data == NULL) return MYDBM_ERR_MEM;

            fseek(this->datFp, bkt.datPos, SEEK_SET); 
            ret = fread(data, sizeof(char), bkt.keySize + bkt.valSize, this->datFp);
            if (ret != 0 && memcmp(key.c_str(), data, key.size()) == 0)
            {
                if (firstKey)
                {
                    fseek(this->idxFp, hashPos, SEEK_SET);
                    ret = fwrite((void*)&bkt.next, sizeof(uint64_t), 1, this->idxFp);
                    assert(ret == 1);
                }
                else
                {
                    curBkt.next = bkt.next;
                    fseek(this->idxFp, curPos, SEEK_SET);
                    ret = fwrite((void*)&curBkt, sizeof(Bucket), 1, this->idxFp);
                    assert(ret == 1);
                }
                fflush(this->idxFp);
                free(data);
                data = NULL;
                return MYDBM_OK;
            }
            free(data);
            data = NULL;
        }

        curPos = nextPos;
        curBkt = bkt;
        nextPos = bkt.next;
        firstKey = false;
    }

    return MYDBM_ERR_NOT_EXIST;
}

#ifdef __MYDBM_MAIN_TEST__

int main(int argc, char *argv[])
{
    string str("a");
    MYDBM *db = new MYDBM(str);

    string key("my key1");
    string value("my value1");

    int ret = MYDBM_OK;
    string ret_val;

    ret = db->insert(key, value);
    assert(ret == MYDBM_OK || ret == MYDBM_ERR_EXIST);
    ret = db->fetch(key, ret_val);
    assert(ret == MYDBM_OK);
    cout << key << " : " << ret_val << endl;

    char keybuf[1024] = {0};
    char valbuf[1024] = {0};
    for (int i = 0; i < 100000; ++i)
    {
        snprintf(keybuf, sizeof(keybuf), "%s:%d", "key:", i);
        snprintf(valbuf, sizeof(valbuf), "%s:%d", "value-", i);
        db->insert(keybuf, valbuf);

        ret = db->fetch(keybuf, ret_val);
        if (ret == MYDBM_OK)
            cout << keybuf << " == " << ret_val << endl;
    }
    
    key.assign("key::99999");
    ret = db->remove(key);
    assert(ret == MYDBM_OK);
    ret = db->fetch(key, value);
    assert(ret == MYDBM_ERR_NOT_EXIST);


    // binary code test
    uint32_t n = 0x01020304;
    key.assign((char*)&n, sizeof(uint32_t));
    value.assign((char*)&n, sizeof(uint32_t));
    ret = db->insert(key, value);
    assert(ret == MYDBM_OK);
    ret = db->fetch(key, ret_val);
    assert(ret == MYDBM_OK);
    printf("key:%#x, ret_value:%#x\n", *(int*)key.data(), *(int*)ret_val.data());
    ret = db->remove(key);
    assert(ret == MYDBM_OK);

    delete db;

    return 0;
}
#endif
