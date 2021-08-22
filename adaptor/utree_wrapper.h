//
// Created by zzyyyww on 2021/8/16.
//

#ifndef TABLEFS_UTREE_WRAPPER_H
#define TABLEFS_UTREE_WRAPPER_H

#include "kv_wrapper.h"
#include "kv/utree/singleThread/uTree/utree.h"

namespace tablefs {
    class uTreeWrapper: public KvWrapper {
    public:
        uTreeWrapper() = default;
        ~uTreeWrapper() override {statistics.Report();};
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

        btree* utree_;
    };

    class uTreeIterator: public KvIterator {
    public:
        explicit uTreeIterator(btree* utree);
        ~uTreeIterator() override =default;

        bool Valid() override;

        void SeekToFirst() override;

        void SeekToLast() override;

        void Seek(const leveldb::Slice &target) override;

        void Next() override;

        void Prev() override;


        leveldb::Slice key() const override;

        leveldb::Slice value() const override;

    private:
        btree* utree_ {nullptr};
        list_node_t* node_ {nullptr};
        bool init {false};
    };
}

#endif //TABLEFS_UTREE_WRAPPER_H
