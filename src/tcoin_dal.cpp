#include "tcoin_dal.h"
#include <sys/time.h>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <iostream>
#include <sstream>
#include <transport/TBufferTransports.h>
#include <protocol/TBinaryProtocol.h>
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using boost::shared_ptr;

mysqlpp::Connection tcoin_dal::getConnection(const DB_TYPE db_type) {
  mysqlpp::Connection conn;

  try{
    if (db_type==DB_MASTER || slavedb.size()==0){
      conn.connect(masterdb.dbname.c_str(), masterdb.server.c_str(), masterdb.user.c_str(), masterdb.password.c_str());
    }
    else{
      srand((unsigned)time(0));
      int n=slavedb.size();
      int index=rand() % n;
      db_setting db=slavedb[index];
      conn.connect(db.dbname.c_str(), db.server.c_str(), db.user.c_str(), db.password.c_str());
    }
    mysqlpp::Query query = conn.query("SET NAMES 'utf8'");
    query.execute();
  }
  catch(std::exception& ex){
    throw getException(ex, EXECUTION_FAILED);
  }

  return conn;
}

void tcoin_dal::closeConnection(mysqlpp::Connection& conn){
    if (conn.connected())
    {
      conn.disconnect();
    }
}

tcoinException tcoin_dal::getException(std::exception& ex, ErrorCode code){
  tcoinException te;
  te.what=code;
  te.why=ex.what();
  return te;
}

APP_INFO tcoin_dal::getApp(const int app, mysqlpp::Connection& conn, bool check_memcache){
  APP_INFO a;
  a.id=0;

  if (check_memcache){
    a=mc_getApp(app);
    if (a.id>0){
      return a;
    }
  }

  mysqlpp::Query query=conn.query("");
  query.parse();
  mysqlpp::StoreQueryResult res=query.store(app);
  if (res.num_rows()==1){
    a.id=app;
    a.app_name=(std::string)res[0]["app_name"];
    a.status=atoi(res[0]["status"]);
    a.note=(std::string)res[0]["note"];
    a.transfer=atoi(res[0]["transfer"]);
    a.transfer_cost=atof(res[0]["transfer_cost"]);
    a.cost_type=atoi(res[0]["cost_type"]);
    a.system_acc=atoi(res[0]["system_acc"]);
    a.unit=(std::string)res[0]["unit"];
    a.pid=atoi(res[0]["parent_app"]);
    a.ext=atoi(res[0]["external_app"]);

    if (check_memcache){
      mc_setApp(app, a);
    }

  }
  return a;
}

int tcoin_dal::createAccount(const int uid, const int app, const std::string& name, mysqlpp::Connection& conn){
  int t=time(0);
  mysqlpp::Query query = conn.query("");
  query.parse();
  mysqlpp::SimpleResult res=query.execute(uid, name, app, 0, 0, t, 0, (int)STATUS_ENABLED);
  return res.insert_id();
}

ACC_INFO tcoin_dal::getAccount(const int uid, const int app, mysqlpp::Connection& conn, bool check_memcache){
  ACC_INFO acc;
  acc.uid=0;
  if (check_memcache){
    acc=mc_getAccount(uid, app);
    if (acc.uid>0){
      return acc;
    }
  }

  mysqlpp::Query query = conn.query("");
  query.parse();
  mysqlpp::StoreQueryResult res = query.store(uid, app);

  if (res.num_rows()==1)
  {
    acc.id=atoi(res[0]["id"]);
    acc.uid=uid;
    acc.app=app;
    acc.name=(std::string)res[0]["uname"];
    acc.balance=atof(res[0]["balance"]);
    acc.promotion=atof(res[0]["promotion"]);
    acc.created=atoi(res[0]["created"]);
    acc.changed=atoi(res[0]["changed"]);
    acc.status=(ACC_STATUS)atoi(res[0]["status"]);

    if (check_memcache){
        mc_setAccount(uid, app, acc);
    }

  }
  return acc;
}

/*
void tcoin_dal::setAccountStatus(const int uid, const int app, const int status, mysqlpp::Connection& conn){
  mysqlpp::Query query = conn.query("UPDATE coin_balance SET status=%0 WHERE uid=%1 AND app=%2");
  query.parse();
  query.execute(status, uid, app);
}
*/
void tcoin_dal::updateBalance(ACC_INFO &acc, double amount, mysqlpp::Connection& conn, double promotion){
  int updated=time(0);

  acc.balance+=amount;
  acc.promotion+=promotion;
  acc.changed=updated;
  mc_setAccount(acc.uid, acc.app, acc);

  mysqlpp::Query query = conn.query("");
  query.parse();
  query.execute(amount, promotion, updated, acc.id);
}

//double tcoin_dal::getAccountBalance(const int uid, const int app, mysqlpp::Connection& conn){
//  ACC_INFO acc=getAccount(uid, app, conn);
//  return acc.balance;
//}

int tcoin_dal::createTransaction(int trans_type, int trans_date, const double amount, const double promotion, const std::string& description, const std::string& system_note, int credit_uid, int debit_uid, const std::string& credit_name, const std::string& debit_name, const int credit_app, const int debit_app, mysqlpp::Connection& conn){
  mysqlpp::Query query = conn.query("");
  query.parse();
  mysqlpp::SimpleResult res=query.execute(trans_date, trans_type, amount, promotion, description, system_note, credit_uid, debit_uid, credit_name, debit_name, credit_app, debit_app);
  return res.insert_id();
}

int tcoin_dal::createTransDetail(int tid, const int app, int trans_date, int trans_type, int journal, const std::string& description, const int uid, const std::string& uname, const double amount, double promotion, double balance, double promotion_balance, mysqlpp::Connection& conn){
  mysqlpp::Query query = conn.query("");
  query.parse();
  mysqlpp::SimpleResult res=query.execute(tid, app, trans_date, trans_type, journal, description, uid, uname, amount, promotion, balance, promotion_balance);
  return res.insert_id();
}

