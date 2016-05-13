#ifndef TCOIN_MEMCACHE_H_INCLUDED
#define TCOIN_MEMCACHE_H_INCLUDED
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <map>

#include <string.h>

#include <libmemcached/memcached.hpp>
#include <libmemcached/memcached_util.h>

using namespace std;
using namespace memcache;

struct mc_setting{
  std::string server;
  int port;
};

class tcoinCache
{
public:
  static tcoinCache &singleton()
  {
    static tcoinCache instance;
    return instance;
  }

  void init(bool _cache_enabled, std::vector<mc_setting> &_mc, int init_pool_size, int max_pool_size){
    cache_enabled=_cache_enabled;
      if (!cache_enabled){
        return;
      }
      memcached_create(&memc);
      for (unsigned int i=0; i<_mc.size(); i++){
        if (memcached_server_add(&memc, _mc[i].server.c_str(), _mc[i].port)!= MEMCACHED_SUCCESS){
          fprintf(stderr, "Failed to add %s:%d to the server pool\n", _mc[i].server.c_str(), _mc[i].port);
          memcached_free(&memc);
          cache_enabled=false;
          return;
        }
      }

      pool=memcached_pool_create(&memc, init_pool_size, max_pool_size);
      if (pool == NULL) {
          fprintf(stderr, "Failed to create connection pool\n");
          memcached_free(&memc);
          cache_enabled=false;
          return;
      }
  }

  bool get(const std::string &key, std::vector<char> &ret_val) throw (Error)
  {
    if (!cache_enabled){
      return false;
    }
    uint32_t flags= 0;
    memcached_return_t rc;
    size_t value_length= 0;

    if (key.empty())
    {
      throw(Error("the key supplied is empty!", false));
    }

    memcached_st* mem = memcache_get_instance_from_pool();
    if (mem==NULL){
      fprintf(stderr, "Failed to get the memcached instance from pool!\n");
      return false;
    }

    char *value= memcached_get(mem, key.c_str(), key.length(), &value_length, &flags, &rc);

    if (memcached_pool_push(pool, mem) != MEMCACHED_SUCCESS) {
        fprintf(stderr, "Failed to release the memcached instance to pool!\n");
    }

    if (value != NULL && ret_val.empty()) {
      ret_val.reserve(value_length);
      ret_val.assign(value, value + value_length);
      free(value);
      return true;
    }
    return false;
  }

  bool set(const std::string &key, const std::vector<char> &value, time_t expiration, uint32_t flags) throw(Error)
  {
    if (!cache_enabled){
      return false;
    }

    if (key.empty() || value.empty())
    {
      throw(Error("the key or value supplied is empty!", false));
    }

    memcached_st* mem = memcache_get_instance_from_pool();
    if (mem==NULL){
      fprintf(stderr, "Failed to get the memcached instance from pool!\n");
      return false;
    }

    memcached_return_t rc= memcached_set(mem, key.c_str(), key.length(), &value[0], value.size(), expiration, flags);

    if (memcached_pool_push(pool, mem) != MEMCACHED_SUCCESS) {
        fprintf(stderr, "Failed to release the memcached instance to pool!\n");
    }

    return (rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  bool remove(const std::string &key) throw(Error)
  {
    if (!cache_enabled){
      return false;
    }

    if (key.empty())
    {
      throw(Error("the key supplied is empty!", false));
    }

    memcached_st* mem = memcache_get_instance_from_pool();
    if (mem==NULL){
      fprintf(stderr, "Failed to get the memcached instance from pool!\n");
      return false;
    }
    memcached_return_t rc= memcached_delete(mem, key.c_str(), key.length(), 0);
    if (memcached_pool_push(pool, mem) != MEMCACHED_SUCCESS) {
        fprintf(stderr, "Failed to release the memcached instance to pool!\n");
    }
    return (rc == MEMCACHED_SUCCESS);
  }
private:
  tcoinCache(){
     cache_enabled=false;
  }

  ~tcoinCache(){
      if (cache_enabled){
        memcached_pool_destroy(pool);
        memcached_free(&memc);
      }
  }

  memcached_st* memcache_get_instance_from_pool(){
    memcached_return rc;
    return memcached_pool_pop(pool, true, &rc);
  }

  memcached_st memc;
  bool cache_enabled;
  memcached_pool_st* pool;
};
#endif // TCOIN_MEMCACHE_H_INCLUDED
