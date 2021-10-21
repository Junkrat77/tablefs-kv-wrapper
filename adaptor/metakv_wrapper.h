//
// Created by zyw on 2021/9/26.
//

#ifndef TABLEFS_METAKV_WRAPPER_H
#define TABLEFS_METAKV_WRAPPER_H

#include "kv_wrapper.h"
#include <vector>
extern "C" {
    #include "include/metadb.h"
}

namespace tablefs {
    class MetaKVWrapper: public KvWrapper {
    public:
        MetaKVWrapper() = default;
        virtual ~MetaKVWrapper() = default;
        /*
         * Init the KVDB
         * */
        int Init() override;

        /*
         * Destroy the KVDB
         * */
        void Cleanup() override;

        int Put(const leveldb::Slice &key,
                        const leveldb::Slice &value) override;

        int Get(const leveldb::Slice &key,
                        std::string &result) override;

        int Delete(const leveldb::Slice &key) override;

        int Write(leveldb::WriteBatch &batch) override;

        int Sync() override;

        KvIterator* NewIterator() override;

        void Report() {statistics.Report();};

        bool GetStat(std::string stat, std::string* value) override;

        bool GetMetric(std::string* value) override;

    private:
        Statistics statistics;
        // std::string path_{"/mnt/pmem/tablefs-data"};
        struct MetaDb db_;
    };

    typedef std::pair<std::string, std::string> KVPair;
    class MetaKVIterator: public KvIterator {
    public:
        MetaKVIterator() = default;
        // ~MetaKVIterator() = default;

        explicit MetaKVIterator(struct MetaDb* db) : db_(db){

        }

        ~MetaKVIterator() {
            if (scan_result != NULL)
                free(scan_result);
        }

        bool Valid() override;

        void SeekToFirst() override;

        void SeekToLast() override;

        void Seek(const leveldb::Slice &target) override;

        void Next() override;

        void Prev() override;


        leveldb::Slice key() const override;

        leveldb::Slice value() const override;

    private:
        // using KVPair = std::pair<std::string, std::string>;
        // std::vector<KVPair> dentry;
        /* result_len 
        /* value_len(4) + pindo(8) + hash_fname(8) + inode(8) + fname */
        char* scan_result{NULL};
        uint64_t result_len;
        char* cursor{NULL};
        struct MetaDb* db_;
        uint64_t fname_header_len{28};
    };

}

#endif //TABLEFS_METAKV_WRAPPER_H
