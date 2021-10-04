//
// Created by zyw on 2021/10/4.
//

#ifndef TABLEFS_TLHASH_H
#define TABLEFS_TLHASH_H

#include <memory>

#include "kv_wrapper.h"
#include "HybridHash.h"

namespace tablefs {
    class TlhashWrapper: public KvWrapper {
    public:
        TlhashWrapper() = default;
        ~TlhashWrapper() override {statistics.Report();};
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

        std::unique_ptr<HybridHash> db_;
    };

    class TlhashIterator: public KvIterator {
    public:
        TlhashIterator() =default;
        explicit TlhashIterator(HybridHash* tree) : db_(tree) {};
        ~TlhashIterator() =default;

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
        HybridHash* db_;
        std::vector<KVPair> dentry_;
        size_t cursor_{0};
    };
}

#endif //TABLEFS_TLHASH_H