EX_RATE tcoin_dal::getExchangeRate(const int app1, const int app2, mysqlpp::Connection& conn, bool check_memcache){
  EX_RATE rate;
  rate.id=0;
  rate.app1=app1;
  rate.app2=app2;
  rate.rate=0;
  rate.cost=0;
  rate.cost_type=0;

  if (check_memcache){
    rate=mc_getExRate(app1, app2);
    if (rate.id>0){
      return rate;
    }
  }

  mysqlpp::Query query = conn.query("");
  query.parse();
  mysqlpp::StoreQueryResult res = query.store(app1, app2);

  if (res.num_rows()>0){
    rate.id=atoi(res[0]["id"]);
    rate.app1=atoi(res[0]["app1"]);
    rate.app2=atoi(res[0]["app2"]);
    rate.rate=atof(res[0]["ex"]);
    rate.cost=atof(res[0]["cost"]);
    rate.cost_type=atoi(res[0]["cost_type"]);

    if (check_memcache){
      mc_setExRate(app1, app2, rate);
    }
  }
  return rate;
}

int tcoin_dal::getAppStatus(const int app, mysqlpp::Connection& conn){
  APP_INFO ai=getApp(app, conn);
  if (ai.id==0){
    return -1;
  }
  return (int)ai.status;
}

//*Memcache funtions
void tcoin_dal::mc_serialize(ACC_INFO& acc, std::vector<char> &ret_value){
  try{
    /*
    shared_ptr<TMemoryBuffer> strBuffer(new TMemoryBuffer());
    shared_ptr<TBinaryProtocol> binaryProtcol(new TBinaryProtocol(strBuffer));
    acc.write(binaryProtcol.get());
    std::string data=strBuffer.get()->getBufferAsString();
    ret_value.reserve(data.size());
    ret_value.insert(ret_value.begin(), data.begin(), data.end());
    */
    std::ostringstream os(std::ostringstream::out);
    {
      boost::archive::binary_oarchive oa(os);
      oa << acc.id;
      oa << acc.uid;
      oa << acc.name;
      oa << acc.app;
      oa << acc.app_name;
      oa << acc.balance;
      oa << acc.promotion;
      oa << acc.created;
      oa << acc.changed;
      oa << acc.status;
    }
    std::string data=os.str();
    ret_value.reserve(data.size());
    ret_value.insert(ret_value.begin(), data.begin(), data.end());
  }
  catch(std::exception& ex){
    ret_value.clear();
  }
}

void tcoin_dal::mc_unserialize(std::vector<char> &data, ACC_INFO& acc){
  try{
     /*
     shared_ptr<TMemoryBuffer> strBuffer(new TMemoryBuffer());
     shared_ptr<TBinaryProtocol> binaryProtcol(new TBinaryProtocol(strBuffer));
     strBuffer->resetBuffer((uint8_t*) &data[0], data.size());
     acc.read(binaryProtcol.get());
     */
    std::string buff(data.begin(), data.end());
    std::istringstream is(buff, std::ostringstream::in);
    {
      boost::archive::binary_iarchive ia(is);
      ia >> acc.id;
      ia >> acc.uid;
      ia >> acc.name;
      ia >> acc.app;
      ia >> acc.app_name;
      ia >> acc.balance;
      ia >> acc.promotion;
      ia >> acc.created;
      ia >> acc.changed;
      ia >> acc.status;
    }
  }
  catch(std::exception& ex){
    acc.uid=0;
    acc.app=0;
  }
}

void tcoin_dal::mc_serialize(APP_INFO& app, std::vector<char> &ret_value){
  try{
    std::ostringstream os(std::ostringstream::out);
    {
      boost::archive::binary_oarchive oa(os);
      oa << app.id;
      oa << app.app_name;
      oa << app.status;
      oa << app.note;
      oa << app.transfer;
      oa << app.transfer_cost;
      oa << app.cost_type;
      oa << app.system_acc;
      oa << app.unit;
      oa << app.pid;
      oa << app.ext;
    }
    std::string data=os.str();
    ret_value.reserve(data.size());
    ret_value.insert(ret_value.begin(), data.begin(), data.end());
  }
  catch(std::exception& ex){
    ret_value.clear();
  }
}

void tcoin_dal::mc_unserialize(std::vector<char> &data, APP_INFO& app){
  try{
    std::string buff(data.begin(), data.end());
    std::istringstream is(buff, std::ostringstream::in);
    {
      boost::archive::binary_iarchive ia(is);
      ia >> app.id;
      ia >> app.app_name;
      ia >> app.status;
      ia >> app.note;
      ia >> app.transfer;
      ia >> app.transfer_cost;
      ia >> app.cost_type;
      ia >> app.system_acc;
      ia >> app.unit;
      ia >> app.pid;
      ia >> app.ext;
    }
  }
  catch(std::exception& ex){
    app.id=0;
  }
}

void tcoin_dal::mc_serialize(EX_RATE& ex, std::vector<char> &ret_value){
  try{
      std::ostringstream os(std::ostringstream::out);
      {
        boost::archive::binary_oarchive oa(os);
        oa << ex.id;
        oa << ex.app1;
        oa << ex.app2;
        oa << ex.rate;
        oa << ex.cost;
        oa << ex.cost_type;
      }
      std::string data=os.str();
      ret_value.reserve(data.size());
      ret_value.insert(ret_value.begin(), data.begin(), data.end());
  }
  catch(std::exception& e){
     ret_value.clear();
  }
}

void tcoin_dal::mc_unserialize(std::vector<char> &data, EX_RATE& ex){
  try{
    std::string buff(data.begin(), data.end());
    std::istringstream is(buff, std::ostringstream::in);
    {
      boost::archive::binary_iarchive ia(is);
      ia >> ex.id;
      ia >> ex.app1;
      ia >> ex.app2;
      ia >> ex.rate;
      ia >> ex.cost;
      ia >> ex.cost_type;
    }
  }
  catch(std::exception& e){
    ex.id=0;
    ex.app1=0;
    ex.app2=0;
  }
}

std::string tcoin_dal::makeKey(const std::string &prefix, const int param) {
  std::string key=prefix;
  ostringstream oss;
  oss << param;
  key += oss.str();
  return key;
}

std::string tcoin_dal::makeKey(const std::string &prefix, const int uid, const int app) {
  std::string key=prefix;
  ostringstream oss;
  oss << uid << "_" << app;
  key += oss.str();
  return key;
}

ACC_INFO tcoin_dal::mc_getAccount(const int uid, const int app) {
  ACC_INFO acc;
  std::string key=makeKey("tc_", uid, app);
  std::vector<char> ret_value;
  tcoinCache::singleton().get(key, ret_value);
  if (!ret_value.empty()){
    mc_unserialize(ret_value, acc);
  }
  return acc;
}

