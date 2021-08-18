//
// Created by zzyyyww on 2021/8/17.
//

#include <string>
#include "utree_wrapper.h"
#include "include/leveldb/slice.h"

int main() {
    auto wrapper = new tablefs::uTreeWrapper();
    wrapper->Init();
    for (int i = 0; i < 10; i++) {
        std::string key("foo" + std::to_string(i));
        std::string value("bar" + std::to_string(i));
        int s = wrapper->Put(leveldb::Slice(key), leveldb::Slice(value));
        printf("put key [%s] value [%s]\n", key.c_str(), value.c_str());
    }

    for (int i = 0; i < 10; i++) {
        std::string lookup_key("foo" + std::to_string(i));
        std::string res;
        int s = wrapper->Get(leveldb::Slice(lookup_key), res);
        switch (s) {
            case 0:
                printf("not found key [%s]\n", lookup_key.c_str());
                break;
            case 1:
                printf("found key [%s] value [%s]\n", lookup_key.c_str(), res.c_str());
                break;
            default:
                printf("unknow return num\n");
                break;
        }

    }

    for (int i = 0; i < 10; i++) {
        std::string key("foo" + std::to_string(i));
        std::string value("tar" + std::to_string(i));
        int s = wrapper->Put(leveldb::Slice(key), leveldb::Slice(value));
        printf("update key [%s] value [%s]\n", key.c_str(), value.c_str());
    }

    for (int i = 0; i < 10; i++) {
        std::string lookup_key("foo" + std::to_string(i));
        std::string res;
        int s = wrapper->Get(leveldb::Slice(lookup_key), res);
        switch (s) {
            case 0:
                printf("not found key [%s]\n", lookup_key.c_str());
                break;
                case 1:
                    printf("found key [%s] value [%s]\n", lookup_key.c_str(), res.c_str());
                    break;
                    default:
                        printf("unknow return num\n");
                        break;
        }

    }

    std::string lookup_key("foo" + std::to_string(0));
    auto iter = wrapper->NewIterator();
    iter->Seek(leveldb::Slice(lookup_key));
    for (; iter->Valid(); iter->Next()) {
        printf("iter key [%s] value [%s]\n", iter->key().ToString().c_str(), iter->value().ToString().c_str());
    }

    return 0;
}
