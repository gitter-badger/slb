/*
    SLB - Simple Lua Binder
    Copyright (C) 2007 Jose L. Hidalgo Valiño (PpluX)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

	Jose L. Hidalgo (www.pplux.com)
	pplux@pplux.com
*/

#include <SLB/Manager.hpp>
#include <SLB/ClassInfo.hpp>
#include <SLB/lua.hpp>
#include <SLB/Debug.hpp>
#include <SLB/util.hpp>
#include <SLB/Hybrid.hpp>

#include <iostream>

namespace SLB {

	/* Global functions */
	int SLB_type(lua_State *L)
	{
		SLB_DEBUG_CALL;
		const ClassInfo *ci = Manager::getInstance().getClass(L,-1);
		if (ci)
		{
			lua_pushstring(L, ci->getName().c_str());
			return 1;
		}
		return 0;
	}

	int SLB_rawptr(lua_State *L)
	{
		SLB_DEBUG_CALL;
		int top = lua_gettop(L);
		if (lua_getmetatable(L,1))
		{
			lua_getfield(L, -1, "__class_ptr");
			if (!lua_isnil(L,-1))
			{
				ClassInfo* ci = reinterpret_cast<ClassInfo*>( lua_touserdata(L,-1) );
				const void* raw = ci->get_const_ptr(L, 1);
				lua_settop(L, top);
				lua_pushinteger(L, (lua_Integer) raw);
				return 1;
			}
		}
		lua_settop(L, top);
		return 0;
	}

	int SLB_copy(lua_State *L)
	{
		SLB_DEBUG_CALL;
		int top = lua_gettop(L);
		if (lua_getmetatable(L,1))
		{
			lua_getfield(L, -1, "__class_ptr");
			if (!lua_isnil(L,-1))
			{
				ClassInfo* ci = reinterpret_cast<ClassInfo*>( lua_touserdata(L,-1) );
				lua_settop(L, top);
				ci->push_copy(L, lua_touserdata(L,1));
				return 1;
			}
		}
		lua_settop(L, top);
		return 0;
	}

	int SLB_using_index(lua_State *L)
	{
		SLB_DEBUG_CALL;
		lua_pushnil(L);
		while( lua_next(L, lua_upvalueindex(1)) )
		{
			lua_pushvalue(L,2); // key
			lua_gettable(L, -2); 
			if (!lua_isnil(L,-1))
			{
				return 1;
			}
			lua_pop(L,2);
		}
		return 0;
	}

	int SLB_using(lua_State *L)
	{
		SLB_DEBUG_CALL;
		int top = lua_gettop(L);
		luaL_checktype(L, 1, LUA_TTABLE);

		lua_getfield(L, LUA_REGISTRYINDEX, "SLB_using");
		if ( lua_isnil(L,-1) )
		{
			// pop nil value
			lua_pop(L,1);

			// get or create _G's metatable 
			if(!lua_getmetatable(L, LUA_GLOBALSINDEX))
			{
				lua_newtable(L);
				// set as metatable of _B 
				lua_pushvalue(L,-1);
				lua_setmetatable(L, LUA_GLOBALSINDEX);
			}
			else
			{
				luaL_error(L, "Can not use SLB.using,"
					" _G already has a metatable");
			}

			lua_newtable(L); // create the "SLB_using" table
			lua_pushvalue(L,-1); // keep a copy in registry 
			lua_setfield(L, LUA_REGISTRYINDEX, "SLB_using");

			// push the using_index func with the SLB_using table as closure
			lua_pushvalue(L,-1);
			lua_pushcclosure(L, SLB_using_index, 1); // push the closure
			// set this functions as "__index" of the metatable (at -3)
			lua_setfield(L, -3, "__index");

			// leave the SLB_using table at the top
		}
		lua_pushvalue(L,1); // get the first argument
		//TODO check not repeating tables...
		luaL_ref(L, -2); // add the table

		lua_settop(L,top);
		return 0;
	}