void tcoin_dal::mc_setAccount(const int uid, const int app, ACC_INFO &acc) {
  std::string key=makeKey("tc_", uid, app);
  std::vector<char> raw_data;
  mc_serialize(acc, raw_data);
  if (!raw_data.empty()){
    time_t expiry= 1800;
    uint32_t flags= 0;
    tcoinCache::singleton().set(key, raw_data, expiry, flags);
  }
}

APP_INFO tcoin_dal::mc_getApp(const int app){
  APP_INFO ai;
  std::string key=makeKey("app_", app);
  std::vector<char> ret_value;
  tcoinCache::singleton().get(key, ret_value);
  if (!ret_value.empty()){
    mc_unserialize(ret_value, ai);
  }
  return ai;
}

void tcoin_dal::mc_setApp(const int app, APP_INFO& ai){
  std::string key=makeKey("app_", app);
  std::vector<char> raw_data;
  mc_serialize(ai, raw_data);
  if (!raw_data.empty()){
    time_t expiry= 0;
    uint32_t flags= 0;
    tcoinCache::singleton().set(key, raw_data, expiry, flags);
  }
}

EX_RATE tcoin_dal::mc_getExRate(const int app1, const int app2){
  EX_RATE ex;
  std::string key=makeKey("ex_", app1, app2);
  std::vector<char> ret_value;
  tcoinCache::singleton().get(key, ret_value);
  if (!ret_value.empty()){
    mc_unserialize(ret_value, ex);
  }
  return ex;
}

void tcoin_dal::mc_setExRate(const int app1, const int app2, EX_RATE& ex){
  std::string key=makeKey("ex_", app1, app2);
  std::vector<char> raw_data;
  mc_serialize(ex, raw_data);
  if (!raw_data.empty()){
    time_t expiry= 0;
    uint32_t flags= 0;
    tcoinCache::singleton().set(key, raw_data, expiry, flags);
  }
}
//*/

//public functions
ACC_INFO tcoin_dal::tc_getAccount(const int uid, const int app){
  ACC_INFO acc;
  try{
    acc=mc_getAccount(uid, app);
    if (acc.uid==0){
      mysqlpp::Connection conn=getConnection(DB_MASTER);
      acc=getAccount(uid, app, conn, false);
      if (acc.uid>0){
        mc_setAccount(uid, app, acc);
      }
    }
  }
  catch(std::exception& ex){
    throw getException(ex, EXECUTION_FAILED);
  }
  return acc;
}

int tcoin_dal::tc_createAccount(const int uid, const int app, const std::string& name){
  try{
    mysqlpp::Connection conn=getConnection(DB_MASTER);
    ACC_INFO acc=getAccount(uid, app, conn);
    if (acc.uid>0){
      tcoinException te;
      te.what = ACCOUNT_EXISTS;
      te.why = "Account does exists";
      throw te;
    }
    return createAccount(uid, app, name, conn);
  }
  catch(std::exception& ex){
    throw getException(ex, EXECUTION_FAILED);
  }
  return 0;
}

int tcoin_dal::tc_cashin(const int uid, const int app, const double amount, const std::string& description, const double promotion) {
  mysqlpp::Connection conn=getConnection(DB_MASTER);
  int tid=0;
  try{
    ACC_INFO acc=getAccount(uid, app, conn);
    if (acc.uid==0){
      tcoinException te;
      te.what = ACCOUNT_NOT_EXISTS;
      te.why = "Account does not exists";
      throw te;
    }

    if (acc.status==STATUS_LOCKED){
      tcoinException te;
      te.what = ACCOUNT_LOCKED;
      te.why = "Account locked";
      throw te;
    }

    int status=getAppStatus(app, conn);
    if (status!=1){
      tcoinException te;
      te.what = EXECUTION_FAILED;
      te.why = "Could not cashin to this app";
      throw te;
    }

    //mysqlpp::Transaction trans(conn);
    int trans_date=time(0);
    double balance=acc.balance + amount;
    double prom_bal=acc.promotion + promotion;
    char str[200];
    sprintf(str,"CASHING IN");
    std::string system_note(str);
    tid=createTransaction((int)CASHIN, trans_date, amount, promotion, description, system_note, 0, uid, "", acc.name, 0, app, conn);
    createTransDetail(tid, app, trans_date, (int)CASHIN, (int)DEBIT, description, uid, acc.name, amount, promotion, balance, prom_bal, conn);
    updateBalance(acc, amount, conn, promotion);
    //trans.commit();
  }
  catch(tcoinException& te){
    throw te;
  }
  catch(std::exception& ex){
    throw getException(ex, EXECUTION_FAILED);
  }
  return tid;
}

