//
// Created by zyw on 2021/9/26.
//

#ifndef TABLEFS_METAKV_WRAPPER_H
#define TABLEFS_METAKV_WRAPPER_H

#include <fs/tfs_inode.h>
#include "leveldb/slice.h"
#include "leveldb/write_batch.h"

#include "statistics.h"
#include "util/properties.h"
#include "util/tfs_logging.h"
#include "kv_wrapper.h"

#include "src/MetaDB.h"

namespace tablefs {

    struct tfsKey: public MetaKey {

        std::string prefix_;
        std::string suffix_;

        tfsKey(std::string prefix, std::string suffix):
                prefix_(std::move(prefix)),
                suffix_(std::move(suffix)){}

        tfsKey(leveldb::Slice key) {
            //tfs_meta_key_t *raw_key = (tfs_meta_key_t*)(key.data());
            prefix_ = std::string(key.data(), 8);
            suffix_ = std::string(key.data() + 8, 8);
        }

        size_t EncodeSize() override {
            return prefix_.size() + suffix_.size() + 2 * sizeof(uint64_t);
        }
        void Encode() override {
            buff_.clear();
            PutFixed64(&buff_, prefix_.size());
            buff_.append(prefix_);
            PutFixed64(&buff_, suffix_.size());
            buff_.append(suffix_);
            internal_key_ = Slice(buff_);
        }
        void Decode() override {
            Slice temp(internal_key_);
            uint64_t prefix_size = GetFixed64(&temp);
            prefix_ = std::string(temp.data(), prefix_size);
            temp = Slice(temp.data() + prefix_size);
            uint64_t suffix_size = GetFixed64(&temp);
            suffix_ = std::string(temp.data(), suffix_size);
        }
        void GetPrefix(Slice * buff) override {
            *buff = Slice(prefix_);
        }
        void GetSuffix(Slice * buff) override {
            *buff = Slice(suffix_);
        }
    };

    struct tfsValue : public MetaValue {
        std::string value;
        tfsValue(std::string v) : value(std::move(v)) {}
        tfsValue() =default;
        void GetValue(Slice * buff) override {
            *buff = Slice(internal_value_);
        }
        void Decode() override {
            value = internal_value_.ToString();
        }
        void Encode() override {
            buff_ = value;
            internal_value_ = Slice(buff_);
        }
    };

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
        Logging* logs_;
        Properties p_;

        std::string path_{"/mnt/pmem/tablefs-data"};
        MetaDB db_;

        bool logon;
        time_t last_sync_time;
        time_t sync_time_limit;
        int async_data_size;
        int sync_size_limit;
    };

    typedef std::pair<std::string, std::string> KVPair;
    class MetaKVIterator: public KvIterator {
    public:
        MetaKVIterator() = default;
        ~MetaKVIterator() = default;

        explicit MetaKVIterator(MetaDB* db) : db_(db){}

        bool Valid() override;

        void SeekToFirst() override;

        void SeekToLast() override;

        void Seek(const leveldb::Slice &target) override;

        void Next() override;

        void Prev() override;


        leveldb::Slice key() const override;

        leveldb::Slice value() const override;

    private:
        std::vector<LogEntryRecord> dentry;
        //std::vector<LogEntryRecord>::iterator iter;
        size_t cursor{0};
        MetaDB* db_;
    };

}

#endif //TABLEFS_METAKV_WRAPPER_H
