//
// Created by zzyyyww on 2021/8/16.
//

#include <libpmem.h>
#include "utree_wrapper.h"
#include "lib/leveldb/util/coding.h"

using namespace leveldb;

namespace tablefs {
    const std::string NVM_PATH("./pmem");

    const size_t PMEM_SIZE = 10 * 1024 * 1024 * 1024UL;

    void DEBUG_KEY(int64_t key, std::string func){
        printf("[%s]: ", func.c_str());
        size_t* s = (size_t*)key;
        printf("size = %llu\n", *s);
        char* raw_key = (char*)key + sizeof(size_t);
        for (size_t i = 0; i < *s; ++i) {
            printf("%d ", raw_key[i]);
        }
        printf("\n");
    }

    char* persist_slice(const leveldb::Slice &key){
        //size_t key_size_t = key.size();
        /*char *tmp = (char*)(&key_size_t);
        for (int i = 0; i < sizeof(size_t); ++i) {
            printf("%x ", tmp[i]);
        }
        printf("data size = [%d], size = [%lu , %x] \n", sizeof(size_t), key_size_t, key_size_t);
        printf("%lu\n", *(size_t*)(tmp));*/
        char* nvm_key = (char*)alloc(key.size() + sizeof(key.size()));
        //printf("%p\n", nvm_key);
        if (nvm_key == nullptr) printf("null alloc\n");
        size_t key_size = key.size();
        /*
         * Attention: Here is LITTLE ENDIAN !!
         * */
        memcpy(nvm_key, (char*)(&key_size), sizeof(size_t));
        pmem_persist(nvm_key, sizeof(size_t));
        pmem_memcpy_persist(nvm_key + sizeof(size_t), key.data(), key.size());
        //printf("%p\n", nvm_key);
        //DEBUG_KEY((int64_t)nvm_key, "persist_slice");
        return nvm_key;
    }

class uTreeInserter: public WriteBatch::Handler {
public:
    void Put(const Slice& key, const Slice& value) override {
        char* nvm_key = persist_slice(key);
        char* nvm_value = persist_slice(value);
        utree_->insert((int64_t)(nvm_key), nvm_value);
    }
    void Delete(const Slice& key) override {
        char lookup_key[key.size() + sizeof(size_t)];
        size_t key_size = key.size();
        memcpy(lookup_key, (char*)(&key_size), sizeof(size_t));
        memcpy(lookup_key + sizeof(size_t), key.data(), key.size());
        utree_->remove((int64_t)(lookup_key));
    }

    btree* utree_;
};

    int uTreeWrapper::Init() {
#ifndef USE_PMDK
        uint64_t mapped_len = 0;
        int is_pmem = 0;
        void* pmem = pmem_map_file(NVM_PATH.c_str(), PMEM_SIZE, PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
        if (pmem == nullptr) {
            fprintf(stderr, "Map file failed [%s]\n", strerror(errno));
            return -1;
        }
        ::start_addr = (char*)pmem;
        ::curr_addr = ::start_addr;
        printf("start_addr=%p, end_addr=%p\n", start_addr,
               start_addr + PMEM_SIZE);
#endif
        utree_ = new btree();
        return 0;
    }

    void uTreeWrapper::Cleanup() {
        delete utree_;
        utree_ = nullptr;
        pmem_unmap((void*)(::start_addr), PMEM_SIZE);
    }

    int uTreeWrapper::Put(const leveldb::Slice &key, const leveldb::Slice &value) {
        printf("put [");
        for (int i = 0; i < key.size(); ++i) {
            printf("%02X ", key.data()[i]);
        }
        printf("]\n");
        char* nvm_key = persist_slice(key);
        char* nvm_value = persist_slice(value);
        DEBUG_KEY((int64_t)nvm_key, "Wrapper::put");
        utree_->insert((int64_t)(nvm_key), nvm_value);
        std:;string v;
        int res = Get(key, v);
        switch (res) {
            case 0:
                printf("not found just insert\n");
                break;
            case 1:
                printf("found\n");
                break;
            default: break;
        }
        return 0;
    }

    int uTreeWrapper::Get(const leveldb::Slice &key, std::string &result) {
        //printf("get [%s]\n", key.ToString().c_str());
        for (int i = 0; i < key.size(); ++i) {
            printf("%d ", key.data()[i]);
        }
        printf("\n");
        char lookup_key[key.size() + sizeof(size_t)];
        size_t key_size = key.size();
        memcpy(lookup_key, (char*)(&key_size), sizeof(size_t));
        memcpy(lookup_key + sizeof(size_t), key.data(), key.size());
        char* res = utree_->search((int64_t)(lookup_key));
        if (res != nullptr) {
            size_t* s = (size_t*)(res);
            printf("%d\n", *s);
            printf("res [%s]\n", Slice(res + sizeof(size_t), *s).ToString().c_str());
            printf("found[%s]\n", result.c_str());
            return 1;
        } else {
            printf("not found[");
            for (int i = 0; i < key.size(); ++i) {
                printf("%d ", key.data()[i]);
            }
            printf("]\n");
        }
        return 0;
    }

    int uTreeWrapper::Delete(const leveldb::Slice &key) {
        char lookup_key[key.size() + sizeof(size_t)];
        size_t key_size = key.size();
        memcpy(lookup_key, (char*)(&key_size), sizeof(size_t));
        memcpy(lookup_key + sizeof(size_t), key.data(), key.size());
        utree_->remove((int64_t)(lookup_key));
        return 0;
    }

    int uTreeWrapper::Write(leveldb::WriteBatch &batch) {
        uTreeInserter inserter;
        inserter.utree_ = utree_;
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
        return new uTreeIterator(utree_);
    }

    uTreeIterator::uTreeIterator(btree *utree): utree_(utree) {}

    bool uTreeIterator::Valid() {
        return (init && node_ != nullptr);
    }

    void uTreeIterator::SeekToFirst() {
        // not support
    }

    void uTreeIterator::SeekToLast() {
        // not support
    }

    void uTreeIterator::Seek(const leveldb::Slice &target) {
        init = true;
        char lookup_key[target.size() + sizeof(size_t)];
        size_t key_size = target.size();
        memcpy(lookup_key, (char*)(&key_size), sizeof(size_t));
        memcpy(lookup_key + sizeof(size_t), target.data(), target.size());
        char* prev;
        bool f = false;
        char *ptr = utree_->btree_search_pred((int64_t)lookup_key, &f, &prev, false);
        node_ = (list_node_t *)ptr;
    }

    void uTreeIterator::Next() {
        node_ = node_->next;
    }

    void uTreeIterator::Prev() {
        // not supoprted
    }

    leveldb::Slice uTreeIterator::key() const {
        char* raw_data = (char*)(node_->key);
        size_t* key_size = (size_t*)(raw_data);
        return Slice(raw_data + sizeof(size_t), *key_size);
    }

    leveldb::Slice uTreeIterator::value() const {
        char* raw_data = (char*)(node_->ptr);
        size_t* value_size = (size_t*)(raw_data);
        return Slice(raw_data + sizeof(size_t), *value_size);
    }


}
