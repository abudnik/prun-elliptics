#define BOOST_SPIRIT_THREADSAFE

#include "dbelliptics.h"
#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>


struct DbAddr
{
    dnet_route_entry entry;
};

struct DbAddrCompare
{
   bool operator() ( const DbAddr &lhs, const DbAddr &rhs ) const
   {
       if ( lhs.entry.group_id < rhs.entry.group_id )
           return true;
       return dnet_addr_cmp( &lhs.entry.addr, &rhs.entry.addr ) < 0;
   }
};


DbElliptics::DbElliptics()
: namespace_( "prun" )
{}

void DbElliptics::Initialize( const std::string &configPath )
{
    // parse config
    ParseConfig( configPath );
    std::string logPath = config_.get<std::string>( "log" );
    std::string sLogLevel = config_.get<std::string>( "log_level" );
    dnet_log_level logLevel = ParseLogLevel( sLogLevel );

    std::vector< std::string > remotes;
    for( const auto &v : config_.get_child( "remotes" ) )
    {
        remotes.push_back( v.second.get_value< std::string >() );
    }

    if ( remotes.size() % 2 == 0 )
        throw std::logic_error( "DbElliptics::Initialize: number of remote nodes (" + std::to_string( remotes.size() ) +
                                " given) must be an odd number to achieve quorum" );

    std::vector< int > groups;
    for( const auto &v : config_.get_child( "groups" ) )
    {
        groups.push_back( v.second.get_value< int >() );
    }

    // init elliptics node
    size_t numConnectedNodes = 0;
    log_.reset( new file_logger( logPath.c_str(), logLevel ) );
    node_.reset( new node( logger( *log_, blackhole::log::attributes_t() ) ) );
    for( const std::string &remote : remotes )
    {
        try
        {
            node_->add_remote( remote );
            ++numConnectedNodes;
        }
        catch( const std::exception &ex ) {}
    }

    if ( numConnectedNodes <= remotes.size() / 2 )
        throw std::logic_error( "DbElliptics::Initialize: couldn't connect to quorum of nodes: " +
                                std::to_string( numConnectedNodes ) + " connected / " +
                                std::to_string( remotes.size() ) + " total" );

    // create elliptics session
    sess_ = std::make_shared< session >( *node_ );
    sess_->set_groups( groups );
    sess_->set_namespace( namespace_ );
}

void DbElliptics::Shutdown()
{
    config_.clear();
    sess_.reset();
}

void DbElliptics::Put( const std::string &key, const std::string &value )
{
    auto sess = sess_;
    if ( sess )
    {
        std::string kv = namespace_ + std::string( " " ) + key + std::string( " " ) + value;
        data_pointer data = data_pointer::from_raw( kv );
        auto result = sess->write_data( key, data, 0 );
        if ( result.is_valid() )
            result.wait();
    }
}

void DbElliptics::Delete( const std::string &key )
{
    auto sess = sess_;
    if ( sess )
    {
        auto result = sess->remove( key );
        if ( result.is_valid() )
            result.wait();
    }
}

void DbElliptics::GetAll( GetCallback callback )
{
    auto sess = sess_;
    if ( sess )
    {
        auto routes = sess->get_routes();
        if ( routes.empty() )
            throw std::logic_error( "DbElliptics::GetAll: empty routes table" );

        std::set< DbAddr, DbAddrCompare > nodesAddr;
        for( auto it = routes.begin(); it != routes.end(); ++it )
        {
            nodesAddr.insert( DbAddr{ *it } );
        }

        DbKeyToEntry replies;
        for( const DbAddr &addr : nodesAddr )
        {
            dnet_id id;
            memset( &id, 0, sizeof( id ) );
            dnet_setup_id( &id, addr.entry.group_id, addr.entry.id.id );

            IterateNode( sess, id, replies );
        }

        IterateQuorumReplies( sess, nodesAddr.size(), replies, callback );
    }
}

void DbElliptics::IterateNode( const SessionPtr &sess, const dnet_id &id, DbKeyToEntry &replies )
{
    auto res = sess->start_iterator( ioremap::elliptics::key( id ),
                                     {}, DNET_ITYPE_NETWORK, 0 );

    for( auto it = res.begin(), end = res.end(); it != end; ++it )
    {
        if ( it->reply()->status != 0 )
            continue;

        auto response = it->reply();
        DbKey key{ response->key };
        auto it_r = replies.find( key );
        if ( it_r != replies.end() )
        {
            DbEntry &entry = it_r->second;
            entry.groups.push_back( id.group_id );
        }
        else
        {
            std::vector<int> groups{ static_cast<int>( id.group_id ) };
            DbEntry entry{ groups, response->size };
            replies.insert( std::make_pair( key, entry ) );
        }
    }
}

void DbElliptics::IterateQuorumReplies( const SessionPtr &sess, size_t numRoutes, const DbKeyToEntry &replies, GetCallback callback )
{
    const size_t quorum = numRoutes / 2 + 1;

    for( auto it = replies.begin(); it != replies.end(); ++it )
    {
        const DbEntry &entry = it->second;
        const std::vector<int> &groups = entry.groups;
        if ( groups.size() >= quorum )
        {
            auto read_result = sess->read_data( it->first.id, groups, 0, entry.size );
            auto entries = read_result.get();
            const auto &entry = entries.front();

            std::string kv = entry.file().to_string();
            // check magic header
            if ( kv.compare( 0, namespace_.size(), namespace_, 0, namespace_.size() ) != 0 )
                continue;

            size_t keyStart = namespace_.size() + 1;
            size_t keyEnd = kv.find( ' ', keyStart );
            if ( keyEnd == std::string::npos )
                continue;

            std::string key = kv.substr( keyStart, keyEnd - keyStart );
            std::string value  = kv.substr( keyEnd + 1 );
            callback( key, value );
        }
    }
}

void DbElliptics::ParseConfig( const std::string &configPath )
{
    std::ifstream file( configPath.c_str() );
    if ( !file.is_open() )
        throw std::logic_error( "DbElliptics::ParseConfig: couldn't open " + configPath );

    boost::property_tree::read_json( file, config_ );
}

dnet_log_level DbElliptics::ParseLogLevel( const std::string &level ) const
{
    if ( !strcasecmp( level.c_str(), "debug" ) )
        return DNET_LOG_DEBUG;
    if ( !strcasecmp( level.c_str(), "notice" ) )
        return DNET_LOG_NOTICE;
    if ( !strcasecmp( level.c_str(), "info" ) )
        return DNET_LOG_INFO;
    if ( !strcasecmp( level.c_str(), "warning" ) )
        return DNET_LOG_WARNING;
    if ( !strcasecmp( level.c_str(), "error" ) )
        return DNET_LOG_ERROR;

    return DNET_LOG_INFO;
}

common::IHistory *CreateHistory( int interfaceVersion )
{
    return interfaceVersion == common::HISTORY_VERSION ?
        new DbElliptics : nullptr;
}

void DestroyHistory( const common::IHistory *history )
{
    delete history;
}
