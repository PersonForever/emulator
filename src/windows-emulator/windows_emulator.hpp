#pragma once
#include "std_include.hpp"

#include <x64_emulator.hpp>

#include <utils/function.hpp>

#include "syscall_dispatcher.hpp"
#include "process_context.hpp"
#include "logger.hpp"
#include "file_system.hpp"

std::unique_ptr<x64_emulator> create_default_x64_emulator();

struct emulator_callbacks
{
    utils::optional_function<void(std::string_view)> stdout_callback{};
    utils::optional_function<void(uint32_t syscall_id, x64_emulator::pointer_type address, std::string_view mod_name,
                                  std::string_view syscall_name)>
        inline_syscall{};
    utils::optional_function<void(uint32_t syscall_id, x64_emulator::pointer_type address, std::string_view mod_name,
                                  std::string_view syscall_name, x64_emulator::pointer_type prev_address,
                                  std::string_view prev_mod_name)>
        outofline_syscall{};
};

// TODO: Split up into application and emulator settings
struct emulator_settings
{
    windows_path application{};
    windows_path working_directory{};
    std::filesystem::path registry_directory{"./registry"};
    std::filesystem::path emulation_root{};
    std::vector<std::u16string> arguments{};
    bool verbose_calls{false};
    bool disable_logging{false};
    bool silent_until_main{false};
    bool use_relative_time{false};
};

enum class apiset_location : uint8_t
{
    host,
    file,
    default_windows_10,
    default_windows_11
};

class windows_emulator
{
  public:
    windows_emulator(const std::filesystem::path& emulation_root,
                     std::unique_ptr<x64_emulator> emu = create_default_x64_emulator());
    windows_emulator(const emulator_settings& settings, emulator_callbacks callbacks = {},
                     std::unique_ptr<x64_emulator> emu = create_default_x64_emulator());

    windows_emulator(windows_emulator&&) = delete;
    windows_emulator(const windows_emulator&) = delete;
    windows_emulator& operator=(windows_emulator&&) = delete;
    windows_emulator& operator=(const windows_emulator&) = delete;

    ~windows_emulator() = default;

    x64_emulator& emu()
    {
        return *this->emu_;
    }

    const x64_emulator& emu() const
    {
        return *this->emu_;
    }

    process_context& process()
    {
        return this->process_;
    }

    const process_context& process() const
    {
        return this->process_;
    }

    syscall_dispatcher& dispatcher()
    {
        return this->dispatcher_;
    }

    const syscall_dispatcher& dispatcher() const
    {
        return this->dispatcher_;
    }

    emulator_thread& current_thread() const
    {
        if (!this->process_.active_thread)
        {
            throw std::runtime_error("No active thread!");
        }

        return *this->process_.active_thread;
    }

    void start(std::chrono::nanoseconds timeout = {}, size_t count = 0);

    void serialize(utils::buffer_serializer& buffer) const;
    void deserialize(utils::buffer_deserializer& buffer);

    void save_snapshot();
    void restore_snapshot();

    void add_syscall_hook(instruction_hook_callback callback)
    {
        this->syscall_hooks_.push_back(std::move(callback));
    }

    logger log{};
    bool verbose{false};
    bool verbose_calls{false};
    bool buffer_stdout{false};
    bool fuzzing{false};

    void yield_thread();
    void perform_thread_switch();
    bool activate_thread(uint32_t id);

    bool time_is_relative() const
    {
        return this->use_relative_time_;
    }

    emulator_callbacks& callbacks()
    {
        return this->callbacks_;
    }

    file_system& file_sys()
    {
        return this->file_sys_;
    }

    const file_system& file_sys() const
    {
        return this->file_sys_;
    }

    const std::filesystem::path& get_emulation_root()
    {
        return this->emulation_root_;
    }

  private:
    std::filesystem::path emulation_root_{};
    file_system file_sys_;

    emulator_callbacks callbacks_{};

    bool switch_thread_{false};
    bool use_relative_time_{false};
    bool silent_until_main_{false};

    std::unique_ptr<x64_emulator> emu_{};
    std::vector<instruction_hook_callback> syscall_hooks_{};

    process_context process_;
    syscall_dispatcher dispatcher_;

    std::vector<std::byte> process_snapshot_{};
    // std::optional<process_context> process_snapshot_{};

    void setup_hooks();
    void setup_process(const emulator_settings& settings);
    void on_instruction_execution(uint64_t address);
};
