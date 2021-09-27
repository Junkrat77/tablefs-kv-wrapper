//
// Created by zyw on 2021/9/26.
//

#include <rocksdb/table.h>
#include "rocksdb_wrapper.h"

#include "rocksdb/db.h"
#include "rocksdb/cache.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/status.h"
#include "rocksdb/filter_policy.h"

//using namespace rocksdb;
using rocksdb::Status;
using rocksdb::DB;

namespace tablefs {
    int RocksdbWrapper::Init() {
        assert(db_ == nullptr);
        int F_cache_size = p_.getPropertyInt("leveldb.cache.size", 16 << 20);
        cache_ = (F_cache_size >= 0) ? rocksdb::NewLRUCache(F_cache_size) : NULL;
        db_name = p_.getProperty("leveldb.db", "/mnt/pmem/tablefs-meta/rocksdb");

        rocksdb::Options options;
        options.create_if_missing =
                p_.getPropertyBool("leveldb.create.if.missing.db", true);
        //options.block_cache = cache_;

        rocksdb::BlockBasedTableOptions table_options;
        table_options.block_cache = cache_;
        table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(12));
        table_options.block_size = p_.getPropertyInt("leveldb_block_size", 4 << 10);
        options.table_factory.reset(NewBlockBasedTableFactory(table_options));

        options.write_buffer_size =
                p_.getPropertyInt("leveldb_write_buffer_size", 16 << 20);
        options.max_open_files =
                p_.getPropertyInt("leveldb.max.open.files", 800);
        options.max_bytes_for_level_base = p_.getPropertyInt("leveldb_limit_sst_file_size",
                                                             rocksdb::Options().max_bytes_for_level_base);
        options.max_bytes_for_level_multiplier = p_.getPropertyInt("leveldb_factor_level_files",
                                                                   rocksdb::Options().max_bytes_for_level_multiplier);
        options.level0_file_num_compaction_trigger = p_.getPropertyInt("leveldb_factor_level_files",
                                                                       rocksdb::Options().level0_file_num_compaction_trigger);

        options.max_background_jobs = 2;

        // DCPMM
        options.dcpmm_kvs_enable = true;
        options.dcpmm_kvs_mmapped_file_fullpath = "/mnt/pmem/tablefs-meta/pmem-rocksdb";
        options.dcpmm_kvs_mmapped_file_size = 40UL * 1024 * 1024 * 1024;
        options.recycle_dcpmm_sst = true;
        options.env = rocksdb::NewDCPMMEnv(rocksdb::DCPMMEnvOptions());

