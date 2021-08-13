//
// Created by zzyyyww on 21.8.13.
//

#ifndef TABLEFS_STATISTICS_H
#define TABLEFS_STATISTICS_H

#include <cstdint>

namespace tablefs{

enum OperationType {
    PUT,
    GET,
    DELETE,
    PSCAN,
};

class Statistics {
public:
    Statistics() = default;
    Statistics(std::string res_path);
    ~Statistics() = default;

    /*
     * Add metrics
     * */
    void RecordKVOperations(OperationType op);

    void Report();

private:
    std::string path_{"./statistics"};
    struct {
        uint64_t num_put {0};
        uint64_t num_get {0};
        uint64_t num_delete {0};
        uint64_t num_pscan {0};
    } op_;
};

}

#endif //TABLEFS_STATISTICS_H
