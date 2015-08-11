#pragma once
#include "injectory/common.hpp"
#include "injectory/exception.hpp"
#include "injectory/thread.hpp"
#include "injectory/library.hpp"
#include <winnt.h>
#include <boost/optional.hpp>

class ProcessWithThread;

class Process
{
private:
	shared_ptr<void> shared_handle;
	bool resumeOnDestruction;

public:
	const pid_t id;

	explicit Process(pid_t id = 0, handle_t handle = nullptr)
		: id(id)
		, shared_handle(handle, CloseHandle)
		, resumeOnDestruction(false)
	{}

	virtual ~Process()
	{
		try
		{
			if (resumeOnDestruction)
				resume();
		}
		catch (...) {}
	}

	void tryResumeOnDestruction(bool resume = true)
	{
		resumeOnDestruction = resume;
	}

	handle_t handle() const
	{
		return shared_handle.get();
	}

	void waitForInputIdle(DWORD millis = 5000) const
	{
		if (WaitForInputIdle(handle(), millis) != 0)
			BOOST_THROW_EXCEPTION(ex_wait_for_input_idle());
	}

	DWORD wait(DWORD millis = INFINITE)
	{
		DWORD ret = WaitForSingleObject(handle(), millis);
		if (ret == WAIT_FAILED)
			BOOST_THROW_EXCEPTION(ex_wait_for_exit());
		else
			return ret;
	}

	void kill(UINT exitCode = 1)
	{
		BOOL ret = TerminateProcess(handle(), exitCode);
		if (!ret)
			BOOST_THROW_EXCEPTION(ex_injection() << e_text("error killing process") << e_pid(id) << e_last_error());
	}


	void suspend(bool _suspend = true) const;
	void resume(bool _resume = true) const
	{
		suspend(!_resume);
	}

	void inject(const Library& lib, const bool& verbose = false);

	bool is64bit() const;

public:
	static Process open(const pid_t& pid, bool inheritHandle = false, DWORD desiredAccess =
			PROCESS_QUERY_INFORMATION	| // Required by Alpha
			PROCESS_CREATE_THREAD		| // For CreateRemoteThread
			PROCESS_VM_OPERATION		| // For VirtualAllocEx/VirtualFreeEx
			PROCESS_VM_WRITE			| // For WriteProcessMemory
			PROCESS_SUSPEND_RESUME		|
			PROCESS_VM_READ
		);

	// Creates a new process and its primary thread.
	// The new process runs in the security context of the calling process.
	static ProcessWithThread launch(const path& app, const wstring& args = L"",
		boost::optional<const vector<string>&> env = boost::none,
		boost::optional<const wstring&> cwd = boost::none,
		bool inheritHandles = false, DWORD creationFlags = 0,
		SECURITY_ATTRIBUTES* processAttributes = nullptr, SECURITY_ATTRIBUTES* threadAttributes = nullptr,
		STARTUPINFOW* startupInfo = { 0 });
};

class ProcessWithThread : public Process
{
public:
	Thread thread;
public:
	using Process::Process;
	ProcessWithThread(pid_t id, handle_t handle, Thread thread)
		: Process(id, handle)
		, thread(thread)
	{}
};
