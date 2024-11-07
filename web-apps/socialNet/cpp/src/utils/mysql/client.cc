#include "client.hh"
#include <stdexcept>
#include <string.h>
#include <iostream>

namespace socialNet::utils {


  rd_utils::concurrency::mutex MysqlClient::__MUTEX__;
  uint32_t MysqlClient::__OPENED__ = 0;

  MysqlClient::Statement::Statement (MYSQL_STMT * i, MysqlClient * context, uint32_t nbParams, uint32_t nbResults)
    : _context (context)
    , _stmt (i)
    , _nbParams (nbParams)
    , _nbResults (nbResults)
  {
    this-> _param_binds  = new MYSQL_BIND [nbParams];
    this-> _result_binds = new MYSQL_BIND [nbResults];
    this-> _length = new uint64_t [nbParams + nbResults];
    this-> _isNull = new bool [nbParams + nbResults];
    this-> _errors = new bool [nbParams + nbResults];

    ::memset (this-> _param_binds, 0, nbParams * sizeof (MYSQL_BIND));
    ::memset (this-> _result_binds, 0, nbResults * sizeof (MYSQL_BIND));
  }

  void MysqlClient::Statement::setParam (uint32_t i, char * value, uint32_t len) {
    this-> _param_binds [i].buffer_type = MYSQL_TYPE_STRING;
    this-> _param_binds [i].buffer = value;
    this-> _param_binds [i].buffer_length = len;

    this-> _isNull [i] = false;
    this-> _param_binds [i].is_null = &this-> _isNull [i];
    this-> _length [i] = len;
    this-> _param_binds [i].length = &this-> _length [i];
    this-> _param_binds [i].error = &this-> _errors [i];
  }

  void MysqlClient::Statement::setParam (uint32_t i, int * value) {
    this-> _param_binds [i].buffer_type = MYSQL_TYPE_LONG;
    this-> _param_binds [i].buffer = value;
    this-> _isNull [i] = false;
    this-> _param_binds [i].is_null = &this-> _isNull [i];
    this-> _length [i] = 1;
    this-> _param_binds [i].length = &this-> _length [i];
    this-> _param_binds [i].error = &this-> _errors [i];
  }

  void MysqlClient::Statement::setParam (uint32_t i, uint32_t * value) {
    this-> _param_binds [i].buffer_type = MYSQL_TYPE_LONG;
    this-> _param_binds [i].buffer = value;
    this-> _isNull [i] = false;
    this-> _param_binds [i].is_null = &this-> _isNull [i];
    this-> _length [i] = 1;
    this-> _param_binds [i].length = &this-> _length [i];
    this-> _param_binds [i].error = &this-> _errors [i];
  }

  void MysqlClient::Statement::setParam (uint32_t i, uint64_t * value) {
    this-> _param_binds [i].buffer_type = MYSQL_TYPE_LONGLONG;
    this-> _param_binds [i].buffer = value;
    this-> _isNull [i] = false;
    this-> _param_binds [i].is_null = &this-> _isNull [i];
    this-> _length [i] = 1;
    this-> _param_binds [i].length = &this-> _length [i];
    this-> _param_binds [i].error = &this-> _errors [i];
  }

  void MysqlClient::Statement::setResult (uint32_t i, char * value, uint32_t len) {
    this-> _result_binds [i].buffer_type = MYSQL_TYPE_STRING;
    this-> _result_binds [i].buffer = value;
    this-> _result_binds [i].buffer_length = len;
    this-> _result_binds [i].is_null = &this-> _isNull [i + this-> _nbParams];
    this-> _result_binds [i].length = &this-> _length [i + this-> _nbParams];
    this-> _result_binds [i].error = &this-> _errors [i + this-> _nbParams];
  }

  void MysqlClient::Statement::setResult (uint32_t i, int * value) {
    this-> _result_binds [i].buffer_type = MYSQL_TYPE_LONG;
    this-> _result_binds [i].buffer = value;
    this-> _result_binds [i].is_null = &this-> _isNull [i + this-> _nbParams];
    this-> _result_binds [i].length = &this-> _length [i + this-> _nbParams];
    this-> _result_binds [i].error = &this-> _errors [i + this-> _nbParams];
  }

  void MysqlClient::Statement::setResult (uint32_t i, uint32_t * value) {
    this-> _result_binds [i].buffer_type = MYSQL_TYPE_LONG;
    this-> _result_binds [i].buffer = value;
    this-> _result_binds [i].is_null = &this-> _isNull [i + this-> _nbParams];
    this-> _result_binds [i].length = &this-> _length [i + this-> _nbParams];
    this-> _result_binds [i].error = &this-> _errors [i + this-> _nbParams];
  }


  uint32_t MysqlClient::Statement::getResultLen (uint32_t i) const {
    return this-> _length [i + this-> _nbParams];
  }

  uint32_t MysqlClient::Statement::getGeneratedId () const {
    return mysql_stmt_insert_id (this-> _stmt);
  }

