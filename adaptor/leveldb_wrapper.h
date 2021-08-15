//
// Created by zzyyyww on 2021/8/15.
//

#ifndef TABLEFS_LEVELDB_WRAPPER_H
#define TABLEFS_LEVELDB_WRAPPER_H

#include <vector>
#include "kv_wrapper.h"
#include "leveldb/db.h"
#include "leveldb/slice.h"
#include "leveldb/iterator.h"
#include "leveldb/write_batch.h"

namespace tablefs {
    class LeveldbWrapper : public KvWrapper {
    public:
        LeveldbWrapper() =default;
        ~LeveldbWrapper() override = default;

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

    private:
        Statistics statistics;
        Logging* logs_;
        Properties p_;

        std::string db_name;
        leveldb::DB *db_;
        leveldb::Cache *cache_;
        bool logon;
        bool writeahead;
        time_t last_sync_time;
        time_t sync_time_limit;
        int async_data_size;
        int sync_size_limit;

    };

    class LeveldbIterator: public KvIterator{
    public:
        LeveldbIterator(leveldb::Iterator *iter): iter_(iter) {};
        ~LeveldbIterator() override;

        bool Valid() override;

        void SeekToFirst() override;

        void SeekToLast() override;

        void Seek(const leveldb::Slice &target) override;

        void Next() override;

        void Prev() override;


        leveldb::Slice key() const override;

        leveldb::Slice value() const override;

    private:
        leveldb::Iterator *iter_;
    };
}

#endif //TABLEFS_LEVELDB_WRAPPER_H
