//
// Created by zyw on 2021/9/26.
//

#ifndef TABLEFS_ROCKSDB_WRAPPER_H
#define TABLEFS_ROCKSDB_WRAPPER_H
#include <vector>
#include "kv_wrapper.h"
#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/iterator.h"
#include "rocksdb/write_batch.h"

namespace tablefs {
    class RocksdbWrapper : public KvWrapper {
    public:
        RocksdbWrapper() =default;
        ~RocksdbWrapper() override =default;

        int Init() override;

        void Cleanup() override;

        int Put(const leveldb::Slice &key,
                const leveldb::Slice &value) override;

        int Get(const leveldb::Slice &key,
                std::string &result) override;

        int Delete(const leveldb::Slice &key) override;

        int Write(leveldb::WriteBatch &batch) override;

        int Sync() override;

        KvIterator* NewIterator() override;

        //void Report() {statistics.Report();};

        bool GetStat(std::string stat, std::string* value) override;

        bool GetMetric(std::string* value) override;

    private:
        Statistics statistics;
        Logging* logs_;
        Properties p_;

        std::string db_name;
        rocksdb::DB *db_;
        std::shared_ptr<rocksdb::Cache> cache_;
        bool logon;
        bool writeahead;
        time_t last_sync_time;
        time_t sync_time_limit;
        int async_data_size;
        int sync_size_limit;

    };

    class RocksdbIterator: public KvIterator{
    public:
        RocksdbIterator(rocksdb::Iterator *iter): iter_(iter) {};
        ~RocksdbIterator() override;

        bool Valid() override;

        void SeekToFirst() override;

        void SeekToLast() override;

        void Seek(const leveldb::Slice &target) override;

        void Next() override;

        void Prev() override;


        leveldb::Slice key() const override;

        leveldb::Slice value() const override;

    private:
        rocksdb::Iterator *iter_;
    };
}
#endif //TABLEFS_ROCKSDB_WRAPPER_H
