#include "bbtypes.hpp"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "scripting/lotentity.hpp"
#include "scripting/lot.hpp"
#include "log.hpp"
#include "utility.hpp"
#include <jsoncpp/json/json.h>
#include "scripting/eventmanager.hpp"
#include "scripting/tile.hpp"
#include <memory>
#include <vector>
#include <cstring>
#include <string>
#include <iostream>
#include <utility>

namespace BlueBear {
	namespace Scripting {

		Lot::Lot( lua_State* L, const Tick& currentTickReference, InfrastructureFactory& infrastructureFactory, Json::Value& rootObject ) :
			L( L ),
			currentTickReference( currentTickReference ),
			infrastructureFactory( infrastructureFactory ),
			floorX( rootObject[ "floorx" ].asInt() ),
			floorY( rootObject[ "floory" ].asInt() ),
			stories( rootObject[ "stories" ].asInt() ),
			undergroundStories( rootObject[ "subtr" ].asInt() ),
			terrainType( TerrainType( rootObject[ "terrain" ].asInt() ) ) {
			buildFloorMap( rootObject[ "infr" ][ "floor" ] );
		}

		void Lot::buildFloorMap( Json::Value& floor ) {
			Json::Value dict = floor[ "dict" ];
			Json::Value levels = floor[ "levels" ];

			// Create the vector reference of shared_ptrs by iterating through dict
			std::vector< std::shared_ptr< Tile > > lookup;
			for( const Json::Value& dictEntry : dict ) {
				auto tile = infrastructureFactory.getFloorTile( dictEntry.asString() );
				lookup.push_back( tile );
			}

			// Use the pointer lookup to create the floormap
			Log::getInstance().debug( "Lot::buildFloorMap", "Stories: " + std::to_string( stories ) + " " + std::to_string( floorX ) + "x" + std::to_string( floorY ) );
			floorMap = std::make_unique< FloorMap >( stories, floorX, floorY );

			for( const Json::Value& level : levels ) {
				unsigned int entry = level.asUInt();
				floorMap->pushDirect( lookup.at( entry ) );
			}
		}

		int Lot::lua_getLotObjects( lua_State* L ) {

			// Pop the lot off the stack
			Lot* lot = ( Lot* )lua_touserdata( L, lua_upvalueindex( 1 ) );

			// Create an array table with as many entries as the size of this->objects
			lua_createtable( L, lot->objects.size(), 0 );

			// Push 'em on!
			size_t tableIndex = 1;
			for( auto& keyValuePair : lot->objects ) {
				LotEntity& lotEntity = *( keyValuePair.second );

				lua_rawgeti( L, LUA_REGISTRYINDEX, lotEntity.luaVMInstance );
				lua_rawseti( L, -2, tableIndex++ );
			}

			return 1;

		}

		int Lot::lua_getLotObjectsByType( lua_State* L ) {

			// Because Lua requires methods be static in C closure, pop the first argument: the "this" pointer
			Lot* lot = ( Lot* )lua_touserdata( L, lua_upvalueindex( 1 ) );

			// Get argument (the class we are looking for) and remove it from the stack
			// Copy using modern C++ string methods into archaic, unsafe C-string format used by Lua API
			std::string keystring( lua_tostring( L, -1 ) );
			const char* idKey = keystring.c_str();
			lua_pop( L, 1 );

			// This table will be the array of matching lot objects
			lua_newtable( L );

			// Start at index number 1 - Lua arrays (tables) start at 1
			size_t tableIndex = 1;

			// Iterate through each object on the lot, checking to see if each is an instance of "idKey"
			for( auto& keyValuePair : lot->objects ) {
				LotEntity& lotEntity = *( keyValuePair.second );

				// Push bluebear global
				lua_getglobal( L, "bluebear" );

				// Push instance_of utility function
				Utility::getTableValue( L, "instance_of" );

				// Push the two arguments: identifier, and instance
				lua_pushstring( L, idKey );
				lua_rawgeti( L, LUA_REGISTRYINDEX, lotEntity.luaVMInstance );

				// We're ready to call instance_of on object!
				if( lua_pcall( L, 2, 1, 0 ) == 0 ) {
					// Read the answer left on the stack as a boolean
					// lua_getboolean actually returns an int, which we'll need interpreted as a bool
					bool isInstance = !!lua_toboolean( L, -1 );

					// Before we do anything else, get rid of the result, the instance_of function
					// and the bluebear tableIndex from the stack - we don't need them anymore
					// until the loop restarts, and this will put our return table back at the top
					// of the stack.
					lua_pop( L, 2 );

					// If this object is a descendant of idKey, push it onto the table
					if( isInstance ) {
						// Re-push the instance onto the stack
						lua_rawgeti( L, LUA_REGISTRYINDEX, lotEntity.luaVMInstance );
						// Push it onto the table on our stack
						lua_rawseti( L, -2, tableIndex++ );
					}
				}
			}

			return 1;

		}

