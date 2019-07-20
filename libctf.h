struct ctf_context {};

#define CTF_PREFIX Ctf_TEST___
#define CTF_SUFFIX ___TEST_ctF
#define CTF_CIRCUMFIX(name) CTF_CIRCUMFIX_(CTF_PREFIX, name, CTF_SUFFIX)
#define CTF_CIRCUMFIX_(pfx, name, sfx) CTF_CIRCUMFIX__(pfx, name, sfx)
#define CTF_CIRCUMFIX__(pfx, name, sfx) pfx##name##sfx

#if CTF_TESTS_ENABLED
#	define CTF_TEST(name, ...) void CTF_CIRCUMFIX(name) \
		(struct ctf_context *ctf___context) { __VA_ARGS__; }
#else
#	define CTF_TEST(name, ...)
#endif
