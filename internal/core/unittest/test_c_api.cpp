#include <iostream>
#include <string>
#include <random>
#include <gtest/gtest.h>

#include "segcore/collection_c.h"
#include "segcore/segment_c.h"
#include "pb/service_msg.pb.h"

#include <chrono>
namespace chrono = std::chrono;

TEST(CApiTest, CollectionTest) {
    auto schema_tmp_conf = "";
    auto collection = NewCollection(schema_tmp_conf);
    DeleteCollection(collection);
}

TEST(CApiTest, SegmentTest) {
    auto schema_tmp_conf = "";
    auto collection = NewCollection(schema_tmp_conf);
    auto segment = NewSegment(collection, 0);
    DeleteCollection(collection);
    DeleteSegment(segment);
}

TEST(CApiTest, InsertTest) {
    auto schema_tmp_conf = "";
    auto collection = NewCollection(schema_tmp_conf);
    auto segment = NewSegment(collection, 0);

    std::vector<char> raw_data;
    std::vector<uint64_t> timestamps;
    std::vector<int64_t> uids;
    int N = 10000;
    std::default_random_engine e(67);
    for (int i = 0; i < N; ++i) {
        uids.push_back(100000 + i);
        timestamps.push_back(0);
        // append vec
        float vec[16];
        for (auto& x : vec) {
            x = e() % 2000 * 0.001 - 1.0;
        }
        raw_data.insert(raw_data.end(), (const char*)std::begin(vec), (const char*)std::end(vec));
        int age = e() % 100;
        raw_data.insert(raw_data.end(), (const char*)&age, ((const char*)&age) + sizeof(age));
    }

    auto line_sizeof = (sizeof(int) + sizeof(float) * 16);

    auto offset = PreInsert(segment, N);

    auto res = Insert(segment, offset, N, uids.data(), timestamps.data(), raw_data.data(), (int)line_sizeof, N);

    assert(res == 0);

    DeleteCollection(collection);
    DeleteSegment(segment);
}

TEST(CApiTest, DeleteTest) {
    auto schema_tmp_conf = "";
    auto collection = NewCollection(schema_tmp_conf);
    auto segment = NewSegment(collection, 0);

    long delete_row_ids[] = {100000, 100001, 100002};
    unsigned long delete_timestamps[] = {0, 0, 0};

    auto offset = PreDelete(segment, 3);

    auto del_res = Delete(segment, offset, 3, delete_row_ids, delete_timestamps);
    assert(del_res == 0);

    DeleteCollection(collection);
    DeleteSegment(segment);
}

TEST(CApiTest, SearchTest) {
    auto schema_tmp_conf = "";
    auto collection = NewCollection(schema_tmp_conf);
    auto segment = NewSegment(collection, 0);

    std::vector<char> raw_data;
    std::vector<uint64_t> timestamps;
    std::vector<int64_t> uids;
    int N = 10000;
    std::default_random_engine e(67);
    for (int i = 0; i < N; ++i) {
        uids.push_back(100000 + i);
        timestamps.push_back(0);
        // append vec
        float vec[16];
        for (auto& x : vec) {
            x = e() % 2000 * 0.001 - 1.0;
        }
        raw_data.insert(raw_data.end(), (const char*)std::begin(vec), (const char*)std::end(vec));
        int age = e() % 100;
        raw_data.insert(raw_data.end(), (const char*)&age, ((const char*)&age) + sizeof(age));
    }

    auto line_sizeof = (sizeof(int) + sizeof(float) * 16);

    auto offset = PreInsert(segment, N);

    auto ins_res = Insert(segment, offset, N, uids.data(), timestamps.data(), raw_data.data(), (int)line_sizeof, N);
    assert(ins_res == 0);

    const char* dsl_string = R"(
    {
        "bool": {
            "vector": {
                "fakevec": {
                    "metric_type": "L2",
                    "params": {
                        "nprobe": 10
                    },
                    "query": "$0",
                    "topk": 10
                }
            }
        }
    })";

    namespace ser = milvus::proto::service;
    int num_queries = 10;
    int dim = 16;
    std::normal_distribution<double> dis(0, 1);
    ser::PlaceholderGroup raw_group;
    auto value = raw_group.add_placeholders();
    value->set_tag("$0");
    value->set_type(ser::PlaceholderType::VECTOR_FLOAT);
    for (int i = 0; i < num_queries; ++i) {
        std::vector<float> vec;
        for (int d = 0; d < dim; ++d) {
            vec.push_back(dis(e));
        }
        // std::string line((char*)vec.data(), (char*)vec.data() + vec.size() * sizeof(float));
        value->add_values(vec.data(), vec.size() * sizeof(float));
    }
    auto blob = raw_group.SerializeAsString();

    auto plan = CreatePlan(collection, dsl_string);
    auto placeholderGroup = ParsePlaceholderGroup(plan, blob.data(), blob.length());
    std::vector<CPlaceholderGroup> placeholderGroups;
    placeholderGroups.push_back(placeholderGroup);
    timestamps.clear();
    timestamps.push_back(1);

    long result_ids[100];
    float result_distances[100];

    auto sea_res = Search(segment, plan, placeholderGroups.data(), timestamps.data(), 1, result_ids, result_distances);
    assert(sea_res == 0);

    DeletePlan(plan);
    DeletePlaceholderGroup(placeholderGroup);
    DeleteCollection(collection);
    DeleteSegment(segment);
}

