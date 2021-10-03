//
// Created by zyw on 2021/10/3.
//

#include "roart_wrapper.h"

namespace tablefs {
    int RoartWrappr::Init() {
        db_ = std::make_unique<PART_ns::Tree>();
        return 1;
    }

    void RoartWrappr::Cleanup() {

    }

    int RoartWrappr::Put(const leveldb::Slice &key, const leveldb::Slice &value) {
        bool res = db_->Put(key.ToString(), value.ToString());
        if (res) {
            /*{
                std::string check;
                bool check_res = db_->Get(key.ToString(), &check);
                assert(value == leveldb::Slice(check));
                printf("check put: %d\n", check_res);
            }*/
            return 0;
        } else {
            return -1;
        }
    }

    int RoartWrappr::Get(const leveldb::Slice &key, std::string &result) {
        std::string value;
        bool res = db_->Get(key.ToString(), &result);
        if (res) {
            return 1;
        } else {
            return 0;
        }
    }

    int RoartWrappr::Delete(const leveldb::Slice &key) {
        db_->Delete(key.ToString());
        return 0;
    }

    class RoartInserter : public leveldb::WriteBatch::Handler {
    public:
        RoartInserter(PART_ns::Tree *tree) : tree_(tree) {}

        void Put(const leveldb::Slice &key, const leveldb::Slice &value) override {
            tree_->Put(key.ToString(), value.ToString());
        }

        void Delete(const leveldb::Slice &key) override {
            tree_->Delete(key.ToString());
        }

    private:
        PART_ns::Tree *tree_;
    };

    int RoartWrappr::Write(leveldb::WriteBatch &batch) {
        RoartInserter handle(db_.get());
        batch.Iterate(&handle);
        return 0;
    }

    bool RoartWrappr::GetMetric(std::string *value) {
        return true;
    }

    bool RoartWrappr::GetStat(std::string stat, std::string *value) {
        return true;
    }

    int RoartWrappr::Sync() {
        return 0;
    }

    KvIterator *RoartWrappr::NewIterator() {
        return new RoartIterator(db_.get());
    }

    bool RoartIterator::Valid() {
        return cursor_ < dentry_.size();
    }

    void RoartIterator::SeekToFirst() {
        cursor_ = 0;
    }

    void RoartIterator::SeekToLast() {
        cursor_ = dentry_.size() - 1;
    }

    void RoartIterator::Seek(const leveldb::Slice &target) {
        db_->Scan(target.ToString(), dentry_);
    }

    void RoartIterator::Next() {
        cursor_++;
    }

    void RoartIterator::Prev() {
        cursor_--;
    }

    leveldb::Slice RoartIterator::key() const {
        return {dentry_[cursor_].first};
    }

    leveldb::Slice RoartIterator::value() const {
        return {dentry_[cursor_].second};
    }


}
