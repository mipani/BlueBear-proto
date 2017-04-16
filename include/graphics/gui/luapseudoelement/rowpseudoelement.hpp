#ifndef ROWPSEUDOELEMENT
#define ROWPSEUDOELEMENT

#include "graphics/gui/luapseudoelement/luapseudoelement.hpp"
#include "graphics/display.hpp"
#include <string>
#include <memory>
#include <SFGUI/Widgets.hpp>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <tinyxml2.h>

namespace BlueBear {
  namespace Graphics {
    namespace GUI {

      class RowPseudoElement : public LuaPseudoElement {
        std::shared_ptr< sfg::Table > subject;
        int rowNumber;
        Display::MainGameState& displayState;

        std::vector< std::shared_ptr< sfg::Widget > > stagedWidgets;

        std::vector< std::shared_ptr< sfg::Widget > > getWidgetsForRow();
        int getItemById( lua_State* L, const std::string& id );
        int getItemsByClass( lua_State* L, const std::string clss );

      public:
        RowPseudoElement(
          std::shared_ptr< sfg::Table > subject,
          int rowNumber,
          Display::MainGameState& displayState
        );

        void setMetatable( lua_State* L );
        std::string getName();

        static int getRowCount( std::shared_ptr< sfg::Table > table );
        static int lua_gc( lua_State* L );
        static int lua_getName( lua_State* L );
        static int lua_findElement( lua_State* L );
        static int lua_findById( lua_State* L );
        static int lua_findByClass( lua_State* L );
      };

    }
  }
}


#endif