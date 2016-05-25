#ifndef EVENTMANAGER
#define EVENTMANAGER

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <map>
#include <string>

namespace BlueBear {
  class EventManager {

    using EventMap = std::map< std::string, std::string >;

    private:
      lua_State* L;
      std::map< std::string, EventMap > events;

    public:
      EventManager( lua_State* L );
      void reset();
      void registerEvent( const std::string& eventKey, const std::string& cid, const std::string& callback );
      void unregisterEvent( const std::string& eventKey, const std::string& cid );
      void broadcastEvent( const std::string& eventKey );

      static int lua_registerEvent( lua_State* L );
      static int lua_unregisterEvent( lua_State* L );
      static int lua_broadcastEvent( lua_State* L );
      static int lua_gc( lua_State* L );
  };

}

#endif
