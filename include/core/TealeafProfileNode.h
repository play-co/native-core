#ifndef TEALEAF_PROFILE_NODE_H
#define TEALEAF_PROFILE_NODE_H

#include <time.h>

#include "uthash/uthash.h"
#include "uthash/utarray.h"

struct profile_history {
  char *name;
  int call_count;
  double total_time_ms;

  UT_array * times;
  UT_hash_handle hh;
};

class TealeafProfileNode {
  public:
    TealeafProfileNode(const char*);
    ~TealeafProfileNode();
    static struct profile_history* GetProfiles();
  private:
    void RecordProfileEntry(double duration_ms);
    struct profile_history* GetHistory(const char*);

    // Shared history
    static struct profile_history *history;

    // Per profile data
    struct profile_history *node;
    struct timeval start;
    struct timeval duration;
    struct timeval finish;
};

#endif
