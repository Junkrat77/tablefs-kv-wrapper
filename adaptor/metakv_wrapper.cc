//
// Created by zyw on 2021/9/26.
//

#include <sys/stat.h>
#include <iostream>
#include "metakv_wrapper.h"
#include "fs/tfs_inode.h"

namespace tablefs{

    int MetaKVWrapper::Init() {
        struct DBOption db_option;
        SetDefaultDBop(&db_option);
#ifdef DMETAKV_CACHE
        SetCacheOp(1, 1, (100ul * (1ul << 20)), (100ul * (1ul << 20)))
#endif
        DBOpen(&db_option,"/home/test", &db_);
        return 1;
    }

    void MetaKVWrapper::Cleanup() {

    }

    /* return 0: Put successfully */
    int MetaKVWrapper::Put(const leveldb::Slice &key, const leveldb::Slice &value) {
        uint64_t* pinode_hash_fname = (uint64_t*)key.data();
        char* stat_fname = (char*)value.data();
        uint64_t pinode = *pinode_hash_fname;
        uint64_t hash_fname = *(pinode_hash_fname + 1);
        char* fname = stat_fname + TFS_INODE_HEADER_SIZE;
        tfs_inode_header* header = reinterpret_cast<tfs_inode_header*>(stat_fname);
        uint64_t inode = header->fstat.st_ino;
        struct stat* fstat = &header->fstat;
        printf("[MetaKVPut Begin]: pinode: %ld hash_fname: %ld fname: %s, inode: %ld\n", pinode, hash_fname, fname, inode);
        return MetaKVPut(&db_, pinode, hash_fname, fname, inode, fstat);
    }

    int MetaKVWrapper::Get(const leveldb::Slice &key, std::string &result) {
        uint64_t* pinode_hash_fname = (uint64_t*)key.data();
        uint64_t pinode = *pinode_hash_fname;
        uint64_t hash_fname = *(pinode_hash_fname + 1);
        struct Slice value;
        value.data = NULL;
        int res = MetaKVGet(&db_, pinode, hash_fname, &value);
        if (0 == res) return 0;
        result = std::string(value.data, value.len);
        struct stat* temp1 = (struct stat*)result.c_str();
        
        char* fname = (char*)temp1 + TFS_INODE_HEADER_SIZE;
        printf("[MetaKVGet Successfully]: pinode: %ld, hash_fname: %ld, fname: %s, inode: %ld\n", pinode, hash_fname, fname, temp1->st_ino);
        // printf("[MetaKVGet Successfully]: pinode: %ld, hash_fname: %ld, inode: %ld\n", pinode, hash_fname, temp2->st_ino);
        if (value.data != NULL) free(value.data);
        return 1;

        // 对blob怎么处理？

    }

    int MetaKVWrapper::Delete(const leveldb::Slice &key) {
        uint64_t* pinode_hash_fname = (uint64_t*)key.data();
        uint64_t pinode = *pinode_hash_fname;
        uint64_t hash_fname = *(pinode_hash_fname + 1);
        return MetaKVDelete(&db_, pinode, hash_fname);
    }

    class MetaDBInserter : public leveldb::WriteBatch::Handler {
    public:
        MetaDBInserter(MetaKVWrapper *wrapper) : _wrapper(wrapper) {};

        ~MetaDBInserter() {};

        void Put(const leveldb::Slice &key, const leveldb::Slice &value) override {
            this->Put(key, value);
        };

        void Delete(const leveldb::Slice &key) override {
            this->Delete(key);
        };
    private:
        MetaKVWrapper *_wrapper;
    };

    int MetaKVWrapper::Write(leveldb::WriteBatch &batch) {
        MetaDBInserter inserter(this);
        batch.Iterate(&inserter);
        return 1;
    }

    int MetaKVWrapper::Sync() {
        return 1;
    }

    KvIterator *MetaKVWrapper::NewIterator() {
        return new MetaKVIterator(&db_);
    }

    bool MetaKVWrapper::GetStat(std::string stat, std::string *value) {
        return true;
    }

    bool MetaKVWrapper::GetMetric(std::string *value) {
        return true;
    }

    void MetaKVIterator::Seek(const leveldb::Slice &target) {
        uint64_t pinode = *(uint64_t*)(target.ToString().data());
        printf("Readdir pinode: %lu\n", pinode);
        int res = MetaKVScan(db_, pinode, &scan_result);
        if (0 == res) result_len = 0;
        else {
            cursor = scan_result + sizeof(result_len);
            result_len = *(uint64_t*)scan_result;
        }
    }

    void MetaKVIterator::SeekToFirst() {
        //iter = dentry.begin();
        cursor += sizeof(result_len); 
    }

    void MetaKVIterator::SeekToLast() {
        //iter = dentry.end();
        std::cout << "wo zan shi hai mei xiang hao zen me shi xian zhe ge function!\n" << std::endl;
        assert(0);
    }

    void MetaKVIterator::Next() {
        uint64_t fname_len = (*(uint64_t*)cursor) - sizeof(uint64_t); 
        cursor += fname_len + fname_header_len;
    }

    void MetaKVIterator::Prev() {
        std::cout << "zhe ge function ye mei you shi xian!\n" << std::endl;
        assert(0);
    }

    bool MetaKVIterator::Valid() {
        //return iter != dentry.end();
        int len = cursor - scan_result;
        return len != result_len;
        // return cursor != (result_len + scan_result);
    }

    leveldb::Slice MetaKVIterator::key() const {
        //return leveldb::Slice(iter->key.data(), iter->key.size());
        uint64_t key_len = 2 * sizeof(uint64_t);
        char* key = cursor + sizeof(uint32_t);

        return leveldb::Slice(key, key_len);
    }

    leveldb::Slice MetaKVIterator::value() const {
        //return leveldb::Slice(iter->value.data(), iter->value.size());
        uint64_t fname_len = (*(uint32_t*)cursor) - sizeof(uint64_t); 
        char* fname = cursor + fname_header_len;
        //printf("fname_len = %lu\n", fname_len);
        return leveldb::Slice(fname, fname_len);
    }
}