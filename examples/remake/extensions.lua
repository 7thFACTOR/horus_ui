---
-- Remake: Premake proxy
-- Copyright (c) 2018 arcturus/tekgoblin
---
function npath(p)
	return path.translate(path.normalize(p), '/')
end

function normalizepath(p)
	return npath(os.realpath(p))
end

local function getFileName(url)
	return url:match("^.+/(.+)$")
end

local function getFileExtension(url)
	return getFileName(url):match("^.+(%..+)$")
end

function os.winSdkVersion()
	local sdk_version = nil
	if (os.host() == "windows") then
		local reg_arch = iif( os.is64bit(), "\\Wow6432Node\\", "\\" )
		sdk_version = os.getWindowsRegistry("HKLM:SOFTWARE" .. reg_arch .."Microsoft\\Microsoft SDKs\\Windows\\v10.0\\ProductVersion")
	end

	return sdk_version
end

function path.ensure(dir)
	local bits = Table(dir:explode("/"))
	local c = ''
	for _,v in ipairs(bits) do
		if v:contains('.') then goto breakloop end
		c = path.join(c, v)
		if not os.isdir(c) then
			print("Creating path: " .. c)
			os.mkdir(c)
			return c
		end
	end
	::breakloop::
	return c
end

--========================================================================================
--========================================================================================
-- OBJECT EXTENSIONS

function settype(ty, obj)
	if obj == nil then obj={} end
	setmetatable(obj, {__index = ty })
	return obj
end

function Table(t)
	if t == nil then
		t = {}
	elseif type(t) ~= 'table' then
		t = { t }
	end
    return settype(table, t)
end

function Object(def)
	if def == nil then def = {} end
	local obj = def
	obj.__index = obj
	setmetatable(obj, {
		__call = function (cls, ...)
			return cls:new(...)
		end,
	})

	return obj
end

--========================================================================================
--========================================================================================
-- STRING EXTENSIONS

function string:empty()
	return self == nil or #self == 0 or self == ''
end

function string:append (item, delim)
	if item == nil then return "" end
	if delim == nil then delim = ',' end
	if type(item) ~= "table" then
		if type(item) == "string" and self:empty() then
			return item
		end
		return (self .. delim .. item)
	end

	local t = Table{}
	local i,name
	for i,name in ipairs(item) do
		t:append(self:append(name, delim))
	end
	return t
end

function string:pathfold(item)
	return self:append(item, '/')
end

--========================================================================================
--========================================================================================
-- TABLE EXTENSIONS

function table:length()
	if self == nil then return 0 end
	return #self
end

function table:empty()
	return self:length() == 0
end

function table:compact(delim)
	local len = self:length()
	if len == 0 then return "" end
	if delim == nil then delim = ',' end
	local str = self[1]
	for i = 2, len do
		str = str .. delim .. self[i]
	end
	return str
end

function table:each(fn)
	local _,v,r
	for _,v in pairs(self) do
		if type(v) == 'table' then Table(v) end
		r = fn(v,_)
		if r == false then goto breakeach end
	end
	::breakeach::
	return self
end

function table:append(item)
	if item == nil then return end
	if type(item) ~= 'table' then
		if not self:contains(item) then
			self:insert(item)
		end
	else
		item = Table(item)
		item:each(function(v,k)	self:append(v) end)
	end
end

function table:dump (indent, level)
	if not level then level = 0 end
	if not indent then indent = 4 end

	print("{");
	for k, v in pairs(self) do
		formatting = string.rep(" ", indent * (level+1)) .. k .. ": "
		if v == nil then
			print("nil")
		elseif type(v) == "table" then
			io.write(formatting)
			Table(v):dump(indent, level+1)
		elseif type(v) == 'boolean' or type(v) == 'function' then
			print(formatting .. tostring(v))
		else
			print(formatting .. v)
		end
	end
	print(string.rep(" ", indent * level) .. "}");
end

function foreach(list, fn)
	list = Table(list)
	local l = Table{}
	for _,v in pairs(list) do
		if type(v) == 'table' then
			l:append(foreach(v, fn))
		else
			l:append(fn(v))
		end
	end
	return l
end
