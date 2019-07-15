#ifndef __SCRIPT_RUNNER_H_
#define __SCRIPT_RUNNER_H_

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>


#define _RUN_SCRIPT(s) ScriptRunner::instance().run_script(s)


class ScriptRunner
{
public:
	static ScriptRunner& instance() { return _instance; }
	~ScriptRunner();
	void run_script(std::string path);
	void stop();

private:
	ScriptRunner();
	void run_loop();
	void exec_script(std::string &path);

	static ScriptRunner _instance;
	std::queue<std::string> q;
	std::mutex q_mutex;
	std::condition_variable q_cond;
	std::thread thr;
	bool alive;
	bool fake;
};

#endif