        if (logs_ != nullptr) {
            logs_->LogMsg("limit level: %d\n", options.max_bytes_for_level_base);
            logs_->LogMsg("limit level0: %d\n", options.level0_file_num_compaction_trigger);
            logs_->LogMsg("factor level files: %lf\n", options.max_bytes_for_level_multiplier);
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

    void RocksdbWrapper::Cleanup() {
        statistics.Report();
        db_->Close();
        delete db_;
        db_ = nullptr;
    }

    int RocksdbWrapper::Put(const leveldb::Slice &key, const leveldb::Slice &value) {
        //fprintf(stderr, "put\n");
        statistics.RecordKVOperations(PUT);
        if (logon) {
            if (logs_ != nullptr) {
                const int *data = (const int *) key.ToString().data();
                logs_->LogMsg("Put %d %x\n", data[0], data[1]);
            }
        }
        rocksdb::WriteOptions write_options;
        if (sync_size_limit > 0) {
            async_data_size += key.size() + value.size();
            if (async_data_size > sync_size_limit) {
                write_options.sync = true;
                async_data_size = 0;
            }
        } else if (sync_time_limit > 0) {
            time_t now = time(nullptr);
            if (now - last_sync_time > sync_time_limit) {
                write_options.sync = true;
                last_sync_time = now;
            }
        }

        rocksdb::WriteOptions opt;
        Status status = db_->Put(opt, rocksdb::Slice(key.data(), key.size()),
                                 rocksdb::Slice(value.data(), value.size()));
        /*rocksdb::Slice _key = rocksdb::Slice(key.data(), key.size());
        for (int i = 0; i < _key.size(); i++) {
            printf("%x ", _key.data()[i]);
        }
        printf(" inserted \n");*/

        /*rocksdb::Iterator* iter = db_->NewIterator(rocksdb::ReadOptions());
        for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
            rocksdb::Slice ikey = iter->key();
            for (int i = 0; i < ikey.size(); i++) {
                printf("%x ", ikey.data()[i]);
            }
            printf(" iter put \n");
        }*/

        /*std::string res;
        status = db_->Get(rocksdb::ReadOptions(), rocksdb::Slice(key.data(), key.size()), &res);
        printf("not found %s\n", status.IsNotFound() ? "!" : "no, founded");*/

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

    int RocksdbWrapper::Get(const leveldb::Slice &key, std::string &result) {
        //printf("db instance %p\n", this);
        //fprintf(stderr, "get\n");

        /*rocksdb::Iterator* iter = db_->NewIterator(rocksdb::ReadOptions());
        for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
            rocksdb::Slice ikey = iter->key();
            for (int i = 0; i < ikey.size(); i++) {
                printf("%x ", ikey.data()[i]);
            }
            printf(" iter \n");
        }*/

        statistics.RecordKVOperations(GET);
        rocksdb::ReadOptions options;
        options.skip_memtable = false;

        rocksdb::Slice _key = rocksdb::Slice(key.data(), key.size());
        /*for (int i = 0; i < _key.size(); i++) {
            printf("%x ", _key.data()[i]);
        }
        printf(" get \n");*/

        Status s = db_->Get(options, _key, &result);
        if (logon) {
            if (logs_ != NULL) {
                const int *data = (const int *) key.ToString().data();
                logs_->LogMsg("read %s %d %x\n", db_name.c_str(), data[0], data[1]);
            }
        }

        /*rocksdb::Iterator* iter = db_->NewIterator(rocksdb::ReadOptions());
        for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
            rocksdb::Slice ikey = iter->key();
            for (int i = 0; i < ikey.size(); i++) {
                printf("%x ", ikey.data()[i]);
            }
            printf(" iter \n");
        }*/

        if (s.IsNotFound()) {
            //printf("not found\n");
            return 0;
        } else if (!s.ok()) {
            fprintf(stderr, "end get failed %s\n", s.ToString().c_str());
            result = s.ToString();
            return -1;
        } else {
            return 1;
        }
    }

    int RocksdbWrapper::Delete(const leveldb::Slice &key) {
        //fprintf(stderr, "delete\n");
        statistics.RecordKVOperations(DELETE);
        if (logon) {
            if (logs_ != NULL) {
                const int *data = (const int *) key.ToString().data();
                logs_->LogMsg("Delete %d %x\n", data[0], data[1]);
            }
        }
        //rocksdb::WriteOptions write_options;
        db_->Delete(rocksdb::WriteOptions(), rocksdb::Slice(key.data(), key.size()));
        return 0;
    }

    class RocksDBWriteBachHandler : public leveldb::WriteBatch::Handler {
    public:
        RocksDBWriteBachHandler(rocksdb::WriteBatch *batch) : batch_(batch) {};

        ~RocksDBWriteBachHandler() {};

        void Put(const leveldb::Slice &key, const leveldb::Slice &value) override {
            batch_->Put(rocksdb::Slice(key.data(), key.size()), rocksdb::Slice(value.data(), value.size()));
        };

        void Delete(const leveldb::Slice &key) override {
            batch_->Delete(rocksdb::Slice(key.data(), key.size()));
        };
    private:
        rocksdb::WriteBatch *batch_;
    };

    int RocksdbWrapper::Write(leveldb::WriteBatch &batch) {
        //fprintf(stderr, "write\n");
        statistics.RecordKVOperations(PUT);
        statistics.RecordKVOperations(DELETE);
        rocksdb::WriteBatch rbatch;
        RocksDBWriteBachHandler handle(&rbatch);
        batch.Iterate(&handle);
        Status s = db_->Write(rocksdb::WriteOptions(), &rbatch);
        if (!s.ok()) {
            return -1;
        }
        return 0;
    }

    int RocksdbWrapper::Sync() {
        //fprintf(stderr, "sync\n");
        rocksdb::WriteOptions write_options;
        write_options.sync = true;
        statistics.RecordKVOperations(PUT);
        Status status = db_->Put(write_options, "sync", "");
        if (status.ok()) {
            return 0;
        } else {
            return -1;
        }
    }

    KvIterator *RocksdbWrapper::NewIterator() {
        statistics.RecordKVOperations(PSCAN);
        rocksdb::ReadOptions read_options;
        read_options.skip_memtable = false;
        if (logon) {
            if (logs_ != NULL)
                logs_->LogMsg("iterator\n");
        }
        rocksdb::Iterator *iter = db_->NewIterator(read_options);
        return new RocksdbIterator(iter);
    }

    bool RocksdbWrapper::GetStat(std::string stat, std::string *value) {
        return true;
    }

    bool RocksdbWrapper::GetMetric(std::string *value) {
        return true;
    }

    RocksdbIterator::~RocksdbIterator() {
        delete iter_;
    }

    bool RocksdbIterator::Valid() {
        return iter_->Valid();
    }

    void RocksdbIterator::SeekToFirst() {
        iter_->SeekToFirst();
    }

    void RocksdbIterator::SeekToLast() {
        iter_->SeekToLast();
    }

    void RocksdbIterator::Seek(const leveldb::Slice &target) {
        iter_->Seek(rocksdb::Slice(target.data(), target.size()));
    }

    void RocksdbIterator::Next() {
        iter_->Next();
    }

    void RocksdbIterator::Prev() {
        iter_->Next();
    }

    leveldb::Slice RocksdbIterator::key() const {
        return leveldb::Slice(iter_->key().data(), iter_->key().size());
    }

    leveldb::Slice RocksdbIterator::value() const {
        return leveldb::Slice(iter_->value().data(), iter_->value().size());
    }
}