TEST(CApiTest, BuildIndexTest) {
    auto schema_tmp_conf = "";
    auto collection = NewCollection(schema_tmp_conf);
    auto segment = NewSegment(collection, 0);

    std::vector<char> raw_data;
    std::vector<uint64_t> timestamps;
    std::vector<int64_t> uids;
    int N = 10000;
    std::default_random_engine e(67);
    for (int i = 0; i < N; ++i) {
        uids.push_back(100000 + i);
        timestamps.push_back(0);
        // append vec
        float vec[16];
        for (auto& x : vec) {
            x = e() % 2000 * 0.001 - 1.0;
        }
        raw_data.insert(raw_data.end(), (const char*)std::begin(vec), (const char*)std::end(vec));
        int age = e() % 100;
        raw_data.insert(raw_data.end(), (const char*)&age, ((const char*)&age) + sizeof(age));
    }

    auto line_sizeof = (sizeof(int) + sizeof(float) * 16);

    auto offset = PreInsert(segment, N);

    auto ins_res = Insert(segment, offset, N, uids.data(), timestamps.data(), raw_data.data(), (int)line_sizeof, N);
    assert(ins_res == 0);

    // TODO: add index ptr
    Close(segment);
    BuildIndex(collection, segment);

    const char* dsl_string = R"(
    {
        "bool": {
            "vector": {
                "fakevec": {
                    "metric_type": "L2",
                    "params": {
                        "nprobe": 10
                    },
                    "query": "$0",
                    "topk": 10
                }
            }
        }
    })";

    namespace ser = milvus::proto::service;
    int num_queries = 10;
    int dim = 16;
    std::normal_distribution<double> dis(0, 1);
    ser::PlaceholderGroup raw_group;
    auto value = raw_group.add_placeholders();
    value->set_tag("$0");
    value->set_type(ser::PlaceholderType::VECTOR_FLOAT);
    for (int i = 0; i < num_queries; ++i) {
        std::vector<float> vec;
        for (int d = 0; d < dim; ++d) {
            vec.push_back(dis(e));
        }
        // std::string line((char*)vec.data(), (char*)vec.data() + vec.size() * sizeof(float));
        value->add_values(vec.data(), vec.size() * sizeof(float));
    }
    auto blob = raw_group.SerializeAsString();

    auto plan = CreatePlan(collection, dsl_string);
    auto placeholderGroup = ParsePlaceholderGroup(plan, blob.data(), blob.length());
    std::vector<CPlaceholderGroup> placeholderGroups;
    placeholderGroups.push_back(placeholderGroup);
    timestamps.clear();
    timestamps.push_back(1);

    long result_ids[100];
    float result_distances[100];

    auto sea_res = Search(segment, plan, placeholderGroups.data(), timestamps.data(), 1, result_ids, result_distances);
    assert(sea_res == 0);

    DeletePlan(plan);
    DeletePlaceholderGroup(placeholderGroup);

    DeleteCollection(collection);
    DeleteSegment(segment);
}

TEST(CApiTest, IsOpenedTest) {
    auto schema_tmp_conf = "";
    auto collection = NewCollection(schema_tmp_conf);
    auto segment = NewSegment(collection, 0);

    auto is_opened = IsOpened(segment);
    assert(is_opened);

    DeleteCollection(collection);
    DeleteSegment(segment);
}

TEST(CApiTest, CloseTest) {
    auto schema_tmp_conf = "";
    auto collection = NewCollection(schema_tmp_conf);
    auto segment = NewSegment(collection, 0);

    auto status = Close(segment);
    assert(status == 0);

    DeleteCollection(collection);
    DeleteSegment(segment);
}