int tcoin_dal::tc_transfer(const int app, const int uid1, const int uid2, const std::string& uname2, const double amount, const std::string& description){
  mysqlpp::Connection conn=getConnection(DB_MASTER);
  int tid=0;
  try{
    ACC_INFO acc1=getAccount(uid1, app, conn);
    if (acc1.uid==0){
      tcoinException te;
      te.what = ACCOUNT_NOT_EXISTS;
      te.why = "Credit account does not exists";
      throw te;
    }

    if (acc1.status==STATUS_LOCKED){
      tcoinException te;
      te.what = ACCOUNT_LOCKED;
      te.why = "Credit account locked";
      throw te;
    }

    if (acc1.balance<amount){
      tcoinException te;
      te.what = NOT_ENOUGH_MONEY;
      te.why = "Credit account had not anough money";
      throw te;
    }

    ACC_INFO acc2=getAccount(uid2, app, conn);
    if (acc2.uid==0){
      acc2.id=createAccount(uid2, app, uname2, conn);
      acc2.uid=uid2;
      acc2.name=uname2;
      acc2.balance=0;
      acc2.promotion=0;
    }
    if (acc2.id==0){
      tcoinException te;
      te.what = ACCOUNT_NOT_EXISTS;
      te.why = "Debit account does not exists and creation failed";
      throw te;
    }

    if (acc2.status==STATUS_LOCKED){
      tcoinException te;
      te.what = ACCOUNT_LOCKED;
      te.why = "Debit account locked";
      throw te;
    }

    APP_INFO ai=getApp(app, conn);
    if (ai.id==0){
      tcoinException te;
      te.what = EXECUTION_FAILED;
      te.why = "App does not exist";
      throw te;
    }

    if (ai.transfer==0){
      tcoinException te;
      te.what = EXECUTION_FAILED;
      te.why = "Transfer is not allowed in this app";
      throw te;
    }

    double cost=0;
    if  (ai.transfer_cost>0){
      if (ai.cost_type==COST_BY_PERCENT){
        cost=amount*ai.transfer_cost;
      }
      else{
        cost=ai.transfer_cost;
      }
    }

    double debit_amount=amount - cost;
    if (debit_amount<=0){
      tcoinException te;
      te.what = NOT_ENOUGH_MONEY;
      te.why = "Credit account had not anough money to cost";
      throw te;
    }

    //mysqlpp::Transaction trans(conn);
    int trans_date=time(0);
    char str[200];
    sprintf(str,"TRANSFER %.0f\nAPP: %s\nFrom %s to %s\nCost: %.0f", amount, ai.app_name.c_str(), acc1.name.c_str(), acc2.name.c_str(), cost);
    std::string system_note(str);
    tid=createTransaction((int)TRANSFER, trans_date, amount, 0, description, system_note, uid1, uid2, acc1.name, acc2.name, app, app, conn);
    double balance=acc1.balance-amount;
    double debit_balance=acc2.balance + debit_amount;
    createTransDetail(tid, app, trans_date, (int)TRANSFER, (int)CREDIT, description, uid1, acc1.name, -1*amount, 0, balance, 0, conn);
    createTransDetail(tid, app, trans_date, (int)TRANSFER, (int)DEBIT, description, uid2, acc2.name, debit_amount, 0, debit_balance, 0, conn);
    updateBalance(acc1, -1*amount, conn);
    updateBalance(acc2, debit_amount, conn);

    if (cost>0){
      sprintf(str,"TRANSFER COST\nTRANSACTION: %d\nFROM %s to %s", tid, acc1.name.c_str(), acc2.name.c_str());
      system_note=(std::string)str;

      ACC_INFO system_acc=getAccount(ai.system_acc, app, conn);
      balance=system_acc.balance + cost;
      createTransDetail(tid, app, trans_date, (int)TRANSFER, (int)DEBIT, system_note, system_acc.uid, system_acc.name, cost, 0, balance, 0, conn);
      updateBalance(system_acc, cost, conn);
    }

    //trans.commit();
  }
  catch(tcoinException& te){
    throw te;
  }
  catch(std::exception& ex){
    throw getException(ex, EXECUTION_FAILED);
  }
  return tid;
}

int tcoin_dal::tc_transfer2(const int app, const int uid1, const int uid2, const std::string& uname2, const double amount, const std::string& description, const double cost){
  mysqlpp::Connection conn=getConnection(DB_MASTER);
  int tid=0;
  try{
    ACC_INFO acc1=getAccount(uid1, app, conn);
    if (acc1.uid==0){
      tcoinException te;
      te.what = ACCOUNT_NOT_EXISTS;
      te.why = "Credit account does not exists";
      throw te;
    }

    if (acc1.status==STATUS_LOCKED){
      tcoinException te;
      te.what = ACCOUNT_LOCKED;
      te.why = "Credit account locked";
      throw te;
    }

    if (acc1.balance<amount){
      tcoinException te;
      te.what = NOT_ENOUGH_MONEY;
      te.why = "Credit account had not anough money";
      throw te;
    }

    ACC_INFO acc2=getAccount(uid2, app, conn);
    if (acc2.uid==0){
      acc2.id=createAccount(uid2, app, uname2, conn);
      acc2.uid=uid2;
      acc2.name=uname2;
      acc2.balance=0;
      acc2.promotion=0;
    }
    if (acc2.id==0){
      tcoinException te;
      te.what = ACCOUNT_NOT_EXISTS;
      te.why = "Debit account does not exists and creation failed";
      throw te;
    }

    if (acc2.status==STATUS_LOCKED){
      tcoinException te;
      te.what = ACCOUNT_LOCKED;
      te.why = "Debit account locked";
      throw te;
    }

    APP_INFO ai=getApp(app, conn);
    if (ai.id==0){
      tcoinException te;
      te.what = EXECUTION_FAILED;
      te.why = "App does not exist";
      throw te;
    }

    if (ai.transfer==0){
      tcoinException te;
      te.what = EXECUTION_FAILED;
      te.why = "Transfer is not allowed in this app";
      throw te;
    }

    double debit_amount=amount - cost;
    if (debit_amount<=0){
      tcoinException te;
      te.what = NOT_ENOUGH_MONEY;
      te.why = "Credit account had not anough money to cost";
      throw te;
    }

    //mysqlpp::Transaction trans(conn);
    int trans_date=time(0);
    char str[200];
    sprintf(str,"TRANSFER %.0f\nAPP: %s\nFrom %s to %s\nCost: %.0f", amount, ai.app_name.c_str(), acc1.name.c_str(), acc2.name.c_str(), cost);
    std::string system_note(str);
    tid=createTransaction((int)TRANSFER, trans_date, amount, 0, description, system_note, uid1, uid2, acc1.name, acc2.name, app, app, conn);
    double balance=acc1.balance-amount;
    double debit_balance=acc2.balance + debit_amount;
    createTransDetail(tid, app, trans_date, (int)TRANSFER, (int)CREDIT, description, uid1, acc1.name, -1*amount, 0, balance, 0, conn);
    createTransDetail(tid, app, trans_date, (int)TRANSFER, (int)DEBIT, description, uid2, acc2.name, debit_amount, 0, debit_balance, 0, conn);
    updateBalance(acc1, -1*amount, conn);
    updateBalance(acc2, debit_amount, conn);

    if (cost>0){
      sprintf(str,"TRANSFER COST\nTRANSACTION: %d\nFROM %s to %s", tid, acc1.name.c_str(), acc2.name.c_str());
      system_note=(std::string)str;

      ACC_INFO system_acc=getAccount(ai.system_acc, app, conn);
      balance=system_acc.balance + cost;
      createTransDetail(tid, app, trans_date, (int)TRANSFER, (int)DEBIT, system_note, system_acc.uid, system_acc.name, cost, 0, balance, 0, conn);
      updateBalance(system_acc, cost, conn);
    }

    //trans.commit();
  }
  catch(tcoinException& te){
    throw te;
  }
  catch(std::exception& ex){
    throw getException(ex, EXECUTION_FAILED);
  }
  return tid;
}

