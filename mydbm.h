#ifndef __MYDBM_H__
#define __MYDBM_H__

#include <stdint.h>

#include <string>

class MYDBM
{
    public:
        const static uint32_t MYDBM_MAX_SIZE = 1024 * 1024;
        const static uint32_t MYDBM_KEY_PRE_SIZE = 8;

    public:
        MYDBM(const std::string &p);
        MYDBM(const MYDBM &dbm);
        MYDBM& operator=(const MYDBM &dbm);
        ~MYDBM();

    public:
        int fetch(const std::string &key, std::string &value);
        int insert(const std::string &key, const std::string &value);
        int remove(const std::string &key);

    private:
        int init();
        int initIndex(bool first);
        int destroy();
        int getLastPos(uint64_t &pos, const std::string &suf);

    public:
        struct Bucket
        {
            uint64_t next;
            uint32_t keyHash;
            char preKey[MYDBM_KEY_PRE_SIZE];
            uint32_t keySize;
            uint32_t valSize;
            uint64_t datPos;
        };

        struct Index
        {
            uint32_t maxNum;
            uint32_t usedCnt;
            uint64_t hashDir[MYDBM_MAX_SIZE];
            Bucket   buckets[MYDBM_MAX_SIZE];
        };

    private:
        std::string path;
        FILE *idxFp;
        FILE *datFp;
        Index *index;
};

#endif
