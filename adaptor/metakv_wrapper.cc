//
// Created by zyw on 2021/9/26.
//

#include "metakv_wrapper.h"
#include "src/Options.h"

namespace tablefs{

    int MetaKVWrapper::Init() {
        Options options;
        options.cceh_file_size = 100UL * 1024 * 1024 * 1024;
        options.data_file_size = 80UL * 1024 * 1024 * 1024;
        db_ = MetaDB{};

        path_ = p_.getProperty("leveldb.db", "/mnt/pmem/tablefs-metakv");
        db_.Open(options, path_);

        logon = p_.getPropertyBool("leveldb.logon", false);
        sync_time_limit = p_.getPropertyInt("leveldb.sync.time.limit", 5);
        sync_size_limit = p_.getPropertyInt("leveldb.sync.size.limit", -1);
        last_sync_time = time(nullptr);
        async_data_size = 0;
        return 1;
    }

    void MetaKVWrapper::Cleanup() {

    }

    int MetaKVWrapper::Put(const leveldb::Slice &key, const leveldb::Slice &value) {
        if (logon) {
            if (logs_ != nullptr) {
                const int *data = (const int *) key.ToString().data();
                logs_->LogMsg("Put %d %x\n", data[0], data[1]);
            }
        }
        tfsKey metakv_key(key);
        tfsValue metakv_value(value.ToString());

        bool res = db_.Put(metakv_key, metakv_value);
        if (res) {
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

    int MetaKVWrapper::Get(const leveldb::Slice &key, std::string &result) {
        tfsKey metakv_key(key);
        tfsValue metakv_value;
        bool res = db_.Get(metakv_key, metakv_value);
        if (logon) {
            if (logs_ != NULL) {
                const int *data = (const int *) key.ToString().data();
                logs_->LogMsg("read %d %x\n", data[0], data[1]);
            }
        }
        Slice val;
        metakv_value.GetValue(&val);
        result = val.ToString();
        if (!res) {
            return 0;
        } else {
            return 1;
        }
    }

    int MetaKVWrapper::Delete(const leveldb::Slice &key) {
        if (logon) {
            if (logs_ != NULL) {
                const int *data = (const int *) key.ToString().data();
                logs_->LogMsg("Delete %d %x\n", data[0], data[1]);
            }
        }
        tfsKey metakv_key(key);
        db_.Delete(metakv_key);
        return 0;
    }

    class MetaDBInserter : public leveldb::WriteBatch::Handler {
    public:
        MetaDBInserter(MetaDB *db) : db_(db) {};

        ~MetaDBInserter() {};

        void Put(const leveldb::Slice &key, const leveldb::Slice &value) override {
            tfsKey metakv_key(key);
            tfsValue metakv_value(value.ToString());
            db_->Put(metakv_key, metakv_value);
        };

        void Delete(const leveldb::Slice &key) override {
            tfsKey metakv_key(key);
            db_->Delete(metakv_key);
        };
    private:
        MetaDB* db_;
    };

    int MetaKVWrapper::Write(leveldb::WriteBatch &batch) {
        MetaDBInserter inserter(&db_);
        batch.Iterate(&inserter);
    }

    int MetaKVWrapper::Sync() {
        return 1;
    }

    KvIterator *MetaKVWrapper::NewIterator() {
        return new MetaKVIterator(&db_);
    }

    bool MetaKVWrapper::GetStat(std::string stat, std::string *value) {
        return true;
    }

    bool MetaKVWrapper::GetMetric(std::string *value) {
        return true;
    }

    void MetaKVIterator::Seek(const leveldb::Slice &target) {
        tfsKey metakv_key(target);
        Slice prefix;
        metakv_key.GetPrefix(&prefix);
        db_->Scan(prefix, dentry);
    }

    void MetaKVIterator::SeekToFirst() {
        //iter = dentry.begin();
        cursor = 0;
    }

    void MetaKVIterator::SeekToLast() {
        //iter = dentry.end();
        cursor = dentry.size() - 1;
    }

    void MetaKVIterator::Next() {
        //iter++;
        cursor++;
    }

    void MetaKVIterator::Prev() {
        //iter--;
        cursor--;
    }

    bool MetaKVIterator::Valid() {
        //return iter != dentry.end();
        return cursor < dentry.size();
    }

    leveldb::Slice MetaKVIterator::key() const {
        //return leveldb::Slice(iter->key.data(), iter->key.size());
        return leveldb::Slice(dentry[cursor].key.data(), dentry[cursor].key.size());
    }

    leveldb::Slice MetaKVIterator::value() const {
        //return leveldb::Slice(iter->value.data(), iter->value.size());
        return leveldb::Slice(dentry[cursor].value.data(), dentry[cursor].value.size());
    }
}