int tcoin_dal::tc_exchange(const int uid, const int app1, const int app2, const double amount, const std::string& description, const double promotion){
  mysqlpp::Connection conn=getConnection(DB_MASTER);
  int tid=0;
  try{
    ACC_INFO acc1=getAccount(uid, app1, conn);
    if (acc1.uid==0){
      tcoinException te;
      te.what = ACCOUNT_NOT_EXISTS;
      te.why = "Credit account does not exists";
      throw te;
    }

    if (acc1.status==STATUS_LOCKED){
      tcoinException te;
      te.what = ACCOUNT_LOCKED;
      te.why = "Credit account locked";
      throw te;
    }

    if (acc1.balance<amount){
      tcoinException te;
      te.what = NOT_ENOUGH_MONEY;
      te.why = "Credit account had not anough money";
      throw te;
    }

    APP_INFO ai2=getApp(app2, conn);
    ACC_INFO acc2;

    //if app2 is an external application then get system account
    if (ai2.ext==1){
      acc2=getAccount(ai2.system_acc, app2, conn);
    }
    else{
      acc2=getAccount(uid, app2, conn);
    }

    if (acc2.uid==0){
      if (ai2.ext==0){
        acc2.id=createAccount(uid, app2, acc1.name, conn);
        acc2.uid=uid;
        acc2.name=acc1.name;
        acc2.balance=0;
        acc2.promotion=0;
      }
      else{
        tcoinException te;
        te.what = ACCOUNT_NOT_EXISTS;
        te.why = "System debit account does not exists";
        throw te;
      }
    }
    else{
      if (acc2.status==STATUS_LOCKED){
        tcoinException te;
        te.what = ACCOUNT_LOCKED;
        te.why = "Debit account locked";
        throw te;
      }
    }

    EX_RATE ex=getExchangeRate(app1, app2, conn);
    if (ex.id<=0){
      tcoinException te;
      te.what = OPERATION_NOT_PERMIT;
      te.why = "Exchange is not allow";
      throw te;
    }

    double cost=0;
    if (ex.cost>0){
      if (ex.cost_type==COST_BY_PERCENT){
        cost=ex.cost * amount;
      }
      else{
        cost=ex.cost;
      }
    }

    double amount2=(amount-cost)*ex.rate;
    double promotion2=0;
    if (promotion>0)
    {
      promotion2=(promotion-cost)*ex.rate;
    }
    //mysqlpp::Transaction trans(conn);
    int trans_date=time(0);
    char str[200];
    sprintf(str,"EXCHANGE %.0f\nAPP1: %d\nAMT: %.0f\nAPP2 %d\nRATE: %.0f\nCOST: %.0f", amount, app1, amount2, app2, ex.rate, cost);
    std::string system_note(str);
    tid=createTransaction((int)EXCHANGE, trans_date, amount, promotion, description, system_note, uid, acc2.uid, acc1.name, acc2.name, app1, app2, conn);
    double balance=acc1.balance-amount;
    double prom_bal=acc1.promotion-promotion;
    createTransDetail(tid, app1, trans_date, (int)EXCHANGE, (int)CREDIT, description, uid, acc1.name, -1*amount, promotion, balance, prom_bal, conn);
    balance=acc2.balance+amount2+promotion2;
    prom_bal=0;
    createTransDetail(tid, app2, trans_date, (int)EXCHANGE, (int)DEBIT, description, acc2.uid, acc2.name, amount2 + promotion2, 0, balance, prom_bal, conn);
    updateBalance(acc1, -1*amount, conn, -1*promotion);
    updateBalance(acc2, amount2 + promotion2, conn, 0);

    if (cost>0){
      sprintf(str,"EXCHANGE COST\nTRANSACTION: %d\nFROM %s\nTO %s", tid, acc1.name.c_str(), acc2.name.c_str());
      system_note=(std::string)str;
      APP_INFO ai=getApp(app1, conn);
      ACC_INFO system_acc=getAccount(ai.system_acc, app1, conn);
      balance=system_acc.balance + cost;
      createTransDetail(tid, app1, trans_date, (int)TRANSFER, (int)DEBIT, system_note, system_acc.uid, system_acc.name, cost, 0, balance, 0, conn);
      updateBalance(system_acc, cost, conn, 0);
    }

    //trans.commit();
  }
  catch(tcoinException& te){
    throw te;
  }
  catch(std::exception& ex){
    throw getException(ex, EXECUTION_FAILED);
  }
  return tid;
}

void tcoin_dal::tc_search_user_trans(TRANS_DETAIL_RESULT& _return, const int uid, const int app, const SEARCH_COND& cond, const int page, const int limit){
  mysqlpp::Connection conn=getConnection(DB_SLAVE);

  try{
      std::string sql="";
      std::string str_cond="";
      std::stringstream temp;
      mysqlpp::SQLQueryParms params;
      mysqlpp::Query query=conn.query("");
      int param_index=0;

      temp.str("");
      temp << "uid=%" << param_index;
      mysqlpp::SQLTypeAdapter p1(uid);
      params << p1;
      str_cond=temp.str();
      param_index++;

      temp.str("");
      temp << "app=%" << param_index;
      mysqlpp::SQLTypeAdapter p2(app);
      params << p2;
      str_cond=str_cond + " AND " + temp.str();
      param_index++;

      search_criteria crit(cond);
      int from_date=crit.getInt("from_date", 0);
      if (from_date>0){
        if (str_cond!=""){
          str_cond=str_cond + " AND ";
        }
        temp.str("");
        temp << "trans_date>=%" << param_index;
        mysqlpp::SQLTypeAdapter p(from_date);
        params << p;
        str_cond=str_cond + temp.str();
        param_index++;
      }

      int to_date=crit.getInt("to_date", 0);
      if (to_date>0){
        if (str_cond!="") {
          str_cond=str_cond + " AND ";
        }
        temp.str("");
        temp << "trans_date<=%" << param_index;
        mysqlpp::SQLTypeAdapter p(to_date);
        params << p;
        str_cond=str_cond + temp.str();
        param_index++;
      }

      if (str_cond!="")
      {
        sql = sql + " WHERE " + str_cond;
      }
      sql=sql + " ORDER BY id DESC";

      if (page>0 && limit>0)
      {
        int from=(page-1)*limit;
        temp.str("");
        temp << " LIMIT " << from << ", " << limit;
        sql=sql + temp.str();
      }

      query << sql;
      query.parse();

      mysqlpp::StoreQueryResult res=query.store(params);

      int n=res.num_rows();
      _return.trans_list.reserve(n);
      for (int i=0; i<n; i++)
      {
        TRANS_DETAIL t;

        t.tid=atoi(res[i]["tid"]);
        t.trans_date=atoi(res[i]["trans_date"]);
        t.trans_type=(TRANS_TYPE)atoi(res[i]["trans_type"]);
        t.app=app;
        t.amount=atof(res[i]["amount"]);
        t.balance=atof(res[i]["balance"]);
        t.promotion=atof(res[i]["promotion"]);
        t.promotion_balance=atof(res[i]["promotion_balance"]);
        t.description=(std::string)res[i]["description"];
        t.uid=uid;
        t.uname=(std::string)res[i]["uname"];
        t.journal=(JOURNAL)atoi(res[i]["journal"]);
        _return.trans_list.push_back(t);
      }
  }
  catch(std::exception& ex){
    throw getException(ex, EXECUTION_FAILED);
  }
}

