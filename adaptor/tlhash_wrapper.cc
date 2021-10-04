//
// Created by zyw on 2021/10/4.
//

#include "tlhash_wrapper.h"

namespace tablefs {
    int TlhashWrapper::Init() {
        db_ = std::make_unique<HybridHash>();
        db_->Init();
        return 1;
    }

    void TlhashWrapper::Cleanup() {
        db_->Exit();
    }

    int TlhashWrapper::Put(const leveldb::Slice &key, const leveldb::Slice &value) {
        bool res = db_->Put(key.ToString(), value.ToString());
        if (res) {
            {
                std::string check;
                bool check_res = db_->Get(key.ToString(), &check);
                assert(value == leveldb::Slice(check));
                //printf("check put: %d\n", check_res);
            }
            return 0;
        } else {
            return -1;
        }
    }

    int TlhashWrapper::Get(const leveldb::Slice &key, std::string &result) {
        std::string value;
        bool res = db_->Get(key.ToString(), &result);
        if (res) {
            return 1;
        } else {
            return 0;
        }
    }

    int TlhashWrapper::Delete(const leveldb::Slice &key) {
        db_->Delete(key.ToString());
        return 0;
    }

    class TlhashInserter : public leveldb::WriteBatch::Handler {
    public:
        TlhashInserter(HybridHash *tree) : tree_(tree) {}

        void Put(const leveldb::Slice &key, const leveldb::Slice &value) override {
            tree_->Put(key.ToString(), value.ToString());
        }

        void Delete(const leveldb::Slice &key) override {
            tree_->Delete(key.ToString());
        }

    private:
        HybridHash *tree_;
    };

    int TlhashWrapper::Write(leveldb::WriteBatch &batch) {
        TlhashInserter handle(db_.get());
        batch.Iterate(&handle);
        return 0;
    }

    bool TlhashWrapper::GetMetric(std::string *value) {
        return true;
    }

    bool TlhashWrapper::GetStat(std::string stat, std::string *value) {
        return true;
    }

    int TlhashWrapper::Sync() {
        return 0;
    }

    KvIterator *TlhashWrapper::NewIterator() {
        return new TlhashIterator(db_.get());
    }

    bool TlhashIterator::Valid() {
        return cursor_ < dentry_.size();
    }

    void TlhashIterator::SeekToFirst() {
        cursor_ = 0;
    }

    void TlhashIterator::SeekToLast() {
        cursor_ = dentry_.size() - 1;
    }

    void TlhashIterator::Seek(const leveldb::Slice &target) {
        db_->Scan(target.ToString(), dentry_);
    }

    void TlhashIterator::Next() {
        cursor_++;
    }

    void TlhashIterator::Prev() {
        cursor_--;
    }

    leveldb::Slice TlhashIterator::key() const {
        return {dentry_[cursor_].first};
    }

    leveldb::Slice TlhashIterator::value() const {
        return {dentry_[cursor_].second};
    }
}
