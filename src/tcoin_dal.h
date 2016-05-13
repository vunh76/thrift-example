#ifndef TCOIN_DAL_H_INCLUDED
#define TCOIN_DAL_H_INCLUDED
#include "gen-cpp/tcoin_types.h"
#include <mysql++.h>
#include "tcoin_memcache.h"

using namespace tcoin::v2;

struct db_setting{
  std::string server;
  std::string user;
  std::string password;
  std::string dbname;
};

class tcoin_dal
{
    private:
      enum COST_TYPE{
        COST_BY_PERCENT=0,
        COST_BY_CONST=1
      };

      enum DB_TYPE{
        DB_MASTER=0,
        DB_SLAVE=1
      };

      db_setting &masterdb;
      std::vector<db_setting> &slavedb;
      mysqlpp::Connection getConnection(const DB_TYPE db_type=DB_MASTER);
      void closeConnection(mysqlpp::Connection& conn);

      //
      void mc_serialize(ACC_INFO& acc, std::vector<char> &ret_value);
      void mc_unserialize(std::vector<char> &data, ACC_INFO& acc);
      void mc_serialize(APP_INFO& app, std::vector<char> &ret_value);
      void mc_unserialize(std::vector<char> &data, APP_INFO& app);
      void mc_serialize(EX_RATE& ex, std::vector<char> &ret_value);
      void mc_unserialize(std::vector<char> &data, EX_RATE& ex);
      std::string makeKey(const std::string &prefix, const int param);
      std::string makeKey(const std::string &prefix, const int uid, const int app);
      ACC_INFO mc_getAccount(const int uid, const int app);
      void mc_setAccount(const int uid, const int app, ACC_INFO &acc);
      APP_INFO mc_getApp(const int app);
      void mc_setApp(const int app, APP_INFO &ai);
      EX_RATE mc_getExRate(const int app1, const int app2);
      void mc_setExRate(const int app1, const int app2, EX_RATE &ex);
      //

      //core function
      ACC_INFO getAccount(const int uid, const int app, mysqlpp::Connection& conn, bool check_memcache=true);
      int createAccount(const int uid, const int app, const std::string& name, mysqlpp::Connection& conn);
      //void setAccountStatus(const int uid, const int app, const int status, mysqlpp::Connection& conn);
      void updateBalance(ACC_INFO &ai, double amount, mysqlpp::Connection& conn, double promotion=0);
      //double getAccountBalance(const int uid, const int app, mysqlpp::Connection& conn);
      int createTransaction(int trans_type, int trans_date, const double amount, const double promotion, const std::string& description, const std::string& system_note, int credit_uid, int debit_uid, const std::string& credit_name, const std::string& debit_name, const int credit_app, const int debit_app, mysqlpp::Connection& conn);
      int createTransDetail(int tid, const int app, int trans_date, int trans_type, int journal, const std::string& description, const int uid, const std::string& uname, const double amount, const double promotion, double balance, double promotion_balance, mysqlpp::Connection& conn);
      EX_RATE getExchangeRate(const int app1, const int app2, mysqlpp::Connection& conn, bool check_memcache=true);
      int getAppStatus(const int app, mysqlpp::Connection& conn);
      APP_INFO getApp(const int app, mysqlpp::Connection& conn, bool check_memcache=true);
      tcoinException getException(std::exception& ex, ErrorCode code);
    public:
    tcoin_dal(db_setting &_masterdb, std::vector<db_setting> &_slavedb) : masterdb(_masterdb), slavedb(_slavedb){

    }

    ~tcoin_dal() {

    }

    ACC_INFO tc_getAccount(const int uid, const int app);
    int tc_createAccount(const int uid, const int app, const std::string& name);
    int tc_cashin(const int uid, const int app, const double amount, const std::string& description, const double promotion);
    int tc_transfer(const int app, const int uid1, const int uid2, const std::string& uname2, const double amount, const std::string& description);
    int tc_transfer2(const int app, const int uid1, const int uid2, const std::string& uname2, const double amount, const std::string& description, const double cost);
    int tc_exchange(const int uid, const int app1, const int app2, const double amount, const std::string& description, const double promotion);
    void tc_search_user_trans(TRANS_DETAIL_RESULT& _return, const int uid, const int app, const SEARCH_COND& cond, const int page, const int limit);
    EX_RATE tc_getExchangeRate(const int app1, const int app2);
    void tc_getUserAccountList(std::vector<ACC_INFO> & _return, const int32_t uid);
    void tc_getTransferableUserAccountList(std::vector<ACC_INFO> & _return, const int32_t uid);
    void tc_getAppInfo(APP_INFO& _return, const int32_t app);
    void tc_payment(std::map<int32_t, double> & _return, const int32_t app, const std::vector<int32_t> & cre_uid, const std::vector<double> & cre_amt, const std::vector<std::string> & cre_desc, const std::vector<int32_t> & deb_uid, const std::vector<std::string> & deb_name, const std::vector<double> & deb_amt, const std::vector<std::string> & deb_desc);
    void tc_searchAccount(ACC_SEARCH_RESULT& _return, const SEARCH_COND& cond, const int32_t page, const int32_t limit);
    void tc_searchTransaction(TRANS_SEARCH_RESULT& _return, const SEARCH_COND& cond);
    int tc_setAccountStatus(const int uid, const int app, const int status);
};

class search_criteria{
  private:
    const SEARCH_COND& cond;
  public:
    search_criteria(const SEARCH_COND& _cond) : cond(_cond){
    }

    int getInt(const std::string& key, int def=0){
      SEARCH_COND::const_iterator it;
      it=cond.find(key);
      if (it==cond.end()){
        return def;
      }
      std::string val=it->second;
      return atoi(val.c_str());
    }

    double getDouble(const std::string& key, double def=0.0){
      SEARCH_COND::const_iterator it;
      it=cond.find(key);
      if (it==cond.end()){
        return def;
      }
      std::string val=it->second;
      return atof(val.c_str());
    }

    std::string getString(const std::string& key, std::string def=""){
      SEARCH_COND::const_iterator it;
      it=cond.find(key);
      if (it==cond.end()){
        return def;
      }
      std::string val=it->second;
      return val;
    }
};
#endif // TCOIN_DAL_H_INCLUDED