EX_RATE tcoin_dal::tc_getExchangeRate(const int app1, const int app2){
  EX_RATE ex;
  ex.id=0;
  ex.app1=app1;
  ex.app2=app2;
  ex.rate=0;
  ex=mc_getExRate(app1, app2);
  if (ex.id>0){
    return ex;
  }
  mysqlpp::Connection conn=getConnection(DB_SLAVE);
  try{
    ex=getExchangeRate(app1, app2, conn, false);
    if (ex.id>0){
      mc_setExRate(app1, app2, ex);
    }
  }
  catch(std::exception& e){
    throw getException(e, EXECUTION_FAILED);
  }

  return ex;
}

void tcoin_dal::tc_getUserAccountList(std::vector<ACC_INFO> & _return, const int32_t uid){
  mysqlpp::Connection conn=getConnection(DB_SLAVE);
  try{
    mysqlpp::Query query=conn.query("");
    query.parse();
    mysqlpp::StoreQueryResult res = query.store(uid);
    int n=res.num_rows();
     _return.reserve(n);
    for (int i=0; i<n; i++){
      ACC_INFO ai;
      ai.id=atoi(res[i]["id"]);
      ai.uid=uid;
      ai.app=atoi(res[i]["app"]);
      ai.app_name=(std::string)(res[i]["app_name"]);
      ai.balance=atof(res[i]["balance"]);
      ai.name=(std::string)res[0]["uname"];
      ai.promotion=atof(res[0]["promotion"]);
      ai.created=atoi(res[0]["created"]);
      ai.changed=atoi(res[0]["changed"]);
      ai.status=(ACC_STATUS)atoi(res[0]["status"]);
      _return.push_back(ai);
    }
  }
  catch(std::exception& ex){
    throw getException(ex, EXECUTION_FAILED);
  }
}

void tcoin_dal::tc_getTransferableUserAccountList(std::vector<ACC_INFO> & _return, const int32_t uid){
  mysqlpp::Connection conn=getConnection(DB_SLAVE);
  try{
    mysqlpp::Query query=conn.query("");
    query.parse();
    mysqlpp::StoreQueryResult res = query.store(uid);
    int n=res.num_rows();
     _return.reserve(n);
    for (int i=0; i<n; i++){
      ACC_INFO ai;
      ai.id=atoi(res[i]["id"]);
      ai.uid=uid;
      ai.app=atoi(res[i]["app"]);
      ai.app_name=(std::string)(res[i]["app_name"]);
      ai.balance=atof(res[i]["balance"]);
      ai.name=(std::string)res[0]["uname"];
      ai.promotion=atof(res[0]["promotion"]);
      ai.created=atoi(res[0]["created"]);
      ai.changed=atoi(res[0]["changed"]);
      ai.status=(ACC_STATUS)atoi(res[0]["status"]);
      _return.push_back(ai);
    }
  }
  catch(std::exception& ex){
    throw getException(ex, EXECUTION_FAILED);
  }
}

void tcoin_dal::tc_getAppInfo(APP_INFO& _return, const int32_t app){
  _return=mc_getApp(app);
  if (_return.id>0){
    return;
  }
  mysqlpp::Connection conn=getConnection(DB_SLAVE);
  try{
    _return=getApp(app, conn, false);
    if (_return.id>0){
      mc_setApp(app, _return);
    }
  }
  catch(std::exception& ex){
    throw getException(ex, EXECUTION_FAILED);
  }
}

