//
// Created by zyw on 2021/10/3.
//

#ifndef TABLEFS_ROART_WRAPPER_H
#define TABLEFS_ROART_WRAPPER_H

#include "kv_wrapper.h"
#include "Tree.h"
namespace tablefs {
    class RoartWrappr: public KvWrapper {
    public:
        RoartWrappr() = default;
        ~RoartWrappr() override {statistics.Report();};
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

        std::unique_ptr<PART_ns::Tree> db_;
    };

    class RoartIterator: public KvIterator {
    public:
        RoartIterator() =default;
        explicit RoartIterator(PART_ns::Tree* tree) : db_(tree) {};
        ~RoartIterator() =default;

        bool Valid() override;

        void SeekToFirst() override;

        void SeekToLast() override;

        void Seek(const leveldb::Slice &target) override;

        void Next() override;

        void Prev() override;

        leveldb::Slice key() const override;

        leveldb::Slice value() const override;
    private:
        using KVPair = std::pair<std::string, std::string>;
        PART_ns::Tree* db_;
        std::vector<KVPair> dentry_;
        size_t cursor_{0};
    };
}

#endif //TABLEFS_ROART_WRAPPER_H
