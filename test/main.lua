local np = require "luanproto"

print(np.load("AddressBook", "addressbook.capnp", "."))
print(np.encode("AddressBook", {
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
}))


