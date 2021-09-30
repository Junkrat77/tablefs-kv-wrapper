//
// Created by zyw on 2021/9/28.
//

#ifndef TABLEFS_HIKV_ADAPTOR_H
#define TABLEFS_HIKV_ADAPTOR_H

#include "adaptor/kv_wrapper.h"

#include "leveldb/slice.h"
#include "leveldb/write_batch.h"

#include "statistics.h"
#include "util/properties.h"
#include "util/tfs_logging.h"

#include "open_hikv.h"

namespace tablefs {

    using HikvSlice = open_hikv::Slice;

    class HikvIterator;

    class HikvWrapper : public KvWrapper{
    public:
        HikvWrapper() = default;

        ~HikvWrapper() = default;
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

        KvIterator *NewIterator() override;

        void Report() { statistics.Report(); };

        bool GetStat(std::string stat, std::string *value) override;

        bool GetMetric(std::string *value) override;

    private:
        Statistics statistics;
        Logging *logs_;
        Properties p_;

        bool logon;
        time_t last_sync_time;
        time_t sync_time_limit;
        int async_data_size;
        int sync_size_limit;

        std::unique_ptr<open_hikv::OpenHiKV> db_;
    };

    class HikvIterator : public KvIterator {
    public:
        HikvIterator(open_hikv::OpenHiKV*);

        ~HikvIterator() = default;

        bool Valid();

        void SeekToFirst();

        void SeekToLast();

        void Seek(const leveldb::Slice &target);

        void Next();

        void Prev();


        leveldb::Slice key() const;

        leveldb::Slice value() const;

        friend HikvWrapper;
    private:
        open_hikv::OpenHiKV* db_;

        using KVPair = std::pair<HikvSlice, HikvSlice>;
        std::vector<KVPair> dentry_;
        size_t cursor{0};
    };

}
#endif //TABLEFS_HIKV_ADAPTOR_H
