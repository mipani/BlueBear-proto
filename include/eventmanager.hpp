#ifndef EVENTMANAGER
#define EVENTMANAGER

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <jsoncpp/json/json.h>
#include <map>
#include <string>
#include <memory>

namespace BlueBear {
  class Lot;

  class EventManager {

    using EventMap = std::map< std::string, std::string >;

    private:
      lua_State* L;
      std::map< std::string, EventMap > events;
      std::shared_ptr< Lot > currentLot;

    public:
      EventManager( lua_State* L, std::shared_ptr< Lot > currentLot );
      void reset();
      void registerEvent( const std::string& eventKey, const std::string& cid, const std::string& callback );
      void unregisterEvent( const std::string& eventKey, const std::string& cid );
      void broadcastEvent( const std::string& eventKey );
      Json::Value save();
      void load( Json::Value& serializedEventManager );

      static int lua_save( lua_State* L );
      static int lua_load( lua_State* L );
      static int lua_registerEvent( lua_State* L );
      static int lua_unregisterEvent( lua_State* L );
      static int lua_broadcastEvent( lua_State* L );
      static int lua_gc( lua_State* L );
  };

}

#endif