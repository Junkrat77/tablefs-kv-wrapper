//
// Created by zzyyyww on 21.8.13.
//

#ifndef TABLEFS_KV_WRAPPER_H
#define TABLEFS_KV_WRAPPER_H

#include "leveldb/slice.h"
#include "leveldb/write_batch.h"

#include "statistics.h"
#include "util/properties.h"
#include "util/tfs_logging.h"

namespace tablefs {

class KvIterator;

class KvWrapper {
public:
    KvWrapper() = default;
    virtual ~KvWrapper() = default;

    void SetProperties(const Properties &p) {
        p_ = p;
    };

    void SetLogging(Logging *logs) {
        logs_ = logs;
    };

    /*
     * Init the KVDB
     * */
    virtual int Init() = 0;

    /*
     * Destroy the KVDB
     * */
    virtual void Cleanup() = 0;

    virtual int Put(const leveldb::Slice &key,
                    const leveldb::Slice &value) = 0;

    virtual int Get(const leveldb::Slice &key,
                    std::string &result) = 0;

    virtual int Delete(const leveldb::Slice &key) = 0;

    virtual int Write(leveldb::WriteBatch &batch) = 0;

    virtual int Sync() = 0;

    virtual KvIterator* NewIterator() = 0;

    void Report() {statistics.Report();};

    virtual bool GetStat(std::string stat, std::string* value) = 0;

    virtual bool GetMetric(std::string* value) = 0;

private:
    Statistics statistics;
    Logging* logs_;
    Properties p_;
};

class KvIterator {
public:
    KvIterator() = default;
    virtual ~KvIterator() = default;

    virtual bool Valid() = 0;

    virtual void SeekToFirst() = 0;

    virtual void SeekToLast() = 0;

    virtual void Seek(const leveldb::Slice &target) = 0;

    virtual void Next() = 0;

    virtual void Prev() = 0;


    virtual leveldb::Slice key() const = 0;

    virtual leveldb::Slice value() const = 0;


};

}

#endif //TABLEFS_KV_WRAPPER_H
