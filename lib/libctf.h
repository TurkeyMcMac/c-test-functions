#ifndef LIBCTF_H_
#define LIBCTF_H_

#define CTF_PREFIX Ctf_TEST___
#define CTF_SUFFIX ___TEST_ctF
#define CTF_CIRCUMFIX(name) CTF_CIRCUMFIX_(CTF_PREFIX, name, CTF_SUFFIX)
#define CTF_CIRCUMFIX_(pfx, name, sfx) CTF_CIRCUMFIX__(pfx, name, sfx)
#define CTF_CIRCUMFIX__(pfx, name, sfx) pfx##name##sfx
#ifdef __cplusplus
#	define CTF_EXTERN_C extern "C" {
#	define CTF_END_EXTERN_C }
#else
#	define CTF_EXTERN_C
#	define CTF_END_EXTERN_C
#endif

#if CTF_TESTS_ENABLED
#	define CTF_TEST(name, ...) \
	CTF_EXTERN_C \
	/* Call the function name from the test hook so that assertion failures
	 * report that function name: */ \
	static int name(void) { __VA_ARGS__; return 0; } \
	int CTF_CIRCUMFIX(name)(void) { return name(); } \
	CTF_END_EXTERN_C
#else
#	define CTF_TEST(name, ...)
#endif

#endif /* LIBCTF_H_ */
