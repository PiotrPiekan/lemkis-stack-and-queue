/**
 * @file benchmark_script.hpp
 * @brief Provides functions to construct and run benchmark configurations.
 */

#pragma once

#include <fstream>
#include <list_stack.hpp>
#include <lock_free_queue_benchmark.hpp>
#include <memory>
#include <print>
#include <queue_cv_benchmark.hpp>
#include <queue_mutex_benchmark.hpp>
#include <ranges>
#include <reader_writer_queue_benchmark.hpp>
#include <stack_cv_benchmark.hpp>
#include <stack_mutex_benchmark.hpp>
#include <stream_utils.hpp>
#include <timer.hpp>
#include <vector>
#include <vector_stack.hpp>

namespace benchmark_script {

    /**
     * @brief Alias for vector_stack instantiated with int.
     */
    using vector_stack_t = vector_stack<int>;

    /**
     * @brief Alias for list_stack instantiated with int.
     */
    using list_stack_t = list_stack<int>;

    /**
     * @brief Container type for dynamically allocated benchmarks.
     */
    using benchmark_list_t = std::vector<std::unique_ptr<benchmark_base>>;

    /**
     * @brief Constants representing single-threaded benchmark configuration.
     */
    static constexpr int single_producer = 1;
    static constexpr int single_consumer = 1;

    /**
     * @brief Adds vector_stack-based benchmarks to the list.
     * @param list Output container for benchmark instances.
     * @param prod_count Number of producer threads.
     * @param cons_count Number of consumer threads.
     * @param elem_count Total number of elements.
     */
    inline auto add_vector_stack_benchmarks(benchmark_list_t& list,
                                            int prod_count,
                                            int cons_count,
                                            int elem_count) -> void {
        list.emplace_back(
            std::make_unique<stack_mutex_benchmark<vector_stack_t>>(
                "vector_stack (mutex)", prod_count, cons_count, elem_count));
        list.emplace_back(std::make_unique<stack_cv_benchmark<vector_stack_t>>(
            "vector_stack (cv)", prod_count, cons_count, elem_count));
    }

    /**
     * @brief Adds list_stack-based benchmarks to the list.
     * @param list Output container for benchmark instances.
     * @param prod_count Number of producer threads.
     * @param cons_count Number of consumer threads.
     * @param elem_count Total number of elements.
     */
    inline auto add_list_stack_benchmarks(benchmark_list_t& list,
                                          int prod_count,
                                          int cons_count,
                                          int elem_count) -> void {
        list.emplace_back(std::make_unique<stack_mutex_benchmark<list_stack_t>>(
            "list_stack (mutex)", prod_count, cons_count, elem_count));
        list.emplace_back(std::make_unique<stack_cv_benchmark<list_stack_t>>(
            "list_stack (cv)", prod_count, cons_count, elem_count));
    }

    /**
     * @brief Adds all stack-based benchmarks to the list.
     * @param list Output container for benchmark instances.
     * @param prod_count Number of producer threads.
     * @param cons_count Number of consumer threads.
     * @param elem_count Total number of elements.
     */
    inline auto add_stack_benchmarks(benchmark_list_t& list,
                                     int prod_count,
                                     int cons_count,
                                     int elem_count) -> void {
        add_vector_stack_benchmarks(list, prod_count, cons_count, elem_count);
        add_list_stack_benchmarks(list, prod_count, cons_count, elem_count);
    }

    /**
     * @brief Adds queue-based benchmarks using two_stack_queue to the list.
     * @param list Output container for benchmark instances.
     * @param prod_count Number of producer threads.
     * @param cons_count Number of consumer threads.
     * @param elem_count Total number of elements.
     */
    inline auto add_queue_benchmarks(benchmark_list_t& list,
                                     int prod_count,
                                     int cons_count,
                                     int elem_count) -> void {
        list.emplace_back(std::make_unique<queue_mutex_benchmark>(
            "two_stack_queue (mutex)", prod_count, cons_count, elem_count));
        list.emplace_back(std::make_unique<queue_cv_benchmark>(
            "two_stack_queue (cv)", prod_count, cons_count, elem_count));
    }

