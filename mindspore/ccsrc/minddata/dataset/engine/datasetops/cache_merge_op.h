/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef DATASET_ENGINE_DATASETOPS_CACHE_MERGE_OP_H_
#define DATASET_ENGINE_DATASETOPS_CACHE_MERGE_OP_H_

#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include "minddata/dataset/core/tensor_row.h"
#include "minddata/dataset/engine/cache/cache_client.h"
#include "minddata/dataset/engine/datasetops/parallel_op.h"
#include "minddata/dataset/engine/dataset_iterator.h"
#include "minddata/dataset/util/queue.h"
#include "minddata/dataset/util/semaphore.h"

namespace mindspore {
namespace dataset {
/// \brief Provides method to merge two streams (one from CacheLookup and one from cache miss stream) into one single
/// stream
class CacheMergeOp : public ParallelOp {
 public:
  // Some handshake structures among the main thread, cleaner threads and parallel op threads.
  class TensorRowRequest {
   public:
    enum class State : uint8_t {
      kEmpty = 0,  // No row in the deque
      kDirty = 1,  // Cleaner hasn't flushed it to the cache server yet.
      kClean = 2   // The row has been flushed already.
    };
    explicit TensorRowRequest(row_id_type id) : st_(State::kEmpty), use_count_(0) {}
    ~TensorRowRequest() = default;
    State GetState() const { return st_; }
    void SetState(State newState) { st_ = newState; }
    Status Wait(TensorRow *out);
    void WakeUpAny(TensorRow &&row);
    Status Release(TensorRow *out);

   private:
    std::mutex dq_mux_;
    std::atomic<State> st_;
    Semaphore use_count_;
    std::deque<TensorRow> row_;
    TensorRow cleaner_copy_;
  };

  constexpr static int kCacheHitChildIdx = 0;   // Cache hit stream
  constexpr static int kCacheMissChildIdx = 1;  // Cache miss stream

  /// \brief The nested builder class inside of the CacheMergeOp is used to help manage all of
  /// the arguments for constructing it.  Use the builder by setting each argument
  /// with the provided set methods, and then finally call the build method to execute
  /// the actual construction.
  class Builder {
   public:
    /// Builder constructor. Creates the builder object.
    /// \note No default args
    Builder();

    /// Default destructor
    ~Builder() = default;

    /// Setter method.
    /// \return Builder setter method returns reference to the builder.
    Builder &SetNumWorkers(int32_t num_workers) {
      build_num_workers_ = num_workers;
      return *this;
    }

    /// Setter method.
    /// \return Builder setter method returns reference to the builder.
    Builder &SetOpConnectorSize(int32_t connector_size) {
      build_op_connector_size_ = connector_size;
      return *this;
    }

    /// Setter method.
    /// \return Builder setter method returns reference to the builder.
    Builder &SetClient(std::shared_ptr<CacheClient> cache_client) {
      build_cache_client_ = cache_client;
      return *this;
    }

    /// \brief Setter method
    /// \param sampler
    /// \return Builder setter method returns reference to the builder.
    Builder &SetSampler(std::shared_ptr<Sampler> sampler) {
      build_sampler_ = std::move(sampler);
      return *this;
    }

    /// \brief Setter method
    /// \param num_cleaners
    /// \return Builder setter method returns reference to the builder.
    Builder &SetNumCleaner(int32_t num_cleaners) {
      build_num_cleaners_ = num_cleaners;
      return *this;
    }

    /// The builder "build" method creates the final object and does some init on it.
    /// \param ptr The shared_ptr to the new CacheMergeOp object
    /// \return Status
    Status Build(std::shared_ptr<CacheMergeOp> *ptr);

   private:
    int32_t build_num_workers_;
    int32_t build_op_connector_size_;
    int32_t build_num_cleaners_;
    std::shared_ptr<CacheClient> build_cache_client_;
    std::shared_ptr<Sampler> build_sampler_;

    /// Check if the required parameters are set by the builder.
    /// \return Status The error code return
    Status SanityCheck() const;
  };

  /// \brief Constructor
  /// \param numWorkers Number of parallel workers as a derived class of ParallelOp
  /// \param opConnector Size Connector size as a derived class of ParallelOp
  /// \param numCleaners Number of cleaners to move cache miss rows into the cache server
  /// \param cache_client CacheClient to commmunicate with the Cache server
  /// \param sampler as a derived class of ParallelOp
  CacheMergeOp(int32_t numWorkers, int32_t opConnectorSize, int32_t numCleaners,
               std::shared_ptr<CacheClient> cache_client, const std::shared_ptr<Sampler> &sampler);
  ~CacheMergeOp();
  void Print(std::ostream &out, bool show_all) const override;
  friend std::ostream &operator<<(std::ostream &out, const CacheMergeOp &mo) {
    mo.Print(out, false);
    return out;
  }
  /// \brief Master thread responsible to spawn all the necessary worker threads for the two streams and
  /// the threads for the cleaners.
  /// \return
  Status operator()() override;
  /// \brief Entry function for worker thread that fetch rows from CacheLookupOp
  /// \param workerId
  /// \return Status object
  Status WorkerEntry(int32_t workerId) override;
  Status PrepareNodePostAction() override;
  /// \brief Entry function for worker thread that fetch rows from the cache miss stream
  /// \param workerId
  /// \return Status object
  Status CacheMissWorkerEntry(int32_t workerId);
  Status GetRq(row_id_type row_id, TensorRowRequest **);

  /// \brief Base-class override for NodePass pre-visit acceptor
  /// \param[in] p The node to visit
  /// \param[out] modified Indicator if the node was modified
  /// \return Status of the node visit
  Status PreAccept(NodePass *p, bool *modified) override;

  /// \brief Base-class override for NodePass visitor acceptor
  /// \param[in] p The node to visit
  /// \param[out] modified Indicator if the node was modified
  /// \return Status of the node visit
  Status Accept(NodePass *p, bool *modified) override;

  /// \brief Base-class override for eoe handling
  /// \param worker_id
  /// \return Status object
  Status EoeReceived(int32_t worker_id) override;

 protected:
  Status ComputeColMap() override;

 private:
  std::mutex mux_;
  std::map<row_id_type, MemGuard<TensorRowRequest, Allocator<TensorRowRequest>>> cache_miss_map_;
  std::unique_ptr<Queue<row_id_type>> io_que_;
  std::shared_ptr<CacheClient> cache_client_;
  int32_t num_cleaners_;

  /// \brief These are the entry functions for the cleaner threads. Each cleaner is responsible for
  /// moving cache miss TensorRow into the CacheServer.
  /// \return Status object
  Status Cleaner();
};
}  // namespace dataset
}  // namespace mindspore
#endif  // DATASET_ENGINE_DATASETOPS_CACHE_MERGE_OP_H_
