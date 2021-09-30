//
// Created by zyw on 2021/9/28.
//

#include "hikv_adaptor.h"
#include "fs/tfs_inode.h"

namespace tablefs {
    using open_hikv::ErrorCode;
    int HikvWrapper::Init() {
        open_hikv::HiKVConfig config {
                .pm_path_ = "/mnt/pmem/hikv/",
                .store_size = 1024 * 1024 * 1024,
                .shard_size = 625000 * 16 * 4,
                .shard_num = 256,
                .message_queue_shard_num = 1,
                .log_path_ = "/mnt/pmem/hikv/",
                .log_size_ = 60UL * 1024 * 1024 * 1024,
                .cceh_path_ = "/mnt/pmem/hikv/",
                .cceh_size_ = 40UL * 1024 * 1024 * 1024,
        };

        open_hikv::OpenHiKV::OpenPlainVanillaOpenHiKV(&db_, config);

        return 1;
    }

    void HikvWrapper::Cleanup() {
        db_ = nullptr;
    }

    int HikvWrapper::Put(const leveldb::Slice &key, const leveldb::Slice &value) {
        if (logon) {
            if (logs_ != nullptr) {
                const int *data = (const int *) key.ToString().data();
                logs_->LogMsg("Put %d %x\n", data[0], data[1]);
            }
        }

        ErrorCode res = db_->Set(HikvSlice(key.data(), key.size()),
                                            HikvSlice(value.data(), value.size()));
        if (res == ErrorCode::kOk) {
            return 0;
        } else {
            if (logon) {
                if (logs_ != NULL) {
                    logs_->LogMsg("Put Error\n");
                }
            }
            return -1;
        }
    }

    int HikvWrapper::Get(const leveldb::Slice &key, std::string &result) {
        HikvSlice value;
        ErrorCode res = db_->Get(HikvSlice(key.data(), key.size()), &value);
        if (logon) {
            if (logs_ != NULL) {
                const int *data = (const int *) key.ToString().data();
                logs_->LogMsg("read %d %x\n", data[0], data[1]);
            }
        }
        if (res == ErrorCode::kOk) {
            result = value.ToString();
            return 1;
        } else if (res == ErrorCode::kNotFound) {
            return 0;
        } else {
            return -1;
        }
    }

    int HikvWrapper::Delete(const leveldb::Slice &key) {
        if (logon) {
            if (logs_ != NULL) {
                const int *data = (const int *) key.ToString().data();
                logs_->LogMsg("Delete %d %x\n", data[0], data[1]);
            }
        }
        db_->Del(HikvSlice(key.data(), key.size()));
        return 0;
    }

class HikvInserter : public leveldb::WriteBatch::Handler {
    public:
        HikvInserter(std::vector<std::tuple<bool, HikvSlice, HikvSlice>>* kvp): kvp_(kvp) {
        }

        ~HikvInserter() =default;

    void Put(const leveldb::Slice &key, const leveldb::Slice &value) override {
        kvp_->emplace_back(true, HikvSlice(key.data(), key.size()),
                           HikvSlice(value.data(), value.size()));
    }

    void Delete(const leveldb::Slice &key) override {
        kvp_->emplace_back(false, HikvSlice(key.data(), key.size()),
                           HikvSlice());
    }
    private:
        std::vector<std::tuple<bool, HikvSlice, HikvSlice>>* kvp_;
    };

    int HikvWrapper::Write(leveldb::WriteBatch &batch) {
        std::vector<std::tuple<bool, HikvSlice, HikvSlice>> raw;
        HikvInserter handle(&raw);
        batch.Iterate(&handle);
        for (auto item : raw) {
            if (std::get<0>(item)) {
                db_->Set(std::get<1>(item), std::get<2>(item));
            } else {
                db_->Del(std::get<1>(item));
            }
        }
        return 1;
    }

    int HikvWrapper::Sync() {
        return 1;
    }

    bool HikvWrapper::GetStat(std::string stat, std::string *value) {
        return true;
    }

    bool HikvWrapper::GetMetric(std::string *value) {
        return true;
    }

    KvIterator *HikvWrapper::NewIterator() {
        return new HikvIterator(db_.get());
    }

    HikvIterator::HikvIterator(open_hikv::OpenHiKV* db): db_(db)  {
    }

    void HikvIterator::Seek(const leveldb::Slice &target) {
        tfs_meta_key_t *raw_key = (tfs_meta_key_t*)(target.data());
        std::string prefix((char*)(&raw_key->inode_id), 8);
        std::vector<KVPair> res;
        db_->Scan(prefix, [&](const HikvSlice& k, const HikvSlice & v){
            if (k.ToString().find(prefix) != std::string::npos){
                res.emplace_back(KVPair(k, v));
                return true;
            } else {
                return false;
            }
        });
        dentry_.swap(res);
        cursor = 0;
    }

    void HikvIterator::SeekToFirst() {
        cursor = 0;
    }

    void HikvIterator::SeekToLast() {
        cursor = dentry_.size() - 1;
    }

    void HikvIterator::Next() {
        cursor++;
    }

    void HikvIterator::Prev() {
        cursor--;
    }

    bool HikvIterator::Valid() {
        return cursor < dentry_.size();
    }

    leveldb::Slice HikvIterator::key() const {
        //return leveldb::Slice(iter->key.data(), iter->key.size());
        return leveldb::Slice(dentry_[cursor].first.data(), dentry_[cursor].first.size());
    }

    leveldb::Slice HikvIterator::value() const {
        //return leveldb::Slice(iter->value.data(), iter->value.size());
        return leveldb::Slice(dentry_[cursor].second.data(), dentry_[cursor].second.size());
    }
    
}