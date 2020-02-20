#ifndef __SAFE_GETENV_H_
#define __SAFE_GETENV_H_

#include <string>
#include <cstdlib>

static inline std::string safe_getenv(const char *key)
{
    const char *v = std::getenv(key);
    if (!v)
        return std::string(key) + " is undefined!";
    return std::string(v);
}

static inline std::string safe_getenv(const char *key, std::string def)
{
    const char *v = std::getenv(key);
    if (!v)
	    return def;
    return std::string(v);
}

static inline bool getenv_has_key(const char *key)
{
    const char *v = std::getenv(key);
    return v;
}

#endif
