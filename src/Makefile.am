bin_PROGRAMS = tcoin
AM_CPPFLAGS = $(MYSQL_CFLAGS) $(MYSQLPP_CFLAGS) $(THRIFT_CFLAGS) $(CONFIGPP_CFLAGS) $(MEMCACHED_CFFAGS) $(BOOST_CFLAGS)
tcoin_LDADD = $(MYSQL_LIBS) $(MYSQLPP_LIBS) $(THRIFT_LIBS) $(THRIFTNB_LIBS) $(EVENT_LIBS)  $(CONFIGPP_LIBS) $(MEMCACHED_LIBS) $(BOOST_LIBS)
tcoin_SOURCES = gen-cpp/tcoin.cpp gen-cpp/tcoin_constants.cpp gen-cpp/tcoin_server.cpp gen-cpp/tcoin_types.cpp gen-cpp/tcoin.h gen-cpp/tcoin_constants.h gen-cpp/tcoin_types.h tcoin_dal.cpp tcoin_dal.h tcoin_memcache.cpp tcoin_memcache.h