	int SLB_isA(lua_State *L)
	{
		SLB_DEBUG_CALL;
		int top = lua_gettop(L);
		if (top != 2)
			luaL_error(L, "Invalid number of arguments (instance, class)");

		ClassInfo* ci_a = 0;
		if (lua_getmetatable(L,1))
		{
			lua_getfield(L, -1, "__class_ptr");
			if (!lua_isnil(L,-1))
			{
				ci_a = reinterpret_cast<ClassInfo*>( lua_touserdata(L,-1) );
			}
		}
		ClassInfo* ci_b = 0;
		if (lua_getmetatable(L,2))
		{
			lua_getfield(L, -1, "__class_ptr");
			if (!lua_isnil(L,-1))
			{
				ci_b = reinterpret_cast<ClassInfo*>( lua_touserdata(L,-1) );
			}
		}
		lua_settop(L, top);
		if ( ci_a && ci_b ) 
		{
			lua_pushboolean(L, ci_a->isSubClassOf(ci_b) );
		}
		else
		{
			lua_pushboolean(L, false);
		}
		return 1;
	}
	
	int SLB_allTypes(lua_State *L)
	{
		SLB_DEBUG_CALL;
		Manager::ClassMap &map =  Manager::getInstance().getClasses();

		lua_newtable(L);
		for(Manager::ClassMap::iterator i = map.begin(); i != map.end(); ++i)
		{
			lua_pushstring(L, i->second->getName().c_str());
			i->second->push(L);
			lua_rawset(L,-3);
		}
		return 1;
	}

	static const luaL_Reg SLB_funcs[] = {
		{"type", SLB_type},
		{"copy", SLB_copy},
		{"using", SLB_using},
		{"rawptr", SLB_rawptr},
		{"isA", SLB_isA},
		{"allTypes", SLB_allTypes},
		{NULL, NULL}
	};

	Manager* Manager::_singleton = 0;

	Manager::Manager()
	{
		SLB_DEBUG_CALL;
		SLB_DEBUG(0, "Manager initialization");
		_global = new Namespace();
	}

	Manager::~Manager()
	{
		SLB_DEBUG_CALL;
		SLB_DEBUG(0, "Manager destruction");
	}
	
	void Manager::registerSLB(lua_State *L)
	{
		SLB_DEBUG_CALL;
		int top = lua_gettop(L);

		// Register global functions
		luaL_register(L, "SLB", SLB_funcs);
		// metatable of "SLB"
		lua_newtable(L);
		lua_pushstring(L,"__index");
		_global->push(L);
		lua_rawset(L,-3);
		lua_setmetatable(L,-2); // SLB table

		lua_settop(L,top);
	}
	
	void Manager::reset()
	{
		SLB_DEBUG_CALL;
		delete _singleton;
		_singleton = 0;
	}
	
	void Manager::addClass( ClassInfo *c )
	{
		SLB_DEBUG_CALL;
		_classes[ c->getTypeid() ] = c;
	}

	const ClassInfo *Manager::getClass(const std::type_info &ti) const
	{
		SLB_DEBUG_CALL;
		assert("Invalid type_info" && (&ti) );
#ifdef WIN32
		for(ClassMap::const_iterator i = _classes.begin(); i != _classes.end(); ++i)
		{
			if ( *(i->first) == ti ) return i->second.get();
		}
#else
		ClassMap::const_iterator i = _classes.find(&ti);
		if ( i != _classes.end() ) return i->second.get();
#endif
		return 0;
	}

	const ClassInfo *Manager::getClass(const std::string &name) const
	{
		SLB_DEBUG_CALL;
		NameMap::const_iterator i = _names.find(name);
		if ( i != _names.end() ) return getClass( *i->second );
		return 0;
	}

	ClassInfo *Manager::getClass(lua_State *L, int pos) const
	{
		SLB_DEBUG_CALL;
		pos = L_abs_index(L,pos);
		int top = lua_gettop(L);
		ClassInfo* ci = 0L;
		if (lua_getmetatable(L,pos))
		{
			lua_getfield(L, -1, "__class_ptr");
			if (!lua_isnil(L,-1))
			{
				ci = reinterpret_cast<ClassInfo*>( lua_touserdata(L,-1) );
			}
		}
		lua_settop(L, top);
		return ci;
	}


