local np = require "luanproto"
local inspect = require "inspect"

print(np.load("AddressBook", "addressbook.capnp", "."))
local t = {
	people={
		{
			id=123,
			name="Alice",
			email="alice@example.com",
			phones={
				{
					number="555-1212",
					type="MOBILE"
				}
			},
			employment={
				school="MIT"
			}
		},
		{
			id=456,
			name="Bob",
			email="bob@example.com",
			phones={
				{
					number="555-4567",
					type="HOME",
				},
				{
					number="555-7654",
					type="WORK"
				}
			},
			employment={
				unemployed=true
			}
		}
	}
}
local str = inspect(t)
print("\n\noriginal lua:\n")
print(str)
local bin = np.encode("AddressBook", t)
local txt = np.pretty("AddressBook", bin)
print("\n\nencoded txt:\n")
print(txt)
local de = np.decode("AddressBook", bin)
local destr = inspect(t)
print("\n\ndecoded lua:\n")
print(destr)
assert(str==destr, "decoded lua should equal to the original.")

