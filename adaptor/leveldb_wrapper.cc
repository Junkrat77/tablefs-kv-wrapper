//
// Created by zzyyyww on 2021/8/15.
//

#include "leveldb_wrapper.h"
#include "leveldb/db.h"
#include "leveldb/cache.h"
#include "leveldb/write_batch.h"
#include "leveldb/status.h"
#include "leveldb/filter_policy.h"

using namespace leveldb;

namespace tablefs{
    int LeveldbWrapper::Init() {
        assert(db_ == nullptr);
        int F_cache_size = p_.getPropertyInt("leveldb.cache.size", 16<<20);
        cache_ = (F_cache_size >= 0) ? leveldb::NewLRUCache(F_cache_size) : NULL;
        db_name = p_.getProperty("leveldb.db", "/tmp/db");
        Options options;
        options.create_if_missing =
                p_.getPropertyBool("leveldb.create.if.missing.db", true);
        options.block_cache = cache_;
        options.block_size =
                p_.getPropertyInt("leveldb_block_size", 4 << 10);
        options.write_buffer_size =
                p_.getPropertyInt("leveldb_write_buffer_size", 16<<20);
        options.max_open_files =
                p_.getPropertyInt("leveldb.max.open.files", 800);
        options.filter_policy = NewBloomFilterPolicy(12);
        options.limit_sst_file_size =
                p_.getPropertyInt("leveldb_limit_sst_file_size", 2097152);
        options.limit_level_zero=
                p_.getPropertyInt("leveldb_limit_level_zero", 10);
        options.factor_level_files=
                p_.getPropertyInt("leveldb_factor_level_files", 10);

        if (logs_ != nullptr) {
            logs_->LogMsg("limit level: %d\n", options.limit_sst_file_size);
            logs_->LogMsg("limit level0: %d\n", options.limit_level_zero);
            logs_->LogMsg("factor level files: %lf\n", options.factor_level_files);
        }

        writeahead = p_.getPropertyBool("leveldb.writeahead", true);
        logon = p_.getPropertyBool("leveldb.logon", false);
        sync_time_limit = p_.getPropertyInt("leveldb.sync.time.limit", 5);
        sync_size_limit = p_.getPropertyInt("leveldb.sync.size.limit", -1);
        last_sync_time = time(nullptr);
        async_data_size = 0;
        Status s = DB::Open(options, db_name, &db_);
        if (!s.ok()) {
            return -1;
        } else {
            return 0;
        }
    }

    void LeveldbWrapper::Cleanup() {
        statistics.Report();
        delete db_;
        delete cache_;
        db_ = nullptr;
    }

    int LeveldbWrapper::Put(const leveldb::Slice &key, const leveldb::Slice &value) {
        statistics.RecordKVOperations(PUT);
        if (logon) {
            if (logs_ != nullptr) {
                const int *data = (const int *) key.ToString().data();
                logs_->LogMsg("Put %d %x\n", data[0], data[1]);
            }
        }
        WriteOptions write_options;
        if (sync_size_limit > 0) {
            async_data_size += key.size() + value.size();
            if (async_data_size > sync_size_limit) {
                write_options.sync = true;
                async_data_size = 0;
            }
        } else
            if (sync_time_limit > 0) {
                time_t now = time(nullptr);
                if (now - last_sync_time > sync_time_limit) {
                    write_options.sync = true;
                    last_sync_time = now;
                }
            }
            write_options.writeahead = writeahead;
            leveldb::Status status = db_->Put(write_options, key, value);
            if (status.ok()) {
                return 0;
            } else {
                if (logon) {
                    if (logs_ != NULL) {
                        logs_->LogMsg("Put Error: %s\n", status.ToString().c_str());
                    }
                }
                return -1;
            }
    }

    int LeveldbWrapper::Get(const leveldb::Slice &key, std::string &result) {
        statistics.RecordKVOperations(GET);
        ReadOptions options;
        Status s = db_->Get(options, key, &result);
        if (logon) {
            if (logs_ != NULL) {
                const int *data = (const int *) key.ToString().data();
                logs_->LogMsg("read %s %d %x\n", db_name.c_str(), data[0], data[1]);
            }
        }
        if (!s.ok()) {
            result = s.ToString();
            return -1;
        } else {
            return (s.IsNotFound()) ? 0 : 1;
        }
    }

    int LeveldbWrapper::Delete(const leveldb::Slice &key) {
        statistics.RecordKVOperations(DELETE);
        if (logon) {
            if (logs_ != NULL) {
                const int *data = (const int *) key.ToString().data();
                logs_->LogMsg("Delete %d %x\n", data[0], data[1]);
            }
        }
        WriteOptions write_options;
        db_->Delete(write_options, key);
        return 0;
    }

    int LeveldbWrapper::Write(leveldb::WriteBatch &batch) {
        statistics.RecordKVOperations(PUT);
        statistics.RecordKVOperations(DELETE);
        WriteOptions write_options;
        Status s = db_->Write(write_options, &batch);
        if (!s.ok()) {
            return -1;
        }
        return 0;
    }

    int LeveldbWrapper::Sync() {
        WriteOptions write_options;
        write_options.sync = true;
        statistics.RecordKVOperations(PUT);
        leveldb::Status status = db_->Put(write_options, "sync", "");
        if (status.ok()) {
            return 0;
        } else {
            return -1;
        }
    }

    KvIterator *LeveldbWrapper::NewIterator() {
        statistics.RecordKVOperations(PSCAN);
        ReadOptions read_options;
        if (logon) {
            if (logs_ != NULL)
                logs_->LogMsg("iterator\n");
        }
        Iterator* iter = db_->NewIterator(read_options);
        return new LeveldbIterator(iter);
    }

    bool LeveldbWrapper::GetStat(std::string stat, std::string *value) {
        return db_->GetProperty(stat, value);
    }

    bool LeveldbWrapper::GetMetric(std::string *value) {
        return db_->GetProperty(Slice("leveldb.stats"), value);
    }

    LeveldbIterator::~LeveldbIterator() {
        delete iter_;
    }

    bool LeveldbIterator::Valid() {
        return iter_->Valid();
    }

    void LeveldbIterator::SeekToFirst() {
        iter_->SeekToFirst();
    }

    void LeveldbIterator::SeekToLast() {
        iter_->SeekToLast();
    }

    void LeveldbIterator::Seek(const leveldb::Slice &target) {
        iter_->Seek(target);
    }

    void LeveldbIterator::Next() {
        iter_->Next();
    }

    void LeveldbIterator::Prev() {
        iter_->Next();
    }

    leveldb::Slice LeveldbIterator::key() const {
        return iter_->key();
    }

    leveldb::Slice LeveldbIterator::value() const {
        return iter_->value();
    }

}