//
// Created by zzyyyww on 21.8.13.
//

#include <fstream>
#include <iostream>
#include "statistics.h"

namespace tablefs{

Statistics::Statistics(std::string res_path): path_(std::move(res_path)) {}

void Statistics::RecordKVOperations(OperationType op) {
    switch (op) {
        case PUT:
            op_.num_put += 1;
            break;
        case GET:
            op_.num_get += 1;
            break;
        case DELETE:
            op_.num_delete += 1;
            break;
        case PSCAN:
            op_.num_pscan += 1;
            break;
        default:
            break;
    }
}

void Statistics::Report() {
    std::ofstream output;
    output.open(path_, std::ios::out | std::ios::trunc);
    output  << "op num:\n"
            << "Put\tGet\tDelete\tPScan\n"
            << op_.num_put << "\t" << op_.num_get << "\t" << op_.num_delete << "\t" << op_.num_pscan << "\n";
    output.close();
}

}
