#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT(tcoin, 2.0.2, http://tcoin.tamtay.vn)
AM_INIT_AUTOMAKE(tcoin, 2.0.2)

# Checks for programs.
AC_PROG_CXX

# MySQL
AC_ARG_WITH([mysql-include-path],
  	[AS_HELP_STRING([--with-mysql-include-path], [Location of the MySQL headers, defaults to /usr/include/mysql])],
  	[MYSQL_CFLAGS="-I$withval"],
  	[MYSQL_CFLAGS='-I/usr/include/mysql'])
AC_SUBST([MYSQL_CFLAGS])

AC_ARG_WITH([mysql-lib-path],
  [AS_HELP_STRING([--with-mysql-lib-path], [Location of the MySQL libraries])],
  [MYSQL_LIBS="-L$withval -lmysqlclient"],
  [MYSQL_LIBS='-L/usr/lib/mysql -lmysqlclient'])
AC_SUBST([MYSQL_LIBS])

# thrift
AC_ARG_WITH([thrift-include-path],
  	[AS_HELP_STRING([--with-thrift-include-path], [Location of the thrift headers, defaults to /usr/local/include/thrift])],
  	[THRIFT_CFLAGS="-I$withval"],
  	[THRIFT_CFLAGS='-I/usr/local/include/thrift'])
AC_SUBST([THRIFT_CFLAGS])

AC_ARG_WITH([thrift-lib-path],
  [AS_HELP_STRING([--with-thrift-lib-path], [Location of the thrift libraries])],
  [THRIFT_LIBS="-L$withval -lthrift"],
  [THRIFT_LIBS='-L/usr/local/lib -lthrift'])
AC_SUBST([THRIFT_LIBS])

AC_ARG_WITH([thriftnb-lib-path],
  [AS_HELP_STRING([--with-thriftnb-lib-path], [Location of the thrift nonblock libraries])],
  [THRIFTNB_LIBS="-L$withval -lthriftnb"],
  [THRIFTNB_LIBS='-L/usr/local/lib -lthriftnb'])
AC_SUBST([THRIFTNB_LIBS])

AC_ARG_WITH([libevent-lib-path],
  [AS_HELP_STRING([--with-libevent-lib-path], [Location of the libevent libraries])],
  [EVENT_LIBS="-L$withval -levent"],
  [EVENT_LIBS='-L/usr/local/lib -levent'])
AC_SUBST([EVENT_LIBS])

# Mysqlpp
AC_ARG_WITH([mysqlpp-include-path],
  	[AS_HELP_STRING([--with-mysqlpp-include-path], [Location of the mysqlpp headers, defaults to /usr/local/include/mysql++])],
  	[MYSQLPP_CFLAGS="-I$withval"],
  	[MYSQLPP_CFLAGS='-I/usr/local/include/mysql++'])
AC_SUBST([MYSQLPP_CFLAGS])

AC_ARG_WITH([mysqlpp-lib-path],
  [AS_HELP_STRING([--with-mysqlpp-lib-path], [Location of the mysqlpp libraries])],
  [MYSQLPP_LIBS="-L$withval -lmysqlpp"],
  [MYSQLPP_LIBS='-L/usr/local/lib -lmysqlpp'])
AC_SUBST([MYSQLPP_LIBS])

# Libconfig
AC_ARG_WITH([configpp-include-path],
  	[AS_HELP_STRING([--with-configpp-include-path], [Location of the libconfig++ headers, defaults to /usr/local/include])],
  	[CONFIGPP_CFLAGS="-I$withval"],
  	[CONFIGPP_CFLAGS='-I/usr/local/include'])
AC_SUBST([CONFIGPP_CFLAGS])

AC_ARG_WITH([configpp-lib-path],
  [AS_HELP_STRING([--with-configpp-lib-path], [Location of the configpp libraries])],
  [CONFIGPP_LIBS="-L$withval -lconfig++"],
  [CONFIGPP_LIBS='-L/usr/local/lib -lconfig++'])
AC_SUBST([CONFIGPP_LIBS])

# Libmemcached
AC_ARG_WITH([memcached-include-path],
  	[AS_HELP_STRING([--with-memcached-include-path], [Location of the libmemcached headers, defaults to /usr/local/include])],
  	[MEMCACHED_CFLAGS="-I$withval"],
  	[MEMCACHED_CFLAGS='-I/usr/local/include'])
AC_SUBST([MEMCACHED_CFLAGS])

AC_ARG_WITH([memcached-lib-path],
  [AS_HELP_STRING([--with-memcached-lib-path], [Location of the libmemcached libraries])],
  [MEMCACHED_LIBS="-L$withval -lmemcached -lmemcachedutil"],
  [MEMCACHED_LIBS='-L/usr/local/lib -lmemcached -lmemcachedutil'])
AC_SUBST([MEMCACHED_LIBS])

# Libboost
AC_ARG_WITH([boost-include-path],
  	[AS_HELP_STRING([--with-boost-include-path], [Location of the lib boost headers, defaults to /usr/include])],
  	[BOOST_CFLAGS="-I$withval"],
  	[BOOST_CFLAGS='-I/usr/include'])
AC_SUBST([BOOST_CFLAGS])

AC_ARG_WITH([boost-lib-path],
  [AS_HELP_STRING([--with-boost-lib-path], [Location of the lib boost libraries])],
  [BOOST_LIBS="-L$withval -lboost_serialization"],
  [BOOST_LIBS='-L/usr/lib -lboost_serialization'])
AC_SUBST([BOOST_LIBS])
AC_OUTPUT(Makefile src/Makefile)
AC_OUTPUT
