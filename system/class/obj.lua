--[[
  Set up the class system used by the game
--]]

bluebear.engine.require_modpack( "middleclass" )
bluebear.engine.require_modpack( "util" )
bluebear.engine.require_modpack( "json" )

-- Container for registered classes in Luasphere
bluebear.classes = {}

--[[
	Register a new class in the bluebear system

	@param		{Class}			Class			A class.
--]]
bluebear.register_class = function( Class )
	local id = bluebear.util.split( Class.name, '.' )

	if #id == 0 then
		print( "Could not load class  \""..Class.name.."\": Invalid identifier!" )
		return false
	end

	-- Slap the class into the bluebear.classes table, a central registry of all types of classes available to the game
	local currentObject = bluebear.classes
	local max = #id - 1

	for i = 1, max, 1 do
		if currentObject[ id[ i ] ] == nil then
			-- Object doesn't exist: create it
			currentObject[ id[ i ] ] = {}
		end

		currentObject = currentObject[ id[ i ] ]
	end

	-- Mark every class with this hidden property to signify that this table is a class
	-- This distinguishes between classes and namespaces in seek operations
	Class.__middleclass = true

	currentObject[ id[ #id ] ] = Class

	print( "Registered class "..Class.name )
end

bluebear.get_class = function( identifier )
	local id = bluebear.util.split( identifier, '.' )
	local currentObject = bluebear.classes
	local max = #id

	for i = 1, max, 1 do
		if currentObject[ id[ i ] ] == nil then
			-- Object doesn't exist: return nil
			return nil
		end

		currentObject = currentObject[ id[ i ] ]
	end

	return currentObject
end

--[[
	Get classes of type "identifier". This will return not only classes that ARE the base class,
	but classes that derive from the specified base class. Potentially expensive operation:
	make sure you cache its results (new classes should not be registered in the course of
	a game.)

	@param	{String}		identifier		The base class
--]]
local recursive_helper = function( node, Needle, found_classes )
	-- Leaf node
	if node.__middleclass and node:isSubclassOf( Needle ) then
		table.insert( found_classes, node )
	else
		-- Ordinary node
		for key, value in pairs( node ) do
			recursive_helper( value, Needle, found_classes )
		end
	end


end
bluebear.get_classes_of_type = function( identifier )
	-- Get the class to compare to
	local Needle = bluebear.get_class( identifier )

	-- If it doesn't exist, we have nothing else to do here.
	if Needle == nil then return nil end

	-- Use the recursive_helper function to navigate bluebear.classes,
	-- placing the results in at every step of the direction.
	local found_classes = {}
	table.insert( found_classes, Needle )
	recursive_helper( bluebear.classes, Needle, found_classes )

	return found_classes
end

--[[
	Extend a class registered using bluebear.register_class

	@param	{String}		identifier		The class you wish to extend
	@param	{String}		new_name			The name of the new class
	@param	{Table}			class_table		The class table
--]]
bluebear.extend = function( identifier, new_name, class_table )
  local Class = bluebear.get_class( identifier )
  local SubClass = nil

  if Class ~= nil then
    SubClass = bluebear.util.extend( Class:subclass( new_name ), class_table )
  end

  return SubClass
end

bluebear.new_instance = function( identifier )
  local Class = bluebear.get_class( identifier )
  local instance = nil

  if Class ~= nil then
    -- new instance
    instance = Class:new();
  end

  return instance
end

--[[
  Create a new instance from a serialised version of the object.

  @param		{String}		identifier		The object class which will be selected from bluebear.classes
  @param		{String}		savedInstance	The saved instance that will need to be converted back to a Lua table
--]]
bluebear.new_instance_from_file = function( identifier, savedInstance )
  -- May use JSON or a Lua string table serialisation (I'm thinking of getting rid of JSON)
  local savedInstanceTable = JSON:decode( savedInstance )
  local instance = bluebear.new_instance( identifier )

  if instance == nil then error( identifier.." is not a registered class. (did you install the mod under the assets/ directory?)" ) end

  -- deserialise
  instance:load( savedInstanceTable )

  -- and that's it!
  return instance
end

--[[
  Check if a given object is an instance of "identifier"

  @param		{String}		identifier		The identifier for the class to compare to
  @param		{Object}		instance			An instance of at least 'system.object.base'
--]]
bluebear.instance_of = function( identifier, instance )
  local Class = bluebear.get_class( identifier )

  if Class == nil then
    return false
  end

  return instance:isInstanceOf( Class )
end