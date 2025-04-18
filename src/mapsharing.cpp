#include "mapsharing.h"

#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <string>

#include "filesystem.h"
#include "ofstream_wrapper.h"

#if defined(__linux__)
#include <unistd.h>
#endif // __linux__

#if defined(_WIN32)
#include "platform_win.h"
#endif

#if defined(EMSCRIPTEN)
#include <emscripten.h>
#endif

bool MAP_SHARING::sharing;
bool MAP_SHARING::competitive;
bool MAP_SHARING::worldmenu;
std::string MAP_SHARING::username;
std::set<std::string> MAP_SHARING::admins;
std::set<std::string> MAP_SHARING::debuggers;

void MAP_SHARING::setSharing( bool mode )
{
    MAP_SHARING::sharing = mode;
}
void MAP_SHARING::setUsername( const std::string &name )
{
    MAP_SHARING::username = name;
}

bool MAP_SHARING::isSharing()
{
    return MAP_SHARING::sharing;
}
std::string MAP_SHARING::getUsername()
{
    return MAP_SHARING::username;
}

void MAP_SHARING::setCompetitive( bool mode )
{
    MAP_SHARING::competitive = mode;
}
bool MAP_SHARING::isCompetitive()
{
    return MAP_SHARING::competitive;
}

void MAP_SHARING::setWorldmenu( bool mode )
{
    MAP_SHARING::worldmenu = mode;
}
bool MAP_SHARING::isWorldmenu()
{
    return MAP_SHARING::worldmenu;
}

bool MAP_SHARING::isAdmin()
{
    return admins.find( getUsername() ) != admins.end();
}

void MAP_SHARING::setAdmins( const std::set<std::string> &names )
{
    MAP_SHARING::admins = names;
}

void MAP_SHARING::addAdmin( const std::string &name )
{
    MAP_SHARING::admins.insert( name );
    MAP_SHARING::debuggers.insert( name );
}

bool MAP_SHARING::isDebugger()
{
    return debuggers.find( getUsername() ) != debuggers.end();
}

void MAP_SHARING::setDebuggers( const std::set<std::string> &names )
{
    MAP_SHARING::debuggers = names;
}

void MAP_SHARING::addDebugger( const std::string &name )
{
    MAP_SHARING::debuggers.insert( name );
}

void MAP_SHARING::setDefaults()
{
    MAP_SHARING::setSharing( false );
    MAP_SHARING::setCompetitive( false );
    MAP_SHARING::setWorldmenu( true );
    if( const char *user = getenv( "USER" ) ) {
        MAP_SHARING::setUsername( user );
    } else {
        MAP_SHARING::setUsername( "" );
    }
    MAP_SHARING::addAdmin( "admin" );
}

void ofstream_wrapper::open( const std::ios::openmode mode )
{
    // Create a *unique* temporary path. No other running program should
    // use this path. If the file exists, it must be of a *former* program
    // instance and can safely be deleted.
    temp_path = path;
    std::ostringstream suffix;
    // Some locale may insert unwanted thousands separators,
    // which may not be in utf-8 encoding, so imbue with the classic locale.
    suffix.imbue( std::locale::classic() );
#if defined(__linux__)
    suffix << "." << getpid() << ".temp";
    temp_path += std::filesystem::u8path( suffix.str() );
#elif defined(_WIN32)
    suffix << "." << GetCurrentProcessId() << ".temp";
    temp_path += std::filesystem::u8path( suffix.str() );
#else
    // TODO: exclusive I/O for other systems
    temp_path += std::filesystem::u8path( ".temp" );
#endif
    if( !is_lexically_valid( temp_path ) ) {
        throw std::runtime_error( "path has an invalid name" );
    }

    if( std::filesystem::exists( temp_path ) && !std::filesystem::is_directory( temp_path ) ) {
        std::error_code ec;
        std::filesystem::remove( temp_path, ec );
    }

    file_stream.open( temp_path, mode );
    if( !file_stream.is_open() ) {
        throw std::runtime_error( "opening file failed" );
    }
}

void ofstream_wrapper::close()
{
    if( !file_stream.is_open() ) {
        return;
    }

    file_stream.flush();
    bool failed = file_stream.fail();
    file_stream.close();
    if( failed ) {
        // Remove the incomplete or otherwise faulty file (if possible).
        // Failures from it are ignored as we can't really do anything about them.
        std::error_code ec;
        std::filesystem::remove( temp_path, ec );
        throw std::runtime_error( "writing to file failed" );
    }
    std::error_code ec2;
    std::filesystem::rename( temp_path, path, ec2 );
    if( ec2 ) {
        // Leave the temp path, so the user can move it if possible.
        throw std::runtime_error( "moving temporary file \"" + temp_path.u8string() + "\" failed" );
    }

#if defined(EMSCRIPTEN)
    EM_ASM( window.setFsNeedsSync(); );
#endif
}
