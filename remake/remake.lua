---
-- Remake: Premake proxy
-- Copyright (c) 2018 arcturus/tekgoblin
---
local ISDEBUG = false
include "extensions.lua"

function prettyPathing()
	vpaths {
		["Headers/*"] = {
			paths.source:append("**.h"),
		},
		["Headers/Libs/*"] = { paths.libs:append("**.h") },
		["Sources/*"] = {
			paths.source:append("**.c"),
		},
		["Sources/Libs/*"] = {
			paths.libs:append("**.c"),
		},
		["Docs"] = "**.txt"
	}
end

local function debuglog(msg)
	if ISDEBUG then
		print(msg)
	end
end

local gConfig = nil
Config = Object()

function Config:new(source, libs, build, dist, extra)
	local self = settype(Config)

	self.base = _WORKING_DIR
	self.libs = path.join(self.base, npath(libs))
	self.source = path.join(self.base, npath(source))
	self.build = path.join(self.base, npath(build))
	self.dist = path.join(self.base, npath(dist))
	self.buildLib = path.join(self.build, "%{cfg.buildcfg}", "%{cfg.architecture}")
	self.extra = Table(extra)

	location (self.build)
	--debugdir (self.dist)
	targetdir (self.buildLib)
	syslibdirs (self.buildLib)
	libdirs (self.buildLib)

	path.ensure(self.dist)

	self.extra:each(function(v,k)
		self[k] = npath(os.realpath(v))
	end)

	gConfig = self
	return self
end

function Config:dump(indent)
	print("Config")

	print("Base Path      : ", self.base)
	print("Lib Path       : ", self.libs)
	print("Source Path    : ", self.source)
	print("Build Path     : ", self.build)
	print("Build Libs Path: ", self.buildLib)
	print("Output Path    : ", self.dist)

	print("Extra Paths    : ")
	self.extra:each(function(v,k)
		print("\t" .. k .. '\t @ ' .. v)
	end)

	print()
end

function mytarget()
	return path.join("%{cfg.buildcfg}", "%{cfg.architecture}", "%{cfg.buildtarget.name}")
end

function Config:getCRoot()
	return os.getcwd():gsub(gConfig.base .. '/', '')
end

function retarget(list, root)
	if list == nil then
		return nil
	end
	if type(list) == "table" then
		local T = Table{}
		list = Table(list)
		list:each(function(file,k)
			T[file] = retarget(file, root)
		end)
		return T
	end

	if root == nil then root = paths.getCRoot() end
	if list:sub(0, 1) ~= '%' then
		return path.join(paths.dist, root, list)
	end

	return path.join(paths.dist, root)
end

function includeall(loc)
	if not os.isdir(loc) then
		premake.error("includeall path '" .. loc .. "' doesn't exist or can't be found")
	end
	local list = os.matchfiles(path.join(loc, "**/premake5.lua"))
	foreach(list, function(v)
		debuglog("Including " .. v)
		include (v)
	end)
end

function distcopy(list, target)
	if type(list) == 'table' then
		list = Table(list)
		list:each(function(f,k)
			distcopy(f, target)
		end)
		return
	end

	if target == nil then
		target = gConfig.dist
	end

	if list:sub(0, 1) ~= '%' then
		list = normalizepath(list)
		target = path.ensure(target)
	end
	postbuildcommands ("{COPY} \"" .. list .. "\" \"" .. target .. "\"")
end

function distmirror(list, target)
	local l = retarget(list, target)
	if type(l) ~= 'table' then
		distcopy(l, target)
		return
	end
	l:each(function(target, file)
		distcopy(file, target)
	end)
end

local exports = nil
local _m = nil
function getScope()
	if _m ~= nil then return _m.name end

	local p = project()
	if p == nil then
		premake.error("Can't determine project scope. Not a module and project() is nil")
	end
	return p.name
end

