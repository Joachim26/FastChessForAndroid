#pragma once

#include <windows.h>
#include <array>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include <chess.hpp>

#include <affinity/cpu_info.hpp>

namespace affinity {

inline CpuInfo getCpuInfo() noexcept(false) {
    DWORD byte_length = 0;

    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &byte_length);

    std::unique_ptr<char[]> buffer(new char[byte_length]);

    if (!GetLogicalProcessorInformationEx(RelationProcessorCore,
                                          PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX(buffer.get()),
                                          &byte_length)) {
        std::cerr << "GetLogicalProcessorInformationEx failed." << std::endl;
    }

    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX ptr =
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX(buffer.get());

    DWORD offset = 0;

    /*
     SMT Support = yes
     GroupCount = 1
     Group #0 = 0000000000000000000000000000000000000000000000000000000000000011

     SMT Support = yes
     GroupCount = 1
     Group #0 = 0000000000000000000000000000000000000000000000000000000000001100

     SMT Support = yes
     GroupCount = 1
     Group[0] = 0000000000000000000000000000000000000000000000000000000000110000
     */

    // @todo Make this work for multiple physical cpu's (multiple sockets and > 64 threads)
    int idx         = 0;
    int physical_id = 0;

    CpuInfo cpu_info;
    cpu_info.physical_cpus[physical_id].physical_id = physical_id;

    while (offset < byte_length) {
        if (ptr->Relationship == RelationProcessorCore) {
            // If the PROCESSOR_RELATIONSHIP structure represents a processor core, the GroupCount
            // member is always 1.
            ULONG_PTR mask = ptr->Processor.GroupMask[0].Mask;

            while (mask) {
                const int processor = chess::builtin::poplsb(mask);
                // proper way to get this idx?
                cpu_info.physical_cpus[physical_id].cores[idx].processors.emplace_back(processor);
            }

            idx++;
        }

        offset += ptr->Size;

        ptr = PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX(buffer.get() + offset);
    }

    return cpu_info;
}

}  // namespace affinity