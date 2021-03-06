#include <boost/property_tree/ptree.hpp>
#include <elliptics/cppdef.h>
#include "history.h"

using namespace ioremap::elliptics;


struct DbKey
{
    dnet_raw_id id;
};

struct DbEntry
{
    std::vector<int> groups;
    uint64_t size;
};

struct IteratorResponseCompare
{
   bool operator() ( const DbKey &lhs, const DbKey &rhs ) const
   {
       return memcmp( &lhs.id, &rhs.id, sizeof( lhs.id ) ) < 0;
   }
};

class DbElliptics : public common::IHistory
{
    typedef std::shared_ptr< session > SessionPtr;
    typedef std::map< DbKey, DbEntry, IteratorResponseCompare > DbKeyToEntry;

public:
    DbElliptics();
    virtual void Initialize( const std::string &configPath );
    virtual void Shutdown();

    virtual void Put( const std::string &key, const std::string &value );
    virtual void Delete( const std::string &key );

    typedef void (*GetCallback)( const std::string &key, const std::string &value );
    virtual void GetAll( GetCallback callback );

private:
    void IterateNode( const SessionPtr &sess, const dnet_id &id, DbKeyToEntry &replies );
    void IterateQuorumReplies( const SessionPtr &sess, size_t numRoutes, const DbKeyToEntry &replies, GetCallback callback );
    void ParseConfig( const std::string &configPath );
    dnet_log_level ParseLogLevel( const std::string &level ) const;

private:
    boost::property_tree::ptree config_;
    std::unique_ptr< file_logger > log_;
    std::unique_ptr< node > node_;
    SessionPtr sess_;
    const std::string namespace_;
};
