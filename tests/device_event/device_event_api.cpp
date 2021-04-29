/*******************************************************************************
//
//  SYCL 2020 Conformance Test Suite
//
//  Copyright:	(c) 2018 by Codeplay Software LTD. All Rights Reserved.
//
*******************************************************************************/

#include "../common/common.h"

#include <algorithm>
#include <array>
#include <string>

#define TEST_NAME device_event_api

namespace TEST_NAMESPACE {
using namespace sycl_cts;

class device_event_wait;

/** tests the api for cl::sycl::device_event
 */
class TEST_NAME : public util::test_base {
 public:
  /** return information about this test
   */
  void get_info(test_base::info &out) const override {
    set_test_info(out, TOSTRING(TEST_NAME), TEST_FILE);
  }

  /** execute the test
   */
  void run(util::logger &log) override {
    try {
      /** check wait() member function
       */
      {
        auto testQueue = util::get_cts_object::queue();

        constexpr size_t bufferSize = 512;
        constexpr size_t sensorIndex = bufferSize / 2;
        constexpr int referenceValue = 1234;

        std::array<int, bufferSize> data;
        std::fill(data.begin(), data.end(), referenceValue);

        bool error = false;
        {
          auto range = cl::sycl::range<1>(1);
          auto range_buffer = cl::sycl::range<1>(bufferSize);
          auto buf = cl::sycl::buffer<int, 1>(data.data(), range_buffer);
          auto errBuf = cl::sycl::buffer<bool, 1>(&error, range);

          testQueue.submit([&](cl::sycl::handler &cgh) {
            using namespace cl::sycl::access;

            auto globalAcc = buf.get_access<mode::read_write>(cgh);
            auto errorAcc = errBuf.get_access<mode::write>(cgh);
            auto localAcc =
                cl::sycl::accessor<int, 1, mode::read_write, target::local>(
                    range_buffer, cgh);

            cgh.parallel_for<class device_event_wait>(
                cl::sycl::nd_range<1>(range, range),
                [=](cl::sycl::nd_item<1> ndItem) {
                  // Run asynchronous copy for full buffer
                  cl::sycl::device_event deviceEvent =
                      ndItem.async_work_group_copy(localAcc.get_pointer(),
                                                   globalAcc.get_pointer(),
                                                   bufferSize);

                  deviceEvent.wait();

                  // Check sensor data was updated
                  if (localAcc[sensorIndex] != referenceValue) {
                    errorAcc[0] = true;
                  }
                });
          });
          auto errAcc = errBuf.get_access<cl::sycl::access::mode::read>();
          if (errAcc[0]) {
            FAIL(log, "cl::sycl::device_event async_work_group_copy failed");
          }
        }
      }

    } catch (const cl::sycl::exception &e) {
      log_exception(log, e);
      const auto errorMsg =
          std::string("a SYCL exception was caught: ") + e.what();
      FAIL(log, errorMsg);
    }
  }
};

// register this test with the test_collection
util::test_proxy<TEST_NAME> proxy;

}  // namespace TEST_NAMESPACE
