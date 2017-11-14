#!./lua
local np = require "luanproto"
local inspect = require "inspect"

local p = np.parse("TestInterface", "test.capnp", ".");

local test = function(interface, method, side, t)
	local str = inspect(t)
	print("\n\noriginal lua:\n")
	print(str)
	local bin = np.encode(interface, method, side, t)
	local txt = np.pretty(interface, method, side, bin)
	print("\n\nencoded txt:\n")
	print(txt)
	local de = np.decode(interface, method, side, bin)
	local destr = inspect(de)
	print("\n\ndecoded lua:\n")
	print(destr)
	assert(str==destr, "decoded lua should equal to the original.")
end

test(p, "foo", 0, {
	i = 42,
	j = true
})

test(p, 0, 0, {
	i = 42,
	j = false
})


test(p, 0, 1, {
	x = "TesT"
})

test(p, "bar", 0, {
})

test(p, 1, 1, {
})