    /**
     * @brief Adds lock-free queue benchmarks to the list.
     *
     * Includes moodycamel::ConcurrentQueue and optionally ReaderWriterQueue
     * if producer and consumer counts are both 1.
     *
     * @param list Output container for benchmark instances.
     * @param prod_count Number of producer threads.
     * @param cons_count Number of consumer threads.
     * @param elem_count Total number of elements.
     */
    inline auto add_lockfree_benchmarks(benchmark_list_t& list,
                                        int prod_count,
                                        int cons_count,
                                        int elem_count) -> void {
        list.emplace_back(std::make_unique<lock_free_queue_benchmark>(
            "moodycamel::ConcurrentQueue", prod_count, cons_count, elem_count));
        if (prod_count == single_producer && cons_count == single_consumer) {
            list.emplace_back(std::make_unique<reader_writer_queue_benchmark>(
                "moodycamel::ReaderWriterQueue", elem_count));
        }
    }

    /**
     * @brief Creates all benchmark variants for a given configuration.
     * @param prod_count Number of producer threads.
     * @param cons_count Number of consumer threads.
     * @param elem_count Total number of elements.
     * @return A list of dynamically allocated benchmark instances.
     */
    inline auto create_all_benchmarks(int prod_count,
                                      int cons_count,
                                      int elem_count) -> benchmark_list_t {
        benchmark_list_t list;
        add_stack_benchmarks(list, prod_count, cons_count, elem_count);
        add_queue_benchmarks(list, prod_count, cons_count, elem_count);
        add_lockfree_benchmarks(list, prod_count, cons_count, elem_count);
        return list;
    }

    /**
     * @brief Executes and reports results for a list of benchmarks.
     * @param list List of benchmark instances.
     * @param file_name Path to output CSV file.
     */
    inline auto run_and_report(benchmark_list_t list,
                               std::string_view file_name) -> void {
        for (auto& bench : list) {
            timer t;
            bench->prepare_threads();
            t.start();
            bench->run();
            const auto duration = t.elapsed();
            bench->print_result(duration);
            bench->write_result_to_file(duration, file_name);
        }
    }

    /**
     * @brief Runs all benchmarks for a single configuration.
     * @param prod_count Number of producer threads.
     * @param cons_count Number of consumer threads.
     * @param elem_count Total number of elements.
     * @param file_name Path to output CSV file.
     */
    inline auto run_for_config(int prod_count,
                               int cons_count,
                               int elem_count,
                               std::string_view file_name) -> void {
        std::print("{} producer(s), {} consumer(s):\n", prod_count, cons_count);
        auto list = create_all_benchmarks(prod_count, cons_count, elem_count);
        run_and_report(std::move(list), file_name);
        std::print("\n");
    }

    /**
     * @brief Writes CSV header to the result file.
     * @param file_name Path to the output file.
     */
    inline auto write_csv_header(std::string_view file_name) -> void {
        if (std::ofstream out(std::string{file_name}); out) {
            constexpr auto header =
                "benchmark,producers,consumers,items,duration_ms\n";
            out.write(header,
                      to_streamsize(std::char_traits<char>::length(header)));
        }
    }

    /**
     * @brief Runs all benchmark configurations (Cartesian product of thread
     * counts).
     * @param file_name Path to the output file.
     */
    inline auto run_all_configurations(std::string_view file_name) -> void {
        constexpr int total_elements = 100000;
        const std::vector<int> prod_counts{1, 2, 4};
        const std::vector<int> cons_counts{1, 2, 4};
        for (const int prod : prod_counts) {
            for (const int cons : cons_counts) {
                run_for_config(prod, cons, total_elements, file_name);
            }
        }
    }

    /**
     * @brief Entry function to run all benchmarks and write results to CSV.
     * @param file_name Output file name.
     */
    inline auto run_all_benchmarks(std::string_view file_name) -> void {
        write_csv_header(file_name);
        std::print("Running all benchmarks:\n========================\n\n");
        run_all_configurations(file_name);
    }

}  // namespace benchmark_script
