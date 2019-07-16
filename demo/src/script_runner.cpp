#include "script_runner.h"
#include "resources.h"

#include <cstdlib>
#include <iostream>
#include <execinfo.h>


ScriptRunner ScriptRunner::_instance;


ScriptRunner::ScriptRunner()
	: q_mutex()
	, thr(&ScriptRunner::run_loop, this)
	, alive(true)
	, fake(false)
{
	if (getenv_has_key("FAKE_SCRIPT_RUNNER"))
		fake = true;
}

ScriptRunner::~ScriptRunner()
{
	if (alive)
		stop();
}

void ScriptRunner::stop()
{
	alive = false;
	q_cond.notify_one();
	thr.join();
}

void ScriptRunner::run_script(std::string path)
{
	if (fake) {
		/* Print stack trace for debugging */
		int i, nptrs;
		void *buffer[100];
		char **strings;

		nptrs = backtrace(buffer, 100);
		if (!(strings = backtrace_symbols(buffer, nptrs)))
			return;
		std::cerr << "Enqueueing " << path << " from:" << std::endl;
		for (i = 1; i < nptrs; i++)
			std::cerr << "\t" << strings[i] << std::endl;
		std::cerr << "==================================" << std::endl;
		free(strings);
	}
	{
		std::unique_lock<std::mutex> lock(q_mutex);
		q.push(path);
	}
	/* Ensure we release the lock before the cv wakes a thread */
	q_cond.notify_one();
}

void ScriptRunner::run_loop()
{
	while (alive || !q.empty()) {
		std::unique_lock<std::mutex> lock(q_mutex);
		while (q.empty()) {
			q_cond.wait(lock);
			if (!alive)
				return;
		}
		std::string script = q.front();
		q.pop();
		exec_script(script);
	}
}

void ScriptRunner::exec_script(std::string &path)
{
	int rc;

	std::cerr
		<< "-- " << (fake ? "Faking " : "")
		<< "Running [" << path << "]" << std::endl;
	if (!fake && (rc = std::system(path.c_str())))
	    std::cerr << "Non-zero exit (" << rc << ") for " << path << std::endl;
}
