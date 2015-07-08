#include <boost/property_tree/ptree.hpp>
#include <elliptics/cppdef.h>
#include "history.h"

using namespace ioremap::elliptics;


class DbElliptics : public common::IHistory
{
public:
    DbElliptics();
    virtual void Initialize( const std::string &configPath );
    virtual void Shutdown();

    virtual void Put( const std::string &key, const std::string &value );
    virtual void Delete( const std::string &key );

    typedef void (*GetCallback)( const std::string &key, const std::string &value );
    virtual void GetAll( GetCallback callback );

private:
    void ParseConfig( const std::string &configPath );
    dnet_log_level ParseLogLevel( const std::string &level ) const;

private:
    boost::property_tree::ptree config_;
    std::unique_ptr< file_logger > log_;
    std::unique_ptr< node > node_;
    std::shared_ptr< session > sess_;
    const std::string namespace_;
};
