#pragma once
#ifdef _DEBUG
#ifndef DEBUG_PANEL_HPP
#define DEBUG_PANEL_HPP
#include "Panels.h"
#include "spdlog/sinks/base_sink.h"

namespace PAIN {

    struct DebugLogEntry {
        spdlog::level::level_enum level;
        std::string message;
    };

    class DebugConsoleSink : public spdlog::sinks::base_sink<std::mutex> {
    public:
        static constexpr size_t MAX_ENTRIES = 200;

        static std::deque<DebugLogEntry>& getEntries() {
            static std::deque<DebugLogEntry> entries;
            return entries;
        }

        static std::mutex& getEntriesMutex() {
            static std::mutex m;
            return m;
        }

        static void clear() {
            std::lock_guard<std::mutex> lock(getEntriesMutex());
            getEntries().clear();
        }

    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override {
            spdlog::memory_buf_t formatted;
            base_sink<std::mutex>::formatter_->format(msg, formatted);

            DebugLogEntry entry;
            entry.level = msg.level;
            entry.message = fmt::to_string(formatted);

            if (!entry.message.empty() && entry.message.back() == '\n')
                entry.message.pop_back();

            std::lock_guard<std::mutex> lock(getEntriesMutex());
            auto& entries = getEntries();
            entries.push_back(std::move(entry));
            if (entries.size() > MAX_ENTRIES)
                entries.pop_front();
        }

        void flush_() override {}
    };

    namespace Editor {
        namespace Panel {
            class DebugPanel : public IPanel {
            public:
                DebugPanel();
                ~DebugPanel() override = default;
                void nextWindowSettings() override;
                void onAttach() override;
                void onUpdate(AppTiming timing) override;
            private:
                std::vector<float> frameTimes;
                bool log_filter_trace = true;
                bool log_filter_info = true;
                bool log_filter_warn = true;
                bool log_filter_error = true;
                bool log_scroll_to_bottom = true;
                bool log_sink_registered = false;
            };
        } // namespace Panel
    } // namespace Editor
} // namespace PAIN
#endif
#endif