/**
 * @file SharedQueue.h
 * @brief Thread-safe queue used to hand peers to worker threads.
 */

#ifndef TORRENT_SHARED_QUEUE
#define TORRENT_SHARED_QUEUE

#include <deque>
#include <mutex>
#include <condition_variable>

/**
 * @class SharedQueue
 * @brief Blocking deque protected by a mutex and condition variable.
 *
 * @tparam T Element type stored in the queue.
 */
template <typename T>
class SharedQueue
{