  void MysqlClient::Statement::finalize () {
    if (this-> _nbResults != 0) {
      if (mysql_stmt_bind_result (this-> _stmt, this-> _result_binds)) {
        mysql_stmt_close (this-> _stmt);
        this-> _stmt = nullptr;

        std::string msg = mysql_error (this-> _context-> _conn);
        this-> _context-> dispose ();
        throw std::runtime_error ("Failed to bind result " + msg);
      }
    }

    if (this-> _nbParams != 0) {
      if (mysql_stmt_bind_param (this-> _stmt, this-> _param_binds)) {
        mysql_stmt_close (this-> _stmt);
        this-> _stmt = nullptr;

        std::string msg = mysql_error (this-> _context-> _conn);
        this-> _context-> dispose ();
        throw std::runtime_error ("Failed to bind params " + msg);
      }
    }
  }


  uint32_t MysqlClient::Statement::getNbParams () const {
    return this-> _nbParams;
  }

  uint32_t MysqlClient::Statement::getNbResults () const {
    return this-> _nbResults;
  }

  void MysqlClient::Statement::execute () {
    if (mysql_stmt_execute (this-> _stmt)) {
      mysql_stmt_close (this-> _stmt);
      std::string err = mysql_error (this-> _context-> _conn);
      this-> _stmt = nullptr;
      this-> _context-> dispose ();
      throw std::runtime_error ("Failed to execute stmt : " + err);
    }
  }

  bool MysqlClient::Statement::next () {
    if (this-> _stmt != nullptr) {
      return mysql_stmt_fetch (this-> _stmt) == 0;
    } else {
      this-> _context-> dispose ();
      throw std::runtime_error ("Statement is empty");
    }
  }

  MysqlClient::Statement::~Statement () {
    if (this-> _stmt != nullptr) {
      mysql_stmt_close (this-> _stmt);
      this-> _stmt = nullptr;
    }

    if (this-> _param_binds != nullptr) {
      delete [] this-> _param_binds;
      this-> _param_binds = nullptr;

      delete [] this-> _result_binds;
      this-> _result_binds = nullptr;

      delete [] this-> _length;
      this-> _length = nullptr;

      delete [] this-> _isNull;
      this-> _isNull = nullptr;

      delete [] this-> _errors;
      this-> _errors = nullptr;
    }
  }

  MysqlClient::MysqlClient (const std::string & server, const std::string & user, const std::string & password, const std::string & db) :
    _server (server)
    , _user (user)
    , _db (db)
    , _password (password)
    , _conn (nullptr)
  {}

  void MysqlClient::connect (uint32_t timeout) {
    WITH_LOCK (__MUTEX__) {
      __OPENED__ += 1;
      if (__OPENED__ == 1) {
        if (mysql_library_init (0, NULL, NULL)) {
          throw std::runtime_error ("Failed to initialize mysql");
        }
      }
    }

    this-> _conn = mysql_init (nullptr);
    mysql_options(this-> _conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    auto code = mysql_real_connect (this-> _conn, this-> _server.c_str (), this-> _user.c_str (), this-> _password.c_str (), this-> _db.c_str (), 0, nullptr, 0);
    if (code == 0) {
      auto msg = std::string (mysql_error (this-> _conn));
      mysql_close (this-> _conn);
      this-> _conn = nullptr;
      throw std::runtime_error ("Failed to connect to database : (" + msg + ")");
    }
  }

  void MysqlClient::autocommit (bool set) {
    if (!set) {
      mysql_autocommit (this-> _conn, 0);
    } else {
      mysql_autocommit (this-> _conn, 1);
    }
  }

  void MysqlClient::commit () {
    mysql_commit (this-> _conn);
  }

  std::shared_ptr <MysqlClient::Statement> MysqlClient::prepare (const std::string & query) {
    if (this-> _conn == nullptr)  this-> connect ();

    MYSQL_STMT * stmt = mysql_stmt_init (this-> _conn);
    if (stmt == nullptr) {
      this-> dispose ();
      throw std::runtime_error ("Failed to init a new stmt");
    }

    if (mysql_stmt_prepare (stmt, query.c_str (), query.length ())) {
      mysql_stmt_close (stmt);
      this-> dispose ();
      throw std::runtime_error ("Failed to prepare stmt from query : " + query);
    }

    auto param_count = mysql_stmt_param_count (stmt);
    auto  prepare_meta_result = mysql_stmt_result_metadata(stmt);
    uint32_t result_count = 0;

    if (prepare_meta_result != nullptr) {
      result_count = mysql_num_fields (prepare_meta_result);
      mysql_free_result (prepare_meta_result);
    }

    return std::make_shared <MysqlClient::Statement> (stmt, this, param_count, result_count);
  }


  void MysqlClient::dispose () {
    if (this-> _conn != nullptr) {
      mysql_close (this-> _conn);
      this-> _conn = nullptr;
      WITH_LOCK (__MUTEX__) {
        __OPENED__ -= 1;
        if (__OPENED__ == 0) {
          mysql_library_end ();
        }
      }
    }
  }

  MysqlClient::~MysqlClient () {
    this-> dispose ();
  }

}