local Module = Object()
function Module:new(name,ismodule)
	if ismodule == nil or type(ismodule) ~= 'boolean' then
		ismodule = false
	end

	local mtable = function()
		return Table {
			['includedirs'] = Table{},
			['libdirs'] = Table{},
			['links'] = Table{},
			['defines'] = Table{},
			--['_defines'] = Table{},
			['using'] = Table{},
		}
	end

	local self = settype(Module, {
		['name'] = name,
		['ismodule'] = ismodule,
		['merged'] = false,
		['public'] = Table{},
	})

	return self
end

function Module:ensureKey(key)
	if self.public[key] == nil then
		self.public[key] = Table{}
	end
	return self.public[key]
end

function Module:dump()
	local m = self
	settype(table, m)
	m:dump()
end

function Module:process()
	print("Processing Module: " .. self.name)
	if self.merged then
		print("Module(" .. self.name .. ") already processed")
		return
	end
	local list = self.public['using'] or Table{}
	local validk = Table { 'includedirs', 'libdirs', 'links', 'defines', 'using' }
	list:each(function(mod,k)
		local m = exports:getModule(mod)
		debuglog("\t" .. mod)
		validk:each(function(v,_)
			if m.public[v] == nil then return end
			debuglog("\t\t" .. v)
			if v ~= 'using' then
				shared[v](m.public[v])
			end
		end)
		if not m.ismodule then
			shared.links(m.name)
		end
		debuglog("...")
	end)
	print("Module(" .. self.name .. ") processed")
	self.merged = true
	debuglog("------")
	--self:dump()
end

Export = Object()

function Export:new()
	local self = settype(Export)

	self.modules = Table{}

	return self
end

function Export:getModule(name)
	return self.modules[name] or nil
end

function Export:newModule(name, ismodule)
	self.modules[name] = Module(name, ismodule)
	return self.modules[name]
end

function Export:ensureModule(name)
	return self:getModule(name) or self:newModule(name)
end

function Export:kmerge(name, key, list)
	local ctx = self:ensureModule(name)

	if key == 'includedirs' or key == 'libdirs' then
		ctx:ensureKey(key):append(foreach(list, normalizepath))
	else
		ctx:ensureKey(key):append(list)
	end
end

function Export:dump()
	self.modules:dump()
end

exports = Export()

public = Table {
	['includedirs'] = function(list) exports:kmerge(getScope(), 'includedirs', list) end,
	['libdirs'] = function(list) exports:kmerge(getScope(), 'libdirs', list) end,
	['defines'] = function(list) exports:kmerge(getScope(), 'defines', list) end,
	['links'] = function(list) exports:kmerge(getScope(), 'links', list) end,
}

shared = Table {
	['includedirs'] = function(list)
		public.includedirs(list)
		includedirs(list)
	end,
	['libdirs'] = function(list)
		public.libdirs(list)
		libdirs(list)
	end,
	['defines'] = function(list)
		public.defines(list)
		defines(list)
	end,
	['links'] = function(list)
		public.links(list)
		links(list)
	end,
}

function dumpModule()
	exports:getModule(getScope()):dump()
end

function dumpModules()
	exports:dump()
end

function using(list)
	if list == nil then
		premake.error("'using' missing parameter(s)")
	end

	local name = getScope()
	if name == nil then
		premake.error("'using' must be used in a project or library scope")
	end

	exports:kmerge(name, 'using', list)
	exports:getModule(name):process()
end

function library(name, fn)
	if _m ~= nil then
		premake.error("Already in library scope '" .. _m.name .. "'")
	end
	local m = exports:getModule(name)
	if m ~= nil then
		premake.error("Library already exists '" .. name .. "'")
	end
		
	_m = exports:newModule(name, true)
	fn()
	if _m ~= nil then		
		if _m.name ~= name then
			premake.error("Library error: Somehow module was reset while processing '" .. name .. "'!!!")
		end
		_m = nil
	end
end
