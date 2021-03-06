// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef DORIS_BE_SRC_UTIL_UID_UTIL_H
#define DORIS_BE_SRC_UTIL_UID_UTIL_H

#include <ostream>

#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "gen_cpp/Types_types.h"  // for TUniqueId
#include "gen_cpp/types.pb.h"  // for PUniqueId
// #include "util/debug_util.h"
#include "util/hash_util.hpp"

namespace doris {

// convert int to a hex format string, buf must enough to hold coverted hex string
template<typename T>
inline void to_hex(T val, char* buf) {
    static const char* digits = "0123456789abcdef";
    for (int i = 0; i < 2 * sizeof(T); ++i) {
        buf[2 * sizeof(T) - 1 - i] = digits[val & 0x0F];
        val >>= 4;
    }
}

struct UniqueId {
    int64_t hi;
    int64_t lo;

    UniqueId() {
        auto uuid = boost::uuids::basic_random_generator<boost::mt19937>()();
        memcpy(&hi, uuid.data, sizeof(int64_t));
        memcpy(&lo, uuid.data + sizeof(int64_t), sizeof(int64_t));
    }
    UniqueId(int64_t hi_, int64_t lo_) : hi(hi_), lo(lo_) { }
    UniqueId(const TUniqueId& tuid) : hi(tuid.hi), lo(tuid.lo) { }
    UniqueId(const PUniqueId& puid) : hi(puid.hi()), lo(puid.lo()) { }
    ~UniqueId() noexcept { }

    std::string to_string() const {
        char buf[33];
        to_hex(hi, buf);
        buf[16] = ':';
        to_hex(lo, buf + 17);
        return {buf, 33};
    }

    size_t hash(size_t seed = 0) const {
        return doris::HashUtil::hash(this, sizeof(*this), seed);
    }

    bool operator==(const UniqueId& rhs) const {
        return hi == rhs.hi && lo == rhs.lo;
    }

    TUniqueId to_thrift() const {
        TUniqueId tid;
        tid.__set_hi(hi);
        tid.__set_lo(lo);
        return tid;
    }

    PUniqueId to_proto() const {
        PUniqueId pid;
        pid.set_hi(hi);
        pid.set_lo(lo);
        return pid;
    }
};

// This function must be called 'hash_value' to be picked up by boost.
inline std::size_t hash_value(const doris::TUniqueId& id) {
    std::size_t seed = 0;
    boost::hash_combine(seed, id.lo);
    boost::hash_combine(seed, id.hi);
    return seed;
}

/// generates a 16 byte UUID
inline std::string generate_uuid_string() {
    return boost::uuids::to_string(boost::uuids::basic_random_generator<boost::mt19937>()());
}

/// generates a 16 byte UUID
inline TUniqueId generate_uuid() {
    auto uuid = boost::uuids::basic_random_generator<boost::mt19937>()();
    TUniqueId uid;
    memcpy(&uid.hi, uuid.data, sizeof(int64_t));
    memcpy(&uid.lo, uuid.data + sizeof(int64_t), sizeof(int64_t));
    return uid;
}

std::ostream& operator<<(std::ostream& os, const UniqueId& uid);

std::string print_id(const TUniqueId& id);
std::string print_id(const PUniqueId& id);

// Parse 's' into a TUniqueId object.  The format of s needs to be the output format
// from PrintId.  (<hi_part>:<low_part>)
// Returns true if parse succeeded.
bool parse_id(const std::string& s, TUniqueId* id);


} // namespace doris

namespace std {

template<>
struct hash<doris::UniqueId> {
    size_t operator()(const doris::UniqueId& uid) const {
        return uid.hash();
    }
};

}

#endif // DORIS_BE_SRC_UTIL_UID_UTIL_H

