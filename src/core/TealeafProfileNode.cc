#include "core/TealeafProfileNode.h"

#include <sys/time.h>
#include <pthread.h>

#include <stdlib.h>

#include "uthash/uthash.h"
#include "uthash/utarray.h"

#define MILLION 1000000

struct profile_history *TealeafProfileNode::history = NULL;
static const UT_icd ut_double_icd = {sizeof(double), NULL, NULL, NULL};
static pthread_mutex_t profile_mutex = PTHREAD_MUTEX_INITIALIZER;

TealeafProfileNode::TealeafProfileNode(const char *location) {
  node = GetHistory(location);

  gettimeofday(&start, NULL);
}

TealeafProfileNode::~TealeafProfileNode() {
  gettimeofday(&finish, NULL);

  // Need to calculate duration = finish - start. Carry usec to sec if
  // necessary.
  if (start.tv_usec > finish.tv_usec) {
    duration.tv_sec = finish.tv_sec + 1 - start.tv_sec;
    duration.tv_usec = MILLION + finish.tv_usec - start.tv_usec;
  } else {
    duration.tv_sec = finish.tv_sec - start.tv_sec;
    duration.tv_usec = finish.tv_usec - start.tv_usec;
  }

  RecordProfileEntry((duration.tv_usec / 1000.0) + (duration.tv_sec * 1000.0));
}

struct profile_history* TealeafProfileNode::GetProfiles() {
  return history;
}

struct profile_history* TealeafProfileNode::GetHistory(char const *location) {
  pthread_mutex_lock(&profile_mutex);

  struct profile_history* profile;
  HASH_FIND_STR(history, location, profile);
  if (profile == NULL) {
    profile = (struct profile_history*)malloc(sizeof(struct profile_history));

    profile->name = strdup(location);
    profile->call_count = 0;
    profile->total_time_ms = 0.0;
    utarray_new(profile->times, &ut_double_icd);

    HASH_ADD_KEYPTR(hh, history, location, strlen(location), profile);
  }

  pthread_mutex_unlock(&profile_mutex);

  return profile;
}

void TealeafProfileNode::RecordProfileEntry(double duration_ms) {
  pthread_mutex_lock(&profile_mutex);

  node->call_count++;
  node->total_time_ms += duration_ms;
  utarray_push_back(node->times, &duration_ms);

  pthread_mutex_unlock(&profile_mutex);
}