		int Lot::lua_getLotObjectByCid( lua_State* L ) {

			// Because Lua requires methods be static in C closure, pop the first argument: the "this" pointer
			Lot* lot = ( Lot* )lua_touserdata( L, lua_upvalueindex( 1 ) );

			// Get argument (the class we are looking for) and remove it from the stack
			std::string keystring( lua_tostring( L, -1 ) );
			lua_pop( L, 1 );

			// Go looking for the item
			int reference = lot->getLotObjectByCid( keystring );

			if( reference != -1 ) {
				// Push table on the stack
				lua_rawgeti( L, LUA_REGISTRYINDEX, reference );
			} else {
				// The object wasn't found, push the nil value
				lua_pushnil( L );
			}

			return 1;
		}

		/**
		 * Get a lot object by its cid. cid takes the form of "bbXXX".
		 * @returns		-1 if the object wasn't found, the numeric object if it was
		 */
		int Lot::getLotObjectByCid( const std::string& cid ) {
			// lot->objects is a map, reducing the cost of this operation
			auto object = objects.find( cid );

			if( object != objects.end() ) {
				// You can use this with lua_rawgeti( L, LUA_REGISTRYINDEX, <returned id> );
				return object->second->luaVMInstance;
			}

			return -1;
		}

		/**
		 * Create a lot entity from a JSON value
		 */
		int Lot::createLotEntityFromJSON( const Json::Value& serialEntity ) {
			// Simple proxy to LotEntity's JSON constructor
			std::unique_ptr< LotEntity > entity = std::make_unique< LotEntity >( L, currentTickReference, serialEntity );

			if( entity->ok ) {
				int ref = entity->luaVMInstance;
				if( !objects.count( entity->cid ) ) {
					objects[ entity->cid ] = std::move( entity );
					return ref;
				}
			}

			// Entity didn't build successfully
			return -1;
		}

		/**
		 * Create a new instance of "classID" from scratch
		 */
		int Lot::createLotEntity( const std::string& classID ) {
			std::unique_ptr< LotEntity > entity = std::make_unique< LotEntity >( L, currentTickReference, classID );

			// Add the pointer to our objects map if everything is A-OK
			if( entity->ok ) {
				int ref = entity->luaVMInstance;
				if( !objects.count( entity->cid ) ) {
					objects[ entity->cid ] = std::move( entity );
					return ref;
				}
			}

			// Entity didn't build successfully
			return -1;
		}

		/**
		 * Proxies to Lot::createLotEntity
		 */
		int Lot::lua_createLotEntity( lua_State* L ) {

			Lot* lot = ( Lot* )lua_touserdata( L, lua_upvalueindex( 1 ) );

			// Get argument (the class we are looking for) and remove it from the stack
			std::string classID( lua_tostring( L, -1 ) );
			lua_pop( L, 1 );

			// Create the lot entity itself
			int reference = lot->createLotEntity( classID );

			if( reference != -1 ) {
				// Push instance on the stack
				lua_rawgeti( L, LUA_REGISTRYINDEX, reference );
			} else {
				// Object didn't build successfully, there's no instance to return
				lua_pushnil( L );
			}

			return 1;
		}
	}
}