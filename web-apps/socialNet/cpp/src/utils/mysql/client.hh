#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mysql/mysql.h>
#include <cstdint>
#include <rd_utils/concurrency/mutex.hh>

namespace socialNet::utils {

  class MysqlClient {
  public :

    class Statement {
    private:

      MysqlClient * _context;

      MYSQL_STMT * _stmt;

      // MYSQL_RES * _res = nullptr;

      uint32_t _nbParams;

      uint32_t _nbResults;

      MYSQL_BIND * _param_binds;

      MYSQL_BIND * _result_binds;

      uint64_t * _length;

      bool * _isNull;

      bool * _errors;

    private:

      Statement (const Statement &);
      void operator=(const Statement&);

    public:

      Statement (MYSQL_STMT * stmt, MysqlClient * context, uint32_t nbParams, uint32_t nbResults);

      void setParam (uint32_t index, char * value, uint32_t len);

      void setParam (uint32_t index, uint32_t * value);

      void setParam (uint32_t index, uint64_t * value);

      void setParam (uint32_t index, int * value);

      void setResult (uint32_t index, char * value, uint32_t len);

      void setResult (uint32_t index, int * value);

      void setResult (uint32_t index, uint32_t * value);

      void setResult (uint32_t index, uint64_t * value);

      /**
       * @returns: the number of parameters
       */
      uint32_t getNbParams () const;

      /**
       * @return: the number of fields
       */
      uint32_t getNbResults () const;

      /**
       * @returns: the length of a result
       * @info: after execute
       */
      uint32_t getResultLen (uint32_t i) const;

      /**
       * @info: to call after execute
       * @returns: the id of the /update/ or /insert/ from an AUTO_INCREMENT key in the table
       */
      uint32_t getGeneratedId () const;

      /**
       * Fecth the next result, and return true if any
       */
      bool next ();

      void finalize ();

      void execute ();

      ~Statement ();
    };

  public:

    // The address of the server
    std::string _server;

    // The username
    std::string _user;

    // The database
    std::string _db;

    std::string _password;

    // The connection to mysql
    MYSQL * _conn = nullptr;

    static rd_utils::concurrency::mutex __MUTEX__;

    static uint32_t __OPENED__;

  private:

    MysqlClient (const MysqlClient &);
    void operator=(const MysqlClient&);

  public:

    /**
     * Create an unconfigured mysql client
     */
    MysqlClient ();

    /**
     * Create a mysql client
     * @info: not connected yet
     */
    MysqlClient (const std::string & addr, const std::string & user, const std::string & password, const std::string & db);

    /**
     * Connect to the database
     * @throws: if fails
     */
    void connect (uint32_t timeout = 2);

    /**
     * Configure the autocommit of the DB
     */
    void autocommit (bool set);

    /**
     * Commit pending statements
     */
    void commit ();

    /**
     * Prepare a statement
     */
    std::shared_ptr<MysqlClient::Statement> prepare (const std::string & query);

    /**
     * Close the connection to the DB
     */
    void dispose ();

    /**
     * this-> dispose ();
     */
    ~MysqlClient ();

  };

}