void tcoin_dal::tc_payment(std::map<int32_t, double> & _return, const int32_t app, const std::vector<int32_t> & cre_uid, const std::vector<double> & cre_amt, const std::vector<std::string> & cre_desc, const std::vector<int32_t> & deb_uid, const std::vector<std::string> & deb_name, const std::vector<double> & deb_amt, const std::vector<std::string> & deb_desc){
  mysqlpp::Connection conn=getConnection(DB_MASTER);
  try{
    APP_INFO ai=getApp(app, conn);
    if (ai.id==0){
      tcoinException te;
      te.what=EXECUTION_FAILED;
      te.why="Invalid app value";
      throw te;
    }

    int credit_app=app;
    if (ai.pid>0){
      credit_app=ai.pid;
    }

    ACC_INFO system_acc=getAccount(ai.system_acc, app, conn);
    //mysqlpp::Transaction trans(conn);
    int trans_date=time(0);
    char str[200];
    std::string system_note;
    double total=0;
    double system_balance=system_acc.balance;
    for (int i=0; i<(int)cre_uid.size(); i++){
      int uid=cre_uid[i];
      double amount=cre_amt[i];
      std::string desc=cre_desc[i];

      ACC_INFO acc=getAccount(uid, credit_app, conn);
      if (acc.uid==0){
        tcoinException te;
        te.what = ACCOUNT_NOT_EXISTS;
        te.why = "Credit account does not exists";
        throw te;
      }

      if (acc.status==STATUS_LOCKED){
        tcoinException te;
        te.what = ACCOUNT_LOCKED;
        te.why = "Account locked";
        throw te;
      }

      if (acc.balance<amount){
        tcoinException te;
        te.what = NOT_ENOUGH_MONEY;
        te.why = "Account had not anough money";
        throw te;
      }

      sprintf(str,"PAYMENT %.0f\nAPP: %s\nFROM: %s\nTO: %s", amount, ai.app_name.c_str(), acc.name.c_str(), system_acc.name.c_str());
      std::string system_note(str);
      int tid=createTransaction((int)PAYMENT, trans_date, amount, 0, desc, system_note, uid, ai.system_acc, acc.name, system_acc.name, credit_app, app, conn);
      double balance=acc.balance-amount;
      createTransDetail(tid, credit_app, trans_date, (int)PAYMENT, (int)CREDIT, desc, uid, acc.name, -1*amount, 0, balance, 0, conn);
      updateBalance(acc, -1*amount, conn);

      _return[uid]=balance;

      system_balance=system_balance+amount;
      createTransDetail(tid, app, trans_date, (int)PAYMENT, (int)DEBIT, desc, ai.system_acc, system_acc.name, amount, 0, system_balance, 0, conn);

      total=total + amount;
    }

    //only transfer from system account to user accounts if credit_app=app (ai.pid==0)
    if (ai.pid==0){
      for (int i=0; i<(int)deb_uid.size(); i++){
        int uid=deb_uid[i];
        double amount=deb_amt[i];
        std::string desc=deb_desc[i];

        ACC_INFO acc=getAccount(uid, app, conn);
        if (acc.uid==0){
          acc.id=createAccount(uid, app, deb_name[i], conn);
          acc.uid=uid;
          acc.name=deb_name[i];
          acc.app=app;
          acc.balance=0;
          acc.promotion=0;
        }
        if (acc.id==0){
          tcoinException te;
          te.what = ACCOUNT_NOT_EXISTS;
          te.why = "Debit account does not exists and creation failed";
          throw te;
        }

        if (acc.status==STATUS_LOCKED){
          tcoinException te;
          te.what = ACCOUNT_LOCKED;
          te.why = "Account locked";
          throw te;
        }

        sprintf(str,"PAYMENT %.0f\nAPP: %s\nFROM %s\nTO: %s", amount, ai.app_name.c_str(), system_acc.name.c_str(), acc.name.c_str());
        std::string system_note(str);
        int tid=createTransaction((int)PAYMENT, trans_date, amount, 0, desc, system_note, ai.system_acc, uid, system_acc.name, acc.name, app, app, conn);
        double balance=acc.balance+amount;
        createTransDetail(tid, app, trans_date, (int)PAYMENT, (int)DEBIT, desc, uid, acc.name, amount, 0, balance, 0, conn);
        updateBalance(acc, amount, conn);

        _return[uid]=balance;

        system_balance=system_balance-amount;
        createTransDetail(tid, app, trans_date, (int)PAYMENT, (int)CREDIT, desc, ai.system_acc, system_acc.name, amount, 0, system_balance, 0, conn);
        total=total - amount;
      }
    }

    if (total!=0){
      updateBalance(system_acc, total, conn);
    }

    //trans.commit();
  }
  catch(tcoinException& te){
    throw te;
  }
  catch(std::exception& ex){
    throw getException(ex, EXECUTION_FAILED);
  }
}

void tcoin_dal::tc_searchAccount(ACC_SEARCH_RESULT& _return, const SEARCH_COND& cond, const int32_t page, const int32_t limit){
  std::string sql="";
  std::string sql_count="";
  std::string sql_cond="";

  search_criteria crit(cond);

  mysqlpp::Connection conn=getConnection(DB_SLAVE);
  mysqlpp::Query query=conn.query("");
  mysqlpp::SQLQueryParms params;
  int param_index=-1;
  std::stringstream temp;
  int uid=crit.getInt("uid");
  if (uid!=0){
    param_index++;
    temp << "uid=%" << param_index;
    sql_cond=temp.str();
    mysqlpp::SQLTypeAdapter p(uid);
    params << p;
  }
  std::string  uname=crit.getString("uname");
  if (uname!=""){
    if (sql_cond!=""){
      sql_cond = sql_cond + " AND ";
    }
    param_index++;
    temp.str("");
    temp << "uname LIKE %" << param_index << "q";
    sql_cond = sql_cond + temp.str();
    mysqlpp::SQLTypeAdapter p(uname);
    params << p;
  }

  int status=crit.getInt("status");
  if (status!=-1){
    if (sql_cond!=""){
      sql_cond = sql_cond + " AND ";
    }
    param_index++;
    temp.str("");
    temp << "status=%" << param_index;
    sql_cond = sql_cond + temp.str();
    mysqlpp::SQLTypeAdapter p(status);
    params << p;
  }

  int app=crit.getInt("app");
  if (app>0){
    if (sql_cond!=""){
      sql_cond = sql_cond + " AND ";
    }
    param_index++;
    temp.str("");
    temp << "app=%" << param_index;
    sql_cond = sql_cond + temp.str();
    mysqlpp::SQLTypeAdapter p(app);
    params << p;
  }

  int type=crit.getInt("type");
  if (type>=0){
    if (sql_cond!=""){
      sql_cond = sql_cond + " AND ";
    }
    temp.str("");
    if (type==0){
      temp << "uid>0";
    }
    else{
      temp << "uid<0";
    }
    sql_cond = sql_cond + temp.str();
  }

  if (sql_cond!="")
  {
    sql=sql + " WHERE " + sql_cond;
    sql_count=sql_count + " WHERE " + sql_cond;
  }

  sql=sql + " ORDER BY id DESC";

  if (page>0 && limit>0)
  {
    int from=(page-1)*limit;
    temp.str("");
    temp << " LIMIT " << from << ", " << limit;
    sql=sql + temp.str();
  }

  query << sql;
  query.parse();

  //LOG_OPER("searchAccount: sql: %s", sql.c_str());

  mysqlpp::StoreQueryResult res=query.store(params);

  int n=res.num_rows();
  _return.acc_list.reserve(n);
  for (int i=0; i<n; i++)
  {
    ACC_INFO acc;
    acc.uid=atoi(res[i]["uid"]);
    acc.name=(std::string)res[i]["uname"];
    acc.created=atoi(res[i]["created"]);
    acc.changed=atoi(res[i]["changed"]);
    acc.balance=atof(res[i]["balance"]);
    acc.promotion=atof(res[i]["promotion"]);
    acc.status=(ACC_STATUS)atoi(res[i]["status"]);
    acc.app=atoi(res[i]["app"]);
    _return.acc_list.push_back(acc);
  }

  int recount=crit.getInt("recount");
  if (recount==1)
  {
    query.reset();
    query << sql_count;
    query.parse();
    res=query.store(params);

    if (res.num_rows()>0)
    {
      _return.total=atoi(res[0][0]);
    }
    else
    {
      _return.total=-1;
    }
  }
  else
  {
    _return.total=0;
  }
}

