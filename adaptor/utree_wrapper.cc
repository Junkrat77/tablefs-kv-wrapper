//
// Created by zzyyyww on 2021/8/16.
//

#include <libpmem.h>
#include "utree_wrapper.h"
#include "lib/leveldb/util/coding.h"

using namespace leveldb;

namespace tablefs {

class uTreeInserter: public WriteBatch::Handler {
public:

    uTreeInserter(treedb::TreeDB* db): db_(db) {}

    void Put(const Slice& key, const Slice& value) override {
        db_->Put(key.ToString(), value.ToString());
    }

    void Delete(const Slice& key) override {
        db_->Delete(key.ToString());
    }

private:
    treedb::TreeDB* db_;
};

const std::string log_path("/mnt/pmem/utree/log");
const uint64_t log_size = 60UL * 1024 * 1024 * 1024;

    int uTreeWrapper::Init() {
        db_ = std::make_unique<treedb::TreeDB>(log_path, log_size);
        return 1;
    }

    void uTreeWrapper::Cleanup() {

    }

    int uTreeWrapper::Put(const leveldb::Slice &key, const leveldb::Slice &value) {
        bool res = db_->Put(key.ToString(), value.ToString());
        if (res) {
            return 0;
        } else {
            return -1;
        }
    }

    int uTreeWrapper::Get(const leveldb::Slice &key, std::string &result) {
        bool res = db_->Get(key.ToString(), &result);
        if (res) {
            return 1;
        } else {
            return 0;
        }
    }

    int uTreeWrapper::Delete(const leveldb::Slice &key) {
        db_->Delete(key.ToString());
        return 0;
    }

    int uTreeWrapper::Write(leveldb::WriteBatch &batch) {
        uTreeInserter inserter(db_.get());
        batch.Iterate(&inserter);
        return 0;
    }

    bool uTreeWrapper::GetMetric(std::string *value) {
        return true;
    }

    bool uTreeWrapper::GetStat(std::string stat, std::string *value) {
        return true;
    }

    int uTreeWrapper::Sync() {
        return 0;
    }

    KvIterator *uTreeWrapper::NewIterator() {
        return new uTreeIterator(db_.get());
    }

    uTreeIterator::uTreeIterator(treedb::TreeDB* db): db_(db) {}

    bool uTreeIterator::Valid() {
        return cursor_ < dentry_.size();
    }

    void uTreeIterator::SeekToFirst() {
        cursor_ = 0;
    }

    void uTreeIterator::SeekToLast() {
        cursor_ = dentry_.size() - 1;
    }

    void uTreeIterator::Seek(const leveldb::Slice &target) {
        db_->Scan(target.ToString(), dentry_);
    }

    void uTreeIterator::Next() {
        cursor_++;
    }

    void uTreeIterator::Prev() {
        cursor_--;
    }

    leveldb::Slice uTreeIterator::key() const {
        return {dentry_[cursor_].first};
    }

    leveldb::Slice uTreeIterator::value() const {
        return {dentry_[cursor_].second};
    }


}