	ClassInfo *Manager::getClass(const std::string &name)
	{
		SLB_DEBUG_CALL;
		NameMap::iterator i = _names.find(name);
		if ( i != _names.end() ) return getClass( *i->second );
		return 0;
	}

	ClassInfo *Manager::getClass(const std::type_info &ti)
	{
		SLB_DEBUG_CALL;
		ClassInfo *result = 0;
#ifdef WIN32
		for(ClassMap::iterator i = _classes.begin(); result == 0 && i != _classes.end(); ++i)
		{
			if ( *(i->first) == ti) result = i->second.get(); 
		}
#else
		ClassMap::iterator i = _classes.find(&ti);
		if ( i != _classes.end() ) result = i->second.get();
#endif		
		SLB_DEBUG(6, "ClassInfo = %p", (void*) result);
		return result;
	}
		
	bool Manager::copy(lua_State *from, int pos, lua_State *to)
	{
		SLB_DEBUG_CALL;
		SLB_DEBUG_CLEAN_STACK(from,0);
		
		if (from == to)
		{
			lua_pushvalue(from, pos);
			return true;
		}

		switch(lua_type(from, pos))
		{
			case LUA_TNIL:
				{
					SLB_DEBUG(20, "copy from %p(%d)->%p [nil]", from,pos,to);
					lua_pushnil(to);
					return true;
				}
			case LUA_TNUMBER:
				{
					SLB_DEBUG(20, "copy from %p(%d)->%p [number]", from,pos,to);
					lua_Number n = lua_tonumber(from,pos);
					lua_pushnumber(to, n);
					return true;
				}
			case LUA_TBOOLEAN:
				{
					SLB_DEBUG(20, "copy from %p(%d)->%p [boolean]", from,pos,to);
					int b = lua_toboolean(from,pos);
					lua_pushboolean(to,b);
					return true;
				}
			case LUA_TSTRING:
				{
					SLB_DEBUG(20, "copy from %p(%d)->%p [string]", from,pos,to);
					const char *s = lua_tostring(from,pos);
					lua_pushstring(to,s);
					return true;
				}
			case LUA_TTABLE:
				{
					SLB_DEBUG(0, "*WARNING* copy of tables Not yet Implemented!!!");
					return false;
				}
			case LUA_TUSERDATA:
				{
					SLB_DEBUG(20, "copy from %p(%d)->%p [Object]", from,pos,to);
					ClassInfo *ci = getClass(from, pos);
					if (ci != 0L)
					{
						const void* ptr = ci->get_const_ptr(from, pos);
						SLB_DEBUG(25, "making a copy of the object %p", ptr);
						// now copy it
						ci->push_copy(to,ptr);
						return true;
					}
					else
					{
						SLB_DEBUG(25, "Could not recognize the object");
						return false;
					}
				}
		}
		SLB_DEBUG(10,
			"Invalid copy from %p(%d)->%p %s",
			from,pos,to, lua_typename(from, lua_type(from,pos)));
		return false;
	}

	ClassInfo *Manager::getOrCreateClass(const std::type_info &ti)
	{
		SLB_DEBUG_CALL;
		assert("Invalid type_info" && (&ti) );
		ClassInfo *c = 0;
#ifdef WIN32
		ClassMap::iterator i = _classes.begin();
		for(; i != _classes.end(); ++i)
		{
			if ( *(i->first) == ti ) break;
		}
#else
		ClassMap::iterator i = _classes.find(&ti);
#endif
		if ( i != _classes.end() )
		{
			c = i->second.get();
		}
		else
		{
			c = new ClassInfo(ti);
		}
		return c;
	}
	
	void Manager::set(const std::string &name, Object *obj)
	{
		SLB_DEBUG_CALL;
		_global->set(name, obj);
	}
	
	void Manager::rename(ClassInfo *ci, const std::string &new_name)
	{
		SLB_DEBUG_CALL;
		const std::string old_name = ci->getName();

		NameMap::iterator i = _names.find(old_name);
		if ( i != _names.end() )
		{
			_global->erase(old_name);
			_names.erase(i);
		}

		_global->set(new_name, ci);
		_names[ new_name ] = ci->getTypeid();

	}
}