void tcoin_dal::tc_searchTransaction(TRANS_SEARCH_RESULT& _return, const SEARCH_COND& cond){
  std::string sql="";
  std::string sql_count="";
  std::string str_cond="";
  mysqlpp::Connection conn=getConnection(DB_SLAVE);
  mysqlpp::Query query=conn.query("");
  std::stringstream temp;
  mysqlpp::SQLQueryParms params;
  int param_index=0;
  search_criteria crit(cond);
  int uid=crit.getInt("uid", 0);
  if (uid!=0){
    temp.str("");
    temp << "credit_uid=%" << param_index;
    mysqlpp::SQLTypeAdapter p1(uid);
    params << p1;
    str_cond="(" + temp.str();
    param_index++;

    temp.str("");
    temp << "debit_uid=%" << param_index;
    mysqlpp::SQLTypeAdapter p2(uid);
    params << p2;
    str_cond=str_cond + " OR " + temp.str() + ")";
    param_index++;
  }

  int trans_type=crit.getInt("trans_type", -1);
  if (trans_type!=-1){
    if (str_cond!=""){
      str_cond=str_cond + " AND ";
    }
    temp.str("");
    temp << "trans_type=%" << param_index;
    mysqlpp::SQLTypeAdapter p(trans_type);
    params << p;
    str_cond=str_cond + temp.str();
    param_index++;
  }

  int from_date=crit.getInt("from_date", 0);
  if (from_date>0){
    if (str_cond!=""){
      str_cond=str_cond + " AND ";
    }
    temp.str("");
    temp << "trans_date>=%" << param_index;
    mysqlpp::SQLTypeAdapter p(from_date);
    params << p;
    str_cond=str_cond + temp.str();
    param_index++;
  }

  int to_date=crit.getInt("to_date", 0);
  if (to_date>0){
    if (str_cond!=""){
      str_cond=str_cond + " AND ";
    }
    temp.str("");
    temp << "trans_date<=%" << param_index;
    mysqlpp::SQLTypeAdapter p(to_date);
    params << p;
    str_cond=str_cond + temp.str();
    param_index++;
  }

  std::string note=crit.getString("note", "");
  if (note!=""){
    if (str_cond!=""){
      str_cond=str_cond + " AND ";
    }
    temp.str("");
    temp << "(description LIKE %" << param_index << "q OR system_note LIKE %" << (param_index + 1) << "q)";
    mysqlpp::SQLTypeAdapter p1(("%" + note + "%").c_str());
    params << p1;
    mysqlpp::SQLTypeAdapter p2(("%" + note + "%").c_str());
    params << p2;
    str_cond=str_cond + temp.str();
  }

  int app=crit.getInt("app", 0);
  if (app>0){
    if (str_cond!=""){
      str_cond=str_cond + " AND ";
    }

    temp.str("");
    temp << "credit_app=%" << param_index;
    mysqlpp::SQLTypeAdapter p1(app);
    params << p1;
    str_cond=str_cond + "(" + temp.str();
    param_index++;

    temp.str("");
    temp << "debit_app=%" << param_index;
    mysqlpp::SQLTypeAdapter p2(app);
    params << p2;
    str_cond=str_cond + " OR " + temp.str() + ")";
    param_index++;
  }

  if (str_cond!=""){
    sql = sql + " WHERE " + str_cond;
  }
  sql=sql + " ORDER BY id DESC";

  int page=crit.getInt("page", 1);
  int limit=crit.getInt("limit", 30);
  if (page>0 && limit>0){
    int from=(page-1)*limit;
    temp.str("");
    temp << " LIMIT " << from << ", " << limit;
    sql=sql + temp.str();
  }

  //printf("sql: %s\n", sql.c_str());
  query << sql;
  query.parse();

  mysqlpp::StoreQueryResult res=query.store(params);

  int n=res.num_rows();
  _return.trans_list.reserve(n);
  for (int i=0; i<n; i++){
    TRANSACTION t;

    t.tid=atoi(res[i]["id"]);
    t.trans_date=atoi(res[i]["trans_date"]);
    t.trans_type=(TRANS_TYPE)atoi(res[i]["trans_type"]);
    t.amount=atof(res[i]["amount"]);
    t.promotion=atof(res[i]["promotion"]);
    t.description=(std::string)res[i]["description"];
    t.system_note=(std::string)res[i]["system_note"];
    t.credit_uid=atoi(res[i]["credit_uid"]);
    t.debit_uid=atoi(res[i]["debit_uid"]);
    t.credit_name=(std::string)res[i]["credit_name"];
    t.debit_name=(std::string)res[i]["debit_name"];
    t.credit_app=atoi(res[i]["credit_app"]);
    t.debit_app=atoi(res[i]["debit_app"]);
    _return.trans_list.push_back(t);
  }

  int recount=crit.getInt("recount", 0);
  if (recount>0){
    if (str_cond!=""){
      sql_count = sql_count + " WHERE " + str_cond;
    }
    query.reset();
    query << sql_count;
    query.parse();
    res=query.store(params);
    if (res.num_rows()>0){
      _return.total=atoi(res[0][0]);
    }
    else{
      _return.total=-1;
    }
  }
}

int tcoin_dal::tc_setAccountStatus(const int uid, const int app, const int status){
  mysqlpp::Connection conn=getConnection(DB_MASTER);
  try{
    ACC_INFO acc=getAccount(uid, app, conn);
    if (acc.uid==0){
      return -1;
    }
    mysqlpp::Query query = conn.query("");
    query.parse();
    query.execute(status, uid, app);

    acc.status=(ACC_STATUS)status;
    mc_setAccount(uid, app, acc);
    return 0;
  }
  catch(std::exception& ex){
    throw getException(ex, EXECUTION_FAILED);
  }
  return -1;
}
