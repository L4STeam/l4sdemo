#include "script_runner.h"
#include "resources.h"

#include <cstdlib>
#include <iostream>


ScriptRunner ScriptRunner::_instance;


ScriptRunner::ScriptRunner()
	: q_mutex()
	, thr(&ScriptRunner::run_loop, this)
	, alive(true)
	, fake(false)
{
	if (safe_getenv("FAKE_SCRIPT_RUNNER", "") != "")
		fake = true;
}

ScriptRunner::~ScriptRunner()
{
	alive = false;
	thr.join();
}

void ScriptRunner::run_script(std::string path)
{
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
		while (q.empty()) { q_cond.wait(lock); }
		std::string script = q.front();
		q.pop();
		exec_script(script);
	}
}

void ScriptRunner::exec_script(std::string &path)
{
	std::cerr << "-- " << (fake ? "Faking " : "")
		<< "Running [" << path << "]" << std::endl;
	if (!fake)
		std::system(path.c_str());
}