TEST(CApiTest, GetMemoryUsageInBytesTest) {
    auto schema_tmp_conf = "";
    auto collection = NewCollection(schema_tmp_conf);
    auto segment = NewSegment(collection, 0);

    auto old_memory_usage_size = GetMemoryUsageInBytes(segment);
    std::cout << "old_memory_usage_size = " << old_memory_usage_size << std::endl;

    std::vector<char> raw_data;
    std::vector<uint64_t> timestamps;
    std::vector<int64_t> uids;
    int N = 10000;
    std::default_random_engine e(67);
    for (int i = 0; i < N; ++i) {
        uids.push_back(100000 + i);
        timestamps.push_back(0);
        // append vec
        float vec[16];
        for (auto& x : vec) {
            x = e() % 2000 * 0.001 - 1.0;
        }
        raw_data.insert(raw_data.end(), (const char*)std::begin(vec), (const char*)std::end(vec));
        int age = e() % 100;
        raw_data.insert(raw_data.end(), (const char*)&age, ((const char*)&age) + sizeof(age));
    }

    auto line_sizeof = (sizeof(int) + sizeof(float) * 16);

    auto offset = PreInsert(segment, N);

    auto res = Insert(segment, offset, N, uids.data(), timestamps.data(), raw_data.data(), (int)line_sizeof, N);

    assert(res == 0);

    auto memory_usage_size = GetMemoryUsageInBytes(segment);

    std::cout << "new_memory_usage_size = " << memory_usage_size << std::endl;

    assert(memory_usage_size == 2785280);

    DeleteCollection(collection);
    DeleteSegment(segment);
}

namespace {
auto
generate_data(int N) {
    std::vector<char> raw_data;
    std::vector<uint64_t> timestamps;
    std::vector<int64_t> uids;
    std::default_random_engine er(42);
    std::uniform_real_distribution<> distribution(0.0, 1.0);
    std::default_random_engine ei(42);
    for (int i = 0; i < N; ++i) {
        uids.push_back(10 * N + i);
        timestamps.push_back(0);
        // append vec
        float vec[16];
        for (auto& x : vec) {
            x = distribution(er);
        }
        raw_data.insert(raw_data.end(), (const char*)std::begin(vec), (const char*)std::end(vec));
        int age = ei() % 100;
        raw_data.insert(raw_data.end(), (const char*)&age, ((const char*)&age) + sizeof(age));
    }
    return std::make_tuple(raw_data, timestamps, uids);
}
}  // namespace

// TEST(CApiTest, TestSearchPreference) {
//    auto schema_tmp_conf = "";
//    auto collection = NewCollection(schema_tmp_conf);
//    auto segment = NewSegment(collection, 0);
//
//    auto beg = chrono::high_resolution_clock::now();
//    auto next = beg;
//    int N = 1000 * 1000 * 10;
//    auto [raw_data, timestamps, uids] = generate_data(N);
//    auto line_sizeof = (sizeof(int) + sizeof(float) * 16);
//
//    next = chrono::high_resolution_clock::now();
//    std::cout << "generate_data: " << chrono::duration_cast<chrono::milliseconds>(next - beg).count() << "ms"
//              << std::endl;
//    beg = next;
//
//    auto offset = PreInsert(segment, N);
//    auto res = Insert(segment, offset, N, uids.data(), timestamps.data(), raw_data.data(), (int)line_sizeof, N);
//    assert(res == 0);
//    next = chrono::high_resolution_clock::now();
//    std::cout << "insert: " << chrono::duration_cast<chrono::milliseconds>(next - beg).count() << "ms" << std::endl;
//    beg = next;
//
//    auto N_del = N / 100;
//    std::vector<uint64_t> del_ts(N_del, 100);
//    auto pre_off = PreDelete(segment, N_del);
//    Delete(segment, pre_off, N_del, uids.data(), del_ts.data());
//
//    next = chrono::high_resolution_clock::now();
//    std::cout << "delete1: " << chrono::duration_cast<chrono::milliseconds>(next - beg).count() << "ms" << std::endl;
//    beg = next;
//
//    auto row_count = GetRowCount(segment);
//    assert(row_count == N);
//
//    std::vector<long> result_ids(10 * 16);
//    std::vector<float> result_distances(10 * 16);
//
//    CQueryInfo queryInfo{1, 10, "fakevec"};
//    auto sea_res =
//        Search(segment, queryInfo, 104, (float*)raw_data.data(), 16, result_ids.data(), result_distances.data());
//
//    //    ASSERT_EQ(sea_res, 0);
//    //    ASSERT_EQ(result_ids[0], 10 * N);
//    //    ASSERT_EQ(result_distances[0], 0);
//
//    next = chrono::high_resolution_clock::now();
//    std::cout << "query1: " << chrono::duration_cast<chrono::milliseconds>(next - beg).count() << "ms" << std::endl;
//    beg = next;
//    sea_res = Search(segment, queryInfo, 104, (float*)raw_data.data(), 16, result_ids.data(),
//    result_distances.data());
//
//    //    ASSERT_EQ(sea_res, 0);
//    //    ASSERT_EQ(result_ids[0], 10 * N);
//    //    ASSERT_EQ(result_distances[0], 0);
//
//    next = chrono::high_resolution_clock::now();
//    std::cout << "query2: " << chrono::duration_cast<chrono::milliseconds>(next - beg).count() << "ms" << std::endl;
//    beg = next;
//
//    // Close(segment);
//    // BuildIndex(segment);
//
//    next = chrono::high_resolution_clock::now();
//    std::cout << "build index: " << chrono::duration_cast<chrono::milliseconds>(next - beg).count() << "ms"
//              << std::endl;
//    beg = next;
//
//    std::vector<int64_t> result_ids2(10);
//    std::vector<float> result_distances2(10);
//
//    sea_res =
//        Search(segment, queryInfo, 104, (float*)raw_data.data(), 16, result_ids2.data(), result_distances2.data());
//
//    //    sea_res = Search(segment, nullptr, 104, result_ids2.data(),
//    //    result_distances2.data());
//
//    next = chrono::high_resolution_clock::now();
//    std::cout << "search10: " << chrono::duration_cast<chrono::milliseconds>(next - beg).count() << "ms" << std::endl;
//    beg = next;
//
//    sea_res =
//        Search(segment, queryInfo, 104, (float*)raw_data.data(), 16, result_ids2.data(), result_distances2.data());
//
//    next = chrono::high_resolution_clock::now();
//    std::cout << "search11: " << chrono::duration_cast<chrono::milliseconds>(next - beg).count() << "ms" << std::endl;
//    beg = next;
//
//    //    std::cout << "case 1" << std::endl;
//    //    for (int i = 0; i < 10; ++i) {
//    //        std::cout << result_ids[i] << "->" << result_distances[i] << std::endl;
//    //    }
//    //    std::cout << "case 2" << std::endl;
//    //    for (int i = 0; i < 10; ++i) {
//    //        std::cout << result_ids2[i] << "->" << result_distances2[i] << std::endl;
//    //    }
//    //
//    //    for (auto x : result_ids2) {
//    //        ASSERT_GE(x, 10 * N + N_del);
//    //        ASSERT_LT(x, 10 * N + N);
//    //    }
//
//    //    auto iter = 0;
//    //    for(int i = 0; i < result_ids.size(); ++i) {
//    //        auto uid = result_ids[i];
//    //        auto dis = result_distances[i];
//    //        if(uid >= 10 * N + N_del) {
//    //            auto uid2 = result_ids2[iter];
//    //            auto dis2 = result_distances2[iter];
//    //            ASSERT_EQ(uid, uid2);
//    //            ASSERT_EQ(dis, dis2);
//    //            ++iter;
//    //        }
//    //    }
//
//    DeleteCollection(collection);
//    DeleteSegment(segment);
//}

TEST(CApiTest, GetDeletedCountTest) {
    auto schema_tmp_conf = "";
    auto collection = NewCollection(schema_tmp_conf);
    auto segment = NewSegment(collection, 0);

    long delete_row_ids[] = {100000, 100001, 100002};
    unsigned long delete_timestamps[] = {0, 0, 0};

    auto offset = PreDelete(segment, 3);

    auto del_res = Delete(segment, offset, 3, delete_row_ids, delete_timestamps);
    assert(del_res == 0);

    // TODO: assert(deleted_count == len(delete_row_ids))
    auto deleted_count = GetDeletedCount(segment);
    assert(deleted_count == 0);

    DeleteCollection(collection);
    DeleteSegment(segment);
}

TEST(CApiTest, GetRowCountTest) {
    auto schema_tmp_conf = "";
    auto collection = NewCollection(schema_tmp_conf);
    auto segment = NewSegment(collection, 0);

    int N = 10000;
    auto [raw_data, timestamps, uids] = generate_data(N);
    auto line_sizeof = (sizeof(int) + sizeof(float) * 16);
    auto offset = PreInsert(segment, N);
    auto res = Insert(segment, offset, N, uids.data(), timestamps.data(), raw_data.data(), (int)line_sizeof, N);
    assert(res == 0);

    auto row_count = GetRowCount(segment);
    assert(row_count == N);

    DeleteCollection(collection);
    DeleteSegment(segment);
}

// TEST(CApiTest, SchemaTest) {
//    std::string schema_string =
//        "id: 6873737669791618215\nname: \"collection0\"\nschema: \u003c\n  "
//        "field_metas: \u003c\n    field_name: \"age\"\n    type: INT32\n    dim: 1\n  \u003e\n  "
//        "field_metas: \u003c\n    field_name: \"field_1\"\n    type: VECTOR_FLOAT\n    dim: 16\n  \u003e\n"
//        "\u003e\ncreate_time: 1600416765\nsegment_ids: 6873737669791618215\npartition_tags: \"default\"\n";
//
//    auto collection = NewCollection(schema_string.data());
//    auto segment = NewSegment(collection, 0);
//    DeleteCollection(collection);
//    DeleteSegment(segment);
//}